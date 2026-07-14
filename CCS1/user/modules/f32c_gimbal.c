#include "user/modules/f32c_gimbal.h"

#include <limits.h>
#include <stddef.h>

#include "ti_msp_dl_config.h"
#include "user/board/board_config.h"

#if ((F32C_X_DIRECTION != 1) && (F32C_X_DIRECTION != -1))
#error "F32C_X_DIRECTION must be +1 or -1"
#endif

#if ((F32C_Y_DIRECTION != 1) && (F32C_Y_DIRECTION != -1))
#error "F32C_Y_DIRECTION must be +1 or -1"
#endif

#define F32C_FRAME_HEADER                 (0x7AU)
#define F32C_FRAME_TAIL                   (0x7BU)

#define F32C_CMD_MODE                     (0x00U)
#define F32C_CMD_SPEED                    (0x01U)
#define F32C_CMD_MULTI_TURN_POSITION      (0x02U)
#define F32C_CMD_DISABLE                  (0x05U)
#define F32C_CMD_ENABLE                   (0x06U)
#define F32C_CMD_ACCELERATION             (0x07U)
#define F32C_CMD_FEEDBACK_REQUEST         (0x0EU)

#define F32C_PROTOCOL_MODE_SPEED          (0x0000U)
#define F32C_PROTOCOL_MODE_MULTI_T        (0x0001U)
#define F32C_PROTOCOL_MODE_MULTI_DIRECT   (0x0003U)

#define F32C_FEEDBACK_TYPE_SPEED          (0x00U)
#define F32C_FEEDBACK_TYPE_MULTI_ANGLE    (0x01U)
#define F32C_FEEDBACK_TYPE_MECH_ANGLE     (0x02U)
#define F32C_FEEDBACK_TYPE_ACCELERATION   (0x03U)
#define F32C_FEEDBACK_TYPE_BUS_VOLTAGE    (0x04U)
#define F32C_FEEDBACK_TYPE_COUNT          (5U)

#define F32C_MAX_PAYLOAD_LENGTH           (4U)
#define F32C_MAX_TX_FRAME_LENGTH          (9U)
#define F32C_RX_RING_CAPACITY             (128U)
#define F32C_UART_TX_TIMEOUT_MS           (20U)
#define F32C_INTER_FRAME_GAP_MS           (2U)
#define F32C_PARTIAL_FRAME_TIMEOUT_MS     (20U)
#define F32C_FEEDBACK_REQUEST_TIMEOUT_MS  (250U)
#define F32C_UART_INTERRUPT_PRIORITY      (3U)

typedef struct {
    volatile uint8_t ring[F32C_RX_RING_CAPACITY];
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint32_t last_byte_ms;
    uint8_t parser_frame[F32C_FEEDBACK_FRAME_LENGTH];
    uint8_t parser_length;
} F32C_RxChannel;

static F32C_AxisState g_axis_state[F32C_AXIS_COUNT];
static F32C_RxDiagnostics g_rx_diagnostics;
static F32C_RxChannel g_rx_channel[F32C_AXIS_COUNT];
static uint32_t g_request_start_ms[F32C_AXIS_COUNT];
static volatile uint32_t g_time_ms;
static bool g_initialized;
static bool g_timebase_ready;

static bool f32c_axis_valid(F32C_Axis axis)
{
    return ((uint32_t) axis < (uint32_t) F32C_AXIS_COUNT);
}

static uint8_t f32c_axis_address(F32C_Axis axis)
{
    return (axis == F32C_AXIS_X) ? F32C_X_ADDRESS : F32C_Y_ADDRESS;
}

static int32_t f32c_axis_direction(F32C_Axis axis)
{
    return (axis == F32C_AXIS_X) ? F32C_X_DIRECTION : F32C_Y_DIRECTION;
}

static F32C_Axis f32c_address_axis(uint8_t address)
{
    return (address == F32C_X_ADDRESS) ? F32C_AXIS_X : F32C_AXIS_Y;
}

static UART_Regs *f32c_uart(void)
{
#if BOARD_HAS_F32C_GIMBAL
    return BOARD_F32C_UART_INST;
#else
    return NULL;
#endif
}

static IRQn_Type f32c_irqn(void)
{
#if BOARD_HAS_F32C_GIMBAL
    return BOARD_F32C_UART_IRQN;
#else
    return (IRQn_Type) 0;
#endif
}

static uint8_t f32c_calculate_bcc(const uint8_t *data, uint32_t length)
{
    uint8_t bcc = 0U;
    uint32_t i;

    for (i = 0U; i < length; i++) {
        bcc ^= data[i];
    }

    return bcc;
}

static bool f32c_protocol_self_test(void)
{
    static const uint8_t x_enable[] = {0x7AU, F32C_X_ADDRESS, 0x06U};
    static const uint8_t y_enable[] = {0x7AU, F32C_Y_ADDRESS, 0x06U};
    static const uint8_t y_multi_t[] = {
        0x7AU, F32C_Y_ADDRESS, 0x00U, 0x00U, 0x01U
    };
    static const uint8_t y_360_deg[] = {
        0x7AU, F32C_Y_ADDRESS, 0x02U, 0x00U, 0x00U, 0x0EU, 0x10U
    };
    static const uint8_t y_multi_feedback[] = {
        0x7AU, F32C_Y_ADDRESS, 0x0EU, 0x01U
    };

    return (f32c_calculate_bcc(x_enable, sizeof(x_enable)) ==
            (uint8_t) (0x7CU ^ F32C_X_ADDRESS)) &&
           (f32c_calculate_bcc(y_enable, sizeof(y_enable)) ==
            (uint8_t) (0x7CU ^ F32C_Y_ADDRESS)) &&
           (f32c_calculate_bcc(y_multi_t, sizeof(y_multi_t)) ==
            (uint8_t) (0x7BU ^ F32C_Y_ADDRESS)) &&
           (f32c_calculate_bcc(y_360_deg, sizeof(y_360_deg)) ==
            (uint8_t) (0x66U ^ F32C_Y_ADDRESS)) &&
           (f32c_calculate_bcc(y_multi_feedback,
                sizeof(y_multi_feedback)) ==
            (uint8_t) (0x75U ^ F32C_Y_ADDRESS));
}

static void f32c_start_timebase(void)
{
    if (!g_timebase_ready) {
        g_time_ms = 0U;
        DL_SYSTICK_init(BOARD_CPUCLK_HZ / 1000U);
        DL_SYSTICK_enableInterrupt();
        DL_SYSTICK_enable();
        g_timebase_ready = true;
    }
}

static void f32c_delay_ms(uint32_t milliseconds)
{
    uint32_t i;

    for (i = 0U; i < milliseconds; i++) {
        delay_cycles(BOARD_CPUCLK_HZ / 1000U);
    }
}

static bool f32c_wait_expired(uint32_t start_ms, uint32_t timeout_ms,
    uint32_t *loop_budget)
{
    if ((g_time_ms - start_ms) >= timeout_ms) {
        return true;
    }
    if (*loop_budget == 0U) {
        return true;
    }

    (*loop_budget)--;
    return false;
}

static F32C_Result f32c_uart_send(F32C_Axis axis,
    const uint8_t *data, uint32_t length)
{
    UART_Regs *uart;
    uint32_t i;
    uint32_t start_ms;
    uint32_t loop_budget;

    if (!f32c_axis_valid(axis) || (data == NULL)) {
        return F32C_ERROR_INVALID_AXIS;
    }
    uart = f32c_uart();
    if (uart == NULL) {
        return F32C_ERROR_NOT_AVAILABLE;
    }

    for (i = 0U; i < length; i++) {
        start_ms = g_time_ms;
        loop_budget = (BOARD_CPUCLK_HZ / 1000U) * F32C_UART_TX_TIMEOUT_MS;
        while (DL_UART_Main_isTXFIFOFull(uart)) {
            if (f32c_wait_expired(start_ms, F32C_UART_TX_TIMEOUT_MS,
                    &loop_budget)) {
                return F32C_ERROR_UART_TIMEOUT;
            }
        }
        DL_UART_Main_transmitData(uart, data[i]);
    }

    start_ms = g_time_ms;
    loop_budget = (BOARD_CPUCLK_HZ / 1000U) * F32C_UART_TX_TIMEOUT_MS;
    while (DL_UART_Main_isBusy(uart)) {
        if (f32c_wait_expired(start_ms, F32C_UART_TX_TIMEOUT_MS,
                &loop_budget)) {
            return F32C_ERROR_UART_TIMEOUT;
        }
    }

    return F32C_OK;
}

static F32C_Result f32c_send_command(F32C_Axis axis, uint8_t command,
    const uint8_t *payload, uint8_t payload_length)
{
    uint8_t frame[F32C_MAX_TX_FRAME_LENGTH];
    uint8_t bcc_index;
    uint8_t frame_length;
    uint8_t i;
    F32C_Result result;

    if (!f32c_axis_valid(axis)) {
        return F32C_ERROR_INVALID_AXIS;
    }
    if (payload_length > F32C_MAX_PAYLOAD_LENGTH) {
        return F32C_ERROR_FRAME;
    }

    frame[0] = F32C_FRAME_HEADER;
    frame[1] = f32c_axis_address(axis);
    frame[2] = command;
    for (i = 0U; i < payload_length; i++) {
        frame[3U + i] = payload[i];
    }

    bcc_index = (uint8_t) (3U + payload_length);
    frame[bcc_index] = f32c_calculate_bcc(frame, bcc_index);
    frame[bcc_index + 1U] = F32C_FRAME_TAIL;
    frame_length = (uint8_t) (bcc_index + 2U);

    result = f32c_uart_send(axis, frame, frame_length);
    if (result == F32C_OK) {
        f32c_delay_ms(F32C_INTER_FRAME_GAP_MS);
    }
    return result;
}

static F32C_Result f32c_require_axis(F32C_Axis axis)
{
    if (!f32c_axis_valid(axis)) {
        return F32C_ERROR_INVALID_AXIS;
    }
    if (!g_initialized || !g_axis_state[axis].initialized) {
        return F32C_ERROR_NOT_INITIALIZED;
    }
    return F32C_OK;
}

static F32C_Result f32c_require_enabled_axis(F32C_Axis axis)
{
    F32C_Result result = f32c_require_axis(axis);

    if (result != F32C_OK) {
        return result;
    }
    if (!g_axis_state[axis].enabled) {
        return F32C_ERROR_NOT_ENABLED;
    }
    return F32C_OK;
}

static bool f32c_position_mode(F32C_ControlMode mode)
{
    return (mode == F32C_MODE_MULTI_TURN_T) ||
           (mode == F32C_MODE_MULTI_TURN_DIRECT);
}

static F32C_Result f32c_convert_angle(F32C_Axis axis, float angle_deg,
    int32_t *protocol_angle_x10)
{
    float min_angle;
    float max_angle;
    float scaled;
    int32_t rounded;

    if (!f32c_axis_valid(axis)) {
        return F32C_ERROR_INVALID_AXIS;
    }

    min_angle = (axis == F32C_AXIS_X) ?
        F32C_X_MIN_ANGLE_DEG : F32C_Y_MIN_ANGLE_DEG;
    max_angle = (axis == F32C_AXIS_X) ?
        F32C_X_MAX_ANGLE_DEG : F32C_Y_MAX_ANGLE_DEG;
    if ((angle_deg != angle_deg) || (angle_deg < min_angle) ||
        (angle_deg > max_angle)) {
        return F32C_ERROR_ANGLE_OUT_OF_RANGE;
    }

    scaled = angle_deg * 10.0f;
    if ((scaled > ((float) INT32_MAX - 0.5f)) ||
        (scaled < ((float) INT32_MIN + 0.5f))) {
        return F32C_ERROR_ANGLE_OUT_OF_RANGE;
    }

    rounded = (scaled >= 0.0f) ?
        (int32_t) (scaled + 0.5f) : (int32_t) (scaled - 0.5f);
    if (f32c_axis_direction(axis) < 0) {
        if (rounded == INT32_MIN) {
            return F32C_ERROR_ANGLE_OUT_OF_RANGE;
        }
        rounded = -rounded;
    }
    *protocol_angle_x10 = rounded;
    return F32C_OK;
}

static F32C_Result f32c_validate_position_command(F32C_Axis axis,
    float angle_deg)
{
    int32_t ignored_angle;
    F32C_Result result = f32c_require_enabled_axis(axis);

    if (result != F32C_OK) {
        return result;
    }
    if (!f32c_position_mode(g_axis_state[axis].commanded_mode)) {
        return F32C_ERROR_INVALID_MODE;
    }
    return f32c_convert_angle(axis, angle_deg, &ignored_angle);
}

static uint32_t f32c_read_be32(const uint8_t *data)
{
    return ((uint32_t) data[0] << 24) |
           ((uint32_t) data[1] << 16) |
           ((uint32_t) data[2] << 8) |
           (uint32_t) data[3];
}

static uint32_t f32c_feedback_mask(uint8_t feedback_type)
{
    return (feedback_type < F32C_FEEDBACK_TYPE_COUNT) ?
        ((uint32_t) 1U << feedback_type) : 0U;
}

static void f32c_parser_resynchronize(F32C_RxChannel *channel)
{
    uint8_t source;
    uint8_t destination;

    for (source = 1U; source < channel->parser_length; source++) {
        if (channel->parser_frame[source] == F32C_FRAME_HEADER) {
            destination = 0U;
            while (source < channel->parser_length) {
                channel->parser_frame[destination] =
                    channel->parser_frame[source];
                destination++;
                source++;
            }
            channel->parser_length = destination;
            return;
        }
    }
    channel->parser_length = 0U;
}

static void f32c_note_frame_error(const F32C_RxChannel *channel,
    F32C_Result error)
{
    uint8_t address = channel->parser_frame[1];

    if ((address == F32C_X_ADDRESS) || (address == F32C_Y_ADDRESS)) {
        g_axis_state[f32c_address_axis(address)].last_error = error;
    }
}

static bool f32c_validate_feedback_frame(F32C_RxChannel *channel)
{
    uint8_t address = channel->parser_frame[1];
    uint8_t feedback_type = channel->parser_frame[2];

    if ((channel->parser_frame[0] != F32C_FRAME_HEADER) ||
        (channel->parser_frame[8] != F32C_FRAME_TAIL) ||
        ((address != F32C_X_ADDRESS) && (address != F32C_Y_ADDRESS)) ||
        (feedback_type >= F32C_FEEDBACK_TYPE_COUNT)) {
        g_rx_diagnostics.frame_error_count++;
        f32c_note_frame_error(channel, F32C_ERROR_FRAME);
        return false;
    }

    if (f32c_calculate_bcc(channel->parser_frame, 7U) !=
        channel->parser_frame[7]) {
        g_rx_diagnostics.bcc_error_count++;
        f32c_note_frame_error(channel, F32C_ERROR_BCC);
        return false;
    }
    return true;
}

static void f32c_apply_feedback_frame(F32C_RxChannel *channel)
{
    F32C_Axis axis = f32c_address_axis(channel->parser_frame[1]);
    F32C_AxisState *state = &g_axis_state[axis];
    uint8_t feedback_type = channel->parser_frame[2];
    uint32_t value = f32c_read_be32(&channel->parser_frame[3]);
    uint32_t mask = f32c_feedback_mask(feedback_type);
    uint32_t now = g_time_ms;
    uint32_t i;

    state->last_feedback_type = feedback_type;
    state->last_feedback_value_raw = value;
    for (i = 0U; i < F32C_FEEDBACK_FRAME_LENGTH; i++) {
        state->last_feedback_frame[i] = channel->parser_frame[i];
    }

    switch (feedback_type) {
    case F32C_FEEDBACK_TYPE_SPEED:
        state->feedback_speed_raw = (int32_t) value;
        break;
    case F32C_FEEDBACK_TYPE_MULTI_ANGLE:
        state->feedback_multi_turn_angle_deg =
            ((float) (int32_t) value / 10.0f) *
            (float) f32c_axis_direction(axis);
        state->feedback_multi_turn_valid = true;
        state->feedback_multi_turn_time_ms = now;
        break;
    case F32C_FEEDBACK_TYPE_MECH_ANGLE:
        state->feedback_mechanical_angle_deg =
            ((float) value / 10.0f) * (float) f32c_axis_direction(axis);
        break;
    case F32C_FEEDBACK_TYPE_ACCELERATION:
        state->feedback_acceleration_raw = value;
        break;
    case F32C_FEEDBACK_TYPE_BUS_VOLTAGE:
        state->feedback_bus_voltage_v = (float) value / 100.0f;
        break;
    default:
        return;
    }

    state->feedback_received_mask |= mask;
    state->feedback_pending_mask &= ~mask;
    state->feedback_valid = true;
    state->last_feedback_time_ms = now;
    state->last_error = F32C_OK;
    g_rx_diagnostics.valid_frame_count++;
}

static void f32c_consume_rx_byte(F32C_Axis axis, uint8_t byte)
{
    F32C_RxChannel *channel = &g_rx_channel[axis];

    if (channel->parser_length == 0U) {
        if (byte == F32C_FRAME_HEADER) {
            channel->parser_frame[0] = byte;
            channel->parser_length = 1U;
        }
        return;
    }

    channel->parser_frame[channel->parser_length] = byte;
    channel->parser_length++;
    if (channel->parser_length == F32C_FEEDBACK_FRAME_LENGTH) {
        if (f32c_validate_feedback_frame(channel)) {
            f32c_apply_feedback_frame(channel);
            channel->parser_length = 0U;
        } else {
            f32c_parser_resynchronize(channel);
        }
    }
}

static bool f32c_rx_pop(F32C_Axis axis, uint8_t *byte)
{
    F32C_RxChannel *channel = &g_rx_channel[axis];
    uint16_t tail = channel->tail;

    if (tail == channel->head) {
        return false;
    }
    *byte = channel->ring[tail];
    channel->tail = (uint16_t) ((tail + 1U) % F32C_RX_RING_CAPACITY);
    return true;
}

static void f32c_reset_state(void)
{
    uint32_t axis;

    for (axis = 0U; axis < (uint32_t) F32C_AXIS_COUNT; axis++) {
        g_rx_channel[axis].head = 0U;
        g_rx_channel[axis].tail = 0U;
        g_rx_channel[axis].parser_length = 0U;
        g_rx_channel[axis].last_byte_ms = 0U;
        g_axis_state[axis] = (F32C_AxisState) {0};
        g_axis_state[axis].device_address =
            f32c_axis_address((F32C_Axis) axis);
        g_axis_state[axis].commanded_mode = F32C_MODE_DISABLED;
        g_request_start_ms[axis] = 0U;
    }
    NVIC_DisableIRQ(f32c_irqn());
    g_rx_diagnostics = (F32C_RxDiagnostics) {0};
}

static F32C_Result f32c_finish_init_failure(F32C_Result error)
{
    uint32_t axis;

    (void) F32C_EmergencyDisable();
    g_initialized = false;
    for (axis = 0U; axis < (uint32_t) F32C_AXIS_COUNT; axis++) {
        g_axis_state[axis].initialized = false;
        g_axis_state[axis].last_error = error;
    }
    return error;
}

static void f32c_uart_irq(void)
{
    UART_Regs *uart = f32c_uart();
    F32C_RxChannel *channel = &g_rx_channel[F32C_AXIS_X];
    uint16_t head;
    uint16_t next;
    uint8_t byte;
    uint32_t errors;

    if (uart == NULL) {
        return;
    }

    while (!DL_UART_Main_isRXFIFOEmpty(uart)) {
        byte = DL_UART_Main_receiveData(uart);
        head = channel->head;
        next = (uint16_t) ((head + 1U) % F32C_RX_RING_CAPACITY);
        if (next == channel->tail) {
            g_rx_diagnostics.rx_overflow_count++;
        } else {
            channel->ring[head] = byte;
            channel->head = next;
            g_rx_diagnostics.rx_byte_count++;
            channel->last_byte_ms = g_time_ms;
        }
    }

    errors = DL_UART_Main_getRawInterruptStatus(uart,
        DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR |
        DL_UART_MAIN_INTERRUPT_BREAK_ERROR |
        DL_UART_MAIN_INTERRUPT_PARITY_ERROR |
        DL_UART_MAIN_INTERRUPT_FRAMING_ERROR |
        DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR);
    if (errors != 0U) {
        DL_UART_Main_clearInterruptStatus(uart, errors);
        g_rx_diagnostics.frame_error_count++;
    }
}

int F32C_IsAvailable(void)
{
#if BOARD_HAS_F32C_GIMBAL
    return 1;
#else
    return 0;
#endif
}

void SysTick_Handler(void)
{
    g_time_ms++;
}

#if BOARD_HAS_F32C_GIMBAL
#if defined(UART_BLDC_INST_IRQHandler)
void UART_BLDC_INST_IRQHandler(void)
#elif defined(UART_BLDC_X_INST_IRQHandler)
void UART_BLDC_X_INST_IRQHandler(void)
#else
void UART2_IRQHandler(void)
#endif
{
    f32c_uart_irq();
}
#endif

F32C_Result F32C_GimbalInit(void)
{
    uint32_t axis;

    if (!F32C_IsAvailable()) {
        return F32C_ERROR_NOT_AVAILABLE;
    }
    if (g_initialized) {
        return F32C_OK;
    }

    f32c_start_timebase();
    f32c_reset_state();
    if (!f32c_protocol_self_test()) {
        return f32c_finish_init_failure(F32C_ERROR_PROTOCOL_SELF_TEST);
    }

#if BOARD_HAS_F32C_GIMBAL
    {
        UART_Regs *uart = f32c_uart();
        IRQn_Type irqn = f32c_irqn();

        DL_UART_Main_setRXFIFOThreshold(
            uart, DL_UART_MAIN_RX_FIFO_LEVEL_ONE_ENTRY);
        DL_UART_Main_enableInterrupt(uart,
            DL_UART_MAIN_INTERRUPT_RX |
            DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR |
            DL_UART_MAIN_INTERRUPT_BREAK_ERROR |
            DL_UART_MAIN_INTERRUPT_PARITY_ERROR |
            DL_UART_MAIN_INTERRUPT_FRAMING_ERROR |
            DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR);
        NVIC_SetPriority(irqn, F32C_UART_INTERRUPT_PRIORITY);
        NVIC_ClearPendingIRQ(irqn);
        NVIC_EnableIRQ(irqn);
    }
#endif

    g_initialized = true;
    for (axis = 0U; axis < (uint32_t) F32C_AXIS_COUNT; axis++) {
        g_axis_state[axis].initialized = true;
    }

    return F32C_OK;
}

void F32C_GimbalProcess(void)
{
    uint8_t byte;
    uint32_t now = g_time_ms;
    uint32_t axis;

    for (axis = 0U; axis < (uint32_t) F32C_AXIS_COUNT; axis++) {
        F32C_RxChannel *channel = &g_rx_channel[axis];

        while (f32c_rx_pop((F32C_Axis) axis, &byte)) {
            f32c_consume_rx_byte((F32C_Axis) axis, byte);
        }

        if ((channel->parser_length != 0U) &&
            ((now - channel->last_byte_ms) >=
             F32C_PARTIAL_FRAME_TIMEOUT_MS)) {
            channel->parser_length = 0U;
            g_rx_diagnostics.rx_timeout_count++;
        }

        if (g_axis_state[axis].feedback_valid &&
            ((now - g_axis_state[axis].last_feedback_time_ms) >
             F32C_FEEDBACK_ONLINE_TIMEOUT_MS)) {
            g_axis_state[axis].feedback_valid = false;
        }
        if ((g_axis_state[axis].feedback_pending_mask != 0U) &&
            ((now - g_request_start_ms[axis]) >=
             F32C_FEEDBACK_REQUEST_TIMEOUT_MS)) {
            g_axis_state[axis].feedback_pending_mask = 0U;
            g_axis_state[axis].last_error = F32C_ERROR_RX_TIMEOUT;
            g_rx_diagnostics.rx_timeout_count++;
        }
    }
}

uint32_t F32C_GetTimeMs(void)
{
    return g_time_ms;
}

F32C_Result F32C_EnableAxis(F32C_Axis axis)
{
    F32C_Result result = f32c_require_axis(axis);

    if (result != F32C_OK) {
        return result;
    }
    result = f32c_send_command(axis, F32C_CMD_ENABLE, NULL, 0U);
    if (result == F32C_OK) {
        g_axis_state[axis].enabled = true;
        g_axis_state[axis].last_error = F32C_OK;
    } else {
        g_axis_state[axis].last_error = result;
    }
    return result;
}

F32C_Result F32C_DisableAxis(F32C_Axis axis)
{
    F32C_Result result = f32c_require_axis(axis);

    if (result != F32C_OK) {
        return result;
    }
    result = f32c_send_command(axis, F32C_CMD_DISABLE, NULL, 0U);
    if (result == F32C_OK) {
        g_axis_state[axis].enabled = false;
        g_axis_state[axis].commanded_speed_rpm = 0;
        g_axis_state[axis].last_error = F32C_OK;
    } else {
        g_axis_state[axis].last_error = result;
    }
    return result;
}

F32C_Result F32C_EnableAll(void)
{
    F32C_Result x_result = F32C_EnableAxis(F32C_AXIS_X);
    F32C_Result y_result = F32C_EnableAxis(F32C_AXIS_Y);

    return (x_result != F32C_OK) ? x_result : y_result;
}

F32C_Result F32C_DisableAll(void)
{
    F32C_Result x_result = F32C_DisableAxis(F32C_AXIS_X);
    F32C_Result y_result = F32C_DisableAxis(F32C_AXIS_Y);

    return (x_result != F32C_OK) ? x_result : y_result;
}

F32C_Result F32C_SetMode(F32C_Axis axis, F32C_ControlMode mode)
{
    uint16_t protocol_mode;
    uint8_t payload[2];
    F32C_Result result = f32c_require_enabled_axis(axis);

    if (result != F32C_OK) {
        return result;
    }

    switch (mode) {
    case F32C_MODE_SPEED:
        protocol_mode = F32C_PROTOCOL_MODE_SPEED;
        break;
    case F32C_MODE_MULTI_TURN_T:
        protocol_mode = F32C_PROTOCOL_MODE_MULTI_T;
        break;
    case F32C_MODE_MULTI_TURN_DIRECT:
        protocol_mode = F32C_PROTOCOL_MODE_MULTI_DIRECT;
        break;
    case F32C_MODE_DISABLED:
        return F32C_ERROR_UNSUPPORTED;
    default:
        return F32C_ERROR_INVALID_MODE;
    }

    payload[0] = (uint8_t) (protocol_mode >> 8);
    payload[1] = (uint8_t) protocol_mode;
    result = f32c_send_command(axis, F32C_CMD_MODE, payload,
        sizeof(payload));
    if (result == F32C_OK) {
        g_axis_state[axis].commanded_mode = mode;
        g_axis_state[axis].last_error = F32C_OK;
    } else {
        g_axis_state[axis].last_error = result;
    }
    return result;
}

F32C_Result F32C_SetSpeedMode(F32C_Axis axis)
{
    return F32C_SetMode(axis, F32C_MODE_SPEED);
}

F32C_Result F32C_SetMultiTurnTMode(F32C_Axis axis)
{
    return F32C_SetMode(axis, F32C_MODE_MULTI_TURN_T);
}

F32C_Result F32C_SetMultiTurnDirectMode(F32C_Axis axis)
{
    return F32C_SetMode(axis, F32C_MODE_MULTI_TURN_DIRECT);
}

F32C_Result F32C_SetVelocity(F32C_Axis axis, int16_t rpm)
{
    int16_t protocol_rpm;
    uint16_t raw;
    uint8_t payload[2];
    F32C_Result result = f32c_require_enabled_axis(axis);

    if (result != F32C_OK) {
        return result;
    }
    if (g_axis_state[axis].commanded_mode != F32C_MODE_SPEED) {
        return F32C_ERROR_INVALID_MODE;
    }
    if ((rpm < -(int16_t) F32C_MAX_SPEED_RPM) ||
        (rpm > (int16_t) F32C_MAX_SPEED_RPM)) {
        return F32C_ERROR_INVALID_SPEED;
    }

    protocol_rpm = (int16_t) (rpm * f32c_axis_direction(axis));
    raw = (uint16_t) protocol_rpm;
    payload[0] = (uint8_t) (raw >> 8);
    payload[1] = (uint8_t) raw;
    result = f32c_send_command(axis, F32C_CMD_SPEED, payload,
        sizeof(payload));
    if (result == F32C_OK) {
        g_axis_state[axis].commanded_speed_rpm = rpm;
        g_axis_state[axis].last_error = F32C_OK;
    } else {
        g_axis_state[axis].last_error = result;
    }
    return result;
}

F32C_Result F32C_SetPositionSpeed(F32C_Axis axis, uint16_t rpm)
{
    uint8_t payload[2];
    F32C_Result result = f32c_require_enabled_axis(axis);

    if (result != F32C_OK) {
        return result;
    }
    if (!f32c_position_mode(g_axis_state[axis].commanded_mode)) {
        return F32C_ERROR_INVALID_MODE;
    }
    if ((rpm == 0U) || (rpm > F32C_MAX_SPEED_RPM)) {
        return F32C_ERROR_INVALID_SPEED;
    }

    payload[0] = (uint8_t) (rpm >> 8);
    payload[1] = (uint8_t) rpm;
    result = f32c_send_command(axis, F32C_CMD_SPEED, payload,
        sizeof(payload));
    if (result == F32C_OK) {
        g_axis_state[axis].commanded_position_speed_rpm = rpm;
        g_axis_state[axis].last_error = F32C_OK;
    } else {
        g_axis_state[axis].last_error = result;
    }
    return result;
}

F32C_Result F32C_SetAcceleration(F32C_Axis axis, uint16_t acceleration)
{
    uint8_t payload[2];
    F32C_Result result = f32c_require_enabled_axis(axis);

    if (result != F32C_OK) {
        return result;
    }
    if ((acceleration == 0U) ||
        (acceleration > F32C_MAX_ACCELERATION)) {
        return F32C_ERROR_INVALID_ACCELERATION;
    }

    payload[0] = (uint8_t) (acceleration >> 8);
    payload[1] = (uint8_t) acceleration;
    result = f32c_send_command(axis, F32C_CMD_ACCELERATION, payload,
        sizeof(payload));
    if (result == F32C_OK) {
        g_axis_state[axis].commanded_acceleration = acceleration;
        g_axis_state[axis].last_error = F32C_OK;
    } else {
        g_axis_state[axis].last_error = result;
    }
    return result;
}

F32C_Result F32C_SetAngleDeg(F32C_Axis axis, float angle_deg)
{
    int32_t protocol_angle;
    uint32_t raw;
    uint8_t payload[4];
    F32C_Result result = f32c_validate_position_command(axis, angle_deg);

    if (result != F32C_OK) {
        return result;
    }
    result = f32c_convert_angle(axis, angle_deg, &protocol_angle);
    if (result != F32C_OK) {
        return result;
    }

    raw = (uint32_t) protocol_angle;
    payload[0] = (uint8_t) (raw >> 24);
    payload[1] = (uint8_t) (raw >> 16);
    payload[2] = (uint8_t) (raw >> 8);
    payload[3] = (uint8_t) raw;
    result = f32c_send_command(axis, F32C_CMD_MULTI_TURN_POSITION, payload,
        sizeof(payload));
    if (result == F32C_OK) {
        g_axis_state[axis].commanded_angle_deg = angle_deg;
        g_axis_state[axis].commanded_angle_valid = true;
        g_axis_state[axis].last_error = F32C_OK;
    } else {
        g_axis_state[axis].last_error = result;
    }
    return result;
}

F32C_Result F32C_MoveTo(float x_deg, float y_deg)
{
    F32C_Result result = f32c_validate_position_command(F32C_AXIS_X, x_deg);

    if (result == F32C_OK) {
        result = f32c_validate_position_command(F32C_AXIS_Y, y_deg);
    }
    if (result == F32C_OK) {
        result = F32C_SetAngleDeg(F32C_AXIS_X, x_deg);
    }
    if (result == F32C_OK) {
        result = F32C_SetAngleDeg(F32C_AXIS_Y, y_deg);
    }
    return result;
}

F32C_Result F32C_MoveBy(float delta_x_deg, float delta_y_deg)
{
    float base_x;
    float base_y;
    const F32C_AxisState *x = &g_axis_state[F32C_AXIS_X];
    const F32C_AxisState *y = &g_axis_state[F32C_AXIS_Y];
    F32C_Result result = f32c_require_enabled_axis(F32C_AXIS_X);

    if (result == F32C_OK) {
        result = f32c_require_enabled_axis(F32C_AXIS_Y);
    }
    if (result != F32C_OK) {
        return result;
    }

    if (F32C_IsFeedbackValid(F32C_AXIS_X) && x->feedback_multi_turn_valid) {
        base_x = x->feedback_multi_turn_angle_deg;
    } else if (x->commanded_angle_valid) {
        base_x = x->commanded_angle_deg;
    } else {
        return F32C_ERROR_FEEDBACK_UNAVAILABLE;
    }

    if (F32C_IsFeedbackValid(F32C_AXIS_Y) && y->feedback_multi_turn_valid) {
        base_y = y->feedback_multi_turn_angle_deg;
    } else if (y->commanded_angle_valid) {
        base_y = y->commanded_angle_deg;
    } else {
        return F32C_ERROR_FEEDBACK_UNAVAILABLE;
    }

    return F32C_MoveTo(base_x + delta_x_deg, base_y + delta_y_deg);
}

F32C_Result F32C_ReturnHome(void)
{
    return F32C_MoveTo(0.0f, 0.0f);
}

F32C_Result F32C_StopAxis(F32C_Axis axis)
{
    F32C_Result result = f32c_require_enabled_axis(axis);
    const F32C_AxisState *state;

    if (result != F32C_OK) {
        return result;
    }
    state = &g_axis_state[axis];
    if (state->commanded_mode == F32C_MODE_SPEED) {
        return F32C_SetVelocity(axis, 0);
    }
    if (f32c_position_mode(state->commanded_mode)) {
        if (!F32C_IsFeedbackValid(axis) ||
            !state->feedback_multi_turn_valid) {
            return F32C_ERROR_FEEDBACK_UNAVAILABLE;
        }
        return F32C_SetAngleDeg(axis, state->feedback_multi_turn_angle_deg);
    }
    return F32C_ERROR_INVALID_MODE;
}

F32C_Result F32C_StopAll(void)
{
    F32C_Result x_result = F32C_StopAxis(F32C_AXIS_X);
    F32C_Result y_result = F32C_StopAxis(F32C_AXIS_Y);

    return (x_result != F32C_OK) ? x_result : y_result;
}

F32C_Result F32C_EmergencyDisable(void)
{
    F32C_Result x_result;
    F32C_Result y_result;

    f32c_start_timebase();
    x_result = f32c_send_command(F32C_AXIS_X, F32C_CMD_DISABLE, NULL, 0U);
    y_result = f32c_send_command(F32C_AXIS_Y, F32C_CMD_DISABLE, NULL, 0U);
    if (x_result == F32C_OK) {
        g_axis_state[F32C_AXIS_X].enabled = false;
    }
    if (y_result == F32C_OK) {
        g_axis_state[F32C_AXIS_Y].enabled = false;
    }
    return (x_result != F32C_OK) ? x_result : y_result;
}

const F32C_AxisState *F32C_GetAxisState(F32C_Axis axis)
{
    return f32c_axis_valid(axis) ? &g_axis_state[axis] : NULL;
}

bool F32C_IsFeedbackValid(F32C_Axis axis)
{
    const F32C_AxisState *state;

    if (!f32c_axis_valid(axis)) {
        return false;
    }
    state = &g_axis_state[axis];
    return state->feedback_valid &&
           ((g_time_ms - state->last_feedback_time_ms) <=
            F32C_FEEDBACK_ONLINE_TIMEOUT_MS);
}

bool F32C_IsOnline(F32C_Axis axis)
{
    return F32C_IsFeedbackValid(axis);
}

F32C_Result F32C_RequestFeedback(F32C_Axis axis)
{
    uint8_t payload[1];
    uint8_t feedback_type;
    F32C_Result result = f32c_require_axis(axis);

    if (result != F32C_OK) {
        return result;
    }

    g_axis_state[axis].feedback_pending_mask = F32C_FEEDBACK_ALL_MASK;
    g_request_start_ms[axis] = g_time_ms;
    for (feedback_type = 0U;
         feedback_type < F32C_FEEDBACK_TYPE_COUNT;
         feedback_type++) {
        payload[0] = feedback_type;
        result = f32c_send_command(axis, F32C_CMD_FEEDBACK_REQUEST, payload,
            sizeof(payload));
        if (result != F32C_OK) {
            g_axis_state[axis].last_error = result;
            return result;
        }
    }
    return F32C_OK;
}

const F32C_RxDiagnostics *F32C_GetRxDiagnostics(void)
{
    return &g_rx_diagnostics;
}
