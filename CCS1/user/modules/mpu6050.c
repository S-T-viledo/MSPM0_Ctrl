#include "user/modules/mpu6050.h"

#include "user/bsp/bsp_i2c.h"
#include "user/bsp/bsp_timer.h"

#define MPU6050_REG_SMPLRT_DIV      (0x19U)
#define MPU6050_REG_CONFIG          (0x1AU)
#define MPU6050_REG_GYRO_CONFIG     (0x1BU)
#define MPU6050_REG_ACCEL_CONFIG    (0x1CU)
#define MPU6050_REG_PWR_MGMT_1      (0x6BU)
#define MPU6050_REG_WHO_AM_I        (0x75U)
#define MPU6050_REG_GYRO_ZOUT_H     (0x47U)

#define MPU6050_I2C_ADDR            (0x68U)
#define MPU6050_WHO_AM_I_VAL        (0x68U)
#define MPU6050_CALIB_SAMPLES       (2000U)

static const float g_gyro_lsb_per_dps[] = {
    131.0f, 65.5f, 32.8f, 16.4f
};

static bool    g_mpu_ready;
static float   g_gyro_z_bias;
static float   g_gyro_scale = 1.0f;
static float   g_angle_z;
static uint8_t g_gyro_fs;

static bool mpu6050_write_reg(uint8_t reg, uint8_t val)
{
    return bsp_i2c_mpu_write(MPU6050_I2C_ADDR, reg, val);
}

static bool mpu6050_read_byte(uint8_t reg, uint8_t *val)
{
    return bsp_i2c_mpu_read(MPU6050_I2C_ADDR, reg, val, 1U);
}

static int16_t mpu6050_read_int16(uint8_t reg_hi)
{
    uint8_t hi, lo;
    if (!mpu6050_read_byte(reg_hi,     &hi) ||
        !mpu6050_read_byte(reg_hi + 1U, &lo)) {
        return 0;
    }
    return (int16_t) (((uint16_t) hi << 8U) | (uint16_t) lo);
}

static int16_t mpu6050_read_gyro_z_raw(void)
{
    return mpu6050_read_int16(MPU6050_REG_GYRO_ZOUT_H);
}

void mpu6050_init(void)
{
    uint8_t whoami;

    if (!bsp_i2c_mpu_is_available()) {
        g_mpu_ready = false;
        return;
    }

    /* reset device */
    mpu6050_write_reg(MPU6050_REG_PWR_MGMT_1, 0x80U);
    bsp_delay_ms(100U);

    /* wake up */
    mpu6050_write_reg(MPU6050_REG_PWR_MGMT_1, 0x00U);
    bsp_delay_ms(50U);

    /* verify identity */
    if (!mpu6050_read_byte(MPU6050_REG_WHO_AM_I, &whoami) ||
        whoami != MPU6050_WHO_AM_I_VAL) {
        g_mpu_ready = false;
        return;
    }

    /* configure: ±250 dps, 200 Hz sample rate, DLPF on */
    g_gyro_fs = MPU6050_GYRO_FS_250;
    mpu6050_write_reg(MPU6050_REG_GYRO_CONFIG, (g_gyro_fs << 3U));
    mpu6050_write_reg(MPU6050_REG_ACCEL_CONFIG, (0U << 3U));
    mpu6050_write_reg(MPU6050_REG_SMPLRT_DIV, 4U);
    mpu6050_write_reg(MPU6050_REG_CONFIG, 0x06U);  /* DLPF_CFG=6, 5Hz BW */

    bsp_delay_ms(20U);

    g_gyro_z_bias = 0.0f;
    g_angle_z     = 0.0f;
    g_mpu_ready   = true;

    mpu6050_calibrate(MPU6050_CALIB_SAMPLES);
}

bool mpu6050_is_ready(void)
{
    return g_mpu_ready;
}

bool mpu6050_read_raw(mpu6050_raw_t *raw)
{
    if ((raw == NULL) || !g_mpu_ready) {
        return false;
    }

    raw->gyro_z  = mpu6050_read_gyro_z_raw();
    raw->gyro_x  = 0;
    raw->gyro_y  = 0;
    raw->accel_x = 0;
    raw->accel_y = 0;
    raw->accel_z = 0;
    raw->temp    = 0;

    return true;
}

bool mpu6050_read_data(mpu6050_data_t *data)
{
    mpu6050_raw_t raw;

    if (!mpu6050_read_raw(&raw)) {
        return false;
    }

    data->accel_x = 0.0f;
    data->accel_y = 0.0f;
    data->accel_z = 0.0f;
    data->gyro_x  = 0.0f;
    data->gyro_y  = 0.0f;
    data->gyro_z  = (float) raw.gyro_z / g_gyro_lsb_per_dps[g_gyro_fs];

    return true;
}

float mpu6050_get_gyro_z_dps(void)
{
    int16_t raw;
    if (!g_mpu_ready) {
        return 0.0f;
    }
    raw = mpu6050_read_gyro_z_raw();
    return (((float) raw / g_gyro_lsb_per_dps[g_gyro_fs]) - g_gyro_z_bias) * g_gyro_scale;
}

float mpu6050_get_gyro_z_bias(void)
{
    return g_gyro_z_bias;
}

void mpu6050_set_scale(float s)
{
    if (s > 0.5f && s < 2.0f) {
        g_gyro_scale = s;
    }
}

float mpu6050_get_scale(void)
{
    return g_gyro_scale;
}

void mpu6050_calibrate(uint16_t samples)
{
    uint16_t i;
    int32_t sum = 0L;
    uint16_t valid = 0U;
    int16_t raw;

    for (i = 0U; i < samples; i++) {
        raw = mpu6050_read_gyro_z_raw();
        if ((raw > -10000) && (raw < 10000)) {
            sum += (int32_t) raw;
            valid++;
        }
        bsp_delay_ms(1U);
    }

    if (valid > 0U) {
        g_gyro_z_bias = (float) sum / ((float) valid * g_gyro_lsb_per_dps[g_gyro_fs]);
    }
}

float mpu6050_get_angle_z(void)
{
    return g_angle_z;
}

void mpu6050_reset_angle(void)
{
    g_angle_z = 0.0f;
}

void mpu6050_update(float dt_s)
{
    g_angle_z += mpu6050_get_gyro_z_dps() * dt_s;
}
