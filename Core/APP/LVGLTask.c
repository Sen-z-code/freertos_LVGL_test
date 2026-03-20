#include "LVGLTask.h"
#include "xpt2046.h"

#define DEMO_BG_COLOR    ST7789_COLOR_BLACK
#define DEMO_BAR_HEIGHT  30U
#define DEMO_BTN_COUNT   6U
#define DEMO_PEN_SIZE    5U

// 绘制顶部工具栏：前5个按钮选画笔颜色，最后1个按钮用于清屏。
static void Demo_DrawToolbar(uint16_t width, uint16_t selected_pen)
{
    uint16_t btn_w = (uint16_t)(width / DEMO_BTN_COUNT);
    uint16_t x;
    uint16_t i;
    uint16_t colors[5] = {
        ST7789_COLOR_WHITE,
        ST7789_COLOR_RED,
        ST7789_COLOR_GREEN,
        ST7789_COLOR_BLUE,
        ST7789_COLOR_BLACK
    };

    for (i = 0U; i < 5U; i++) {
        x = (uint16_t)(i * btn_w);
        ST7789_FillRect(x, 0U, btn_w, DEMO_BAR_HEIGHT, colors[i]);

        // 被选中的颜色按钮画一个边框，便于识别当前画笔。
        if (selected_pen == colors[i]) {
            uint16_t border = (colors[i] == ST7789_COLOR_WHITE) ? ST7789_COLOR_BLACK : ST7789_COLOR_WHITE;
            ST7789_FillRect(x, 0U, btn_w, 2U, border);
            ST7789_FillRect(x, (uint16_t)(DEMO_BAR_HEIGHT - 2U), btn_w, 2U, border);
            ST7789_FillRect(x, 0U, 2U, DEMO_BAR_HEIGHT, border);
            ST7789_FillRect((uint16_t)(x + btn_w - 2U), 0U, 2U, DEMO_BAR_HEIGHT, border);
        }
    }

    // 最后一个按钮：黄色表示“清屏”。
    x = (uint16_t)(5U * btn_w);
    ST7789_FillRect(x, 0U, (uint16_t)(width - x), DEMO_BAR_HEIGHT, 0xFFE0U);
}

void StartLVGLTask(void *argument)
{
    XPT2046_State_t ts;
    uint16_t width;
    uint16_t height;
    uint16_t btn_w;
    uint16_t pen_color = ST7789_COLOR_WHITE;

    (void)argument;

    ST7789_Init();  // 初始化 LCD
    XPT2046_Init();  // 初始化触摸控制器

    width = ST7789_GetWidth();
    height = ST7789_GetHeight();
    btn_w = (uint16_t)(width / DEMO_BTN_COUNT);

    ST7789_FillColor(DEMO_BG_COLOR);
    Demo_DrawToolbar(width, pen_color);

    for (;;) {
        if (XPT2046_ReadState(&ts) && ts.pressed) {
            ST7789_SetBacklight(100U);

            if (ts.y < DEMO_BAR_HEIGHT) {
                uint8_t idx = (uint8_t)(ts.x / btn_w);

                if (idx < 5U) {
                    uint16_t colors[5] = {
                        ST7789_COLOR_WHITE,
                        ST7789_COLOR_RED,
                        ST7789_COLOR_GREEN,
                        ST7789_COLOR_BLUE,
                        ST7789_COLOR_BLACK
                    };
                    pen_color = colors[idx];
                    Demo_DrawToolbar(width, pen_color);
                } else {
                    ST7789_FillRect(0U, DEMO_BAR_HEIGHT, width, (uint16_t)(height - DEMO_BAR_HEIGHT), DEMO_BG_COLOR);
                    Demo_DrawToolbar(width, pen_color);
                }

                vTaskDelay(pdMS_TO_TICKS(120));
            } else {
                uint16_t x0 = (ts.x >= (DEMO_PEN_SIZE / 2U)) ? (uint16_t)(ts.x - (DEMO_PEN_SIZE / 2U)) : 0U;
                uint16_t y0 = (ts.y >= (DEMO_PEN_SIZE / 2U)) ? (uint16_t)(ts.y - (DEMO_PEN_SIZE / 2U)) : DEMO_BAR_HEIGHT;

                if (y0 < DEMO_BAR_HEIGHT) {
                    y0 = DEMO_BAR_HEIGHT;
                }

                ST7789_FillRect(x0, y0, DEMO_PEN_SIZE, DEMO_PEN_SIZE, pen_color);
            }
        } else {
            ST7789_SetBacklight(65U);
        }

        vTaskDelay(pdMS_TO_TICKS(12));
    }
}