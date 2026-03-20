#ifndef ST7789_H
#define ST7789_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ST7789_COLOR_BLACK 0x0000U
#define ST7789_COLOR_WHITE 0xFFFFU
#define ST7789_COLOR_RED   0xF800U
#define ST7789_COLOR_GREEN 0x07E0U
#define ST7789_COLOR_BLUE  0x001FU

void ST7789_Init(void);  // 初始化ST7789
void ST7789_SetBacklight(uint8_t percent);  // 设置背光亮度百分比（0~100）
void ST7789_FillColor(uint16_t color);  // 全屏填充单一 RGB565 颜色
void ST7789_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);  // 填充指定矩形区域
bool ST7789_WritePixels(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *pixels);  // 按区域写入 RGB565 像素缓冲（LVGL flush 可直接调用）
uint16_t ST7789_GetWidth(void);  // 获取当前显示宽度
uint16_t ST7789_GetHeight(void);  // 获取当前显示高度

// LVGL 对接提示：在 flush_cb 里调用 ST7789_WritePixels(area->x1, area->y1, w, h, (const uint16_t *)color_p)

#ifdef __cplusplus
}
#endif

#endif
