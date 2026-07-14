#ifndef USER_BSP_BSP_UART_H_
#define USER_BSP_BSP_UART_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void bsp_uart0_init(void);
void bsp_uart0_write_byte(uint8_t data);
void bsp_uart0_write(const uint8_t *data, size_t length);
void bsp_uart0_write_string(const char *str);
bool bsp_uart0_read_byte(uint8_t *data);
size_t bsp_uart0_read(uint8_t *data, size_t length);
void uart0_write_string(const char *str);
int uart0_printf(const char *fmt, ...);
int bsp_uart0_is_available(void);

void bsp_uart_k230_init(void);
void bsp_uart_k230_write_byte(uint8_t data);
void bsp_uart_k230_write(const uint8_t *data, size_t length);
void bsp_uart_k230_write_string(const char *str);
bool bsp_uart_k230_read_byte(uint8_t *data);
size_t bsp_uart_k230_read(uint8_t *data, size_t length);
int bsp_uart_k230_is_available(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_BSP_BSP_UART_H_ */
