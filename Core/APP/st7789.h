#ifndef ST7789_H
#define ST7789_H

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

#ifdef __cplusplus
}
#endif

#endif
