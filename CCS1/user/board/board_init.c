#include "user/board/board_init.h"

#include "user/bsp/bsp_gpio.h"
#include "user/bsp/bsp_adc.h"
#include "user/bsp/bsp_i2c.h"
#include "user/bsp/bsp_spi.h"
#include "user/bsp/bsp_timer.h"
#include "user/bsp/bsp_uart.h"

void board_init(void)
{
    bsp_gpio_init();
    bsp_adc_init();
    bsp_i2c_oled_init();
    bsp_i2c_mpu_init();
    bsp_spi0_init();
    bsp_timer_init();
    bsp_uart0_init();
    bsp_uart_k230_init();
}
