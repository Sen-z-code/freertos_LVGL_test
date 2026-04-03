/*
 * mpu6050.c
 * 精简版 MPU6050 驱动：只保留 I2C 读写、原始/标度读取以及调试查询接口。
 */
#include "mpu6050.h"
#include "mpu6050_fusion.h"
#include "i2c.h"
#include "main.h"
#include <string.h>
#include <math.h>

/* 使用外部声明的 hi2c1 */
extern I2C_HandleTypeDef hi2c1;

// 将单字节写入 MPU6050 指定寄存器，通过 HAL I2C MEM 写操作。
// 参数: reg - 寄存器地址, val - 要写入的字节
// 返回: HAL 状态（HAL_OK 表示成功）
static HAL_StatusTypeDef write_reg(uint8_t reg, uint8_t val)
{
	return HAL_I2C_Mem_Write(&hi2c1, MPU6050_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 100);
}

// 从 MPU6050 连续读取 len 字节，从寄存器 reg 开始，通过 HAL I2C MEM 读操作。
// 参数: reg - 起始寄存器, buf - 数据缓冲区, len - 要读取的字节数
// 返回: HAL 状态（HAL_OK 表示成功）
static HAL_StatusTypeDef read_regs(uint8_t reg, uint8_t *buf, uint16_t len)
{
	return HAL_I2C_Mem_Read(&hi2c1, MPU6050_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 100);
}

bool MPU6050_Init(void)
{
	uint8_t who = 0;
	// 初始化 MPU6050: 唤醒设备，检查 WHO_AM_I，设置采样率/低通/量程
	// 返回 true 表示初始化成功
	if (write_reg(MPU6050_REG_PWR_MGMT_1, 0x00) != HAL_OK) return false;
	HAL_Delay(50);

	if (read_regs(MPU6050_REG_WHO_AM_I, &who, 1) != HAL_OK) return false;
	if (who != 0x68) return false;

	// 常用配置（示例）：采样率分频、DLPF、陀螺/加速度量程（此处使用默认 ±2g / ±250°/s）
	write_reg(MPU6050_REG_SMPLRT_DIV, 7);
	write_reg(MPU6050_REG_CONFIG, 0);
	write_reg(MPU6050_REG_GYRO_CFG, 0x00);
	write_reg(MPU6050_REG_ACCEL_CFG, 0x00);
	HAL_Delay(10);
	return true;
}

bool MPU6050_ReadRaw(int16_t *ax, int16_t *ay, int16_t *az,
					 int16_t *gx, int16_t *gy, int16_t *gz,
					 int16_t *temp_raw)
{
	uint8_t buf[14];
	// 读取原始传感器数据（14 字节）：加速度 6 字节、温度 2 字节、陀螺 6 字节
	if (read_regs(MPU6050_REG_ACCEL_XOUT_H, buf, 14) != HAL_OK) return false;
	if (ax) *ax = (int16_t)((buf[0] << 8) | buf[1]);
	if (ay) *ay = (int16_t)((buf[2] << 8) | buf[3]);
	if (az) *az = (int16_t)((buf[4] << 8) | buf[5]);
	if (temp_raw) *temp_raw = (int16_t)((buf[6] << 8) | buf[7]);
	if (gx) *gx = (int16_t)((buf[8] << 8) | buf[9]);
	if (gy) *gy = (int16_t)((buf[10] << 8) | buf[11]);
	if (gz) *gz = (int16_t)((buf[12] << 8) | buf[13]);
	return true;
}

bool MPU6050_ReadScaled(float *ax_g, float *ay_g, float *az_g,
						float *gx_dps, float *gy_dps, float *gz_dps,
						float *temp_c)
{
	int16_t ax, ay, az, gx, gy, gz, temp_raw;
	if (!MPU6050_ReadRaw(&ax, &ay, &az, &gx, &gy, &gz, &temp_raw)) return false;

	const float A_SCALE = 16384.0f; /* ±2g */
	const float G_SCALE = 131.0f;   /* ±250°/s */
	// 将原始整数值转换为物理量：加速度 g，角速度 °/s，温度 °C
	if (ax_g) *ax_g = (float)ax / A_SCALE;
	if (ay_g) *ay_g = (float)ay / A_SCALE;
	if (az_g) *az_g = (float)az / A_SCALE;

	if (gx_dps) *gx_dps = (float)gx / G_SCALE;
	if (gy_dps) *gy_dps = (float)gy / G_SCALE;
	if (gz_dps) *gz_dps = (float)gz / G_SCALE;

	if (temp_c) *temp_c = ((float)temp_raw) / 340.0f + 36.53f; // MPU6050 温度公式

	return true;
}

/* Debug helpers */
// 获取当前陀螺角速度（°/s），用于调试
// 返回值通过指针输出，读取失败时写 0
void MPU6050_GetGyro(float *gx_deg, float *gy_deg, float *gz_deg)
{
	float ax,ay,az,gx,gy,gz,temp;
	if (!MPU6050_ReadScaled(&ax,&ay,&az,&gx,&gy,&gz,&temp)) {
		if (gx_deg) *gx_deg = 0.0f;
		if (gy_deg) *gy_deg = 0.0f;
		if (gz_deg) *gz_deg = 0.0f;
		return;
	}
	if (gx_deg) *gx_deg = gx;
	if (gy_deg) *gy_deg = gy;
	if (gz_deg) *gz_deg = gz;
}

// 获取当前陀螺零偏（°/s），由姿态解算模块维护
void MPU6050_GetBias(float *bx_deg, float *by_deg, float *bz_deg)
{
	MPU6050_Fusion_GetGyroBias(bx_deg, by_deg, bz_deg);
}

// 获取当前加速度（单位 g），用于调试或姿态估计的加速度观测
void MPU6050_GetAccel(float *ax_g, float *ay_g, float *az_g)
{
	float ax,ay,az,gx,gy,gz,temp;
	if (!MPU6050_ReadScaled(&ax,&ay,&az,&gx,&gy,&gz,&temp)) {
		if (ax_g) *ax_g = 0.0f;
		if (ay_g) *ay_g = 0.0f;
		if (az_g) *az_g = 0.0f;
		return;
	}
	if (ax_g) *ax_g = ax;
	if (ay_g) *ay_g = ay;
	if (az_g) *az_g = az;
}

// 获取姿态欧拉角（°），由姿态解算模块输出
void MPU6050_GetAttitude(float *roll_deg, float *pitch_deg, float *yaw_deg)
{
	MPU6050_Fusion_GetEuler(roll_deg, pitch_deg, yaw_deg);
}




