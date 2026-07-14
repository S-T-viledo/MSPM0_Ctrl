#include "user/modules/k230.h"

#include "user/bsp/bsp_uart.h"

static char g_k230_line[K230_RX_LINE_MAX];
static k230_frame_t g_k230_frame;
static size_t g_k230_line_index;
static bool g_k230_line_ready;
static bool g_k230_frame_ready;
static bool g_k230_overflow;
static uint32_t g_k230_rx_bytes;
static uint32_t g_k230_tx_bytes;
static uint32_t g_k230_rx_frames;
static uint32_t g_k230_rx_frame_errors;

typedef enum {
    K230_PARSE_WAIT_SOF = 0,
    K230_PARSE_COMMAND,
    K230_PARSE_LENGTH,
    K230_PARSE_PAYLOAD,
    K230_PARSE_CHECKSUM,
    K230_PARSE_EOF
} k230_parse_state_t;

static k230_parse_state_t g_k230_parse_state;
static k230_frame_t g_k230_parse_frame;
static uint8_t g_k230_parse_index;

static void k230_store_byte(uint8_t data)
{
    if (data == (uint8_t) '\r') {
        return;
    }

    if (data == (uint8_t) '\n') {
        g_k230_line[g_k230_line_index] = '\0';
        g_k230_line_ready = true;
        g_k230_line_index = 0U;
        return;
    }

    if (g_k230_line_ready) {
        return;
    }

    if (g_k230_line_index < (K230_RX_LINE_MAX - 1U)) {
        g_k230_line[g_k230_line_index] = (char) data;
        g_k230_line_index++;
    } else {
        g_k230_overflow = true;
        g_k230_line_index = 0U;
        g_k230_line[0] = '\0';
    }
}

uint8_t k230_calc_checksum(uint8_t command, const uint8_t *payload, uint8_t length)
{
    uint8_t checksum = command ^ length;
    uint8_t index;

    if (payload == 0) {
        return checksum;
    }

    for (index = 0U; index < length; index++) {
        checksum ^= payload[index];
    }

    return checksum;
}

static void k230_reset_parser(void)
{
    g_k230_parse_state = K230_PARSE_WAIT_SOF;
    g_k230_parse_frame.command = 0U;
    g_k230_parse_frame.length = 0U;
    g_k230_parse_frame.checksum = 0U;
    g_k230_parse_index = 0U;
}

static void k230_frame_error(uint8_t data)
{
    g_k230_rx_frame_errors++;

    if (data == K230_FRAME_SOF) {
        k230_reset_parser();
        g_k230_parse_state = K230_PARSE_COMMAND;
    } else {
        k230_reset_parser();
    }
}

static void k230_store_frame(void)
{
    uint8_t index;

    if (g_k230_frame_ready) {
        g_k230_overflow = true;
        return;
    }

    g_k230_frame.command = g_k230_parse_frame.command;
    g_k230_frame.length = g_k230_parse_frame.length;
    g_k230_frame.checksum = g_k230_parse_frame.checksum;
    for (index = 0U; index < g_k230_parse_frame.length; index++) {
        g_k230_frame.payload[index] = g_k230_parse_frame.payload[index];
    }

    g_k230_frame_ready = true;
    g_k230_rx_frames++;
}

static void k230_parse_frame_byte(uint8_t data)
{
    switch (g_k230_parse_state) {
    case K230_PARSE_WAIT_SOF:
        if (data == K230_FRAME_SOF) {
            g_k230_parse_state = K230_PARSE_COMMAND;
        } else {
            k230_store_byte(data);
        }
        break;

    case K230_PARSE_COMMAND:
        g_k230_parse_frame.command = data;
        g_k230_parse_state = K230_PARSE_LENGTH;
        break;

    case K230_PARSE_LENGTH:
        if (data > K230_FRAME_MAX_PAYLOAD) {
            k230_frame_error(data);
            break;
        }
        g_k230_parse_frame.length = data;
        g_k230_parse_index = 0U;
        g_k230_parse_state =
            (data == 0U) ? K230_PARSE_CHECKSUM : K230_PARSE_PAYLOAD;
        break;

    case K230_PARSE_PAYLOAD:
        g_k230_parse_frame.payload[g_k230_parse_index] = data;
        g_k230_parse_index++;
        if (g_k230_parse_index >= g_k230_parse_frame.length) {
            g_k230_parse_state = K230_PARSE_CHECKSUM;
        }
        break;

    case K230_PARSE_CHECKSUM:
        g_k230_parse_frame.checksum = data;
        if (data != k230_calc_checksum(g_k230_parse_frame.command,
                g_k230_parse_frame.payload, g_k230_parse_frame.length)) {
            k230_frame_error(data);
            break;
        }
        g_k230_parse_state = K230_PARSE_EOF;
        break;

    case K230_PARSE_EOF:
        if (data == K230_FRAME_EOF) {
            k230_store_frame();
            k230_reset_parser();
        } else {
            k230_frame_error(data);
        }
        break;

    default:
        k230_reset_parser();
        break;
    }
}

void k230_init(void)
{
    g_k230_line[0] = '\0';
    g_k230_line_index = 0U;
    g_k230_line_ready = false;
    g_k230_frame_ready = false;
    g_k230_overflow = false;
    g_k230_rx_bytes = 0UL;
    g_k230_tx_bytes = 0UL;
    g_k230_rx_frames = 0UL;
    g_k230_rx_frame_errors = 0UL;
    k230_reset_parser();
}

bool k230_is_available(void)
{
    return (bsp_uart_k230_is_available() != 0);
}

void k230_poll(void)
{
    uint8_t data;

    while (bsp_uart_k230_read_byte(&data)) {
        g_k230_rx_bytes++;
        k230_parse_frame_byte(data);
    }
}

k230_status_t k230_get_status(void)
{
    k230_status_t status;

    status.uart_available = k230_is_available();
    status.line_ready = g_k230_line_ready;
    status.frame_ready = g_k230_frame_ready;
    status.overflow = g_k230_overflow;
    status.rx_bytes = g_k230_rx_bytes;
    status.tx_bytes = g_k230_tx_bytes;
    status.rx_frames = g_k230_rx_frames;
    status.rx_frame_errors = g_k230_rx_frame_errors;
    status.line = g_k230_line;

    return status;
}

bool k230_has_line(void)
{
    return g_k230_line_ready;
}

const char *k230_get_line(void)
{
    return g_k230_line;
}

void k230_clear_line(void)
{
    g_k230_line[0] = '\0';
    g_k230_line_index = 0U;
    g_k230_line_ready = false;
}

bool k230_has_frame(void)
{
    return g_k230_frame_ready;
}

const k230_frame_t *k230_get_frame(void)
{
    return &g_k230_frame;
}

void k230_clear_frame(void)
{
    g_k230_frame.command = 0U;
    g_k230_frame.length = 0U;
    g_k230_frame.checksum = 0U;
    g_k230_frame_ready = false;
}

bool k230_send_frame(uint8_t command, const uint8_t *payload, uint8_t length)
{
    uint8_t checksum;

    if (length > K230_FRAME_MAX_PAYLOAD) {
        return false;
    }

    if ((payload == 0) && (length > 0U)) {
        return false;
    }

    checksum = k230_calc_checksum(command, payload, length);

    bsp_uart_k230_write_byte(K230_FRAME_SOF);
    bsp_uart_k230_write_byte(command);
    bsp_uart_k230_write_byte(length);
    if (length > 0U) {
        bsp_uart_k230_write(payload, length);
    }
    bsp_uart_k230_write_byte(checksum);
    bsp_uart_k230_write_byte(K230_FRAME_EOF);

    g_k230_tx_bytes += (uint32_t) length + 5UL;
    return true;
}

bool k230_send_ack(uint8_t command)
{
    uint8_t payload = command;

    return k230_send_frame((uint8_t) K230_CMD_ACK, &payload, 1U);
}

bool k230_send_text_frame(const char *str)
{
    uint8_t payload[K230_FRAME_MAX_PAYLOAD];
    uint8_t length = 0U;

    if (str == 0) {
        return false;
    }

    while ((str[length] != '\0') && (length < K230_FRAME_MAX_PAYLOAD)) {
        payload[length] = (uint8_t) str[length];
        length++;
    }

    return k230_send_frame((uint8_t) K230_CMD_TEXT, payload, length);
}

void k230_send_bytes(const uint8_t *data, size_t length)
{
    if (data == 0) {
        return;
    }

    bsp_uart_k230_write(data, length);
    g_k230_tx_bytes += (uint32_t) length;
}

void k230_send_string(const char *str)
{
    const char *cursor = str;

    if (str == 0) {
        return;
    }

    bsp_uart_k230_write_string(str);
    while (*cursor != '\0') {
        g_k230_tx_bytes++;
        if (*cursor == '\n') {
            g_k230_tx_bytes++;
        }
        cursor++;
    }
}

void k230_send_line(const char *str)
{
    k230_send_string(str);
    k230_send_string("\n");
}

void k230_send_ping(uint32_t sequence)
{
    uint8_t payload[4];

    payload[0] = (uint8_t) sequence;
    payload[1] = (uint8_t) (sequence >> 8);
    payload[2] = (uint8_t) (sequence >> 16);
    payload[3] = (uint8_t) (sequence >> 24);
    (void) k230_send_frame((uint8_t) K230_CMD_PING, payload, sizeof(payload));
}
