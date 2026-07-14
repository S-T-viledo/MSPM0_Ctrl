#include "user/bsp/bsp_uart.h"

#include <stdarg.h>
#include <stdio.h>

#include "user/board/board_config.h"

#define BSP_UART0_PRINTF_BUFFER_SIZE    (128U)
#define BSP_UART_TX_TIMEOUT             (100000U)

static void bsp_uart_write_byte_impl(UART_Regs *uart, uint8_t data)
{
    uint32_t timeout = BSP_UART_TX_TIMEOUT;

    while (DL_UART_Main_isTXFIFOFull(uart) && (timeout > 0U)) {
        timeout--;
    }

    if (timeout == 0U) {
        return;
    }

    DL_UART_Main_transmitData(uart, data);

    timeout = BSP_UART_TX_TIMEOUT;
    while (DL_UART_Main_isBusy(uart) && (timeout > 0U)) {
        timeout--;
    }
}

static void bsp_uart_write_impl(UART_Regs *uart, const uint8_t *data, size_t length)
{
    size_t index;

    if (data == NULL) {
        return;
    }

    for (index = 0U; index < length; index++) {
        bsp_uart_write_byte_impl(uart, data[index]);
    }
}

static void bsp_uart_write_string_impl(UART_Regs *uart, const char *str)
{
    if (str == NULL) {
        return;
    }

    while (*str != '\0') {
        if (*str == '\n') {
            bsp_uart_write_byte_impl(uart, (uint8_t) '\r');
        }
        bsp_uart_write_byte_impl(uart, (uint8_t) *str);
        str++;
    }
}

static bool bsp_uart_read_byte_impl(UART_Regs *uart, uint8_t *data)
{
    if (data == NULL) {
        return false;
    }

    if (DL_UART_Main_isRXFIFOEmpty(uart)) {
        return false;
    }

    *data = DL_UART_Main_receiveData(uart);
    return true;
}

static size_t bsp_uart_read_impl(UART_Regs *uart, uint8_t *data, size_t length)
{
    size_t count = 0U;

    if (data == NULL) {
        return 0U;
    }

    while ((count < length) && bsp_uart_read_byte_impl(uart, &data[count])) {
        count++;
    }

    return count;
}

void bsp_uart0_init(void)
{
}

void bsp_uart0_write_byte(uint8_t data)
{
#if BOARD_HAS_UART0
    bsp_uart_write_byte_impl(BOARD_UART0_INST, data);
#else
    (void) data;
#endif
}

void bsp_uart0_write(const uint8_t *data, size_t length)
{
#if BOARD_HAS_UART0
    bsp_uart_write_impl(BOARD_UART0_INST, data, length);
#else
    (void) data;
    (void) length;
#endif
}

void bsp_uart0_write_string(const char *str)
{
#if BOARD_HAS_UART0
    bsp_uart_write_string_impl(BOARD_UART0_INST, str);
#else
    (void) str;
#endif
}

bool bsp_uart0_read_byte(uint8_t *data)
{
#if BOARD_HAS_UART0
    return bsp_uart_read_byte_impl(BOARD_UART0_INST, data);
#else
    (void) data;
    return false;
#endif
}

size_t bsp_uart0_read(uint8_t *data, size_t length)
{
#if BOARD_HAS_UART0
    return bsp_uart_read_impl(BOARD_UART0_INST, data, length);
#else
    (void) data;
    (void) length;
    return 0U;
#endif
}

void uart0_write_string(const char *str)
{
    bsp_uart0_write_string(str);
}

int uart0_printf(const char *fmt, ...)
{
    char buffer[BSP_UART0_PRINTF_BUFFER_SIZE];
    int length;
    va_list args;

    if (fmt == NULL) {
        return 0;
    }

    va_start(args, fmt);
    length = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (length <= 0) {
        return length;
    }

    if ((size_t) length >= sizeof(buffer)) {
        length = (int) sizeof(buffer) - 1;
    }

    bsp_uart0_write((const uint8_t *) buffer, (size_t) length);
    return length;
}

int bsp_uart0_is_available(void)
{
#if BOARD_HAS_UART0
    return 1;
#else
    return 0;
#endif
}

void bsp_uart_k230_init(void)
{
}

void bsp_uart_k230_write_byte(uint8_t data)
{
#if BOARD_HAS_K230_UART
    bsp_uart_write_byte_impl(BOARD_K230_UART_INST, data);
#else
    (void) data;
#endif
}

void bsp_uart_k230_write(const uint8_t *data, size_t length)
{
#if BOARD_HAS_K230_UART
    bsp_uart_write_impl(BOARD_K230_UART_INST, data, length);
#else
    (void) data;
    (void) length;
#endif
}

void bsp_uart_k230_write_string(const char *str)
{
#if BOARD_HAS_K230_UART
    bsp_uart_write_string_impl(BOARD_K230_UART_INST, str);
#else
    (void) str;
#endif
}

bool bsp_uart_k230_read_byte(uint8_t *data)
{
#if BOARD_HAS_K230_UART
    return bsp_uart_read_byte_impl(BOARD_K230_UART_INST, data);
#else
    (void) data;
    return false;
#endif
}

size_t bsp_uart_k230_read(uint8_t *data, size_t length)
{
#if BOARD_HAS_K230_UART
    return bsp_uart_read_impl(BOARD_K230_UART_INST, data, length);
#else
    (void) data;
    (void) length;
    return 0U;
#endif
}

int bsp_uart_k230_is_available(void)
{
#if BOARD_HAS_K230_UART
    return 1;
#else
    return 0;
#endif
}
