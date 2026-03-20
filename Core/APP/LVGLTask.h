#ifndef LVGLTASK_H
#define LVGLTASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "st7789.h"
#include "lvgl.h"
#include "lv_port_lcd_stm32.h"
#include "lv_port_indev.h"

void StartLVGLTask(void *argument);

#endif