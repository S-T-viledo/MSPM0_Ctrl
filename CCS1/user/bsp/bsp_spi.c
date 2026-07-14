#include "user/bsp/bsp_spi.h"

#include "user/board/board_config.h"

#define BSP_SPI_TIMEOUT     (100000U)
#define BSP_SPI_DUMMY       (0xFFU)
#define BSP_SPI_DRAIN_MAX   (8U)
#define BSP_SPI_BB_DELAY    (20U)

#if BOARD_HAS_SPI0_GPIO_PINS
static void bsp_spi0_bitbang_delay(void)
{
    volatile uint32_t count = BSP_SPI_BB_DELAY;

    while (count > 0U) {
        count--;
    }
}
#endif

static void bsp_spi0_flash_bitbang_init_pins(void)
{
#if BOARD_HAS_SPI0_GPIO_PINS && BOARD_HAS_W25QXX_CS
    DL_GPIO_initDigitalOutput(BOARD_SPI0_SCLK_IOMUX);
    DL_GPIO_initDigitalOutput(BOARD_SPI0_MOSI_IOMUX);
    DL_GPIO_initDigitalInput(BOARD_SPI0_MISO_IOMUX);

    DL_GPIO_clearPins(BOARD_SPI0_SCLK_PORT, BOARD_SPI0_SCLK_PIN);
    DL_GPIO_setPins(BOARD_SPI0_MOSI_PORT, BOARD_SPI0_MOSI_PIN);
    DL_GPIO_enableOutput(BOARD_SPI0_SCLK_PORT, BOARD_SPI0_SCLK_PIN);
    DL_GPIO_enableOutput(BOARD_SPI0_MOSI_PORT, BOARD_SPI0_MOSI_PIN);
    bsp_spi0_flash_cs_high();
#endif
}

static void bsp_spi0_init_peripheral_pins(void)
{
#if BOARD_HAS_SPI0_GPIO_PINS
    DL_GPIO_initPeripheralOutputFunction(
        BOARD_SPI0_SCLK_IOMUX, GPIO_SPI_0_IOMUX_SCLK_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        BOARD_SPI0_MOSI_IOMUX, GPIO_SPI_0_IOMUX_PICO_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        BOARD_SPI0_MISO_IOMUX, GPIO_SPI_0_IOMUX_POCI_FUNC);
#endif
}

static void bsp_spi0_flash_cs_init_output(void)
{
#if BOARD_HAS_W25QXX_CS
#if defined(BOARD_W25QXX_CS_IOMUX)
    DL_GPIO_initDigitalOutput(BOARD_W25QXX_CS_IOMUX);
#endif
    DL_GPIO_setPins(BOARD_W25QXX_CS_PORT, BOARD_W25QXX_CS_PIN);
    DL_GPIO_enableOutput(BOARD_W25QXX_CS_PORT, BOARD_W25QXX_CS_PIN);
#endif
}

void bsp_spi0_init(void)
{
    bsp_spi0_init_peripheral_pins();
    bsp_spi0_flash_cs_init_output();
    bsp_spi0_drain_rx();
}

bool bsp_spi0_is_available(void)
{
#if BOARD_HAS_SPI0
    return true;
#else
    return false;
#endif
}

void bsp_spi0_drain_rx(void)
{
#if BOARD_HAS_SPI0
    uint8_t dummy[BSP_SPI_DRAIN_MAX];

    (void) DL_SPI_drainRXFIFO8(BOARD_SPI0_INST, dummy, BSP_SPI_DRAIN_MAX);
#endif
}

bool bsp_spi0_transfer_byte(uint8_t tx, uint8_t *rx)
{
#if BOARD_HAS_SPI0
    uint32_t timeout = BSP_SPI_TIMEOUT;
    uint8_t value = 0xFFU;

    bsp_spi0_init_peripheral_pins();

    while (DL_SPI_isBusy(BOARD_SPI0_INST) && (timeout > 0U)) {
        timeout--;
    }
    if (timeout == 0U) {
        return false;
    }

    DL_SPI_transmitData8(BOARD_SPI0_INST, tx);

    timeout = BSP_SPI_TIMEOUT;
    while (DL_SPI_isBusy(BOARD_SPI0_INST) && (timeout > 0U)) {
        timeout--;
    }
    if (timeout == 0U) {
        return false;
    }

    timeout = BSP_SPI_TIMEOUT;
    while (DL_SPI_isRXFIFOEmpty(BOARD_SPI0_INST) && (timeout > 0U)) {
        timeout--;
    }
    if (timeout == 0U) {
        return false;
    }

    value = DL_SPI_receiveDataBlocking8(BOARD_SPI0_INST);
    if (rx != 0) {
        *rx = value;
    }
    return true;
#else
    (void) tx;
    (void) rx;
    return false;
#endif
}

bool bsp_spi0_transfer(const uint8_t *tx, uint8_t *rx, size_t length)
{
    size_t index;
    uint8_t tx_byte;
    uint8_t rx_byte;

    for (index = 0U; index < length; index++) {
        tx_byte = (tx != 0) ? tx[index] : BSP_SPI_DUMMY;
        if (!bsp_spi0_transfer_byte(tx_byte, &rx_byte)) {
            return false;
        }
        if (rx != 0) {
            rx[index] = rx_byte;
        }
    }

    return true;
}

bool bsp_spi0_flash_bitbang_is_available(void)
{
#if BOARD_HAS_SPI0_GPIO_PINS && BOARD_HAS_W25QXX_CS
    return true;
#else
    return false;
#endif
}

void bsp_spi0_flash_bitbang_prepare(void)
{
    bsp_spi0_flash_bitbang_init_pins();
}

uint8_t bsp_spi0_flash_bitbang_transfer_byte(uint8_t tx)
{
    return bsp_spi0_flash_bitbang_transfer_byte_mode(tx, false, false);
}

uint8_t bsp_spi0_flash_bitbang_loopback_byte(uint8_t tx)
{
#if BOARD_HAS_SPI0_GPIO_PINS
    bsp_spi0_flash_bitbang_init_pins();
    return bsp_spi0_flash_bitbang_transfer_byte_mode(tx, false, false);
#else
    (void) tx;
    return 0U;
#endif
}

uint8_t bsp_spi0_flash_bitbang_transfer_byte_mode(
    uint8_t tx, bool clock_polarity_high, bool clock_phase_second)
{
#if BOARD_HAS_SPI0_GPIO_PINS && BOARD_HAS_W25QXX_CS
    uint8_t rx = 0U;
    uint8_t bit;
    bool active_high = !clock_polarity_high;

    if (clock_polarity_high) {
        DL_GPIO_setPins(BOARD_SPI0_SCLK_PORT, BOARD_SPI0_SCLK_PIN);
    } else {
        DL_GPIO_clearPins(BOARD_SPI0_SCLK_PORT, BOARD_SPI0_SCLK_PIN);
    }

    for (bit = 0U; bit < 8U; bit++) {
        if (!clock_phase_second) {
            if ((tx & 0x80U) != 0U) {
                DL_GPIO_setPins(BOARD_SPI0_MOSI_PORT, BOARD_SPI0_MOSI_PIN);
            } else {
                DL_GPIO_clearPins(BOARD_SPI0_MOSI_PORT, BOARD_SPI0_MOSI_PIN);
            }
        }

        bsp_spi0_bitbang_delay();
        if (active_high) {
            DL_GPIO_setPins(BOARD_SPI0_SCLK_PORT, BOARD_SPI0_SCLK_PIN);
        } else {
            DL_GPIO_clearPins(BOARD_SPI0_SCLK_PORT, BOARD_SPI0_SCLK_PIN);
        }
        bsp_spi0_bitbang_delay();

        if (clock_phase_second) {
            if ((tx & 0x80U) != 0U) {
                DL_GPIO_setPins(BOARD_SPI0_MOSI_PORT, BOARD_SPI0_MOSI_PIN);
            } else {
                DL_GPIO_clearPins(BOARD_SPI0_MOSI_PORT, BOARD_SPI0_MOSI_PIN);
            }
            bsp_spi0_bitbang_delay();
        }

        rx <<= 1;
        if ((!clock_phase_second) &&
            (DL_GPIO_readPins(BOARD_SPI0_MISO_PORT, BOARD_SPI0_MISO_PIN) != 0U)) {
            rx |= 0x01U;
        }

        if (active_high) {
            DL_GPIO_clearPins(BOARD_SPI0_SCLK_PORT, BOARD_SPI0_SCLK_PIN);
        } else {
            DL_GPIO_setPins(BOARD_SPI0_SCLK_PORT, BOARD_SPI0_SCLK_PIN);
        }
        bsp_spi0_bitbang_delay();

        if (clock_phase_second &&
            (DL_GPIO_readPins(BOARD_SPI0_MISO_PORT, BOARD_SPI0_MISO_PIN) != 0U)) {
            rx |= 0x01U;
        }

        tx <<= 1;
        bsp_spi0_bitbang_delay();
    }

    return rx;
#else
    (void) tx;
    return 0U;
#endif
}

void bsp_spi0_flash_cs_low(void)
{
#if BOARD_HAS_W25QXX_CS
    bsp_spi0_drain_rx();
    DL_GPIO_clearPins(BOARD_W25QXX_CS_PORT, BOARD_W25QXX_CS_PIN);
#endif
}

void bsp_spi0_flash_cs_high(void)
{
#if BOARD_HAS_W25QXX_CS
    DL_GPIO_enableOutput(BOARD_W25QXX_CS_PORT, BOARD_W25QXX_CS_PIN);
    DL_GPIO_setPins(BOARD_W25QXX_CS_PORT, BOARD_W25QXX_CS_PIN);
#endif
}

bool bsp_spi0_flash_cs_read_level(void)
{
#if BOARD_HAS_W25QXX_CS
    return (DL_GPIO_readPins(BOARD_W25QXX_CS_PORT, BOARD_W25QXX_CS_PIN) != 0U);
#else
    return false;
#endif
}

bool bsp_spi0_flash_cs_read_output_latch(void)
{
#if BOARD_HAS_W25QXX_CS
    return ((BOARD_W25QXX_CS_PORT->DOUT31_0 & BOARD_W25QXX_CS_PIN) != 0U);
#else
    return false;
#endif
}

bool bsp_spi0_flash_cs_is_output_enabled(void)
{
#if BOARD_HAS_W25QXX_CS
    return ((BOARD_W25QXX_CS_PORT->DOE31_0 & BOARD_W25QXX_CS_PIN) != 0U);
#else
    return false;
#endif
}

void bsp_spi0_flash_cs_get_debug(bsp_spi0_flash_cs_debug_t *debug)
{
    if (debug == 0) {
        return;
    }

    debug->available = false;
    debug->din_high = false;
    debug->dout_high = false;
    debug->doe_enabled = false;
    debug->pin_mask = 0U;
    debug->din_reg = 0U;
    debug->dout_reg = 0U;
    debug->doe_reg = 0U;

#if BOARD_HAS_W25QXX_CS
    debug->available = true;
    debug->pin_mask = BOARD_W25QXX_CS_PIN;
    debug->din_reg = BOARD_W25QXX_CS_PORT->DIN31_0;
    debug->dout_reg = BOARD_W25QXX_CS_PORT->DOUT31_0;
    debug->doe_reg = BOARD_W25QXX_CS_PORT->DOE31_0;
    debug->din_high = ((debug->din_reg & debug->pin_mask) != 0U);
    debug->dout_high = ((debug->dout_reg & debug->pin_mask) != 0U);
    debug->doe_enabled = ((debug->doe_reg & debug->pin_mask) != 0U);
#endif
}

void bsp_spi0_get_bus_debug(bsp_spi0_bus_debug_t *debug)
{
    if (debug == 0) {
        return;
    }

    debug->available = false;
    debug->sclk_din = false;
    debug->sclk_dout = false;
    debug->sclk_doe = false;
    debug->mosi_din = false;
    debug->mosi_dout = false;
    debug->mosi_doe = false;
    debug->miso_din = false;
    debug->miso_dout = false;
    debug->miso_doe = false;
    debug->port_a_din = 0U;
    debug->port_a_dout = 0U;
    debug->port_a_doe = 0U;
    debug->port_b_din = 0U;
    debug->port_b_dout = 0U;
    debug->port_b_doe = 0U;

#if BOARD_HAS_SPI0_GPIO_PINS
    debug->available = true;
    debug->port_a_din = GPIOA->DIN31_0;
    debug->port_a_dout = GPIOA->DOUT31_0;
    debug->port_a_doe = GPIOA->DOE31_0;
#if defined(GPIOB)
    debug->port_b_din = GPIOB->DIN31_0;
    debug->port_b_dout = GPIOB->DOUT31_0;
    debug->port_b_doe = GPIOB->DOE31_0;
#endif

    debug->sclk_din =
        ((BOARD_SPI0_SCLK_PORT->DIN31_0 & BOARD_SPI0_SCLK_PIN) != 0U);
    debug->sclk_dout =
        ((BOARD_SPI0_SCLK_PORT->DOUT31_0 & BOARD_SPI0_SCLK_PIN) != 0U);
    debug->sclk_doe =
        ((BOARD_SPI0_SCLK_PORT->DOE31_0 & BOARD_SPI0_SCLK_PIN) != 0U);
    debug->mosi_din =
        ((BOARD_SPI0_MOSI_PORT->DIN31_0 & BOARD_SPI0_MOSI_PIN) != 0U);
    debug->mosi_dout =
        ((BOARD_SPI0_MOSI_PORT->DOUT31_0 & BOARD_SPI0_MOSI_PIN) != 0U);
    debug->mosi_doe =
        ((BOARD_SPI0_MOSI_PORT->DOE31_0 & BOARD_SPI0_MOSI_PIN) != 0U);
    debug->miso_din =
        ((BOARD_SPI0_MISO_PORT->DIN31_0 & BOARD_SPI0_MISO_PIN) != 0U);
    debug->miso_dout =
        ((BOARD_SPI0_MISO_PORT->DOUT31_0 & BOARD_SPI0_MISO_PIN) != 0U);
    debug->miso_doe =
        ((BOARD_SPI0_MISO_PORT->DOE31_0 & BOARD_SPI0_MISO_PIN) != 0U);
#endif
}

void bsp_spi0_miso_pull_debug(bsp_spi0_miso_pull_debug_t *debug)
{
    if (debug == 0) {
        return;
    }

    debug->available = false;
    debug->pull_down_read_high = false;
    debug->pull_up_read_high = false;

#if BOARD_HAS_SPI0_GPIO_PINS
    debug->available = true;

    DL_GPIO_initDigitalInputFeatures(BOARD_SPI0_MISO_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    bsp_spi0_bitbang_delay();
    debug->pull_down_read_high =
        (DL_GPIO_readPins(BOARD_SPI0_MISO_PORT, BOARD_SPI0_MISO_PIN) != 0U);

    DL_GPIO_initDigitalInputFeatures(BOARD_SPI0_MISO_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    bsp_spi0_bitbang_delay();
    debug->pull_up_read_high =
        (DL_GPIO_readPins(BOARD_SPI0_MISO_PORT, BOARD_SPI0_MISO_PIN) != 0U);

    DL_GPIO_initDigitalInput(BOARD_SPI0_MISO_IOMUX);
#endif
}

uint8_t bsp_spi0_flash_cs_self_test(void)
{
    uint8_t result = 0U;

#if BOARD_HAS_W25QXX_CS
    bsp_spi0_flash_cs_init_output();
    bsp_spi0_flash_cs_high();
    if (bsp_spi0_flash_cs_read_output_latch()) {
        result |= 0x04U;
    }

    bsp_spi0_flash_cs_low();
    if (bsp_spi0_flash_cs_read_output_latch()) {
        result |= 0x02U;
    }

    bsp_spi0_flash_cs_high();
    if (bsp_spi0_flash_cs_read_output_latch()) {
        result |= 0x01U;
    }
#endif

    return result;
}

bool bsp_spi0_flash_cs_is_available(void)
{
#if BOARD_HAS_W25QXX_CS
    return true;
#else
    return false;
#endif
}
