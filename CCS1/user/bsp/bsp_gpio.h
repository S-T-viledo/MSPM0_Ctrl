#ifndef USER_BSP_BSP_GPIO_H_
#define USER_BSP_BSP_GPIO_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void bsp_gpio_init(void);

void bsp_led1_on(void);
void bsp_led1_off(void);
void bsp_led1_toggle(void);
void bsp_led1_write(bool on);
bool bsp_led1_is_available(void);

bool bsp_key1_is_pressed(void);
bool bsp_key1_is_available(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_BSP_BSP_GPIO_H_ */
