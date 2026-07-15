#ifndef USER_BSP_BSP_I2C_H_
#define USER_BSP_BSP_I2C_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void bsp_i2c_oled_init(void);
bool bsp_i2c_oled_is_available(void);
bool bsp_i2c_oled_write(uint8_t addr7, const uint8_t *data, size_t length);
bool bsp_i2c_oled_probe(uint8_t addr7);

void bsp_i2c_mpu_init(void);
bool bsp_i2c_mpu_is_available(void);
bool bsp_i2c_mpu_write(uint8_t addr7, uint8_t reg, uint8_t data);
bool bsp_i2c_mpu_read(uint8_t addr7, uint8_t reg, uint8_t *data, size_t length);

#endif /* USER_BSP_BSP_I2C_H_ */
