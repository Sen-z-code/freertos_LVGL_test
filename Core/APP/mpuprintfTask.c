#include "mpuprintfTask.h"
#include "cmsis_os.h"
#include <stdio.h>
#include "mpu6050.h"
#include "step_counter.h"
#include <math.h>
#include "main.h"

/* 外部队列句柄（由 CubeMX 生成在 freertos.c） */
extern osMessageQueueId_t mympu6050QueueHandle;

void StartmpuprintfTask(void *argument)
{
    mpu_msg_t msg;

    StepCounter_Init();

    for (;;) {
        osStatus_t st = osMessageQueueGet(mympu6050QueueHandle, &msg, NULL, osWaitForever);
        if (st == osOK) {
                float roll, pitch, yaw;
                // 使用队列中的 msg 数据作为采样（避免重复 I2C 读取）
                // msg.ax/msg.ay/msg.az 为加速度（g），msg.gx/msg.gy/msg.gz 为陀螺（°/s）
                bool step_detected = StepCounter_ProcessSample(msg.ax, msg.ay, msg.az,
                                                              msg.gx, msg.gy, msg.gz,
                                                              HAL_GetTick());
                MPU6050_GetAttitude(&roll, &pitch, &yaw);
                // 打印角度和累计步数
                printf("RPY=%.2f,%.2f,%.2f Steps=%lu\r\n", roll, pitch, yaw, (unsigned long)StepCounter_GetCount());
                /* 保持 50~100ms 打印间隔以便收集 10-20Hz 的诊断日志 */
                vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
