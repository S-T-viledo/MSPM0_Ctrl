#include "user/app/app_sensor_test.h"

#include "user/bsp/bsp_adc.h"
#include "user/bsp/bsp_spi.h"
#include "user/bsp/bsp_timer.h"
#include "user/bsp/bsp_uart.h"
#include "user/modules/light_sensor.h"
#include "user/modules/w25qxx.h"

#define APP_FLASH_TEST_ADDRESS      (0x000000UL)

static bool app_flash_id_looks_valid(uint8_t manufacturer_id)
{
    return (manufacturer_id == 0xEFU) || /* Winbond */
           (manufacturer_id == 0xC8U) || /* GigaDevice */
           (manufacturer_id == 0xA1U) || /* FM */
           (manufacturer_id == 0x20U);   /* Micron/Numonyx */
}

static void app_print_adc_debug(void)
{
    /*
    bsp_adc_debug_t adc;

    bsp_adc_light_get_debug(&adc);
    uart0_printf("adc dbg av=%u to=%u inst=0x%08lX sample=%u memres=%lu\n",
        adc.available ? 1U : 0U,
        adc.timeout ? 1U : 0U,
        (unsigned long) adc.inst_addr,
        adc.sample,
        (unsigned long) adc.memres0);
    uart0_printf("adc reg ctl0=0x%08lX ctl1=0x%08lX ctl2=0x%08lX scomp0=%lu\n",
        (unsigned long) adc.ctl0,
        (unsigned long) adc.ctl1,
        (unsigned long) adc.ctl2,
        (unsigned long) adc.scomp0);
    uart0_printf("adc reg mem=%lu memctl=0x%08lX status=0x%08lX ris=0x%08lX\n",
        (unsigned long) adc.mem,
        (unsigned long) adc.memctl0,
        (unsigned long) adc.status,
        (unsigned long) adc.ris);
    uart0_printf("adc reg iidx=0x%08lX\n",
        (unsigned long) adc.iidx);*/
}

static void app_print_flash_info(void)
{
    w25qxx_info_t info = w25qxx_read_info();
    bsp_spi0_flash_cs_debug_t cs_debug;
    bsp_spi0_bus_debug_t bus_debug;
    bsp_spi0_miso_pull_debug_t miso_pull;
    uint32_t bb_mode0;
    uint32_t bb_mode1;
    uint32_t bb_mode2;
    uint32_t bb_mode3;
    uint8_t loop_a5;
    uint8_t loop_3c;

    uart0_printf("flash spi=%u cs=%u jedec=%02X%02X%02X\n",
        info.spi_available ? 1U : 0U,
        info.cs_available ? 1U : 0U,
        info.manufacturer_id,
        info.memory_type,
        info.capacity_id);
    uart0_printf("flash status1=0x%02X legacy=%02X%02X\n",
        info.status1,
        info.legacy_manufacturer_id,
        info.legacy_device_id);
    uart0_printf("flash bb jedec=%02X%02X%02X\n",
        info.bitbang_manufacturer_id,
        info.bitbang_memory_type,
        info.bitbang_capacity_id);
    uart0_printf("flash cs level=%u\n",
        bsp_spi0_flash_cs_read_level() ? 1U : 0U);
    uart0_printf("flash cs self=0x%02X\n",
        bsp_spi0_flash_cs_self_test());
    bsp_spi0_flash_cs_get_debug(&cs_debug);
    uart0_printf("flash cs dbg av=%u din=%u dout=%u doe=%u pin=0x%08lX\n",
        cs_debug.available ? 1U : 0U,
        cs_debug.din_high ? 1U : 0U,
        cs_debug.dout_high ? 1U : 0U,
        cs_debug.doe_enabled ? 1U : 0U,
        (unsigned long) cs_debug.pin_mask);
    uart0_printf("flash cs reg din=0x%08lX dout=0x%08lX doe=0x%08lX\n",
        (unsigned long) cs_debug.din_reg,
        (unsigned long) cs_debug.dout_reg,
        (unsigned long) cs_debug.doe_reg);

    bsp_spi0_get_bus_debug(&bus_debug);
    uart0_printf("spi pins av=%u sclk %u/%u/%u mosi %u/%u/%u miso %u/%u/%u\n",
        bus_debug.available ? 1U : 0U,
        bus_debug.sclk_din ? 1U : 0U,
        bus_debug.sclk_dout ? 1U : 0U,
        bus_debug.sclk_doe ? 1U : 0U,
        bus_debug.mosi_din ? 1U : 0U,
        bus_debug.mosi_dout ? 1U : 0U,
        bus_debug.mosi_doe ? 1U : 0U,
        bus_debug.miso_din ? 1U : 0U,
        bus_debug.miso_dout ? 1U : 0U,
        bus_debug.miso_doe ? 1U : 0U);
    uart0_printf("spi regs A din=0x%08lX dout=0x%08lX doe=0x%08lX\n",
        (unsigned long) bus_debug.port_a_din,
        (unsigned long) bus_debug.port_a_dout,
        (unsigned long) bus_debug.port_a_doe);
    uart0_printf("spi regs B din=0x%08lX dout=0x%08lX doe=0x%08lX\n",
        (unsigned long) bus_debug.port_b_din,
        (unsigned long) bus_debug.port_b_dout,
        (unsigned long) bus_debug.port_b_doe);

    bsp_spi0_miso_pull_debug(&miso_pull);
    uart0_printf("spi miso pull av=%u down=%u up=%u\n",
        miso_pull.available ? 1U : 0U,
        miso_pull.pull_down_read_high ? 1U : 0U,
        miso_pull.pull_up_read_high ? 1U : 0U);

    loop_a5 = bsp_spi0_flash_bitbang_loopback_byte(0xA5U);
    loop_3c = bsp_spi0_flash_bitbang_loopback_byte(0x3CU);
    uart0_printf("spi loopback txA5=%02X tx3C=%02X\n", loop_a5, loop_3c);

    bb_mode0 = w25qxx_read_jedec_id_bitbang_mode(false, false);
    bb_mode1 = w25qxx_read_jedec_id_bitbang_mode(false, true);
    bb_mode2 = w25qxx_read_jedec_id_bitbang_mode(true, false);
    bb_mode3 = w25qxx_read_jedec_id_bitbang_mode(true, true);
    uart0_printf("flash bb modes m0=%06lX m1=%06lX m2=%06lX m3=%06lX\n",
        (unsigned long) bb_mode0,
        (unsigned long) bb_mode1,
        (unsigned long) bb_mode2,
        (unsigned long) bb_mode3);

    if (!app_flash_id_looks_valid(info.manufacturer_id)) {
        uart0_write_string("flash id invalid, skip rw test\n");
        return;
    }

    uart0_printf("flash rw test: %s\n",
        w25qxx_self_test(APP_FLASH_TEST_ADDRESS) ? "ok" : "fail");
}

void app_sensor_test_init(void)
{
    light_sensor_init();
    w25qxx_init();

    uart0_write_string("\nMSPM0 sensor test start\n");
    /*uart0_printf("light ao=%u do=%u\n",
        bsp_adc_light_is_available() ? 1U : 0U,
        light_sensor_read().do_available ? 1U : 0U);
    */
    app_print_adc_debug();
    app_print_flash_info();
}

void app_sensor_test_task(void)
{
    /*
    static unsigned int tick = 0U;
    light_sensor_data_t light = light_sensor_read();
    
    uart0_printf("light raw=%u pct=%u%% do=%u valid=%u tick=%u\n",
        light.raw,
        light.percent,
        light.digital_active ? 1U : 0U,
        light.analog_valid ? 1U : 0U,
        tick);

    if ((tick % 4U) == 0U) {
        app_print_adc_debug();
    }

    tick++;
    bsp_delay_ms(500U);
    */
}
