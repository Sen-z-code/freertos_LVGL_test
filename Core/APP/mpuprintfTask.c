#include "mpuprintfTask.h"
#include "cmsis_os.h"
#include <stdio.h>
#include "mpu6050.h"
#include <math.h>
#include "main.h"

/* 外部队列句柄（由 CubeMX 生成在 freertos.c） */
extern osMessageQueueId_t mympu6050QueueHandle;

void StartmpuprintfTask(void *argument)
{
    mpu_msg_t msg;

    for (;;) {
        osStatus_t st = osMessageQueueGet(mympu6050QueueHandle, &msg, NULL, osWaitForever);
        if (st == osOK) {
            // 打印来自采样任务的角度与步数（msg 已由采样任务填充）
            printf("RPY=%.2f,%.2f,%.2f Steps=%lu\r\n", msg.roll, msg.pitch, msg.yaw, (unsigned long)msg.steps);
                /* 保持 50~100ms 打印间隔以便收集 10-20Hz 的诊断日志 */
                vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
