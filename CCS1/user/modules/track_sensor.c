#include "user/modules/track_sensor.h"

#include "user/board/board_config.h"
#include "user/bsp/bsp_adc.h"
#include "user/bsp/bsp_timer.h"

#define TRACK_SENSOR_SETTLE_US          (80U)
#define TRACK_SENSOR_TOTAL_SAMPLES      (5U)
#define TRACK_SENSOR_DISCARD_SAMPLES    (3U)
#define TRACK_SENSOR_DEFAULT_THRESH     (2000U)

#if BOARD_HAS_TRACK_ADC
static const bsp_adc_channel_t g_track_adc_channel = BOARD_TRACK_ADC_CHANNEL;
#endif

static const uint8_t g_track_used_channels[TRACK_SENSOR_USED_CHANNELS] = {
    1U, 2U, 3U, 4U, 5U, 6U
};

static const int16_t g_track_weights[TRACK_SENSOR_USED_CHANNELS] = {
    -2500, -1500, -500, 500, 1500, 2500
};

static track_sensor_config_t g_track_config = {
    TRACK_SENSOR_DEFAULT_THRESH,
    true
};

static bool track_sensor_sample_selected(uint16_t *raw)
{
#if BOARD_HAS_TRACK_ADC
    uint8_t sample;
    uint16_t one_raw;
    uint32_t sum = 0UL;
    uint8_t valid_samples = 0U;

    if (raw == 0) {
        return false;
    }

    for (sample = 0U; sample < TRACK_SENSOR_TOTAL_SAMPLES; sample++) {
        if (!bsp_adc_read_channel(&g_track_adc_channel, &one_raw)) {
            return false;
        }

        if (sample >= TRACK_SENSOR_DISCARD_SAMPLES) {
            sum += one_raw;
            valid_samples++;
        }
    }

    if (valid_samples == 0U) {
        return false;
    }

    *raw = (uint16_t) (sum / valid_samples);
    return true;
#else
    (void) raw;
    return false;
#endif
}

static void track_sensor_select_channel(uint8_t channel)
{
#if BOARD_HAS_TRACK_SELECT
    if ((channel & 0x01U) != 0U) {
        DL_GPIO_setPins(BOARD_TRACK_S0_PORT, BOARD_TRACK_S0_PIN);
    } else {
        DL_GPIO_clearPins(BOARD_TRACK_S0_PORT, BOARD_TRACK_S0_PIN);
    }

    if ((channel & 0x02U) != 0U) {
        DL_GPIO_setPins(BOARD_TRACK_S1_PORT, BOARD_TRACK_S1_PIN);
    } else {
        DL_GPIO_clearPins(BOARD_TRACK_S1_PORT, BOARD_TRACK_S1_PIN);
    }

    if ((channel & 0x04U) != 0U) {
        DL_GPIO_setPins(BOARD_TRACK_S2_PORT, BOARD_TRACK_S2_PIN);
    } else {
        DL_GPIO_clearPins(BOARD_TRACK_S2_PORT, BOARD_TRACK_S2_PIN);
    }
#else
    (void) channel;
#endif
}

void track_sensor_debug_select_channel(uint8_t channel)
{
    track_sensor_select_channel(channel);
}

uint8_t track_sensor_debug_read_select_mask(void)
{
    uint8_t mask = 0U;

#if BOARD_HAS_TRACK_SELECT
    if ((BOARD_TRACK_S0_PORT->DOUT31_0 & BOARD_TRACK_S0_PIN) != 0U) {
        mask |= 0x01U;
    }
    if ((BOARD_TRACK_S1_PORT->DOUT31_0 & BOARD_TRACK_S1_PIN) != 0U) {
        mask |= 0x02U;
    }
    if ((BOARD_TRACK_S2_PORT->DOUT31_0 & BOARD_TRACK_S2_PIN) != 0U) {
        mask |= 0x04U;
    }
#endif

    return mask;
}

bool track_sensor_debug_read_channel(uint8_t channel, uint16_t *raw)
{
    if ((channel >= TRACK_SENSOR_TOTAL_CHANNELS) || !track_sensor_is_available()) {
        return false;
    }

    track_sensor_select_channel(channel);
    bsp_delay_ms(10U);
    return track_sensor_sample_selected(raw);
}

static bool track_sensor_is_line(uint16_t raw)
{
    if (g_track_config.line_is_low) {
        return (raw <= g_track_config.threshold);
    }

    return (raw >= g_track_config.threshold);
}

void track_sensor_init(void)
{
#if BOARD_HAS_TRACK_ADC
    bsp_adc_init_single_sample(g_track_adc_channel.inst, 50U);
#endif
    track_sensor_select_channel(0U);
}

void track_sensor_set_config(const track_sensor_config_t *config)
{
    if (config == 0) {
        return;
    }

    g_track_config = *config;
}

track_sensor_config_t track_sensor_get_config(void)
{
    return g_track_config;
}

bool track_sensor_is_available(void)
{
#if BOARD_HAS_TRACK_ADC && BOARD_HAS_TRACK_SELECT
    return bsp_adc_is_channel_available(&g_track_adc_channel);
#else
    return false;
#endif
}

bool track_sensor_read_all(uint16_t raw[TRACK_SENSOR_TOTAL_CHANNELS])
{
    uint8_t index;
    if ((raw == 0) || !track_sensor_is_available()) {
        return false;
    }

    for (index = 0U; index < TRACK_SENSOR_TOTAL_CHANNELS; index++) {
        track_sensor_select_channel(index);
        bsp_delay_us(TRACK_SENSOR_SETTLE_US);

        if (!track_sensor_sample_selected(&raw[index])) {
            return false;
        }
    }

    return true;
}

uint8_t track_sensor_raw_to_mask(const uint16_t raw[TRACK_SENSOR_USED_CHANNELS])
{
    uint8_t index;
    uint8_t mask = 0U;

    if (raw == 0) {
        return 0U;
    }

    for (index = 0U; index < TRACK_SENSOR_USED_CHANNELS; index++) {
        if (track_sensor_is_line(raw[index])) {
            mask |= (uint8_t) (1U << index);
        }
    }

    return mask;
}

int16_t track_sensor_calc_position(const uint16_t raw[TRACK_SENSOR_USED_CHANNELS])
{
    uint8_t index;
    uint32_t strength;
    uint32_t sum = 0UL;
    int32_t weighted = 0L;

    if (raw == 0) {
        return TRACK_SENSOR_POSITION_INVALID;
    }

    for (index = 0U; index < TRACK_SENSOR_USED_CHANNELS; index++) {
        if (g_track_config.line_is_low) {
            strength = (raw[index] >= TRACK_SENSOR_ADC_MAX) ?
                0UL : (uint32_t) (TRACK_SENSOR_ADC_MAX - raw[index]);
        } else {
            strength = raw[index];
        }

        sum += strength;
        weighted += (int32_t) strength * (int32_t) g_track_weights[index];
    }

    if (sum == 0UL) {
        return TRACK_SENSOR_POSITION_INVALID;
    }

    return (int16_t) (weighted / (int32_t) sum);
}

track_sensor_data_t track_sensor_read(void)
{
    track_sensor_data_t data;
    uint8_t index;

    data.available = track_sensor_is_available();
    data.valid = false;
    data.active_mask = 0U;
    data.active_count = 0U;
    data.position = TRACK_SENSOR_POSITION_INVALID;

    for (index = 0U; index < TRACK_SENSOR_TOTAL_CHANNELS; index++) {
        data.raw[index] = 0U;
    }
    for (index = 0U; index < TRACK_SENSOR_USED_CHANNELS; index++) {
        data.used_raw[index] = 0U;
    }

    if (!track_sensor_read_all(data.raw)) {
        return data;
    }

    for (index = 0U; index < TRACK_SENSOR_USED_CHANNELS; index++) {
        data.used_raw[index] = data.raw[g_track_used_channels[index]];
    }

    data.active_mask = track_sensor_raw_to_mask(data.used_raw);
    for (index = 0U; index < TRACK_SENSOR_USED_CHANNELS; index++) {
        if ((data.active_mask & (uint8_t) (1U << index)) != 0U) {
            data.active_count++;
        }
    }
    data.position = track_sensor_calc_position(data.used_raw);
    data.valid = true;

    return data;
}
