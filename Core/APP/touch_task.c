/*
 * touch_task.c
 * 触摸中断唤醒 + 后台采样任务
 */

#include "touch_task.h"
#include "main.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "xpt2046.h"

// 由此文件定义并导出，freertos.c 中会使用 StartTouchTask 作为任务入口。
osSemaphoreId_t g_touch_sem = NULL;

// EXTI 中断回调（在 IRQ 中被调用）
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == T_IRQ_Pin) {
    if (g_touch_sem != NULL) {
      (void)osSemaphoreRelease(g_touch_sem);
    }
  }
}

// 触摸后台任务：被 EXTI 唤醒后在任务内轮询采样直到释放，减少 LVGL 里的阻塞。
void StartTouchTask(void *argument)
{
  (void)argument;

  // 信号量由 MX_FREERTOS_Init 提前创建，这里直接等待。

  for (;;) {
    if (g_touch_sem != NULL) {
      if (osSemaphoreAcquire(g_touch_sem, osWaitForever) == osOK) {
        XPT2046_State_t st;
        // 触发后连续采样直到释放，采样间隔短以提升响应流畅性
        do {
          (void)XPT2046_ReadState(&st);
          vTaskDelay(pdMS_TO_TICKS(4));
        } while (XPT2046_IsTouched());
      }
    } else {
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }
}
