#include "user/modules/car_motor.h"

#include <stdbool.h>

#include "ti_msp_dl_config.h"
#include "user/board/board_config.h"
#include "user/bsp/bsp_timer.h"

#define CAR_MOTOR_PWM_PERIOD_COUNTS      (3200U)
#define CAR_MOTOR_PWM_LOAD_VALUE         (CAR_MOTOR_PWM_PERIOD_COUNTS - 1U)

static car_motor_direction_t g_direction[2] = {CAR_MOTOR_STOP, CAR_MOTOR_STOP};

volatile uint8_t g_car_motor_requested_duty_a;
volatile uint8_t g_car_motor_requested_duty_b;
volatile uint8_t g_car_motor_applied_duty_a;
volatile uint8_t g_car_motor_applied_duty_b;
volatile uint32_t g_car_motor_written_cc_a;
volatile uint32_t g_car_motor_written_cc_b;

static uint8_t clamp_duty(uint8_t d)
{
    return (d > CAR_MOTOR_MAX_DUTY_PERCENT) ? CAR_MOTOR_MAX_DUTY_PERCENT : d;
}

static DL_TIMER_CC_INDEX cc_idx(car_motor_channel_t ch)
{
    return (ch == CAR_MOTOR_A) ? BOARD_MOTOR_PWMA_CC_INDEX
                               : BOARD_MOTOR_PWMB_CC_INDEX;
}

static void set_pins(car_motor_channel_t ch, uint32_t in1, uint32_t in2)
{
    GPIO_Regs *port = (ch == CAR_MOTOR_A) ?
        BOARD_MOTOR_AIN1_PORT : BOARD_MOTOR_BIN1_PORT;
    DL_GPIO_clearPins(port, in1 | in2);
}

static void set_dir_pins(car_motor_channel_t ch, car_motor_direction_t dir)
{
    uint32_t in1, in2;
    GPIO_Regs *port;

    if (ch == CAR_MOTOR_A) {
        port = BOARD_MOTOR_AIN1_PORT;
        in1  = BOARD_MOTOR_AIN1_PIN;
        in2  = BOARD_MOTOR_AIN2_PIN;
    } else {
        port = BOARD_MOTOR_BIN1_PORT;
        in1  = BOARD_MOTOR_BIN1_PIN;
        in2  = BOARD_MOTOR_BIN2_PIN;
    }

    DL_GPIO_clearPins(port, in1 | in2);

    if (dir == CAR_MOTOR_FORWARD) {
        DL_GPIO_setPins(port, in1);
    } else if (dir == CAR_MOTOR_REVERSE) {
        DL_GPIO_setPins(port, in2);
    } else if (dir == CAR_MOTOR_BRAKE) {
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
    uint32_t ticks;
    uint8_t safe;

    if ((channel != CAR_MOTOR_A) && (channel != CAR_MOTOR_B)) return;

    safe  = clamp_duty(duty_percent);
    ticks = ((uint32_t) CAR_MOTOR_PWM_PERIOD_COUNTS * safe) / 100U;

    DL_TimerA_setCaptureCompareValue(BOARD_MOTOR_PWM_INST, ticks,
        cc_idx(channel));

    if (channel == CAR_MOTOR_A) {
        g_car_motor_requested_duty_a = duty_percent;
        g_car_motor_applied_duty_a   = safe;
        g_car_motor_written_cc_a     = ticks;
    } else {
        g_car_motor_requested_duty_b = duty_percent;
        g_car_motor_applied_duty_b   = safe;
        g_car_motor_written_cc_b     = ticks;
    }
#else
    (void) channel; (void) duty_percent;
#endif
}

void car_motor_stop(car_motor_channel_t channel)
{
#if BOARD_HAS_CAR_MOTOR
    car_motor_set_duty(channel, 0U);
    set_dir_pins(channel, CAR_MOTOR_STOP);
    g_direction[channel] = CAR_MOTOR_STOP;
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
    if ((channel != CAR_MOTOR_A) && (channel != CAR_MOTOR_B)) return;

    if (direction == CAR_MOTOR_STOP) {
        car_motor_stop(channel);
        return;
    }

    car_motor_set_duty(channel, 0U);

    if ((g_direction[channel] != CAR_MOTOR_STOP) &&
        (g_direction[channel] != direction)) {
        set_dir_pins(channel, CAR_MOTOR_STOP);
        bsp_delay_ms(CAR_MOTOR_DIRECTION_SETTLE_MS);
    }

    set_dir_pins(channel, direction);
    g_direction[channel] = direction;
#else
    (void) channel; (void) direction;
#endif
}

void car_motor_run(car_motor_channel_t channel,
    car_motor_direction_t direction, uint8_t duty_percent)
{
    car_motor_set_direction(channel, direction);
    bsp_delay_ms(CAR_MOTOR_DIRECTION_SETTLE_MS);
    car_motor_set_duty(channel, duty_percent);
}

void car_motor_ramp_to_duty(car_motor_channel_t channel,
    car_motor_direction_t direction, uint8_t target)
{
    uint8_t safe = clamp_duty(target);
    uint8_t d;

    car_motor_stop(channel);
    bsp_delay_ms(CAR_MOTOR_DIRECTION_SETTLE_MS);
    car_motor_set_direction(channel, direction);
    bsp_delay_ms(CAR_MOTOR_DIRECTION_SETTLE_MS);

    for (d = CAR_MOTOR_RAMP_STEP_PERCENT; d < safe;
         d = (uint8_t) (d + CAR_MOTOR_RAMP_STEP_PERCENT)) {
        car_motor_set_duty(channel, d);
        bsp_delay_ms(CAR_MOTOR_RAMP_STEP_DELAY_MS);
    }
    car_motor_set_duty(channel, safe);
}

void car_motor_run_both(car_motor_direction_t dir_a,
    car_motor_direction_t dir_b, uint8_t target)
{
    uint8_t safe = clamp_duty(target);
    uint8_t d;

    car_motor_stop_all();
    bsp_delay_ms(CAR_MOTOR_DIRECTION_SETTLE_MS);
    car_motor_set_direction(CAR_MOTOR_A, dir_a);
    car_motor_set_direction(CAR_MOTOR_B, dir_b);
    bsp_delay_ms(CAR_MOTOR_DIRECTION_SETTLE_MS);

    for (d = CAR_MOTOR_RAMP_STEP_PERCENT; d < safe;
         d = (uint8_t) (d + CAR_MOTOR_RAMP_STEP_PERCENT)) {
        car_motor_set_duty(CAR_MOTOR_A, d);
        car_motor_set_duty(CAR_MOTOR_B, d);
        bsp_delay_ms(CAR_MOTOR_RAMP_STEP_DELAY_MS);
    }
    car_motor_set_duty(CAR_MOTOR_A, safe);
    car_motor_set_duty(CAR_MOTOR_B, safe);
}

void car_motor_init(void)
{
#if BOARD_HAS_CAR_MOTOR
    /* STBY high to enable motor driver */
    DL_GPIO_initDigitalOutput(GPIO_GRP_KEY_KEY4_IOMUX);
    DL_GPIO_setPins(GPIO_GRP_KEY_PORT, GPIO_GRP_KEY_KEY4_PIN);

    /* enable both PWM channels - never disable them */
    DL_TimerA_setCCPOutputDisabled(BOARD_MOTOR_PWM_INST,
        DL_TIMER_CCP_DIS_OUT_SET_BY_OCTL,   /* motor A */
        DL_TIMER_CCP_DIS_OUT_SET_BY_OCTL);  /* motor B */

    car_motor_stop_all();
    DL_TimerA_startCounter(BOARD_MOTOR_PWM_INST);
#endif
}
