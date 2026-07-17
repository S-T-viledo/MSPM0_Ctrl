#include "user/modules/encoder.h"

#include <stdbool.h>

#include "ti_msp_dl_config.h"

#define ENCODER_UPDATE_MS    (5U)

static volatile int32_t g_enc_left_count;
static int32_t g_enc_left_prev;
static int32_t g_enc_right_prev;
static int32_t g_enc_left_rpm;
static int32_t g_enc_right_rpm;
static uint8_t g_left_a_last;

void encoder_init(void)
{
    g_enc_left_count = 0L;
    g_enc_left_prev  = 0L;
    g_enc_right_prev = 0L;
    g_enc_left_rpm   = 0L;
    g_enc_right_rpm  = 0L;

    g_left_a_last = (DL_GPIO_readPins(GPIO_QEI_RIGHT_MOTOR_PORT,
        GPIO_QEI_RIGHT_MOTOR_ENCODER_A_PIN) &
        GPIO_QEI_RIGHT_MOTOR_ENCODER_A_PIN) != 0U ? 1U : 0U;

    QEI_LEFT_MOTOR_INST->COUNTERREGS.CTRCTL |= 1U;
}

void encoder_poll_left(void)
{
    uint8_t a = (DL_GPIO_readPins(GPIO_QEI_RIGHT_MOTOR_PORT,
        GPIO_QEI_RIGHT_MOTOR_ENCODER_A_PIN) &
        GPIO_QEI_RIGHT_MOTOR_ENCODER_A_PIN) != 0U ? 1U : 0U;
    uint8_t b = (DL_GPIO_readPins(GPIO_QEI_RIGHT_MOTOR_PORT,
        GPIO_QEI_RIGHT_MOTOR_ENCODER_B_PIN) &
        GPIO_QEI_RIGHT_MOTOR_ENCODER_B_PIN) != 0U ? 1U : 0U;

    if ((a ^ g_left_a_last) != 0U) {
        g_enc_left_count += ((a ^ b) != 0U) ? 1L : -1L;
        g_left_a_last = a;
    }
}

int32_t encoder_get_left(void)
{
    return -g_enc_left_count * 2L;
}

int32_t encoder_get_right(void)
{
    int32_t raw = (int32_t) (QEI_LEFT_MOTOR_INST->COUNTERREGS.CTR & 0xFFFFU);
    if (raw > 32767L) {
        raw -= 65536L;
    }
    return raw;
}

void encoder_reset_left(void)
{
    g_enc_left_count = 0L;
}

void encoder_reset_right(void)
{
    QEI_LEFT_MOTOR_INST->COUNTERREGS.CTR = 0U;
}

void encoder_reset_both(void)
{
    encoder_reset_left();
    encoder_reset_right();
}

int32_t encoder_get_left_rpm(void)
{
    return g_enc_left_rpm;
}

int32_t encoder_get_right_rpm(void)
{
    return g_enc_right_rpm;
}

void encoder_update(void)
{
    int32_t left_now, right_now;
    int32_t left_delta, right_delta;

    left_now  = encoder_get_left();
    right_now = encoder_get_right();

    left_delta  = left_now  - g_enc_left_prev;
    right_delta = right_now - g_enc_right_prev;

    g_enc_left_prev  = left_now;
    g_enc_right_prev = right_now;

    g_enc_left_rpm = (left_delta * 60000L) /
        ((int32_t) ENCODER_COUNTS_PER_REV * (int32_t) ENCODER_UPDATE_MS);

    g_enc_right_rpm = (right_delta * 60000L) /
        ((int32_t) ENCODER_COUNTS_PER_REV * (int32_t) ENCODER_UPDATE_MS);
}
