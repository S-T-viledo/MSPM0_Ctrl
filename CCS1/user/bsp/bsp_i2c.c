#include "user/bsp/bsp_i2c.h"

#include "ti_msp_dl_config.h"
#include "user/board/board_config.h"

#if BOARD_HAS_I2C_OLED

#define BSP_I2C_OLED_TIMEOUT       (100000UL)
#define BSP_I2C_OLED_FIFO_BYTES    (8U)

static bool g_oled_i2c_ready;

static bool i2c_wait_idle(I2C_Regs *i2c)
{
    uint32_t timeout = BSP_I2C_OLED_TIMEOUT;

    while ((DL_I2C_getControllerStatus(i2c) & DL_I2C_CONTROLLER_STATUS_IDLE) == 0U) {
        if (timeout-- == 0U) {
            return false;
        }
    }

    return true;
}

static bool i2c_wait_not_busy(I2C_Regs *i2c)
{
    uint32_t timeout = BSP_I2C_OLED_TIMEOUT;

    while ((DL_I2C_getControllerStatus(i2c) & DL_I2C_CONTROLLER_STATUS_BUSY) != 0U) {
        if (timeout-- == 0U) {
            return false;
        }
    }

    return true;
}

static bool i2c_wait_tx_space(I2C_Regs *i2c)
{
    uint32_t timeout = BSP_I2C_OLED_TIMEOUT;

    while (DL_I2C_isControllerTXFIFOFull(i2c)) {
        if (timeout-- == 0U) {
            return false;
        }
    }

    return true;
}

static bool i2c_has_error(I2C_Regs *i2c)
{
    return ((DL_I2C_getControllerStatus(i2c) & DL_I2C_CONTROLLER_STATUS_ERROR) != 0U);
}

static void i2c_recover(I2C_Regs *i2c)
{
    DL_I2C_resetControllerTransfer(i2c);
    DL_I2C_flushControllerTXFIFO(i2c);
    DL_I2C_flushControllerRXFIFO(i2c);
}

void bsp_i2c_oled_init(void)
{
    DL_I2C_resetControllerTransfer(BOARD_I2C_OLED_INST);

    if (DL_I2C_getTimerPeriod(BOARD_I2C_OLED_INST) == 0U) {
        DL_I2C_setTimerPeriod(BOARD_I2C_OLED_INST, 7U);
    }

    DL_I2C_setControllerTXFIFOThreshold(BOARD_I2C_OLED_INST, DL_I2C_TX_FIFO_LEVEL_EMPTY);
    DL_I2C_setControllerRXFIFOThreshold(BOARD_I2C_OLED_INST, DL_I2C_RX_FIFO_LEVEL_BYTES_1);
    DL_I2C_enableControllerClockStretching(BOARD_I2C_OLED_INST);

    if (!DL_I2C_isControllerEnabled(BOARD_I2C_OLED_INST)) {
        DL_I2C_enableController(BOARD_I2C_OLED_INST);
    }

    g_oled_i2c_ready = true;
}

bool bsp_i2c_oled_is_available(void)
{
    return g_oled_i2c_ready;
}

bool bsp_i2c_oled_write(uint8_t addr7, const uint8_t *data, size_t length)
{
    size_t written = 0U;
    size_t preload;

    if (!g_oled_i2c_ready || (data == NULL) || (length == 0U) || (length > 0x0FFFU)) {
        return false;
    }

    if (!i2c_wait_idle(BOARD_I2C_OLED_INST)) {
        return false;
    }

    DL_I2C_flushControllerTXFIFO(BOARD_I2C_OLED_INST);
    DL_I2C_flushControllerRXFIFO(BOARD_I2C_OLED_INST);

    preload = (length < BSP_I2C_OLED_FIFO_BYTES) ? length : BSP_I2C_OLED_FIFO_BYTES;
    written = DL_I2C_fillControllerTXFIFO(
        BOARD_I2C_OLED_INST, data, (uint16_t) preload);

    DL_I2C_startControllerTransfer(BOARD_I2C_OLED_INST, addr7,
        DL_I2C_CONTROLLER_DIRECTION_TX, (uint16_t) length);

    delay_cycles(128);

    while (written < length) {
        if (i2c_has_error(BOARD_I2C_OLED_INST) ||
            !i2c_wait_tx_space(BOARD_I2C_OLED_INST)) {
            i2c_recover(BOARD_I2C_OLED_INST);
            return false;
        }

        DL_I2C_transmitControllerData(BOARD_I2C_OLED_INST, data[written]);
        written++;
    }

    if (!i2c_wait_not_busy(BOARD_I2C_OLED_INST)) {
        i2c_recover(BOARD_I2C_OLED_INST);
        return false;
    }

    if (i2c_has_error(BOARD_I2C_OLED_INST)) {
        i2c_recover(BOARD_I2C_OLED_INST);
        return false;
    }

    return true;
}

bool bsp_i2c_oled_probe(uint8_t addr7)
{
    uint8_t probe = 0x00U;

    return bsp_i2c_oled_write(addr7, &probe, 1U);
}

#else

void bsp_i2c_oled_init(void)
{
}

bool bsp_i2c_oled_is_available(void)
{
    return false;
}

bool bsp_i2c_oled_write(uint8_t addr7, const uint8_t *data, size_t length)
{
    (void) addr7;
    (void) data;
    (void) length;
    return false;
}

bool bsp_i2c_oled_probe(uint8_t addr7)
{
    (void) addr7;
    return false;
}

#endif
