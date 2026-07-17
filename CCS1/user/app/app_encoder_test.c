#include "user/app/app_encoder_test.h"

#include <stdbool.h>
#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "user/bsp/bsp_gpio.h"
#include "user/bsp/bsp_timer.h"
#include "user/modules/oled.h"
#include "user/modules/encoder.h"

#define ENC_OLED_DIV      (10U)
#define ENC_RPM_INTERVAL  (20U)

static bool     g_oled_ok;
static uint32_t g_tick;

static void draw_val(uint8_t y, const char *label, int32_t val)
{
    char line[18];
    uint8_t i = 0U;
    uint32_t mag;
    while (*label) line[i++] = *label++;
    if (val < 0L) { line[i++] = '-'; mag = (uint32_t) (-val); }
    else          { line[i++] = '+'; mag = (uint32_t) val; }
    if (mag > 99999UL) mag = 99999UL;
    line[i++] = (char) ('0' + (mag / 10000UL) % 10UL);
    line[i++] = (char) ('0' + (mag / 1000UL)  % 10UL);
    line[i++] = (char) ('0' + (mag / 100UL)   % 10UL);
    line[i++] = (char) ('0' + (mag / 10UL)    % 10UL);
    line[i++] = (char) ('0' + (mag % 10UL));
    line[i] = '\0';
    oled_draw_string(0U, y, line);
}

void app_encoder_test_init(void)
{
    g_oled_ok = oled_init();
    encoder_init();
}

void app_encoder_test_task(void)
{
    int32_t enc_l, enc_r;
    int32_t rpm_l, rpm_r;
    uint32_t poll_i;

    for (poll_i = 0U; poll_i < 100U; poll_i++) {
        encoder_poll_left();
    }

    if ((g_tick % ENC_RPM_INTERVAL) == 0U) {
        encoder_update();
    }

    enc_l = encoder_get_left();
    enc_r = encoder_get_right();
    rpm_l = encoder_get_left_rpm();
    rpm_r = encoder_get_right_rpm();

    if (g_oled_ok && ((g_tick % ENC_OLED_DIV) == 0U)) {
        oled_clear();
        oled_draw_string(0U, 0U, "ENCODER TEST");
        draw_val(16U, "L:", enc_l);
        draw_val(30U, "R:", enc_r);
        draw_val(46U, "L:", rpm_l);
        draw_val(56U, "R:", rpm_r);
        g_oled_ok = oled_display();
    }

    bsp_led1_toggle();
    g_tick++;
    bsp_delay_us(200U);
}
