#ifndef XPT2046_H
#define XPT2046_H

#include <stdbool.h>
#include <stdint.h>
#include "cmsis_os.h"

#ifdef __cplusplus
extern "C" {
#endif

// 屏幕分辨率（与当前 ST7789 横屏配置一致）
#define XPT2046_LCD_WIDTH  320U
#define XPT2046_LCD_HEIGHT 240U

// 原始 ADC 校准范围（先用通用默认值，后续可实测微调）
#define XPT2046_RAW_X_MIN 200U
#define XPT2046_RAW_X_MAX 3900U
#define XPT2046_RAW_Y_MIN 200U
#define XPT2046_RAW_Y_MAX 3900U

// 坐标方向映射（按你屏幕安装方向调整）
#define XPT2046_SWAP_XY   1U
#define XPT2046_INVERT_X  0U
#define XPT2046_INVERT_Y  1U

typedef struct {
	bool pressed;
	uint16_t x;
	uint16_t y;
} XPT2046_State_t;  // 触摸状态（可直接映射到 LVGL indev 的状态与坐标）

void XPT2046_Init(void);  // 初始化 XPT2046 驱动（默认释放片选）
bool XPT2046_IsTouched(void);  // 检测当前是否按下触摸（T_IRQ 低电平为按下）
bool XPT2046_ReadRaw(uint16_t *x_raw, uint16_t *y_raw);  // 读取原始 ADC 坐标，不做像素映射
bool XPT2046_ReadPoint(uint16_t *x, uint16_t *y);  // 读取并转换为屏幕像素坐标
bool XPT2046_ReadState(XPT2046_State_t *state);  // 读取稳定化触摸状态（按下/坐标），建议在 LVGL 读回调中调用
bool XPT2046_GetState(XPT2046_State_t *state);  // 获取最近一次任务更新的触摸状态（非阻塞）
void XPT2046_SetCalibration(uint16_t x_min, uint16_t x_max, uint16_t y_min, uint16_t y_max);  // 设置原始坐标校准范围

// LVGL 对接提示：在 indev read_cb 里调用 XPT2046_ReadState()，把 pressed/x/y 映射到 data->state/data->point

#ifdef __cplusplus
}
#endif

#endif
