#include "LVGLTask.h"
#include "xpt2046.h"

#define LVGL_LOOP_MS 5U

static lv_obj_t * g_counter_label;
static lv_obj_t * g_status_label;
static lv_obj_t * g_touch_label;
static lv_obj_t * g_arc;
static lv_obj_t * g_touch_dot;
static uint32_t g_touch_count;

// 对象创建失败时闪红黑，方便上板快速定位。
static void lvgl_panic_blink(void)
{
  for (;;) {
    ST7789_FillColor(ST7789_COLOR_RED);
    vTaskDelay(pdMS_TO_TICKS(250));
    ST7789_FillColor(ST7789_COLOR_BLACK);
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

// 点击按钮后更新计数，验证触摸输入链路。
static void on_btn_click(lv_event_t * e)
{
  LV_UNUSED(e);
  g_touch_count++;
  if (g_counter_label != NULL) {
    lv_label_set_text_fmt(g_counter_label, "Touch count: %lu", (unsigned long)g_touch_count);
  }
}

void StartLVGLTask(void *argument)
{
  lv_obj_t * scr;
  lv_obj_t * panel;
  lv_obj_t * title;
  lv_obj_t * hint;
  lv_obj_t * btn;
  lv_obj_t * btn_text;
  lv_obj_t * arc_title;
  lv_obj_t * tip;
  XPT2046_State_t ts;
  int32_t arc_value = 0;
  int32_t arc_dir = 1;
  uint32_t tick = 0U;

  (void)argument;

  // 阶段1：先做底层显示自检，确认 ST7789 刷新链路稳定。
  ST7789_Init();
  ST7789_FillColor(ST7789_COLOR_RED);
  vTaskDelay(pdMS_TO_TICKS(300));
  ST7789_FillColor(ST7789_COLOR_GREEN);
  vTaskDelay(pdMS_TO_TICKS(300));
  ST7789_FillColor(ST7789_COLOR_BLUE);
  vTaskDelay(pdMS_TO_TICKS(300));
  ST7789_FillColor(ST7789_COLOR_BLACK);
  vTaskDelay(pdMS_TO_TICKS(300));

  lv_init();
  lv_port_display_init();
  lv_port_indev_init();

  // 阶段2：创建完整体验页面（状态面板 + 动态环 + 触摸反馈）。
  scr = lv_screen_active();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x0D121F), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

  panel = lv_obj_create(scr);
  if (panel == NULL) {
    lvgl_panic_blink();
  }
  lv_obj_set_size(panel, 308, 212);
  lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(panel, lv_color_hex(0x151E32), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(panel, lv_color_hex(0x2A3E68), LV_PART_MAIN);
  lv_obj_set_style_border_width(panel, 2, LV_PART_MAIN);
  lv_obj_set_style_radius(panel, 12, LV_PART_MAIN);

  title = lv_label_create(panel);
  if (title == NULL) {
    lvgl_panic_blink();
  }
  lv_label_set_text(title, "LVGL Experience Panel");
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 8);

  hint = lv_label_create(panel);
  if (hint == NULL) {
    lvgl_panic_blink();
  }
  lv_label_set_text(hint, "FreeRTOS + LVGL + ST7789 + XPT2046");
  lv_obj_set_style_text_color(hint, lv_color_hex(0xFFE066), LV_PART_MAIN);
  lv_obj_align(hint, LV_ALIGN_TOP_LEFT, 10, 30);

  g_status_label = lv_label_create(panel);
  if (g_status_label == NULL) {
    lvgl_panic_blink();
  }
  lv_label_set_text(g_status_label, "Uptime: 0.0 s");
  lv_obj_set_style_text_color(g_status_label, lv_color_hex(0x8FD3FF), LV_PART_MAIN);
  lv_obj_align(g_status_label, LV_ALIGN_TOP_LEFT, 10, 62);

  g_counter_label = lv_label_create(panel);
  if (g_counter_label == NULL) {
    lvgl_panic_blink();
  }
  g_touch_count = 0U;
  lv_label_set_text(g_counter_label, "Touch count: 0");
  lv_obj_set_style_text_color(g_counter_label, lv_color_hex(0x7CFF6B), LV_PART_MAIN);
  lv_obj_align(g_counter_label, LV_ALIGN_TOP_LEFT, 10, 86);

  g_touch_label = lv_label_create(panel);
  if (g_touch_label == NULL) {
    lvgl_panic_blink();
  }
  lv_label_set_text(g_touch_label, "Touch: ---, --- (released)");
  lv_obj_set_style_text_color(g_touch_label, lv_color_hex(0x66D9FF), LV_PART_MAIN);
  lv_obj_align(g_touch_label, LV_ALIGN_TOP_LEFT, 10, 110);

  tip = lv_label_create(panel);
  if (tip != NULL) {
    lv_label_set_text(tip, "Tap button or screen to test touch");
    lv_obj_set_style_text_color(tip, lv_color_hex(0xB9C3D8), LV_PART_MAIN);
    lv_obj_align(tip, LV_ALIGN_TOP_LEFT, 10, 132);
  }

  g_arc = lv_arc_create(panel);
  if (g_arc == NULL) {
    lvgl_panic_blink();
  }
  lv_obj_set_size(g_arc, 98, 98);
  lv_obj_align(g_arc, LV_ALIGN_TOP_RIGHT, -16, 48);
  lv_arc_set_rotation(g_arc, 135);
  lv_arc_set_bg_angles(g_arc, 0, 270);
  lv_arc_set_range(g_arc, 0, 100);
  lv_arc_set_value(g_arc, 0);
  lv_obj_set_style_arc_width(g_arc, 10, LV_PART_MAIN);
  lv_obj_set_style_arc_width(g_arc, 10, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(g_arc, lv_color_hex(0x2F405F), LV_PART_MAIN);
  lv_obj_set_style_arc_color(g_arc, lv_color_hex(0x35E0A1), LV_PART_INDICATOR);

  arc_title = lv_label_create(panel);
  if (arc_title != NULL) {
    lv_label_set_text(arc_title, "Load");
    lv_obj_set_style_text_color(arc_title, lv_color_hex(0xDDE6F5), LV_PART_MAIN);
    lv_obj_align_to(arc_title, g_arc, LV_ALIGN_OUT_TOP_MID, 0, -6);
  }

  btn = lv_button_create(panel);
  if (btn == NULL) {
    lvgl_panic_blink();
  }
  lv_obj_set_size(btn, 126, 38);
  lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0x1D7AF1), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_add_event_cb(btn, on_btn_click, LV_EVENT_CLICKED, NULL);

  btn_text = lv_label_create(btn);
  if (btn_text != NULL) {
    lv_label_set_text(btn_text, "Touch +1");
    lv_obj_center(btn_text);
  }

  g_touch_dot = lv_obj_create(scr);
  if (g_touch_dot != NULL) {
    lv_obj_set_size(g_touch_dot, 8, 8);
    lv_obj_set_style_bg_color(g_touch_dot, lv_color_hex(0xFF4D4F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_touch_dot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_touch_dot, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(g_touch_dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_add_flag(g_touch_dot, LV_OBJ_FLAG_HIDDEN);
  }

  // 强制首帧立即刷新，便于现场快速判断 LVGL 显示链路是否工作。
  lv_refr_now(NULL);

  for (;;) {
    lv_tick_inc(LVGL_LOOP_MS);
    lv_timer_handler();

    tick++;
    if ((tick % 2U) == 0U) {
      arc_value += arc_dir;
      if (arc_value >= 100) {
        arc_value = 100;
        arc_dir = -1;
      } else if (arc_value <= 0) {
        arc_value = 0;
        arc_dir = 1;
      }

      if (g_arc != NULL) {
        lv_arc_set_value(g_arc, arc_value);
      }
    }

    if ((tick % 2U) == 0U) {
      uint32_t ms = tick * LVGL_LOOP_MS;
      if (g_status_label != NULL) {
        lv_label_set_text_fmt(g_status_label, "Uptime: %lu.%lus", (unsigned long)(ms / 1000U), (unsigned long)((ms % 1000U) / 100U));
      }

      if (XPT2046_ReadState(&ts) && ts.pressed) {
        if (g_touch_label != NULL) {
          lv_label_set_text_fmt(g_touch_label, "Touch: %u, %u (pressed)", ts.x, ts.y);
        }

        if (g_touch_dot != NULL) {
          lv_obj_clear_flag(g_touch_dot, LV_OBJ_FLAG_HIDDEN);
          lv_obj_set_pos(g_touch_dot, (int32_t)ts.x - 4, (int32_t)ts.y - 4);
        }
      } else {
        if (g_touch_label != NULL) {
          lv_label_set_text(g_touch_label, "Touch: ---, --- (released)");
        }

        if (g_touch_dot != NULL) {
          lv_obj_add_flag(g_touch_dot, LV_OBJ_FLAG_HIDDEN);
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(LVGL_LOOP_MS));
  }
}