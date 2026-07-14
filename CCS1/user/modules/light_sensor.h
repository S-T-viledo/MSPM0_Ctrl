#ifndef USER_MODULES_LIGHT_SENSOR_H_
#define USER_MODULES_LIGHT_SENSOR_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool ao_available;
    bool do_available;
    bool analog_valid;
    uint16_t raw;
    uint8_t percent;
    bool digital_active;
} light_sensor_data_t;

void light_sensor_init(void);
bool light_sensor_read_raw(uint16_t *raw);
bool light_sensor_read_percent(uint8_t *percent);
bool light_sensor_read_digital(bool *active);
light_sensor_data_t light_sensor_read(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_MODULES_LIGHT_SENSOR_H_ */
