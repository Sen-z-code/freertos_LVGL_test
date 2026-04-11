/*
 * touch_task.h
 * 触摸任务声明与外部信号量
 */
#ifndef TOUCH_TASK_H
#define TOUCH_TASK_H

#include "cmsis_os.h"
#include "xpt2046.h"

#ifdef __cplusplus
extern "C" {
#endif

extern osSemaphoreId_t g_touch_sem;

void StartTouchTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif // TOUCH_TASK_H
