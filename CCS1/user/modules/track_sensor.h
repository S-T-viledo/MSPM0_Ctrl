#ifndef USER_MODULES_TRACK_SENSOR_H_
#define USER_MODULES_TRACK_SENSOR_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TRACK_SENSOR_TOTAL_CHANNELS     (8U)
#define TRACK_SENSOR_USED_CHANNELS      (6U)
#define TRACK_SENSOR_ADC_MAX            (4095U)
#define TRACK_SENSOR_POSITION_INVALID   (32767)

typedef struct {
    uint16_t threshold;
    bool line_is_low;
} track_sensor_config_t;

typedef struct {
    bool available;
    bool valid;
    uint16_t raw[TRACK_SENSOR_TOTAL_CHANNELS];
    uint16_t used_raw[TRACK_SENSOR_USED_CHANNELS];
    uint8_t active_mask;
    uint8_t active_count;
    int16_t position;
} track_sensor_data_t;

void track_sensor_init(void);
void track_sensor_set_config(const track_sensor_config_t *config);
track_sensor_config_t track_sensor_get_config(void);
bool track_sensor_is_available(void);
bool track_sensor_read_all(uint16_t raw[TRACK_SENSOR_TOTAL_CHANNELS]);
track_sensor_data_t track_sensor_read(void);
uint8_t track_sensor_raw_to_mask(const uint16_t raw[TRACK_SENSOR_USED_CHANNELS]);
int16_t track_sensor_calc_position(const uint16_t raw[TRACK_SENSOR_USED_CHANNELS]);
void track_sensor_debug_select_channel(uint8_t channel);
uint8_t track_sensor_debug_read_select_mask(void);
bool track_sensor_debug_read_channel(uint8_t channel, uint16_t *raw);

#ifdef __cplusplus
}
#endif

#endif /* USER_MODULES_TRACK_SENSOR_H_ */
