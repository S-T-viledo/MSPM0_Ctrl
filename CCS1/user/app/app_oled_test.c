#include "user/app/app_oled_test.h"

#include <stdbool.h>
#include <stdint.h>

#include "user/bsp/bsp_gpio.h"
#include "user/bsp/bsp_timer.h"
#include "user/bsp/bsp_uart.h"
#include "user/modules/oled.h"

static uint32_t g_tick;
static bool g_oled_ok;

static void draw_static_screen(void)
{
    oled_clear();
    oled_draw_rect(0U, 0U, OLED_WIDTH, OLED_HEIGHT, true);
    oled_draw_string(8U, 8U, "MSPM0 OLED");
    oled_draw_string(8U, 22U, "I2C0 0X3C");
    oled_draw_string(8U, 36U, "SCL PA31");
    oled_draw_string(8U, 50U, "SDA PA28");
}

void app_oled_test_init(void)
{
    bsp_uart0_write_string("\r\nMSPM0 OLED test start\r\n");
    bsp_uart0_write_string("OLED I2C0 SCL=PA31 SDA=PA28 ADDR=0x3C/0x3D\r\n");

    g_oled_ok = oled_init();
    if (g_oled_ok) {
        draw_static_screen();
        g_oled_ok = oled_display();
        bsp_uart0_write_string("OLED init ok\r\n");
    } else {
        bsp_uart0_write_string("OLED init fail\r\n");
    }
}

void app_oled_test_task(void)
{
    uint8_t x;

    if (!g_oled_ok) {
        bsp_led1_toggle();
        bsp_delay_ms(200U);
        return;
    }

    draw_static_screen();
    x = (uint8_t) (8U + ((g_tick % 28U) * 4U));
    oled_draw_hline(8U, 61U, 112U, false);
    oled_draw_rect(x, 58U, 8U, 5U, true);
    g_oled_ok = oled_display();

    bsp_led1_toggle();
    g_tick++;
    bsp_delay_ms(500U);
}
