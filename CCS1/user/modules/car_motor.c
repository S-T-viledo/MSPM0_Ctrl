#include "user/modules/car_motor.h"

#include <stdbool.h>

#include "ti_msp_dl_config.h"
#include "user/board/board_config.h"
#include "user/bsp/bsp_timer.h"

#define CAR_MOTOR_PWM_PERIOD_COUNTS      (3200U)
#define CAR_MOTOR_PWM_LOAD_VALUE         (CAR_MOTOR_PWM_PERIOD_COUNTS - 1U)

static car_motor_direction_t g_motor_direction[2] = {
    CAR_MOTOR_STOP,
    CAR_MOTOR_STOP
};
static bool g_pwm_output_enabled[2] = {false, false};

volatile uint8_t g_car_motor_requested_duty_a;
volatile uint8_t g_car_motor_requested_duty_b;
volatile uint8_t g_car_motor_applied_duty_a;
volatile uint8_t g_car_motor_applied_duty_b;
volatile uint32_t g_car_motor_written_cc_a;
volatile uint32_t g_car_motor_written_cc_b;

static uint8_t car_motor_clamp_duty(uint8_t duty_percent)
{
    return (duty_percent > CAR_MOTOR_MAX_DUTY_PERCENT) ?
        CAR_MOTOR_MAX_DUTY_PERCENT : duty_percent;
}

static GPIO_Regs *car_motor_get_port(car_motor_channel_t channel)
{
    return (channel == CAR_MOTOR_A) ?
        BOARD_MOTOR_AIN1_PORT : BOARD_MOTOR_BIN1_PORT;
}

static uint32_t car_motor_get_pin(car_motor_channel_t channel, uint8_t in1)
{
    if (channel == CAR_MOTOR_A) {
        return (in1 != 0U) ? BOARD_MOTOR_AIN1_PIN : BOARD_MOTOR_AIN2_PIN;
    }

    return (in1 != 0U) ? BOARD_MOTOR_BIN1_PIN : BOARD_MOTOR_BIN2_PIN;
}

static DL_TIMER_CC_INDEX car_motor_get_compare_index(car_motor_channel_t channel)
{
    return (channel == CAR_MOTOR_A) ?
        BOARD_MOTOR_PWMA_CC_INDEX : BOARD_MOTOR_PWMB_CC_INDEX;
}

static void car_motor_set_pwm_output_enabled(car_motor_channel_t channel,
    bool enabled)
{
    DL_TIMER_CCP_DIS_OUT channel_a_output;
    DL_TIMER_CCP_DIS_OUT channel_b_output;

    g_pwm_output_enabled[channel] = enabled;
    channel_a_output = g_pwm_output_enabled[CAR_MOTOR_A] ?
        DL_TIMER_CCP_DIS_OUT_SET_BY_OCTL : DL_TIMER_CCP_DIS_OUT_LOW;
    channel_b_output = g_pwm_output_enabled[CAR_MOTOR_B] ?
        DL_TIMER_CCP_DIS_OUT_SET_BY_OCTL : DL_TIMER_CCP_DIS_OUT_LOW;

    DL_TimerA_setCCPOutputDisabled(BOARD_MOTOR_PWM_INST,
        channel_a_output, channel_b_output);
}

static void car_motor_record_pwm_write(car_motor_channel_t channel,
    uint8_t requested_duty, uint8_t applied_duty, uint32_t compare_ticks)
{
    if (channel == CAR_MOTOR_A) {
        g_car_motor_requested_duty_a = requested_duty;
        g_car_motor_applied_duty_a = applied_duty;
        g_car_motor_written_cc_a = compare_ticks;
    } else {
        g_car_motor_requested_duty_b = requested_duty;
        g_car_motor_applied_duty_b = applied_duty;
        g_car_motor_written_cc_b = compare_ticks;
    }
}

static car_motor_direction_t car_motor_apply_polarity(
    car_motor_channel_t channel, car_motor_direction_t direction)
{
    uint8_t reverse = (channel == CAR_MOTOR_A) ?
        CAR_MOTOR_A_REVERSED : CAR_MOTOR_B_REVERSED;

    if ((reverse != 0U) && (direction == CAR_MOTOR_FORWARD)) {
        return CAR_MOTOR_REVERSE;
    }
    if ((reverse != 0U) && (direction == CAR_MOTOR_REVERSE)) {
        return CAR_MOTOR_FORWARD;
    }

    return direction;
}

static void car_motor_write_inputs(car_motor_channel_t channel,
    car_motor_direction_t direction)
{
    GPIO_Regs *port = car_motor_get_port(channel);
    uint32_t in1 = car_motor_get_pin(channel, 1U);
    uint32_t in2 = car_motor_get_pin(channel, 0U);

    DL_GPIO_clearPins(port, in1 | in2);

    if (direction == CAR_MOTOR_FORWARD) {
        DL_GPIO_setPins(port, in1);
    } else if (direction == CAR_MOTOR_REVERSE) {
        DL_GPIO_setPins(port, in2);
    } else if (direction == CAR_MOTOR_BRAKE) {
        DL_GPIO_setPins(port, in1 | in2);
    }
}

int car_motor_is_available(void)
{
#if BOARD_HAS_CAR_MOTOR
    return 1;
#else
    return 0;
#endif
}

void car_motor_set_duty(car_motor_channel_t channel, uint8_t duty_percent)
{
#if BOARD_HAS_CAR_MOTOR
    uint32_t compare_ticks;
    uint8_t safe_duty;

    if ((channel != CAR_MOTOR_A) && (channel != CAR_MOTOR_B)) {
        return;
    }

    safe_duty = car_motor_clamp_duty(duty_percent);
    compare_ticks = (CAR_MOTOR_PWM_PERIOD_COUNTS * safe_duty) / 100U;
    if (compare_ticks > CAR_MOTOR_PWM_LOAD_VALUE) {
        compare_ticks = CAR_MOTOR_PWM_LOAD_VALUE;
    }

    DL_TimerA_setCaptureCompareValue(BOARD_MOTOR_PWM_INST, compare_ticks,
        car_motor_get_compare_index(channel));
    car_motor_record_pwm_write(channel, duty_percent, safe_duty, compare_ticks);
    car_motor_set_pwm_output_enabled(channel, (safe_duty != 0U));
#else
    (void) channel;
    (void) duty_percent;
#endif
}

void car_motor_stop(car_motor_channel_t channel)
{
#if BOARD_HAS_CAR_MOTOR
    if ((channel != CAR_MOTOR_A) && (channel != CAR_MOTOR_B)) {
        return;
    }

    car_motor_set_duty(channel, 0U);
    car_motor_write_inputs(channel, CAR_MOTOR_STOP);
    g_motor_direction[channel] = CAR_MOTOR_STOP;
#else
    (void) channel;
#endif
}

void car_motor_stop_all(void)
{
    car_motor_stop(CAR_MOTOR_A);
    car_motor_stop(CAR_MOTOR_B);
}

void car_motor_set_direction(car_motor_channel_t channel,
    car_motor_direction_t direction)
{
#if BOARD_HAS_CAR_MOTOR
    car_motor_direction_t physical_direction;

    if ((channel != CAR_MOTOR_A) && (channel != CAR_MOTOR_B)) {
        return;
    }

    if (direction == CAR_MOTOR_STOP) {
        car_motor_stop(channel);
        return;
    }

    car_motor_set_duty(channel, 0U);
    if ((g_motor_direction[channel] != CAR_MOTOR_STOP) &&
        (g_motor_direction[channel] != direction)) {
        car_motor_write_inputs(channel, CAR_MOTOR_STOP);
        bsp_delay_ms(CAR_MOTOR_DIRECTION_SETTLE_MS);
    }

    physical_direction = car_motor_apply_polarity(channel, direction);
    car_motor_write_inputs(channel, physical_direction);
    g_motor_direction[channel] = direction;
#else
    (void) channel;
    (void) direction;
#endif
}

void car_motor_run(car_motor_channel_t channel,
    car_motor_direction_t direction, uint8_t duty_percent)
{
    car_motor_set_direction(channel, direction);
    car_motor_set_duty(channel, duty_percent);
}

void car_motor_ramp_to_duty(car_motor_channel_t channel,
    car_motor_direction_t direction, uint8_t target_duty_percent)
{
    uint8_t duty;
    uint8_t safe_target = car_motor_clamp_duty(target_duty_percent);

    car_motor_stop(channel);
    bsp_delay_ms(CAR_MOTOR_DIRECTION_SETTLE_MS);
    car_motor_set_direction(channel, direction);
    bsp_delay_ms(CAR_MOTOR_DIRECTION_SETTLE_MS);

    for (duty = CAR_MOTOR_RAMP_STEP_PERCENT; duty < safe_target;
         duty = (uint8_t) (duty + CAR_MOTOR_RAMP_STEP_PERCENT)) {
        car_motor_set_duty(channel, duty);
        bsp_delay_ms(CAR_MOTOR_RAMP_STEP_DELAY_MS);
    }
    car_motor_set_duty(channel, safe_target);
}

void car_motor_run_both(car_motor_direction_t direction_a,
    car_motor_direction_t direction_b, uint8_t target_duty_percent)
{
    uint8_t duty;
    uint8_t safe_target = car_motor_clamp_duty(target_duty_percent);

    car_motor_stop_all();
    bsp_delay_ms(CAR_MOTOR_DIRECTION_SETTLE_MS);
    car_motor_set_direction(CAR_MOTOR_A, direction_a);
    car_motor_set_direction(CAR_MOTOR_B, direction_b);
    bsp_delay_ms(CAR_MOTOR_DIRECTION_SETTLE_MS);

    for (duty = CAR_MOTOR_RAMP_STEP_PERCENT; duty < safe_target;
         duty = (uint8_t) (duty + CAR_MOTOR_RAMP_STEP_PERCENT)) {
        car_motor_set_duty(CAR_MOTOR_A, duty);
        car_motor_set_duty(CAR_MOTOR_B, duty);
        bsp_delay_ms(CAR_MOTOR_RAMP_STEP_DELAY_MS);
    }
    car_motor_set_duty(CAR_MOTOR_A, safe_target);
    car_motor_set_duty(CAR_MOTOR_B, safe_target);
}

void car_motor_init(void)
{
#if BOARD_HAS_CAR_MOTOR
    car_motor_stop_all();
    DL_TimerA_startCounter(BOARD_MOTOR_PWM_INST);
#endif
}
