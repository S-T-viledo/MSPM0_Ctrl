#include "user/modules/w25qxx.h"

#include "user/bsp/bsp_spi.h"
#include "user/bsp/bsp_timer.h"

#define W25QXX_CMD_WRITE_ENABLE     (0x06U)
#define W25QXX_CMD_READ_STATUS1     (0x05U)
#define W25QXX_CMD_PAGE_PROGRAM     (0x02U)
#define W25QXX_CMD_READ_DATA        (0x03U)
#define W25QXX_CMD_SECTOR_ERASE     (0x20U)
#define W25QXX_CMD_LEGACY_ID        (0x90U)
#define W25QXX_CMD_JEDEC_ID         (0x9FU)
#define W25QXX_CMD_RELEASE_POWERDOWN (0xABU)

#define W25QXX_STATUS_BUSY          (0x01U)
#define W25QXX_DEFAULT_TIMEOUT      (500000UL)

static bool w25qxx_command_only(uint8_t command)
{
    bool ok;

    if (!w25qxx_is_available()) {
        return false;
    }

    bsp_spi0_flash_cs_low();
    ok = bsp_spi0_transfer_byte(command, 0);
    bsp_spi0_flash_cs_high();
    return ok;
}

static bool w25qxx_write_enable(void)
{
    return w25qxx_command_only(W25QXX_CMD_WRITE_ENABLE);
}

static bool w25qxx_send_address(uint32_t address)
{
    return bsp_spi0_transfer_byte((uint8_t) (address >> 16), 0) &&
           bsp_spi0_transfer_byte((uint8_t) (address >> 8), 0) &&
           bsp_spi0_transfer_byte((uint8_t) address, 0);
}

void w25qxx_init(void)
{
    bsp_spi0_flash_cs_high();
    bsp_delay_ms(1U);
    (void) w25qxx_command_only(W25QXX_CMD_RELEASE_POWERDOWN);
    bsp_delay_ms(1U);
}

bool w25qxx_is_available(void)
{
    return bsp_spi0_is_available() && bsp_spi0_flash_cs_is_available();
}

w25qxx_info_t w25qxx_read_info(void)
{
    uint32_t id = w25qxx_read_jedec_id();
    uint16_t legacy_id = w25qxx_read_legacy_id();
    w25qxx_info_t info;

    info.spi_available = bsp_spi0_is_available();
    info.cs_available = bsp_spi0_flash_cs_is_available();
    info.manufacturer_id = (uint8_t) (id >> 16);
    info.memory_type = (uint8_t) (id >> 8);
    info.capacity_id = (uint8_t) id;
    info.status1 = w25qxx_read_status1();
    info.legacy_manufacturer_id = (uint8_t) (legacy_id >> 8);
    info.legacy_device_id = (uint8_t) legacy_id;
    id = w25qxx_read_jedec_id_bitbang();
    info.bitbang_manufacturer_id = (uint8_t) (id >> 16);
    info.bitbang_memory_type = (uint8_t) (id >> 8);
    info.bitbang_capacity_id = (uint8_t) id;

    return info;
}

uint32_t w25qxx_read_jedec_id(void)
{
    uint8_t manufacturer = 0U;
    uint8_t memory_type = 0U;
    uint8_t capacity = 0U;

    if (!w25qxx_is_available()) {
        return 0U;
    }

    bsp_spi0_flash_cs_low();
    if (!bsp_spi0_transfer_byte(W25QXX_CMD_JEDEC_ID, 0) ||
        !bsp_spi0_transfer_byte(0xFFU, &manufacturer) ||
        !bsp_spi0_transfer_byte(0xFFU, &memory_type) ||
        !bsp_spi0_transfer_byte(0xFFU, &capacity)) {
        bsp_spi0_flash_cs_high();
        return 0U;
    }
    bsp_spi0_flash_cs_high();

    return ((uint32_t) manufacturer << 16) |
           ((uint32_t) memory_type << 8) |
           (uint32_t) capacity;
}

uint32_t w25qxx_read_jedec_id_bitbang(void)
{
    return w25qxx_read_jedec_id_bitbang_mode(false, false);
}

uint32_t w25qxx_read_jedec_id_bitbang_mode(
    bool clock_polarity_high, bool clock_phase_second)
{
    uint8_t manufacturer;
    uint8_t memory_type;
    uint8_t capacity;

    if (!bsp_spi0_flash_bitbang_is_available()) {
        return 0U;
    }

    bsp_spi0_flash_bitbang_prepare();
    bsp_spi0_flash_cs_low();
    (void) bsp_spi0_flash_bitbang_transfer_byte_mode(
        W25QXX_CMD_JEDEC_ID, clock_polarity_high, clock_phase_second);
    manufacturer = bsp_spi0_flash_bitbang_transfer_byte_mode(
        0xFFU, clock_polarity_high, clock_phase_second);
    memory_type = bsp_spi0_flash_bitbang_transfer_byte_mode(
        0xFFU, clock_polarity_high, clock_phase_second);
    capacity = bsp_spi0_flash_bitbang_transfer_byte_mode(
        0xFFU, clock_polarity_high, clock_phase_second);
    bsp_spi0_flash_cs_high();

    return ((uint32_t) manufacturer << 16) |
           ((uint32_t) memory_type << 8) |
           (uint32_t) capacity;
}

uint16_t w25qxx_read_legacy_id(void)
{
    uint8_t manufacturer = 0U;
    uint8_t device = 0U;

    if (!w25qxx_is_available()) {
        return 0U;
    }

    bsp_spi0_flash_cs_low();
    if (!bsp_spi0_transfer_byte(W25QXX_CMD_LEGACY_ID, 0) ||
        !bsp_spi0_transfer_byte(0x00U, 0) ||
        !bsp_spi0_transfer_byte(0x00U, 0) ||
        !bsp_spi0_transfer_byte(0x00U, 0) ||
        !bsp_spi0_transfer_byte(0xFFU, &manufacturer) ||
        !bsp_spi0_transfer_byte(0xFFU, &device)) {
        bsp_spi0_flash_cs_high();
        return 0U;
    }
    bsp_spi0_flash_cs_high();

    return ((uint16_t) manufacturer << 8) | (uint16_t) device;
}

uint8_t w25qxx_read_status1(void)
{
    uint8_t status = 0xFFU;

    if (!w25qxx_is_available()) {
        return status;
    }

    bsp_spi0_flash_cs_low();
    (void) bsp_spi0_transfer_byte(W25QXX_CMD_READ_STATUS1, 0);
    (void) bsp_spi0_transfer_byte(0xFFU, &status);
    bsp_spi0_flash_cs_high();

    return status;
}

bool w25qxx_wait_busy(uint32_t timeout)
{
    while (timeout > 0UL) {
        if ((w25qxx_read_status1() & W25QXX_STATUS_BUSY) == 0U) {
            return true;
        }
        timeout--;
    }

    return false;
}

bool w25qxx_read(uint32_t address, uint8_t *data, size_t length)
{
    bool ok;

    if ((data == 0) || !w25qxx_is_available()) {
        return false;
    }

    bsp_spi0_flash_cs_low();
    ok = bsp_spi0_transfer_byte(W25QXX_CMD_READ_DATA, 0) &&
         w25qxx_send_address(address) &&
         bsp_spi0_transfer(0, data, length);
    bsp_spi0_flash_cs_high();

    return ok;
}

bool w25qxx_page_program(uint32_t address, const uint8_t *data, size_t length)
{
    bool ok;
    size_t page_left = W25QXX_PAGE_SIZE - (address % W25QXX_PAGE_SIZE);

    if ((data == 0) || (length == 0U) || (length > page_left) ||
        !w25qxx_is_available()) {
        return false;
    }

    if (!w25qxx_wait_busy(W25QXX_DEFAULT_TIMEOUT) || !w25qxx_write_enable()) {
        return false;
    }

    bsp_spi0_flash_cs_low();
    ok = bsp_spi0_transfer_byte(W25QXX_CMD_PAGE_PROGRAM, 0) &&
         w25qxx_send_address(address) &&
         bsp_spi0_transfer(data, 0, length);
    bsp_spi0_flash_cs_high();

    if (!ok) {
        return false;
    }

    return w25qxx_wait_busy(W25QXX_DEFAULT_TIMEOUT);
}

bool w25qxx_sector_erase(uint32_t address)
{
    bool ok;

    if (!w25qxx_is_available()) {
        return false;
    }

    if (!w25qxx_wait_busy(W25QXX_DEFAULT_TIMEOUT) || !w25qxx_write_enable()) {
        return false;
    }

    address &= ~(W25QXX_SECTOR_SIZE - 1UL);

    bsp_spi0_flash_cs_low();
    ok = bsp_spi0_transfer_byte(W25QXX_CMD_SECTOR_ERASE, 0) &&
         w25qxx_send_address(address);
    bsp_spi0_flash_cs_high();

    if (!ok) {
        return false;
    }

    return w25qxx_wait_busy(W25QXX_DEFAULT_TIMEOUT * 20UL);
}

bool w25qxx_self_test(uint32_t address)
{
    static const uint8_t tx[16] = {
        0x4DU, 0x53U, 0x50U, 0x4DU, 0x30U, 0x2DU, 0x57U, 0x32U,
        0x35U, 0x51U, 0x2DU, 0x54U, 0x45U, 0x53U, 0x54U, 0x21U
    };
    uint8_t rx[sizeof(tx)];
    size_t index;

    address &= ~(W25QXX_SECTOR_SIZE - 1UL);

    if (!w25qxx_sector_erase(address)) {
        return false;
    }
    bsp_delay_ms(5U);

    if (!w25qxx_page_program(address, tx, sizeof(tx))) {
        return false;
    }

    if (!w25qxx_read(address, rx, sizeof(rx))) {
        return false;
    }

    for (index = 0U; index < sizeof(tx); index++) {
        if (rx[index] != tx[index]) {
            return false;
        }
    }

    return true;
}
