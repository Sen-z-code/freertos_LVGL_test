#ifndef MPU6050_FUSION_H
#define MPU6050_FUSION_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 初始化姿态解算（四元数清零为单位四元数，积分项清零）
void MPU6050_Fusion_Init(void); // 初始化姿态解算器（设置单位四元数，清零积分项）

// 设置 Mahony-IMU 参数：kp 为比例系数，ki 为积分系数（ki=0 表示不积分）
void MPU6050_Fusion_SetGains(float kp, float ki); // 设置 Mahony 算法的比例/积分增益，调整融合响应与稳定性

// 设置陀螺仪零偏（单位：°/s）
void MPU6050_Fusion_SetGyroBias(float bx_dps, float by_dps, float bz_dps); // 设置/校准陀螺零偏，单位 °/s

// 获取当前陀螺仪零偏（单位：°/s）
void MPU6050_Fusion_GetGyroBias(float *bx_dps, float *by_dps, float *bz_dps); // 获取当前陀螺零偏，单位 °/s

// Mahony-IMU 更新：加速度单位 g，角速度单位 °/s，dt 单位 s
// 说明：无磁力计情况下 yaw 会随时间漂移；roll/pitch 由加速度约束较稳定。
void MPU6050_Fusion_Update(float ax_g, float ay_g, float az_g,
                           float gx_dps, float gy_dps, float gz_dps,
                           float dt_s); // 使用 Mahony-IMU 更新四元数（加速度 g，角速度 °/s，时间 s）

// 获取欧拉角（单位：°）
void MPU6050_Fusion_GetEuler(float *roll_deg, float *pitch_deg, float *yaw_deg); // 获取当前欧拉角：roll/pitch/yaw，单位 °

// 获取四元数（q0 为实部）
void MPU6050_Fusion_GetQuat(float *q0, float *q1, float *q2, float *q3); // 获取当前四元数 (q0 为实部)

#ifdef __cplusplus
}
#endif

#endif /* MPU6050_FUSION_H */
