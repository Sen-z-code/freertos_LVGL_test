/**
 * @file lv_port_lcd_stm32_template.h
 *
 */

/* 本工程直接使用该文件，不再另存为其他文件名 */
#if 1

#ifndef LV_PORT_LCD_STM32_H
#define LV_PORT_LCD_STM32_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void lv_port_display_init(void);  // 初始化 LVGL 显示端口（绑定 ST7789 flush）

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PORT_LCD_STM32_H*/

#endif /*Disable/Enable content*/
