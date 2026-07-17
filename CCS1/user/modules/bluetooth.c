#include "user/modules/bluetooth.h"

#include "ti_msp_dl_config.h"

#define BLUETOOTH_TX_BUFFER_SIZE    (128U)

#if defined(UART_HC05_INST)
#define BLUETOOTH_UART_INST          UART_HC05_INST
#elif defined(UART_Bluetooth_INST)
#define BLUETOOTH_UART_INST          UART_Bluetooth_INST
#elif defined(UART_reserved_INST)
/* Compatibility with SysConfig output generated before UART_HC05 was renamed. */
#define BLUETOOTH_UART_INST          UART_reserved_INST
#else
#error "UART3 HC-05 instance is missing from SysConfig output"
#endif

static uint8_t g_tx_buffer[BLUETOOTH_TX_BUFFER_SIZE];
static uint8_t g_tx_head;
static uint8_t g_tx_tail;

static void bluetooth_tx_service(void)
{
    while ((g_tx_head != g_tx_tail) &&
           !DL_UART_Main_isTXFIFOFull(BLUETOOTH_UART_INST)) {
        DL_UART_Main_transmitData(BLUETOOTH_UART_INST, g_tx_buffer[g_tx_tail]);
        g_tx_tail = (uint8_t) ((g_tx_tail + 1U) % BLUETOOTH_TX_BUFFER_SIZE);
    }
}

void bluetooth_init(void)
{
    /* UART3 is initialized by SYSCFG_DL_init(). Flush stale RX data. */
    g_tx_head = 0U;
    g_tx_tail = 0U;
    while (!DL_UART_Main_isRXFIFOEmpty(BLUETOOTH_UART_INST)) {
        (void) DL_UART_Main_receiveData(BLUETOOTH_UART_INST);
    }
}

void bluetooth_send_char(uint8_t data)
{
    uint8_t next_head;

    bluetooth_tx_service();
    next_head = (uint8_t) ((g_tx_head + 1U) % BLUETOOTH_TX_BUFFER_SIZE);
    if (next_head != g_tx_tail) {
        g_tx_buffer[g_tx_head] = data;
        g_tx_head = next_head;
    }
    bluetooth_tx_service();
}

void bluetooth_send_string(char *str)
{
    if (str == (char *) 0) {
        return;
    }

    while (*str != '\0') {
        bluetooth_send_char((uint8_t) *str++);
    }
}

uint8_t bluetooth_available(void)
{
    bluetooth_tx_service();
    return (DL_UART_Main_isRXFIFOEmpty(BLUETOOTH_UART_INST) ? 0U : 1U);
}

uint8_t bluetooth_read_char(void)
{
    bluetooth_tx_service();
    if (bluetooth_available() == 0U) {
        return 0U;
    }

    return (uint8_t) DL_UART_Main_receiveData(BLUETOOTH_UART_INST);
}
