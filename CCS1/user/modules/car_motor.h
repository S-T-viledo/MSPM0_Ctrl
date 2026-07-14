#ifndef USER_MODULES_CAR_MOTOR_H_
#define USER_MODULES_CAR_MOTOR_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CAR_MOTOR_PWM_FREQUENCY_HZ       (10000U)
#define CAR_MOTOR_MAX_DUTY_PERCENT       (50U)
#define CAR_MOTOR_RAMP_STEP_PERCENT      (5U)
#define CAR_MOTOR_RAMP_STEP_DELAY_MS     (200U)
#define CAR_MOTOR_DIRECTION_SETTLE_MS    (200U)

#define CAR_MOTOR_A_REVERSED             (0U)
#define CAR_MOTOR_B_REVERSED             (0U)

typedef enum {
    CAR_MOTOR_A = 0,
    CAR_MOTOR_B
} car_motor_channel_t;

typedef enum {
    CAR_MOTOR_STOP = 0,
    CAR_MOTOR_FORWARD,
    CAR_MOTOR_REVERSE,
    CAR_MOTOR_BRAKE
} car_motor_direction_t;

void car_motor_init(void);
int car_motor_is_available(void);
void car_motor_set_direction(car_motor_channel_t channel,
    car_motor_direction_t direction);
void car_motor_set_duty(car_motor_channel_t channel, uint8_t duty_percent);
void car_motor_run(car_motor_channel_t channel,
    car_motor_direction_t direction, uint8_t duty_percent);
void car_motor_stop(car_motor_channel_t channel);
void car_motor_stop_all(void);
void car_motor_ramp_to_duty(car_motor_channel_t channel,
    car_motor_direction_t direction, uint8_t target_duty_percent);
void car_motor_run_both(car_motor_direction_t direction_a,
    car_motor_direction_t direction_b, uint8_t target_duty_percent);

#ifdef __cplusplus
}
#endif

#endif /* USER_MODULES_CAR_MOTOR_H_ */
