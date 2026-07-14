#ifndef USER_MODULES_F32C_GIMBAL_H_
#define USER_MODULES_F32C_GIMBAL_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * F32C two-axis gimbal driver.
 *
 * Current board mapping is selected in user/board/board_config.h:
 *   F32C bus: UART_BLDC, PB15 TX and PB16 RX in the current SysConfig.
 *   X/Y are selected by protocol address, not by different UART channels.
 *
 * The driver initializes only transport/state. It does not enable motors or
 * start motion from F32C_GimbalInit().
 */

#define F32C_X_DIRECTION                 (+1)
#define F32C_Y_DIRECTION                 (+1)

#ifndef F32C_X_ADDRESS
#define F32C_X_ADDRESS                   (0x01U)
#endif
#ifndef F32C_Y_ADDRESS
#define F32C_Y_ADDRESS                   (0x02U)
#endif

#define F32C_DEFAULT_SPEED_RPM           (20U)
#define F32C_MAX_SPEED_RPM               (50U)
#define F32C_DEFAULT_ACCELERATION        (20U)
#define F32C_MAX_ACCELERATION            (50U)

#define F32C_X_MIN_ANGLE_DEG             (-180.0f)
#define F32C_X_MAX_ANGLE_DEG             (180.0f)
#define F32C_Y_MIN_ANGLE_DEG             (-45.0f)
#define F32C_Y_MAX_ANGLE_DEG             (45.0f)

#define F32C_FEEDBACK_FRAME_LENGTH       (9U)
#define F32C_FEEDBACK_ONLINE_TIMEOUT_MS  (1000U)

#define F32C_FEEDBACK_SPEED_MASK         (1U << 0)
#define F32C_FEEDBACK_MULTI_ANGLE_MASK   (1U << 1)
#define F32C_FEEDBACK_MECH_ANGLE_MASK    (1U << 2)
#define F32C_FEEDBACK_ACCEL_MASK         (1U << 3)
#define F32C_FEEDBACK_VOLTAGE_MASK       (1U << 4)
#define F32C_FEEDBACK_ALL_MASK           (0x1FU)

typedef enum {
    F32C_AXIS_X = 0,
    F32C_AXIS_Y,
    F32C_AXIS_COUNT
} F32C_Axis;

typedef enum {
    F32C_MODE_DISABLED = 0,
    F32C_MODE_SPEED,
    F32C_MODE_MULTI_TURN_T,
    F32C_MODE_MULTI_TURN_DIRECT
} F32C_ControlMode;

typedef enum {
    F32C_OK = 0,
    F32C_ERROR_INVALID_AXIS,
    F32C_ERROR_INVALID_MODE,
    F32C_ERROR_INVALID_SPEED,
    F32C_ERROR_INVALID_ACCELERATION,
    F32C_ERROR_ANGLE_OUT_OF_RANGE,
    F32C_ERROR_NOT_INITIALIZED,
    F32C_ERROR_NOT_ENABLED,
    F32C_ERROR_UART_TIMEOUT,
    F32C_ERROR_RX_TIMEOUT,
    F32C_ERROR_FRAME,
    F32C_ERROR_BCC,
    F32C_ERROR_UNSUPPORTED,
    F32C_ERROR_BUSY,
    F32C_ERROR_FEEDBACK_UNAVAILABLE,
    F32C_ERROR_PROTOCOL_SELF_TEST,
    F32C_ERROR_NOT_AVAILABLE
} F32C_Result;

typedef struct {
    bool initialized;
    bool enabled;
    F32C_ControlMode commanded_mode;
    int16_t commanded_speed_rpm;
    uint16_t commanded_position_speed_rpm;
    uint16_t commanded_acceleration;
    float commanded_angle_deg;
    bool commanded_angle_valid;

    bool feedback_valid;
    uint32_t feedback_received_mask;
    uint32_t feedback_pending_mask;
    int32_t feedback_speed_raw;
    float feedback_multi_turn_angle_deg;
    float feedback_mechanical_angle_deg;
    uint32_t feedback_acceleration_raw;
    float feedback_bus_voltage_v;
    bool feedback_multi_turn_valid;
    uint32_t feedback_multi_turn_time_ms;

    uint8_t device_address;
    uint8_t last_feedback_type;
    uint32_t last_feedback_value_raw;
    uint8_t last_feedback_frame[F32C_FEEDBACK_FRAME_LENGTH];
    uint32_t last_feedback_time_ms;
    F32C_Result last_error;
} F32C_AxisState;

typedef struct {
    uint32_t rx_byte_count;
    uint32_t valid_frame_count;
    uint32_t frame_error_count;
    uint32_t bcc_error_count;
    uint32_t rx_overflow_count;
    uint32_t rx_timeout_count;
} F32C_RxDiagnostics;

int F32C_IsAvailable(void);
F32C_Result F32C_GimbalInit(void);
void F32C_GimbalProcess(void);
uint32_t F32C_GetTimeMs(void);

F32C_Result F32C_EnableAxis(F32C_Axis axis);
F32C_Result F32C_DisableAxis(F32C_Axis axis);
F32C_Result F32C_EnableAll(void);
F32C_Result F32C_DisableAll(void);

F32C_Result F32C_SetMode(F32C_Axis axis, F32C_ControlMode mode);
F32C_Result F32C_SetSpeedMode(F32C_Axis axis);
F32C_Result F32C_SetMultiTurnTMode(F32C_Axis axis);
F32C_Result F32C_SetMultiTurnDirectMode(F32C_Axis axis);
F32C_Result F32C_SetVelocity(F32C_Axis axis, int16_t rpm);
F32C_Result F32C_SetPositionSpeed(F32C_Axis axis, uint16_t rpm);
F32C_Result F32C_SetAcceleration(F32C_Axis axis, uint16_t acceleration);
F32C_Result F32C_SetAngleDeg(F32C_Axis axis, float angle_deg);
F32C_Result F32C_MoveTo(float x_deg, float y_deg);
F32C_Result F32C_MoveBy(float delta_x_deg, float delta_y_deg);
F32C_Result F32C_ReturnHome(void);
F32C_Result F32C_StopAxis(F32C_Axis axis);
F32C_Result F32C_StopAll(void);
F32C_Result F32C_EmergencyDisable(void);

const F32C_AxisState *F32C_GetAxisState(F32C_Axis axis);
bool F32C_IsFeedbackValid(F32C_Axis axis);
bool F32C_IsOnline(F32C_Axis axis);
F32C_Result F32C_RequestFeedback(F32C_Axis axis);
const F32C_RxDiagnostics *F32C_GetRxDiagnostics(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_MODULES_F32C_GIMBAL_H_ */
