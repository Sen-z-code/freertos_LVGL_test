// STM32 + XPT2046 的 LVGL 输入端口实现（仅启用触摸指针输入）。

#include "lv_port_indev.h"

#include "xpt2046.h"

static lv_indev_t * g_touch_indev;

// LVGL 输入读取回调：从 XPT2046 获取按下状态与坐标。
static void touchpad_read_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
  static int32_t last_x = 0;
  static int32_t last_y = 0;
  XPT2046_State_t ts;

  LV_UNUSED(indev);

  if (XPT2046_ReadState(&ts) && ts.pressed) {
    last_x = (int32_t)ts.x;
    last_y = (int32_t)ts.y;
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }

  data->point.x = last_x;
  data->point.y = last_y;
}

void lv_port_indev_init(void)
{
  XPT2046_Init();

  g_touch_indev = lv_indev_create();
  lv_indev_set_type(g_touch_indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(g_touch_indev, touchpad_read_cb);
}
