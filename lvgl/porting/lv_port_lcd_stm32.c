// STM32 + ST7789 的 LVGL 显示端口实现（基于项目内自定义 st7789 驱动）

#include "lv_port_lcd_stm32.h"

#include "st7789.h"

#define LVGL_DRAW_BUF_LINES 10U

static lv_display_t * g_disp;
static uint16_t g_buf1[320U * LVGL_DRAW_BUF_LINES];
static uint16_t g_buf2[320U * LVGL_DRAW_BUF_LINES];

// LVGL 刷新回调：把指定区域像素缓冲刷到 ST7789。
static void lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
  int32_t x1 = area->x1;
  int32_t y1 = area->y1;
  int32_t x2 = area->x2;
  int32_t y2 = area->y2;
  uint16_t disp_w = ST7789_GetWidth();
  uint16_t disp_h = ST7789_GetHeight();
  int32_t src_w;
  int32_t clip_w;
  int32_t clip_h;
  int32_t x_off;
  int32_t y_off;
  int32_t row;

  if ((x2 < 0) || (y2 < 0) || (x1 >= (int32_t)disp_w) || (y1 >= (int32_t)disp_h)) {
    lv_display_flush_ready(disp);
    return;
  }

  if (x1 < 0) {
    x1 = 0;
  }
  if (y1 < 0) {
    y1 = 0;
  }
  if (x2 >= (int32_t)disp_w) {
    x2 = (int32_t)disp_w - 1;
  }
  if (y2 >= (int32_t)disp_h) {
    y2 = (int32_t)disp_h - 1;
  }

  src_w = area->x2 - area->x1 + 1;
  clip_w = x2 - x1 + 1;
  clip_h = y2 - y1 + 1;
  x_off = x1 - area->x1;
  y_off = y1 - area->y1;

  // 若发生裁剪，源缓冲每行步长仍是 src_w，因此按行刷新最稳妥。
  for (row = 0; row < clip_h; row++) {
    const uint16_t *line = ((const uint16_t *)px_map) + (y_off + row) * src_w + x_off;
    (void)ST7789_WritePixels((uint16_t)x1, (uint16_t)(y1 + row), (uint16_t)clip_w, 1U, line);
  }

  lv_display_flush_ready(disp);
}

void lv_port_display_init(void)
{
  uint16_t width;
  uint16_t height;
  uint32_t buf_pixels;
  uint32_t bytes;

  // 屏幕由上层任务先初始化并完成自检，这里不再重复复位/初始化。
  width = ST7789_GetWidth();
  height = ST7789_GetHeight();

  g_disp = lv_display_create((int32_t)width, (int32_t)height);
  lv_display_set_color_format(g_disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(g_disp, lvgl_flush_cb);

  buf_pixels = (uint32_t)width * LVGL_DRAW_BUF_LINES;
  bytes = buf_pixels * sizeof(uint16_t);
  lv_display_set_buffers(g_disp, g_buf1, g_buf2, bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
}
