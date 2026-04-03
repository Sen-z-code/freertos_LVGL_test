/*
 * mpu6050.h
 * 简单的基于 HAL 的 MPU6050 驱动接口（使用全局 hi2c1）
 */
#ifndef MPU6050_H
#define MPU6050_H

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

/* MPU6050 I2C 7-bit 地址 0x68，HAL 使用 8-bit 地址 */
#define MPU6050_I2C_ADDR       (0x68 << 1)

/* 寄存器 */
#define MPU6050_REG_WHO_AM_I       0x75
#define MPU6050_REG_PWR_MGMT_1     0x6B
#define MPU6050_REG_SMPLRT_DIV     0x19
#define MPU6050_REG_CONFIG         0x1A
#define MPU6050_REG_GYRO_CFG       0x1B
#define MPU6050_REG_ACCEL_CFG      0x1C
#define MPU6050_REG_ACCEL_XOUT_H   0x3B

typedef struct {
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
    float temp;
    float roll; // 欧拉角 roll (°)
    float pitch; // 欧拉角 pitch (°)
    float yaw; // 欧拉角 yaw (°)
    uint32_t steps; // 累计步数
} mpu_msg_t;
 


/* 公共接口（内部使用全局 hi2c1） */
bool MPU6050_Init(void);

bool MPU6050_ReadRaw(int16_t *ax, int16_t *ay, int16_t *az,
                     int16_t *gx, int16_t *gy, int16_t *gz,
                     int16_t *temp_raw);

bool MPU6050_ReadScaled(float *ax_g, float *ay_g, float *az_g,
                        float *gx_dps, float *gy_dps, float *gz_dps,
                        float *temp_c);

/* 任务入口（保留 CubeMX 要求的签名） */
void StartMPU6050Task(void *argument);

/* Debug / query helpers */
void MPU6050_GetGyro(float *gx_deg, float *gy_deg, float *gz_deg);
void MPU6050_GetBias(float *bx_deg, float *by_deg, float *bz_deg);
void MPU6050_GetAccel(float *ax_g, float *ay_g, float *az_g);

void MPU6050_GetAttitude(float *roll_deg, float *pitch_deg, float *yaw_deg); // 获取姿态角（单位：°，无磁力计时 yaw 会漂移）


#endif /* MPU6050_H */
