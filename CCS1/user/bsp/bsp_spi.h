#ifndef USER_BSP_BSP_SPI_H_
#define USER_BSP_BSP_SPI_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void bsp_spi0_init(void);
bool bsp_spi0_is_available(void);
void bsp_spi0_drain_rx(void);
bool bsp_spi0_transfer_byte(uint8_t tx, uint8_t *rx);
bool bsp_spi0_transfer(const uint8_t *tx, uint8_t *rx, size_t length);
bool bsp_spi0_flash_bitbang_is_available(void);
void bsp_spi0_flash_bitbang_prepare(void);
uint8_t bsp_spi0_flash_bitbang_transfer_byte(uint8_t tx);
uint8_t bsp_spi0_flash_bitbang_transfer_byte_mode(
    uint8_t tx, bool clock_polarity_high, bool clock_phase_second);
uint8_t bsp_spi0_flash_bitbang_loopback_byte(uint8_t tx);

typedef struct {
    bool available;
    bool pull_down_read_high;
    bool pull_up_read_high;
} bsp_spi0_miso_pull_debug_t;

typedef struct {
    bool available;
    bool sclk_din;
    bool sclk_dout;
    bool sclk_doe;
    bool mosi_din;
    bool mosi_dout;
    bool mosi_doe;
    bool miso_din;
    bool miso_dout;
    bool miso_doe;
    uint32_t port_a_din;
    uint32_t port_a_dout;
    uint32_t port_a_doe;
    uint32_t port_b_din;
    uint32_t port_b_dout;
    uint32_t port_b_doe;
} bsp_spi0_bus_debug_t;

typedef struct {
    bool available;
    bool din_high;
    bool dout_high;
    bool doe_enabled;
    uint32_t pin_mask;
    uint32_t din_reg;
    uint32_t dout_reg;
    uint32_t doe_reg;
} bsp_spi0_flash_cs_debug_t;

void bsp_spi0_flash_cs_low(void);
void bsp_spi0_flash_cs_high(void);
bool bsp_spi0_flash_cs_read_level(void);
bool bsp_spi0_flash_cs_read_output_latch(void);
bool bsp_spi0_flash_cs_is_output_enabled(void);
void bsp_spi0_flash_cs_get_debug(bsp_spi0_flash_cs_debug_t *debug);
void bsp_spi0_get_bus_debug(bsp_spi0_bus_debug_t *debug);
void bsp_spi0_miso_pull_debug(bsp_spi0_miso_pull_debug_t *debug);
uint8_t bsp_spi0_flash_cs_self_test(void);
bool bsp_spi0_flash_cs_is_available(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_BSP_BSP_SPI_H_ */
