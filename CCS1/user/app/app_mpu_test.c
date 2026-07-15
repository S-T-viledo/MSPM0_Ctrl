#include "user/app/app_mpu_test.h"

#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "user/bsp/bsp_gpio.h"
#include "user/bsp/bsp_timer.h"
#include "user/bsp/bsp_uart.h"
#include "user/modules/oled.h"
#include "user/modules/mpu6050.h"

#define APP_MPU_GYRO_LSB       (131.0f)
#define APP_MPU_OLED_DIV       (4U)

#define APP_MPU_CD_TICKS       (200U)
#define APP_MPU_WIN_TICKS      (1500U)

typedef enum {
    CAL_COUNTDOWN,
    CAL_ROTATE,
    CAL_DONE
} cal_phase_t;

static bool        g_oled_ok;
static uint32_t    g_tick;
static float       g_angle_z;
static uint32_t    g_last_systick;

static cal_phase_t g_cal_phase;
static uint32_t    g_cal_start_tick;
static float       g_cal_accum;

static void systick_setup(void)
{
    SysTick->LOAD = 0x00FFFFFFUL;
    SysTick->VAL  = 0U;
    SysTick->CTRL = (1UL << 2U) | (1UL << 0U);
}

static float systick_dt_s(void)
{
    uint32_t now    = SysTick->VAL;
    uint32_t reload = SysTick->LOAD;
    uint32_t elapsed;

    if (now <= g_last_systick) {
        elapsed = g_last_systick - now;
    } else {
        elapsed = (reload - now) + g_last_systick + 1UL;
    }
    g_last_systick = now;

    return (float) elapsed / (float) CPUCLK_FREQ;
}

static void u16_to_4(char *out, uint16_t v)
{
    if (v > 9999U) v = 9999U;
    out[0] = (char) ('0' + (v / 1000U) % 10U);
    out[1] = (char) ('0' + (v / 100U)  % 10U);
    out[2] = (char) ('0' + (v / 10U)   % 10U);
    out[3] = (char) ('0' + (v % 10U));
}

static void draw(uint8_t y, const char *label, int16_t val)
{
    char line[18];
    uint8_t i = 0U;
    while (*label) line[i++] = *label++;
    if (val < 0) { line[i++] = '-'; val = (int16_t) (-val); }
    else         { line[i++] = '+'; }
    u16_to_4(&line[i], (uint16_t) val);
    i += 4U;
    line[i] = '\0';
    oled_draw_string(0U, y, line);
}

static void draw_dps(uint8_t y, float val)
{
    char line[18];
    int16_t v;
    uint8_t i = 0U;
    line[i++] = 'D'; line[i++] = ' ';
    v = (int16_t) (val * 10.0f);
    if (v < 0) { line[i++] = '-'; v = (int16_t) (-v); }
    else       { line[i++] = '+'; }
    u16_to_4(&line[i], (uint16_t) v);
    i += 4U;
    line[i++] = ' '; line[i++] = 'd'; line[i++] = 'p'; line[i++] = 's';
    line[i] = '\0';
    oled_draw_string(0U, y, line);
}

void app_mpu_test_init(void)
{
    bsp_uart0_write_string("\r\nMSPM0 MPU6050 test\r\n");

    bsp_i2c_mpu_init();
    mpu6050_init();

    g_oled_ok = oled_init();
    g_angle_z = 0.0f;

    systick_setup();
    g_last_systick = SysTick->VAL;

    mpu6050_set_scale(0.9882f);
    g_cal_phase      = CAL_DONE;
    g_cal_start_tick = 0U;
    g_cal_accum      = 0.0f;
}

void app_mpu_test_task(void)
{
    mpu6050_raw_t raw;
    float gyro_z_dps;
    float dt;
    bool  ready;

    dt    = systick_dt_s();
    ready = mpu6050_is_ready();

    if (ready && mpu6050_read_raw(&raw)) {
        gyro_z_dps = (((float) raw.gyro_z / APP_MPU_GYRO_LSB) - mpu6050_get_gyro_z_bias())
                     * mpu6050_get_scale();
    } else {
        gyro_z_dps = 0.0f;
    }

    switch (g_cal_phase) {

    case CAL_COUNTDOWN:
        if ((g_tick - g_cal_start_tick) >= APP_MPU_CD_TICKS) {
            g_cal_phase      = CAL_ROTATE;
            g_cal_start_tick = g_tick;
            g_cal_accum      = 0.0f;
            g_angle_z        = 0.0f;
        }
        break;

    case CAL_ROTATE:
        g_cal_accum += gyro_z_dps * dt;
        g_angle_z   += gyro_z_dps * dt;
        if ((g_tick - g_cal_start_tick) >= APP_MPU_WIN_TICKS) {
            float abs360 = g_cal_accum;
            if (abs360 < 0.0f) abs360 = -abs360;
            if (abs360 > 180.0f) {
                float s = 360.0f / abs360 * mpu6050_get_scale();
                if (s > 0.5f && s < 2.0f) {
                    mpu6050_set_scale(s);
                    uart0_printf("scale=%.4f\r\n", (double) s);
                }
            }
            g_cal_phase = CAL_DONE;
            g_angle_z   = 0.0f;
        }
        break;

    case CAL_DONE:
    default:
        g_angle_z += gyro_z_dps * dt;
        break;
    }

    if (g_oled_ok && ((g_tick % APP_MPU_OLED_DIV) == 0U)) {
        oled_clear();

        if (!ready) {
            oled_draw_string(0U, 0U,  "MPU6050 FAIL");
            oled_draw_string(0U, 16U, "SDA=PA10 SCL=PA11");
        } else if (g_cal_phase == CAL_COUNTDOWN) {
            uint32_t rem = (APP_MPU_CD_TICKS - (g_tick - g_cal_start_tick) + 99U) / 100U;
            char line[18];
            oled_draw_string(0U, 0U,  "ROTATE 360 IN");
            line[0] = ' '; line[1] = ' '; line[2] = ' ';
            line[3] = (char) ('0' + (rem % 10U));
            line[4] = ' '; line[5] = ' '; line[6] = ' '; line[7] = '\0';
            oled_draw_string(0U, 24U, line);
            oled_draw_string(0U, 48U, "KEEP STILL NOW");
        } else if (g_cal_phase == CAL_ROTATE) {
            uint32_t rem = (APP_MPU_WIN_TICKS - (g_tick - g_cal_start_tick) + 99U) / 100U;
            char line[18];
            oled_draw_string(0U, 0U,  "ROTATING...");
            line[0] = ' '; line[1] = ' '; line[2] = ' ';
            line[3] = (char) ('0' + (rem % 10U));
            line[4] = 's'; line[5] = ' '; line[6] = ' '; line[7] = '\0';
            oled_draw_string(0U, 16U, line);
            draw(32U, "ANG:", (int16_t) g_cal_accum);
        } else {
            oled_draw_string(0U, 0U, "MPU6050 OK");
            draw(12U, "R:", raw.gyro_z);
            draw_dps(24U, gyro_z_dps);
            draw(36U, "A:", (int16_t) g_angle_z);
        }

        g_oled_ok = oled_display();
    }

    bsp_led1_toggle();
    g_tick++;
    bsp_delay_ms(10U);
}
