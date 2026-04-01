#include "MS5611Task.h"

#include "myui.h"

#include "FreeRTOS.h"
#include "task.h"


void StartMS5611Task(void *argument)
{
  (void)argument;

    // 等待系统与外设稳定
    vTaskDelay(pdMS_TO_TICKS(50));

    // 初始化 MS5611：复位、读 PROM、启动一次温度转换
    Baro_Init();

    // UI 发布节流：每 50ms 发布一次（20Hz）
    uint32_t publish_div = 0U;

    for (;;) {
        // 1000Hz 调用一次；内部每 10 次更新一次； 数据最终以 100Hz 发布给 UI 刷新显示。
        Loop_Read_Bar();

      // 不直接操作 LVGL；通过 myui_set_altitude_m 发布给 UI 线程刷新
      publish_div++;
      if (publish_div >= 50U) {
        publish_div = 0U;
        myui_set_altitude_m(Current_altitude);
      }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

}