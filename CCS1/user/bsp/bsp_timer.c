#include "user/bsp/bsp_timer.h"

#include "user/board/board_config.h"

void bsp_timer_init(void)
{
}

void bsp_delay_cycles(uint32_t cycles)
{
    delay_cycles(cycles);
}

void bsp_delay_us(uint32_t us)
{
    while (us > 0U) {
        delay_cycles(BOARD_CPUCLK_HZ / 1000000UL);
        us--;
    }
}

void bsp_delay_ms(uint32_t ms)
{
    while (ms > 0U) {
        bsp_delay_us(1000U);
        ms--;
    }
}
