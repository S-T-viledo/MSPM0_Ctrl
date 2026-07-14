#include "user/modules/key.h"

#include "user/bsp/bsp_gpio.h"

void key_init(void)
{
}

bool key_is_pressed(void)
{
    return bsp_key1_is_pressed();
}
