#include "user/bsp/bsp_gpio.h"

#include "user/board/board_config.h"

void bsp_gpio_init(void)
{
    bsp_led1_off();
}

void bsp_led1_on(void)
{
#if BOARD_HAS_LED1
#if BOARD_LED1_ACTIVE_HIGH
    DL_GPIO_setPins(BOARD_LED1_PORT, BOARD_LED1_PIN);
#else
    DL_GPIO_clearPins(BOARD_LED1_PORT, BOARD_LED1_PIN);
#endif
#endif
}

void bsp_led1_off(void)
{
#if BOARD_HAS_LED1
#if BOARD_LED1_ACTIVE_HIGH
    DL_GPIO_clearPins(BOARD_LED1_PORT, BOARD_LED1_PIN);
#else
    DL_GPIO_setPins(BOARD_LED1_PORT, BOARD_LED1_PIN);
#endif
#endif
}

void bsp_led1_toggle(void)
{
#if BOARD_HAS_LED1
    DL_GPIO_togglePins(BOARD_LED1_PORT, BOARD_LED1_PIN);
#endif
}

void bsp_led1_write(bool on)
{
    if (on) {
        bsp_led1_on();
    } else {
        bsp_led1_off();
    }
}

bool bsp_led1_is_available(void)
{
#if BOARD_HAS_LED1
    return true;
#else
    return false;
#endif
}

bool bsp_key1_is_pressed(void)
{
#if BOARD_HAS_KEY1
    bool high = (DL_GPIO_readPins(BOARD_KEY1_PORT, BOARD_KEY1_PIN) != 0U);
#if BOARD_KEY1_ACTIVE_LOW
    return !high;
#else
    return high;
#endif
#else
    return false;
#endif
}

bool bsp_key1_is_available(void)
{
#if BOARD_HAS_KEY1
    return true;
#else
    return false;
#endif
}
