#include "LVGLTask.h"
#include "xpt2046.h"

void StartLVGLTask(void *argument)
{
    (void)argument;

    ST7789_Init();// 初始化 LCD
    XPT2046_Init();// 初始化触摸控制器

    for (;;) {
        if (XPT2046_ReadPoint(NULL, NULL)) {
            ST7789_SetBacklight(100U);
        } else {
            ST7789_SetBacklight(80U);
        }

        ST7789_FillColor(ST7789_COLOR_RED);
        vTaskDelay(pdMS_TO_TICKS(600));

        ST7789_FillColor(ST7789_COLOR_GREEN);
        vTaskDelay(pdMS_TO_TICKS(600));

        ST7789_FillColor(ST7789_COLOR_BLUE);
        vTaskDelay(pdMS_TO_TICKS(600));

        ST7789_FillColor(ST7789_COLOR_WHITE);
        vTaskDelay(pdMS_TO_TICKS(600));

        ST7789_FillColor(ST7789_COLOR_BLACK);
        vTaskDelay(pdMS_TO_TICKS(600));
    }
}