#ifndef USER_BSP_BSP_TIMER_H_
#define USER_BSP_BSP_TIMER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void bsp_timer_init(void);
void bsp_delay_cycles(uint32_t cycles);
void bsp_delay_us(uint32_t us);
void bsp_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* USER_BSP_BSP_TIMER_H_ */
