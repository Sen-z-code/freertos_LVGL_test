#include "LVGLTask.h"

#include "myui.h"

#define LVGL_LOOP_MS 5U

void StartLVGLTask(void *argument)
{
  (void)argument;

  // 可选：上电自检，确认底层显示链路正常。
  ST7789_Init();
  ST7789_FillColor(ST7789_COLOR_RED);
  vTaskDelay(pdMS_TO_TICKS(300));
  ST7789_FillColor(ST7789_COLOR_GREEN);
  vTaskDelay(pdMS_TO_TICKS(300));
  ST7789_FillColor(ST7789_COLOR_BLUE);
  vTaskDelay(pdMS_TO_TICKS(300));
  ST7789_FillColor(ST7789_COLOR_BLACK);
  vTaskDelay(pdMS_TO_TICKS(300));

  lv_init(); // 初始化 LVGL 内核
  lv_port_display_init(); // 初始化显示端口（绑定 ST7789 flush）
  lv_port_indev_init(); // 初始化输入端口（绑定触摸 read）

  // 在这里创建自己的页面/控件。
  myui_init();

  // 强制首帧立即刷新，便于确认初始化是否成功。
  lv_refr_now(NULL);

  for (;;) {
    lv_tick_inc(LVGL_LOOP_MS);
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(LVGL_LOOP_MS));
  }
}