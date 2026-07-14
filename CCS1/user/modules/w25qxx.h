#ifndef USER_MODULES_W25QXX_H_
#define USER_MODULES_W25QXX_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define W25QXX_PAGE_SIZE       (256U)
#define W25QXX_SECTOR_SIZE     (4096UL)

typedef struct {
    bool spi_available;
    bool cs_available;
    uint8_t manufacturer_id;
    uint8_t memory_type;
    uint8_t capacity_id;
    uint8_t status1;
    uint8_t legacy_manufacturer_id;
    uint8_t legacy_device_id;
    uint8_t bitbang_manufacturer_id;
    uint8_t bitbang_memory_type;
    uint8_t bitbang_capacity_id;
} w25qxx_info_t;

void w25qxx_init(void);
bool w25qxx_is_available(void);
w25qxx_info_t w25qxx_read_info(void);
uint32_t w25qxx_read_jedec_id(void);
uint32_t w25qxx_read_jedec_id_bitbang(void);
uint32_t w25qxx_read_jedec_id_bitbang_mode(
    bool clock_polarity_high, bool clock_phase_second);
uint16_t w25qxx_read_legacy_id(void);
uint8_t w25qxx_read_status1(void);
bool w25qxx_wait_busy(uint32_t timeout);
bool w25qxx_read(uint32_t address, uint8_t *data, size_t length);
bool w25qxx_page_program(uint32_t address, const uint8_t *data, size_t length);
bool w25qxx_sector_erase(uint32_t address);
bool w25qxx_self_test(uint32_t address);

#ifdef __cplusplus
}
#endif

#endif /* USER_MODULES_W25QXX_H_ */
