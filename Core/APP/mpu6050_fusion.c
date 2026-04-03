#include "mpu6050_fusion.h"

#include <math.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#ifndef MPU6050_FUSION_DEFAULT_KP
#define MPU6050_FUSION_DEFAULT_KP 2.0f
#endif

#ifndef MPU6050_FUSION_DEFAULT_KI
#define MPU6050_FUSION_DEFAULT_KI 0.0f
#endif

#ifndef MPU6050_FUSION_ACCEL_NORM_MIN
#define MPU6050_FUSION_ACCEL_NORM_MIN 0.5f
#endif

#ifndef MPU6050_FUSION_ACCEL_NORM_MAX
#define MPU6050_FUSION_ACCEL_NORM_MAX 1.5f
#endif

#define DEG2RAD (0.017453292519943295769f)
#define RAD2DEG (57.295779513082320877f)

typedef struct {
	float q0;
	float q1;
	float q2;
	float q3;
	float exInt;
	float eyInt;
	float ezInt;
	float kp;
	float ki;
	float bx_dps;
	float by_dps;
	float bz_dps;
} fusion_state_t;

static fusion_state_t g_fusion = {
	.q0 = 1.0f, .q1 = 0.0f, .q2 = 0.0f, .q3 = 0.0f,
	.exInt = 0.0f, .eyInt = 0.0f, .ezInt = 0.0f,
	.kp = MPU6050_FUSION_DEFAULT_KP,
	.ki = MPU6050_FUSION_DEFAULT_KI,
	.bx_dps = 0.0f, .by_dps = 0.0f, .bz_dps = 0.0f,
};

static inline void fusion_lock(void)
{
	taskENTER_CRITICAL();
}

static inline void fusion_unlock(void)
{
	taskEXIT_CRITICAL();
}

static inline float inv_sqrtf_safe(float x)
{
	if (x <= 0.0f) return 0.0f;
	return 1.0f / sqrtf(x);
}
// 初始化姿态解算器
// 作用：将四元数重置为单位四元数，清零积分误差和陀螺零偏，恢复默认增益
// 线程安全：内部使用临界段保护共享状态
void MPU6050_Fusion_Init(void)
{
	fusion_lock();
	g_fusion.q0 = 1.0f;
	g_fusion.q1 = 0.0f;
	g_fusion.q2 = 0.0f;
	g_fusion.q3 = 0.0f;
	g_fusion.exInt = 0.0f;
	g_fusion.eyInt = 0.0f;
	g_fusion.ezInt = 0.0f;
	g_fusion.kp = MPU6050_FUSION_DEFAULT_KP;
	g_fusion.ki = MPU6050_FUSION_DEFAULT_KI;
	g_fusion.bx_dps = 0.0f;
	g_fusion.by_dps = 0.0f;
	g_fusion.bz_dps = 0.0f;
	fusion_unlock();
}

// 设置 Mahony 算法增益
// 参数：kp - 比例增益（影响瞬时误差校正强度）；ki - 积分增益（用于估计并补偿陀螺偏置，ki=0 表示不积分）
// 线程安全：写全局状态时使用临界段
void MPU6050_Fusion_SetGains(float kp, float ki)
{
	fusion_lock();
	g_fusion.kp = kp;
	g_fusion.ki = ki;
	fusion_unlock();
}

// 设置/手动校准陀螺零偏（单位：°/s）
// 当已知零偏时可调用此函数以改善长期稳定性
void MPU6050_Fusion_SetGyroBias(float bx_dps, float by_dps, float bz_dps)
{
	fusion_lock();
	g_fusion.bx_dps = bx_dps;
	g_fusion.by_dps = by_dps;
	g_fusion.bz_dps = bz_dps;
	fusion_unlock();
}

// 获取当前陀螺零偏（单位：°/s）
// 参数可以为 NULL（表示不需要该轴），从而灵活读取
void MPU6050_Fusion_GetGyroBias(float *bx_dps, float *by_dps, float *bz_dps)
{
	fusion_lock();
	if (bx_dps) *bx_dps = g_fusion.bx_dps;
	if (by_dps) *by_dps = g_fusion.by_dps;
	if (bz_dps) *bz_dps = g_fusion.bz_dps;
	fusion_unlock();
}

// Mahony IMU 更新主函数
// 参数：加速度 (ax_g,ay_g,az_g) 单位 g，角速度 (gx_dps,gy_dps,gz_dps) 单位 °/s，dt_s 单位 s
// 功能：使用陀螺积分预测四元数，再用加速度约束通过比例+积分（PI）修正四元数以抑制漂移
// 细节：包含陀螺零偏补偿、加速度归一化门限以避免动态加速度误矫正、四元数归一化保护
void MPU6050_Fusion_Update(float ax_g, float ay_g, float az_g,
						   float gx_dps, float gy_dps, float gz_dps,
						   float dt_s)
{
	if (dt_s <= 0.0f) return;
	if (!isfinite(dt_s)) return;
	// 读取当前状态（四元数、积分误差、增益、零偏），使用临界段保护共享状态
	fusion_lock();
	const float kp = g_fusion.kp;
	const float ki = g_fusion.ki;
	const float bx = g_fusion.bx_dps;
	const float by = g_fusion.by_dps;
	const float bz = g_fusion.bz_dps;
	float q0 = g_fusion.q0;
	float q1 = g_fusion.q1;
	float q2 = g_fusion.q2;
	float q3 = g_fusion.q3;
	float exInt = g_fusion.exInt;
	float eyInt = g_fusion.eyInt;
	float ezInt = g_fusion.ezInt;
	fusion_unlock();

	// 1.陀螺去零偏并转换为 rad/s
	float gx = (gx_dps - bx) * DEG2RAD;
	float gy = (gy_dps - by) * DEG2RAD;
	float gz = (gz_dps - bz) * DEG2RAD;

	// 2.加速度归一化，避免剧烈运动时误矫正
	const float a2 = ax_g * ax_g + ay_g * ay_g + az_g * az_g;
	if (a2 > 0.0f) {
		const float a_norm = sqrtf(a2);
		if (a_norm >= MPU6050_FUSION_ACCEL_NORM_MIN && a_norm <= MPU6050_FUSION_ACCEL_NORM_MAX) // 只有在加速度接近 1g 时才使用加速度矢量进行姿态约束
		{
			const float inv = 1.0f / a_norm;
			float ax = ax_g * inv;
			float ay = ay_g * inv;
			float az = az_g * inv;

			// 3.根据当前四元数估计重力方向（Mahony IMU）
			const float vx = 2.0f * (q1 * q3 - q0 * q2);
			const float vy = 2.0f * (q0 * q1 + q2 * q3);
			const float vz = (q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3);

			// 4.误差：测量重力与估计重力的叉乘
			const float ex = (ay * vz - az * vy);
			const float ey = (az * vx - ax * vz);
			const float ez = (ax * vy - ay * vx);

			// 5.积分项累加（用于慢漂移补偿），只有 ki>0 时启用
			if (ki > 0.0f) {
				exInt += ex * ki * dt_s;
				eyInt += ey * ki * dt_s;
				ezInt += ez * ki * dt_s;
			} else {
				exInt = 0.0f;
				eyInt = 0.0f;
				ezInt = 0.0f;
			}

			// 6.比例 + 积分 修正角速度，用于修正四元数积分项
			gx += kp * ex + exInt;
			gy += kp * ey + eyInt;
			gz += kp * ez + ezInt;
		}
	}

	// 7.四元数积分（用修正后的角速度计算四元数增量并积分）
	const float half_dt = 0.5f * dt_s;
	const float qDot0 = (-q1 * gx - q2 * gy - q3 * gz) * half_dt;
	const float qDot1 = ( q0 * gx + q2 * gz - q3 * gy) * half_dt;
	const float qDot2 = ( q0 * gy - q1 * gz + q3 * gx) * half_dt;
	const float qDot3 = ( q0 * gz + q1 * gy - q2 * gx) * half_dt;

	q0 += qDot0;
	q1 += qDot1;
	q2 += qDot2;
	q3 += qDot3;

	// 8.四元数归一化并保护异常情况（防止数值溢出/归一化失败）
	const float n2 = q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3;
	const float invn = inv_sqrtf_safe(n2);
	if (invn > 0.0f) {
		q0 *= invn;
		q1 *= invn;
		q2 *= invn;
		q3 *= invn;
	} else {
		q0 = 1.0f; q1 = 0.0f; q2 = 0.0f; q3 = 0.0f;
		exInt = 0.0f; eyInt = 0.0f; ezInt = 0.0f;
	}
	// 9.最终得到新的四元数，写回状态，使用临界段保护共享状态
	fusion_lock();
	g_fusion.q0 = q0;
	g_fusion.q1 = q1;
	g_fusion.q2 = q2;
	g_fusion.q3 = q3;
	g_fusion.exInt = exInt;
	g_fusion.eyInt = eyInt;
	g_fusion.ezInt = ezInt;
	fusion_unlock();
}

// 获取欧拉角（单位：°）
// 说明：使用标准 Tait-Bryan ZYX 顺序（yaw-pitch-roll），其中 yaw 会随时间漂移（因为没有磁力计约束），roll/pitch 由加速度约束较稳定
// 参数：roll_deg、pitch_deg、yaw_deg 均为输出参数，调用时可传入 NULL 表示不需要该轴数据
void MPU6050_Fusion_GetEuler(float *roll_deg, float *pitch_deg, float *yaw_deg)
{
	fusion_lock();
	const float q0 = g_fusion.q0;
	const float q1 = g_fusion.q1;
	const float q2 = g_fusion.q2;
	const float q3 = g_fusion.q3;
	fusion_unlock();

	// 标准 Tait-Bryan ZYX（yaw-pitch-roll）
	const float sinr_cosp = 2.0f * (q0 * q1 + q2 * q3);
	const float cosr_cosp = 1.0f - 2.0f * (q1 * q1 + q2 * q2);
	const float roll = atan2f(sinr_cosp, cosr_cosp);

	float sinp = 2.0f * (q0 * q2 - q3 * q1);
	if (sinp > 1.0f) sinp = 1.0f;
	if (sinp < -1.0f) sinp = -1.0f;
	const float pitch = asinf(sinp);

	const float siny_cosp = 2.0f * (q0 * q3 + q1 * q2);
	const float cosy_cosp = 1.0f - 2.0f * (q2 * q2 + q3 * q3);
	const float yaw = atan2f(siny_cosp, cosy_cosp);

	if (roll_deg) *roll_deg = roll * RAD2DEG;
	if (pitch_deg) *pitch_deg = pitch * RAD2DEG;
	if (yaw_deg) *yaw_deg = yaw * RAD2DEG;
}

void MPU6050_Fusion_GetQuat(float *q0, float *q1, float *q2, float *q3)
{
	// 获取当前四元数（q0 为实部），参数可为 NULL 表示不需要该分量
	fusion_lock();
	if (q0) *q0 = g_fusion.q0;
	if (q1) *q1 = g_fusion.q1;
	if (q2) *q2 = g_fusion.q2;
	if (q3) *q3 = g_fusion.q3;
	fusion_unlock();
}
