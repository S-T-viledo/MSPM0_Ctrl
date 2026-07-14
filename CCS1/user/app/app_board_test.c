#include "user/app/app_board_test.h"

#include "user/bsp/bsp_gpio.h"
#include "user/bsp/bsp_timer.h"
#include "user/bsp/bsp_uart.h"
#include "user/modules/key.h"

void app_board_test_init(void)
{
    unsigned int index;

    key_init();

    for (index = 0U; index < 3U; index++) {
        bsp_led1_on();
        bsp_delay_ms(120U);
        bsp_led1_off();
        bsp_delay_ms(120U);
    }
}

void app_board_test_task(void)
{
    static unsigned int tick = 0U;
    bool pressed = key_is_pressed();

    bsp_led1_toggle();

    if ((tick % 10U) == 0U) {
        uart0_printf("tick=%u key=%u\n", tick, pressed ? 1U : 0U);
    }

    tick++;

    if (pressed) {
        bsp_delay_ms(120U);
    } else {
        bsp_delay_ms(500U);
    }
}
