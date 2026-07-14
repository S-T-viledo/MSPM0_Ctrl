#include "user/modules/light_sensor.h"

#include "user/board/board_config.h"
#include "user/bsp/bsp_adc.h"

void light_sensor_init(void)
{
}

bool light_sensor_read_raw(uint16_t *raw)
{
    return bsp_adc_light_read_raw(raw);
}

bool light_sensor_read_percent(uint8_t *percent)
{
    uint16_t raw;

    if (percent == 0) {
        return false;
    }

    if (!light_sensor_read_raw(&raw)) {
        return false;
    }

    *percent = bsp_adc_raw_to_percent(raw, BOARD_LIGHT_ADC_MAX);
    return true;
}

bool light_sensor_read_digital(bool *active)
{
#if BOARD_HAS_LIGHT_DO
    bool high;

    if (active == 0) {
        return false;
    }

    high = (DL_GPIO_readPins(BOARD_LIGHT_DO_PORT, BOARD_LIGHT_DO_PIN) != 0U);
#if BOARD_LIGHT_DO_ACTIVE_LOW
    *active = !high;
#else
    *active = high;
#endif
    return true;
#else
    (void) active;
    return false;
#endif
}

light_sensor_data_t light_sensor_read(void)
{
    light_sensor_data_t data;

    data.ao_available = bsp_adc_light_is_available();
#if BOARD_HAS_LIGHT_DO
    data.do_available = true;
#else
    data.do_available = false;
#endif
    data.raw = 0U;
    data.percent = 0U;
    data.digital_active = false;
    data.analog_valid = light_sensor_read_raw(&data.raw);
    if (data.analog_valid) {
        (void) light_sensor_read_percent(&data.percent);
    }
    (void) light_sensor_read_digital(&data.digital_active);

    return data;
}
