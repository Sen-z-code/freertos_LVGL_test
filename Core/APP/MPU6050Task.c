#include "MPU6050Task.h"
#include "mpu6050.h"
#include "mpu6050_fusion.h"
#include "main.h"
#include "i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "step_counter.h"
#include "myui.h"

void StartMPU6050Task(void *argument)
{
	/* CubeMX 生成会传入 NULL 或其它参数；本任务使用全局 hi2c1，不依赖 argument */

	/* 尝试初始化 MPU6050，若失败则持续重试 */
	while (!MPU6050_Init()) {
		vTaskDelay(pdMS_TO_TICKS(200));
	}

	// 初始化：姿态解算与步数计
	MPU6050_Fusion_Init();
	MPU6050_Fusion_SetGains(2.0f, 0.0f);
	StepCounter_Init();

	// 标记是否已完成上电静止标定，只有完成后才把数据发到队列
	volatile bool calibrated = false;

	// 上电静止时做一次陀螺零偏标定（假设此时设备不动）
	{
		const uint32_t calib_samples = 200;
		float sum_gx = 0.0f, sum_gy = 0.0f, sum_gz = 0.0f;
		uint32_t ok_cnt = 0;
		for (uint32_t i = 0; i < calib_samples; i++) {
			float ax, ay, az, gx, gy, gz, temp;
			if (MPU6050_ReadScaled(&ax, &ay, &az, &gx, &gy, &gz, &temp)) {
				sum_gx += gx;
				sum_gy += gy;
				sum_gz += gz;
				ok_cnt++;
			}
			vTaskDelay(pdMS_TO_TICKS(5));
		}
		if (ok_cnt > 0) {
			MPU6050_Fusion_SetGyroBias(sum_gx / (float)ok_cnt,
									 sum_gy / (float)ok_cnt,
									 sum_gz / (float)ok_cnt);
		}
	}

	/* 标定结束，允许把数据发到队列 */
	calibrated = true;

	TickType_t lastWakeTime = xTaskGetTickCount();
	const TickType_t period = pdMS_TO_TICKS(10); // 100Hz
	for (;;) {
		float ax, ay, az, gx, gy, gz, temp; // ax/ay/az 单位：g；gx/gy/gz 单位：°/s；temp 单位：°C
		if (MPU6050_ReadScaled(&ax, &ay, &az, &gx, &gy, &gz, &temp)) {
			// 姿态更新（dt 使用固定周期，避免 tick 抖动）
			MPU6050_Fusion_Update(ax, ay, az, gx, gy, gz, 0.01f);

			// 读取欧拉角
			float roll=0.0f, pitch=0.0f, yaw=0.0f;
			MPU6050_Fusion_GetEuler(&roll, &pitch, &yaw);

			// 步数检测（在采样任务中处理，避免打印任务重复读 I2C）
			StepCounter_ProcessSample(ax, ay, az, gx, gy, gz, xTaskGetTickCount() * portTICK_PERIOD_MS);
			uint32_t steps = StepCounter_GetCount();

			mpu_msg_t msg;
			msg.ax = ax; msg.ay = ay; msg.az = az;
			msg.gx = gx; msg.gy = gy; msg.gz = gz;
			msg.temp = temp;
			msg.roll = roll; msg.pitch = pitch; msg.yaw = yaw;
			msg.steps = steps;

			if (calibrated) {
				// osStatus_t st = osMessageQueuePut(mympu6050QueueHandle, &msg, 0, pdMS_TO_TICKS(200));
				// (void)st;

				/* 发布步数到 UI（LVGL 观察者模式） */
				myui_set_steps(steps, roll, pitch, yaw);
			}
		}
		vTaskDelayUntil(&lastWakeTime, period);
	}
}