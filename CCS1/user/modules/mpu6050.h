#ifndef USER_MODULES_MPU6050_H_
#define USER_MODULES_MPU6050_H_

#include <stdbool.h>
#include <stdint.h>

#define MPU6050_DEFAULT_ADDR        (0x68U)
#define MPU6050_GYRO_FS_250         (0U)
#define MPU6050_GYRO_FS_500         (1U)
#define MPU6050_GYRO_FS_1000        (2U)
#define MPU6050_GYRO_FS_2000        (3U)

typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    int16_t temp;
} mpu6050_raw_t;

typedef struct {
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
} mpu6050_data_t;

void mpu6050_init(void);
bool mpu6050_is_ready(void);
bool mpu6050_read_raw(mpu6050_raw_t *raw);
bool mpu6050_read_data(mpu6050_data_t *data);
float mpu6050_get_gyro_z_dps(void);
float mpu6050_get_gyro_z_bias(void);
void mpu6050_set_scale(float s);
float mpu6050_get_scale(void);
void mpu6050_calibrate(uint16_t samples);
float mpu6050_get_angle_z(void);
void mpu6050_reset_angle(void);
void mpu6050_update(float dt_s);

#endif /* USER_MODULES_MPU6050_H_ */
