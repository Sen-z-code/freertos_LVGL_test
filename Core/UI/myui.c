#include "myui.h"

#include <stdio.h>
#include <string.h>

/*
 * myui.c
 * 模块：开机主题页面（横屏）
 * 说明：
 *  - 本模块创建智能手表的横屏启动主题界面，用常量占位各传感器数据。
 *  - 传感器包括：步数（MPU6050 处理为步数）、心率/血氧（EM7028）、气压/海拔（SPL06-001）、
 *    指南针（LSM303DLHC）、温湿度传感器、以及电量等。
 *  - 后续将传感器作为外部数据通过观察者模式绑定到 `myui_update_sensor_values()`。
 */

/* 传感器占位初始值（可替换为实际数据绑定） */
#define SENSOR_STEPS_INIT 3241
#define SENSOR_HEARTRATE_INIT 78
#define SENSOR_SPO2_INIT 95
#define SENSOR_ALTITUDE_INIT 120
#define SENSOR_TEMP_INIT 25
#define SENSOR_HUMIDITY_INIT 60
#define SENSOR_COMPASS_INIT 180
#define SENSOR_BATTERY_INIT 70

/* 手势灵敏度（数值越小越灵敏）
 * - 距离：累计移动超过该像素数才判定为手势
 * - 速度：释放时的最小速度阈值（像素/周期），越小越容易触发
 * 说明：LVGL 默认值分别约为 50 和 3，模拟器/鼠标拖拽时会显得“不够灵敏”。
 */
#define UI_GESTURE_MIN_DISTANCE  18
#define UI_GESTURE_MIN_VELOCITY  1

/* 局部控件句柄，方便更新 */
static lv_obj_t *lbl_time;
static lv_obj_t *lbl_date;
static lv_obj_t *lbl_steps;
static lv_obj_t *bar_steps;
static lv_obj_t *lbl_spo2;
static lv_obj_t *lbl_alt;
static lv_obj_t *lbl_hum;
static lv_obj_t *lbl_compass;
static lv_obj_t *lbl_batt;
static lv_obj_t *cont_right;
static lv_obj_t *dropdown_panel; // 顶部下拉面板句柄

/* 亮度调节：用一个黑色半透明遮罩模拟“屏幕亮度”（值越低遮罩越黑） */
static lv_obj_t *brightness_overlay;
static lv_obj_t *brightness_btn;
static lv_obj_t *brightness_slider;
static lv_obj_t *brightness_fill;
static lv_obj_t *left_menu_panel;    // 左侧滑出面板

/* 全局左右滑切换主界面/左侧界面 */
static void dropdown_open_anim(void);
static void dropdown_close_anim(void);
static void left_menu_open_anim(void);
static void left_menu_close_anim(void);
static void global_swipe_event_cb(lv_event_t * e);

/* 保存当前主 screen 与正在创建的日历 screen 指针 */
static lv_obj_t *saved_main_scr = NULL;
static lv_obj_t *cal_screen = NULL;
static lv_timer_t *open_calendar_timer = NULL;
/* 计算器 screen 与定时器 */
static lv_obj_t *calc_screen = NULL;
static lv_timer_t *open_calc_timer = NULL;



// 内部：创建底色与基本样式
static void create_base_style(lv_obj_t *parent)
{
  lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(parent, 8, 0);
  /* 设置默认文字为白色，保证深色背景上的可读性 */
  lv_obj_set_style_text_color(parent, lv_color_white(), 0);
  lv_obj_set_style_text_opa(parent, LV_OPA_COVER, 0);
}




//手势模块
/* 左侧菜单：收回面板动画（返回主界面） */
static void left_menu_close_anim(void)
{
  if(!left_menu_panel) return;

  const lv_coord_t panel_w = lv_obj_get_width(left_menu_panel);
  const lv_coord_t hide_x = -((panel_w * 3) / 2);
  lv_coord_t cur_x = lv_obj_get_x(left_menu_panel);

  lv_anim_del(left_menu_panel, (lv_anim_exec_xcb_t)lv_obj_set_x);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, left_menu_panel);
  lv_anim_set_values(&a, cur_x, hide_x);
  lv_anim_set_time(&a, 150);
  lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
  lv_anim_start(&a);
}

/* 左侧菜单：展开面板动画（进入左侧界面） */
static void left_menu_open_anim(void)
{
  if(!left_menu_panel) return;

  const lv_coord_t panel_w = lv_obj_get_width(left_menu_panel);
  const lv_coord_t show_x = -10;                 /* 展开到 -10 */
  const lv_coord_t hide_x = -((panel_w * 3) / 2);/* 隐藏到 -panel_w*1.5 */
  lv_coord_t cur_x = lv_obj_get_x(left_menu_panel);

  if(cur_x < hide_x) cur_x = hide_x;

  lv_anim_del(left_menu_panel, (lv_anim_exec_xcb_t)lv_obj_set_x);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, left_menu_panel);
  lv_anim_set_values(&a, cur_x, show_x);
  lv_anim_set_time(&a, 150);
  lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
  lv_anim_start(&a);
}

/* 下拉面板：展开动画 */
static void dropdown_open_anim(void)
{
  if(!dropdown_panel) return;
  const lv_coord_t panel_h = lv_obj_get_height(dropdown_panel);
  const lv_coord_t show_y = 0;
  const lv_coord_t hide_y = -((panel_h * 3) / 2);
  lv_coord_t cur_y = lv_obj_get_y(dropdown_panel);
  if(cur_y < hide_y) cur_y = hide_y;

  lv_anim_del(dropdown_panel, (lv_anim_exec_xcb_t)lv_obj_set_y);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, dropdown_panel);
  lv_anim_set_values(&a, cur_y, show_y);
  lv_anim_set_time(&a, 150);
  lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
  lv_anim_start(&a);
}

/* 下拉面板：收起动画 */
static void dropdown_close_anim(void)
{
  if(!dropdown_panel) return;
  const lv_coord_t panel_h = lv_obj_get_height(dropdown_panel);
  const lv_coord_t hide_y = -((panel_h * 3) / 2);
  lv_coord_t cur_y = lv_obj_get_y(dropdown_panel);

  lv_anim_del(dropdown_panel, (lv_anim_exec_xcb_t)lv_obj_set_y);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, dropdown_panel);
  lv_anim_set_values(&a, cur_y, hide_y);
  lv_anim_set_time(&a, 150);
  lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
  lv_anim_start(&a);
}

/* 全局手势：任意位置手势切换（依赖子控件开启 GESTURE_BUBBLE） */
static void global_swipe_event_cb(lv_event_t * e)
{
  if(lv_event_get_code(e) != LV_EVENT_GESTURE) return;
  lv_indev_t * indev = lv_indev_get_act();
  if(!indev) return;

  lv_dir_t dir = lv_indev_get_gesture_dir(indev);

  /* 左右滑：切换主界面 <-> 左侧界面 */
  if((dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT) && left_menu_panel) {
    const lv_coord_t panel_w = lv_obj_get_width(left_menu_panel);
    const lv_coord_t hide_x = -((panel_w * 3) / 2);
    const lv_coord_t cur_x = lv_obj_get_x(left_menu_panel);
    const bool is_hidden = (cur_x <= (hide_x + 2));

    if(dir == LV_DIR_RIGHT) {
      if(is_hidden) {
        left_menu_open_anim();
        lv_event_stop_bubbling(e);
      }
    } else {
      if(!is_hidden) {
        left_menu_close_anim();
        lv_event_stop_bubbling(e);
      }
    }
    return;
  }

  /* 上下滑：打开/收起下拉面板 */
  if((dir == LV_DIR_TOP || dir == LV_DIR_BOTTOM) && dropdown_panel) {
    const lv_coord_t panel_h = lv_obj_get_height(dropdown_panel);
    const lv_coord_t hide_y = -((panel_h * 3) / 2);
    const lv_coord_t cur_y = lv_obj_get_y(dropdown_panel);
    const bool is_hidden = (cur_y <= (hide_y + 2));

    if(dir == LV_DIR_BOTTOM) {
      if(is_hidden) {
        dropdown_open_anim();
        lv_event_stop_bubbling(e);
      }
    } else {
      if(!is_hidden) {
        dropdown_close_anim();
        lv_event_stop_bubbling(e);
      }
    }
    return;
  }
}




// 日历界面相关：使用独立 screen 模式并通过定时器安全切换
/* 计算器运行时状态 */
static lv_obj_t *calc_expr_label = NULL;
static lv_obj_t *calc_result_label = NULL;
static char calc_input_buf[64];
static double calc_acc = 0;
static char calc_op = 0; /* '+','-','*','/' 或 0 */
static bool calc_has_input = false;

/* 计算器按钮映射（4x4） */
static const char *calc_btn_map[] = {
  "7","8","9","/",
  "4","5","6","*",
  "1","2","3","-",
  "0",".","=","+"
};

// /* 计算器函数原型 */
// static void create_calculator_on_screen(lv_obj_t * scr);
// static void open_calc_timer_cb(lv_timer_t * t);
// static void open_calculator_cb(lv_event_t * e);
// static void calculator_btn_event_cb(lv_event_t * e);
// static void calc_update_display(void);
// static void calc_apply_op(double val);

/* 定时器回调：安全地切回主界面并删除日历 screen或计算器 screen */
static void open_menu_timer_cb(lv_timer_t * t)
{
  if(!t) return;

  // 切回保存的主 screen
  if(saved_main_scr) lv_scr_load(saved_main_scr);

  // 若 left_menu_panel 存在则展开它
  if(left_menu_panel) {
    const lv_coord_t panel_w = lv_obj_get_width(left_menu_panel);
    lv_coord_t cur_x = lv_obj_get_x(left_menu_panel);
    lv_coord_t target_x = -10;
    lv_anim_del(left_menu_panel, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, left_menu_panel);
    if(cur_x < -panel_w) cur_x = -panel_w;
    if(cur_x > 0) cur_x = 0;
    lv_anim_set_values(&a, cur_x, target_x);
    lv_anim_set_time(&a, 150);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
    lv_anim_start(&a);
  }

  // 删除日历 screen（如果存在）并清理指针
  if(cal_screen) {
    lv_obj_del(cal_screen);
    cal_screen = NULL;
  }
  // 删除计算器 screen（如果存在）并清理指针
  if(calc_screen) {
    lv_obj_del(calc_screen);
    calc_screen = NULL;
  }
  saved_main_scr = NULL;

  lv_timer_del(t);
}

/* 在返回按钮的事件回调中创建一次性定时器，10ms 后执行 open_menu_timer_cb */
static void back_to_left_cb(lv_event_t * e)
{
  LV_UNUSED(e);
  lv_timer_t * t = lv_timer_create(open_menu_timer_cb, 10, NULL);
  (void)t;
}




/* 3.创建日历 UI */
static void create_calendar_on_screen(lv_obj_t * scr)
{
  if(!scr) return;
  create_base_style(scr);
  lv_obj_set_style_pad_all(scr, 0, 0);

  /* 日历控件 */
  lv_obj_t * cal = lv_calendar_create(scr);
  lv_obj_set_size(cal, lv_obj_get_width(scr), lv_obj_get_height(scr));
  lv_obj_align(cal, LV_ALIGN_CENTER, 0, 0);
  (void)lv_calendar_add_header_arrow(cal);
  lv_calendar_set_today_date(cal, 2026, 3, 29);
  lv_calendar_set_month_shown(cal, 2026, 3);


  /* Back 按钮 */
  lv_obj_t *btn_back = lv_btn_create(scr);
  lv_obj_set_size(btn_back, 64, 36);
  lv_obj_align(btn_back, LV_ALIGN_BOTTOM_RIGHT, -8, -8);
  lv_obj_t *lblb = lv_label_create(btn_back);
  lv_label_set_text(lblb, "Back");
  lv_obj_center(lblb);
  lv_obj_add_event_cb(btn_back, back_to_left_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_move_foreground(btn_back);
}

/* 2.定时器回调：创建并加载日历 screen */
static void open_calendar_timer_cb(lv_timer_t * t)
{
  if(!t) return;

  /* 清理“正在打开”的一次性定时器句柄（避免重复触发） */
  if(t == open_calendar_timer) open_calendar_timer = NULL;

  /* 保存当前主 screen，然后创建并加载日历 screen */
  saved_main_scr = lv_scr_act();
  if(!cal_screen) {
    cal_screen = lv_obj_create(NULL);
    create_calendar_on_screen(cal_screen);//创建
  }
  lv_scr_load(cal_screen);//加载

  lv_timer_del(t);
}

/* 1.点击左侧菜单的 Calendar 条目时调用，创建一次性定时器，10ms 后执行 open_calendar_timer_cb */
static void open_calendar_cb(lv_event_t * e)
{
  LV_UNUSED(e);

  /* 如果日历已在当前 screen 上，或已经排队打开，就不重复创建定时器 */
  if(cal_screen && lv_scr_act() == cal_screen) return;
  if(open_calendar_timer) return;

  open_calendar_timer = lv_timer_create(open_calendar_timer_cb, 10, NULL);
}



/* 7.计算器：应用上一次操作到累积值 */
static void calc_apply_op(double val)
{
  if(calc_op == 0) {
    calc_acc = val;
  } else if(calc_op == '+') {
    calc_acc += val;
  } else if(calc_op == '-') {
    calc_acc -= val;
  } else if(calc_op == '*') {
    calc_acc *= val;
  } else if(calc_op == '/') {
    if(val != 0) calc_acc /= val; else calc_acc = 0; // 简单处理除以0
  }
}

/* 6.计算器：将输入/累积值等显示更新到显示标签 */
static void calc_update_display(void)
{
  if(!calc_result_label || !calc_expr_label) return;

  /* 若尚未开始任何输入或运算，则不显示任何数据 */
  if(!calc_has_input && calc_op == 0 && calc_acc == 0) {
    lv_label_set_text(calc_expr_label, "");
    lv_label_set_text(calc_result_label, "");
    return;
  }

  /* 输入中：若已有操作符则显示表达式（如 "22 - 5 ="），否则把输入当作结果显示 */
  if(calc_has_input && calc_input_buf[0] != '\0') {
    if(calc_op != 0) {
      char expr[128];
      snprintf(expr, sizeof(expr), "%g %c %s =", calc_acc, calc_op, calc_input_buf);
      lv_label_set_text(calc_expr_label, expr);
      lv_label_set_text(calc_result_label, "");
    } else {
      lv_label_set_text(calc_expr_label, "");
      lv_label_set_text(calc_result_label, calc_input_buf);
    }
    return;
  }

  /* 无输入时：若有操作符，显示 "acc op"，否则显示计算结果 */
  if(calc_op != 0) {
    char expr[64];
    snprintf(expr, sizeof(expr), "%g %c", calc_acc, calc_op);
    lv_label_set_text(calc_expr_label, expr);
    lv_label_set_text(calc_result_label, "");
  } else {
    lv_label_set_text(calc_expr_label, "");
    char buf[64];
    snprintf(buf, sizeof(buf), "%g", calc_acc);
    lv_label_set_text(calc_result_label, buf);
  }
}

/* 5.计算器按钮事件处理 */
static void calculator_btn_event_cb(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if(code != LV_EVENT_CLICKED) return;
  lv_obj_t *btn = lv_event_get_target(e);
  lv_obj_t *lbl = lv_obj_get_child(btn, 0);
  if(!lbl) return;
  const char *txt = lv_label_get_text(lbl);
  if(!txt || txt[0] == '\0') return;

  /* 处理数字或操作 */
  if((txt[0] >= '0' && txt[0] <= '9') || txt[0] == '.') {
    /* 追加到输入缓冲 */
    if(strlen(calc_input_buf) + 1 < sizeof(calc_input_buf)) {
      size_t len = strlen(calc_input_buf);
      calc_input_buf[len] = txt[0];
      calc_input_buf[len+1] = '\0';
      calc_has_input = true;
      calc_update_display();
    }
    return;
  }

  /* 等号 */
  if(txt[0] == '=') {
    if(calc_has_input) {
      double v = atof(calc_input_buf);
      calc_apply_op(v);
      /* 显示表达式与结果 */
      char expr[128];
      snprintf(expr, sizeof(expr), "%g %c %g =", calc_acc, calc_op, v);
      lv_label_set_text(calc_expr_label, expr);
      char buf[64];
      snprintf(buf, sizeof(buf), "%g", calc_acc);
      lv_label_set_text(calc_result_label, buf);
      calc_input_buf[0] = '\0';
      calc_has_input = false;
      calc_op = 0;
    }
    return;
  }

  /* 运算符 */
  if(txt[0] == '+' || txt[0] == '-' || txt[0] == '*' || txt[0] == '/') {
    if(calc_has_input) {
      double v = atof(calc_input_buf);
      if(calc_op == 0) {
        calc_acc = v;
      } else {
        calc_apply_op(v);
      }
      calc_input_buf[0] = '\0';
      calc_has_input = false;
    }
    calc_op = txt[0];
    calc_update_display();
    return;
  }
}

/* 4.删除键回调（退格） */
static void calc_delete_cb(lv_event_t * e)
{
  if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  if(calc_has_input && calc_input_buf[0] != '\0') {
    size_t len = strlen(calc_input_buf);
    if(len > 0) {
      calc_input_buf[len-1] = '\0';
      if(len-1 == 0) calc_has_input = false;
    }
  } else {
    /* 无输入时删除操作符 */
    calc_op = 0;
  }
  calc_update_display();
}

/* 3.创建计算器 UI（独立 screen） */
static void create_calculator_on_screen(lv_obj_t * scr)
{
  if(!scr) return;
  create_base_style(scr);
  lv_obj_set_style_pad_all(scr, 8, 0);

  /* 显示区：上行为表达式（小字体），下行为结果（大字体） */
  calc_expr_label = lv_label_create(scr);
  lv_label_set_text(calc_expr_label, "");
  lv_obj_set_style_text_font(calc_expr_label, &lv_font_montserrat_12, 0);
  lv_obj_align(calc_expr_label, LV_ALIGN_TOP_LEFT, 12, 8);

  calc_result_label = lv_label_create(scr);
  lv_label_set_text(calc_result_label, "");
  lv_obj_set_style_text_font(calc_result_label, &lv_font_montserrat_20, 0);
  lv_obj_align(calc_result_label, LV_ALIGN_TOP_LEFT, 12, 28);

  /* 删除按钮（退格），放右上 */
  lv_obj_t *btn_del = lv_btn_create(scr);
  lv_obj_set_size(btn_del, 36, 28);
  lv_obj_align(btn_del, LV_ALIGN_TOP_RIGHT, -90, 10);
  lv_obj_set_style_radius(btn_del, 6, 0);
  lv_obj_set_style_border_width(btn_del, 0, LV_PART_MAIN);
  lv_obj_set_style_outline_width(btn_del, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn_del, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn_del, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_t *lbl_del = lv_label_create(btn_del);
  lv_label_set_text(lbl_del, "del");
  lv_obj_center(lbl_del);
  lv_obj_add_event_cb(btn_del, calc_delete_cb, LV_EVENT_CLICKED, NULL);

  /* 主按钮区：左右布局 —— 左侧数字网格，右侧运算符竖列 */
  lv_obj_t *btn_area = lv_obj_create(scr);
  lv_obj_set_size(btn_area, LV_PCT(100), LV_PCT(70));
  lv_obj_align(btn_area, LV_ALIGN_BOTTOM_MID, 0, -8);
  lv_obj_set_flex_flow(btn_area, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_all(btn_area, 4, 0);
  lv_obj_set_style_pad_gap(btn_area, 4, 0);
  lv_obj_set_style_bg_color(btn_area, lv_color_black(), 0);
  lv_obj_set_style_border_width(btn_area, 0, 0);
  lv_obj_set_style_pad_all(btn_area, 0, 0);
  lv_obj_set_style_radius(btn_area, 0, 0);

  /* 左侧：数字与等号网格（3 列） */
  const char *left_map[] = {"1","2","3","4","5","6","7","8","9",".","0","="};
  lv_obj_t *grid = lv_obj_create(btn_area);
  lv_obj_set_width(grid, LV_PCT(78));
  lv_obj_set_height(grid, LV_PCT(100));
  lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(grid, 2,0);
  lv_obj_set_style_pad_gap(grid, 6, 0);
  lv_obj_set_style_bg_color(grid, lv_color_black(), 0);
  lv_obj_set_style_border_width(grid, 0, 0);
  lv_obj_set_style_pad_all(grid, 0, 0);
  lv_obj_set_style_radius(grid, 0, 0);


  for(int i = 0; i < 12; i++) {
    lv_obj_t *b = lv_btn_create(grid);
    /* 宽度占父容器约 30%，保证三列且留出间距，固定高度 */
    lv_obj_set_width(b, LV_PCT(30));
    lv_obj_set_height(b, LV_PCT(20));
    lv_obj_set_style_flex_grow(b, 0, 0);
    lv_obj_set_style_radius(b, 8, 0);
    lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(b, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(b, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(b, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(b, lv_palette_darken(LV_PALETTE_GREY, 3), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(b, 0, 0);
    lv_obj_set_style_margin_all(b, 4, 0);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, left_map[i]);
    lv_obj_set_style_text_color(l, lv_color_white(), 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, calculator_btn_event_cb, LV_EVENT_CLICKED, NULL);
  }

  /* 右侧：运算符竖列（圆形蓝色按钮） */
  const char *ops[] = {"+","-","*","/"};
  lv_obj_t *op_col = lv_obj_create(btn_area);
  lv_obj_set_width(op_col, LV_PCT(18));
  lv_obj_set_height(op_col, LV_PCT(100));
  lv_obj_set_flex_flow(op_col, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(op_col, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(op_col, 4, 0);
  lv_obj_set_style_bg_color(op_col, lv_color_black(), 0);
  lv_obj_set_style_border_width(op_col, 0, 0);
  lv_obj_set_style_radius(op_col, 0, 0);
  for(int i = 0; i < 4; i++) {
    lv_obj_t *b = lv_btn_create(op_col);
    lv_obj_set_size(b, 48, LV_PCT(20));
    lv_obj_set_style_radius(b, 24, 0);
    lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(b, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(b, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(b, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(b, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, ops[i]);
    lv_obj_set_style_text_color(l, lv_color_white(), 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, calculator_btn_event_cb, LV_EVENT_CLICKED, NULL);
  }

  /* Back 按钮 */
  lv_obj_t *btn_back = lv_btn_create(scr);
  lv_obj_set_size(btn_back, 64, 36);
  lv_obj_add_flag(btn_back, LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_align(btn_back, LV_ALIGN_TOP_RIGHT, -8, 8);
  lv_obj_set_style_radius(btn_back, 8, 0);
  lv_obj_set_style_bg_color(btn_back, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn_back, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_t *lblb = lv_label_create(btn_back);
  lv_label_set_text(lblb, "Back");
  lv_obj_center(lblb);
  lv_obj_add_event_cb(btn_back, back_to_left_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_move_foreground(btn_back);

  /* 初始化计算器状态 */
  calc_input_buf[0] = '\0';
  calc_acc = 0;
  calc_op = 0;
  calc_has_input = false;
}

/* 2.定时器回调：创建并加载计算器 screen（一次性） */
static void open_calc_timer_cb(lv_timer_t * t)
{
  if(!t) return;
  if(t == open_calc_timer) open_calc_timer = NULL;

  saved_main_scr = lv_scr_act();
  if(!calc_screen) {
    calc_screen = lv_obj_create(NULL);
    create_calculator_on_screen(calc_screen);
  }
  lv_scr_load(calc_screen);

  lv_timer_del(t);
}

/* 1.点击左侧菜单 Calculator 条目调用，创建一次性定时器，10ms 后执行 open_calc_timer_cb */
static void open_calculator_cb(lv_event_t * e)
{
  LV_UNUSED(e);
  if(calc_screen && lv_scr_act() == calc_screen) return;
  if(open_calc_timer) return;
  open_calc_timer = lv_timer_create(open_calc_timer_cb, 10, NULL);
}



// 创建左侧滑出面板（应用列表风格）
static void create_left_menu(lv_obj_t * scr)
{
const lv_coord_t panel_w = lv_obj_get_width(scr);
left_menu_panel = lv_obj_create(scr);
lv_obj_set_size(left_menu_panel, panel_w, lv_obj_get_height(scr));
lv_obj_align(left_menu_panel, LV_ALIGN_LEFT_MID, -panel_w*1.5, 0);
/* 使面板本身可接收点击/按压事件 */
lv_obj_add_flag(left_menu_panel, LV_OBJ_FLAG_CLICKABLE);
/* 允许手势冒泡到 scr，便于全局左右滑切换 */
lv_obj_add_flag(left_menu_panel, LV_OBJ_FLAG_GESTURE_BUBBLE);
lv_obj_set_style_bg_color(left_menu_panel, lv_color_white(), 0);
lv_obj_set_style_bg_opa(left_menu_panel, LV_OPA_COVER, 0);
lv_obj_set_style_border_width(left_menu_panel, 0, 0);
lv_obj_set_style_radius(left_menu_panel, 0, 0);
lv_obj_set_flex_flow(left_menu_panel, LV_FLEX_FLOW_COLUMN);
lv_obj_set_flex_align(left_menu_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
/* 去除面板内边距，避免顶部/底部留白，使按钮能完全铺满 */
lv_obj_set_style_pad_row(left_menu_panel, 0, 0);
lv_obj_set_style_pad_all(left_menu_panel, 0, 0);
lv_obj_clear_flag(left_menu_panel, LV_OBJ_FLAG_SCROLLABLE);
lv_obj_set_scrollbar_mode(left_menu_panel, LV_SCROLLBAR_MODE_OFF);

/* 示例条目：四个按钮等比例垂直铺满、无边框、文字居中；长按视为返回指令 */
const char *texts[] = {"Calendar", "Calculator", "Stopwatch", "Wallet"};
static int menu_idx[4] = {0, 1, 2, 3};
for(int i = 0; i < 4; i++) {
  lv_obj_t *btn = lv_btn_create(left_menu_panel);
  /* 等比例占满父容器垂直空间 */
  lv_obj_set_width(btn, LV_PCT(100));
  lv_obj_set_height(btn, 0);
  lv_obj_set_style_flex_grow(btn, 1, 0);
  lv_obj_set_style_pad_all(btn, 0, 0);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  /* 允许手势冒泡到 scr（按钮铺满时仍可左右滑切换） */
  lv_obj_add_flag(btn, LV_OBJ_FLAG_GESTURE_BUBBLE);

  /* 去掉边框、外框和背景，使看起来像纯文本行 */
  /* 按钮样式：与面板同色，去掉圆角/边框/阴影，消除按钮之间的可见分隔 */
  lv_obj_set_style_bg_color(btn, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_outline_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);

  /* 居中显示文字 */
  lv_obj_t *lbl = lv_label_create(btn);
  lv_label_set_text(lbl, texts[i]);
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(lbl, lv_color_black(), 0);
  lv_obj_center(lbl);

  /* 菜单交互：点击 Calendar 打开日历界面 */
  if(i == 0) {
    lv_obj_add_event_cb(btn, open_calendar_cb, LV_EVENT_CLICKED, NULL);
  } else if(i == 1) {
    /* Calculator */
    lv_obj_add_event_cb(btn, open_calculator_cb, LV_EVENT_CLICKED, NULL);
  }
}
}



//顶部下拉面板有关函数
// 4.亮度滑块阻止事件冒泡：避免与 dropdown_panel 的拖拽逻辑冲突（否则可能拖不动/卡死）
static void brightness_slider_block_bubble_cb(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);

  /* 这些事件在拖动时会高频触发；若冒泡到 dropdown_panel，会触发面板跟随拖动 */
  if(code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING || code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST ||
     code == LV_EVENT_CLICKED || code == LV_EVENT_GESTURE) {
    lv_event_stop_bubbling(e);
  }
}

// 3.亮度滑块事件：改变遮罩透明度，并按规则切换按钮颜色
static void brightness_slider_event_cb(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if(code != LV_EVENT_VALUE_CHANGED) return;

  /* VALUE_CHANGED 也阻止冒泡，避免父对象收到多余事件 */
  lv_event_stop_bubbling(e);

  lv_obj_t * slider = lv_event_get_target(e);
  int32_t v = lv_slider_get_value(slider); // 0~100

  /* 根据滑块值调整亮度按钮内的蓝色填充高度（从底部增长） */
  if(brightness_fill && brightness_btn) {
    lv_coord_t bh = lv_obj_get_height(brightness_btn);
    if(bh > 0) {
      lv_coord_t new_h = (v * bh) / 100;
      lv_obj_set_height(brightness_fill, new_h);
    }
  }

  /* 亮度模拟：v 越小，遮罩越不透明（越暗）；v 越大越透明（越亮） */
  if(brightness_overlay) {
    lv_opa_t opa = (lv_opa_t)((100 - v) * 200 / 100);
    lv_obj_set_style_bg_opa(brightness_overlay, opa, 0);
  }
}

/* 2.一次性定时器：在布局完成后初始化 brightness_fill 的高度（避免初始高度为 0） */
static void brightness_init_timer_cb(lv_timer_t * t)
{
  LV_UNUSED(t);
  if(brightness_slider && brightness_fill && brightness_btn) {
    int32_t v = lv_slider_get_value(brightness_slider);
    /* 使用百分比设置高度，避免读取父对象像素高度为0的问题 */
    lv_obj_set_height(brightness_fill, LV_PCT(v));

    /* 同步遮罩不透明度 */
    lv_opa_t opa = (lv_opa_t)((100 - v) * 200 / 100);
    lv_obj_set_style_bg_opa(brightness_overlay, opa, 0);
  }
  /* 删除定时器自身（一次性） */
  lv_timer_del(t);
}

// 1.创建顶部下拉面板
static void create_dropdown_panel(lv_obj_t * scr)
{
  /* 下拉面板 */
  const lv_coord_t panel_h = 64;
  dropdown_panel = lv_obj_create(scr);
  lv_obj_set_size(dropdown_panel, lv_obj_get_width(scr), panel_h);
  lv_obj_align(dropdown_panel, LV_ALIGN_TOP_MID, 0, -panel_h*1.5); // 隐藏到屏幕上方
  lv_obj_add_flag(dropdown_panel, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_border_width(dropdown_panel, 0, 0);
  lv_obj_set_style_outline_width(dropdown_panel, 0, 0);
  lv_obj_clear_flag(dropdown_panel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(dropdown_panel, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_style_bg_color(dropdown_panel, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(dropdown_panel, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(dropdown_panel, 0, 0);
  lv_obj_set_style_radius(dropdown_panel, 6, 0);
  lv_obj_set_flex_flow(dropdown_panel, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(dropdown_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(dropdown_panel, 8, 0);
  lv_obj_set_style_pad_gap(dropdown_panel, 25, 0);
  lv_obj_add_flag(dropdown_panel, LV_OBJ_FLAG_GESTURE_BUBBLE);

  /* 前三个切换按钮 */
  {
    const char *btn_labels[] = {"Torch", "DND", "Air"};
    for(int i = 0; i < 3; i++) {
      lv_obj_t *btn = lv_btn_create(dropdown_panel);
      lv_obj_set_width(btn, 0);
      lv_obj_set_height(btn, 48);
      // 让按钮在水平方向上平分可用空间，保持等宽布局
      lv_obj_set_style_flex_grow(btn, 1, 0);
      lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_set_style_radius(btn, 6, 0);
      lv_obj_set_style_border_width(btn, 0, 0);
      lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
      lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_GREY), 0);
      lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN | LV_STATE_CHECKED);
      lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
      lv_obj_t *lbl = lv_label_create(btn);
      lv_label_set_text(lbl, btn_labels[i]);
      lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
      lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
      lv_obj_center(lbl);
    }
  }

  /* 亮度按钮 + 滑块 */
  {
    brightness_btn = lv_btn_create(dropdown_panel);
    lv_obj_set_width(brightness_btn, 0);
    lv_obj_set_height(brightness_btn, 48);
    // 让亮度按钮占用剩余水平空间，使滑块与其他按钮等宽
    lv_obj_set_style_flex_grow(brightness_btn, 1, 0);
    lv_obj_clear_flag(brightness_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(brightness_btn, 6, 0);
    lv_obj_set_style_border_width(brightness_btn, 0, 0);
    lv_obj_set_style_bg_color(brightness_btn, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_bg_opa(brightness_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(brightness_btn, 0, 0);
    lv_obj_add_flag(brightness_btn, LV_OBJ_FLAG_GESTURE_BUBBLE);

    brightness_slider = lv_slider_create(brightness_btn);
    lv_slider_set_range(brightness_slider, 0, 100);
    lv_slider_set_value(brightness_slider, 100, LV_ANIM_OFF);
    lv_slider_set_orientation(brightness_slider, LV_SLIDER_ORIENTATION_VERTICAL);
    lv_obj_set_size(brightness_slider, LV_PCT(100), LV_PCT(100));
    lv_obj_center(brightness_slider);
    lv_obj_add_event_cb(brightness_slider, brightness_slider_block_bubble_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(brightness_slider, brightness_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_flag(brightness_slider, LV_OBJ_FLAG_GESTURE_BUBBLE);

    /* 隐藏滑块外观：MAIN/INDICATOR/KNOB 全透明且无边框/无外框/无阴影 */
    lv_obj_set_style_bg_opa(brightness_slider, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(brightness_slider, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(brightness_slider, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_border_width(brightness_slider, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(brightness_slider, 0, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(brightness_slider, 0, LV_PART_KNOB);
    lv_obj_set_style_outline_width(brightness_slider, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(brightness_slider, 0, LV_PART_INDICATOR);
    lv_obj_set_style_outline_width(brightness_slider, 0, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(brightness_slider, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(brightness_slider, 0, LV_PART_INDICATOR);
    lv_obj_set_style_shadow_width(brightness_slider, 0, LV_PART_KNOB);

    lv_obj_t *lbl = lv_label_create(brightness_btn);
    lv_label_set_text(lbl, "BRI ");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_center(lbl);

    brightness_fill = lv_obj_create(brightness_btn);
    lv_obj_set_size(brightness_fill, LV_PCT(100), 0);
    lv_obj_align(brightness_fill, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(brightness_fill, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_bg_opa(brightness_fill, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(brightness_fill, 6, 0);
    lv_obj_clear_flag(brightness_fill, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_border_width(brightness_fill, 0, 0);
    lv_obj_set_style_outline_width(brightness_fill, 0, 0);
    lv_obj_set_style_pad_all(brightness_fill, 0, 0);
    lv_obj_move_background(brightness_fill);
    lv_obj_move_foreground(lbl);
  }

    /* 亮度遮罩：放到最前面以模拟整体亮度（不响应点击，不影响交互）
      创建在这里可以保证 brightness_slider/brightness_fill 已存在 */
    brightness_overlay = lv_obj_create(scr);
    lv_obj_set_size(brightness_overlay, lv_obj_get_width(scr), lv_obj_get_height(scr));
    lv_obj_align(brightness_overlay, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(brightness_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(brightness_overlay, 0, 0);
    lv_obj_set_style_border_width(brightness_overlay, 0, 0);
    lv_obj_set_style_radius(brightness_overlay, 0, 0);
    lv_obj_clear_flag(brightness_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(brightness_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(brightness_overlay, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_move_foreground(brightness_overlay);

    /* 初始化一次遮罩与亮度按钮颜色（与滑块初始值同步）
      使用一次性定时器在布局完成后设置，避免父尺寸为0导致填充高度为0 */
    if(brightness_slider) {
     lv_timer_create(brightness_init_timer_cb, 20, NULL);
    }
}






// 创建一个彩色圆形模块（圆形条形图/进度环 + 中心图标 + 下方数值）
// arc_color: 圆弧颜色；icon: 显示在圆圈内的文本或符号；value: 下方显示的数值文本；percent: 0~100
static lv_obj_t * create_sensor_widget(lv_obj_t *parent, lv_color_t arc_color, const char *icon, const char *value, int percent)
{
  lv_obj_t * cont = lv_obj_create(parent);
  lv_obj_set_size(cont, 52, 66);
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_pad_all(cont, 0, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_radius(cont, 0, 0);

  if(percent < 0) percent = 0;
  if(percent > 100) percent = 100;

  // 圆形条形图：使用 lv_arc 实现“进度环”
  lv_obj_t *arc = lv_arc_create(cont);
    lv_obj_set_size(arc, 40, 40);
  lv_obj_align(arc, LV_ALIGN_TOP_MID, 0, 0);
  lv_arc_set_range(arc, 0, 100);
  lv_arc_set_value(arc, percent);
  lv_arc_set_bg_angles(arc, 0, 360);
  lv_arc_set_rotation(arc, 270); // 从上方开始
  lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);

  // 背景弧（灰色）+ 指示弧（彩色）
  lv_obj_set_style_arc_width(arc, 3, LV_PART_MAIN);
  lv_obj_set_style_arc_color(arc, lv_palette_lighten(LV_PALETTE_GREY, 1), LV_PART_MAIN);
  lv_obj_set_style_arc_width(arc, 3, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(arc, arc_color, LV_PART_INDICATOR);
  // 去掉 knob（小圆点）
  lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
  lv_obj_set_style_border_width(arc, 0, LV_PART_KNOB);

  // 圆中心图标（使用文本或符号）
  lv_obj_t *lbl_icon = lv_label_create(arc);
    lv_label_set_text(lbl_icon, icon);
    lv_obj_set_style_text_font(lbl_icon, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lbl_icon, arc_color, 0);
  lv_obj_center(lbl_icon);

  // 下方数值
  lv_obj_t *lbl_val = lv_label_create(cont);
    lv_label_set_text(lbl_val, value);
    lv_obj_set_style_text_font(lbl_val, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lbl_val, lv_color_white(), 0);
  lv_obj_align(lbl_val, LV_ALIGN_BOTTOM_MID, 0, -10);

  return cont;
}

// 调整手势灵敏度：增大最小距离和最小速度，避免误触发（尤其在小屏幕上）
static void ui_tune_gesture_sensitivity(void)
{
  /* 对当前已注册的所有输入设备生效（鼠标/触摸/编码器等） */
  for(lv_indev_t * indev = lv_indev_get_next(NULL); indev != NULL; indev = lv_indev_get_next(indev)) {
    lv_indev_set_gesture_min_distance(indev, UI_GESTURE_MIN_DISTANCE);
    lv_indev_set_gesture_min_velocity(indev, UI_GESTURE_MIN_VELOCITY);
  }
}



// 初始化并创建横屏启动主题界面
void myui_init(void)
{
  lv_obj_t * scr = lv_screen_active();
  lv_obj_clean(scr);
  ui_tune_gesture_sensitivity();
  //创建底色与基本样式
  create_base_style(scr);
  /* 全局左右滑切换：在 screen 上接收手势事件 */
  lv_obj_add_flag(scr, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(scr, global_swipe_event_cb, LV_EVENT_GESTURE, NULL);
  /* 关闭屏幕的滚动与滚动条，避免意外显示全局滚动条 */
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

  //1.左侧：大时间显示
  lbl_time = lv_label_create(scr);
  lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_28, 0);
  lv_label_set_text(lbl_time, "11:59");
  lv_obj_set_size(lbl_time, 160, 40);
  lv_obj_align(lbl_time, LV_ALIGN_LEFT_MID, 16, -50);
  lv_obj_set_style_text_color(lbl_time, lv_color_white(), 0);

  //2.右上：日期/星期
  lbl_date = lv_label_create(scr);
  lv_obj_set_style_text_font(lbl_date, &lv_font_montserrat_14, 0);
  lv_label_set_text(lbl_date, "11-08 Tue.");
  lv_obj_align(lbl_date, LV_ALIGN_TOP_RIGHT, -12, 8);
  lv_obj_set_style_text_color(lbl_date, lv_color_white(), 0);

  //3.中部：步数与进度条
  lbl_steps = lv_label_create(scr);
  lv_obj_set_style_text_font(lbl_steps, &lv_font_montserrat_16, 0);
  lv_label_set_text_fmt(lbl_steps, "今日步数\n%u", SENSOR_STEPS_INIT);
  lv_obj_align(lbl_steps, LV_ALIGN_LEFT_MID, 16, -20);
  lv_obj_set_style_text_color(lbl_steps, lv_color_white(), 0);

  bar_steps = lv_bar_create(scr);
  lv_obj_set_size(bar_steps, 200, 10);
  lv_obj_align_to(bar_steps, lbl_steps, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);
  lv_bar_set_range(bar_steps, 0, 10000);
  lv_bar_set_value(bar_steps, SENSOR_STEPS_INIT > 10000 ? 10000 : SENSOR_STEPS_INIT, LV_ANIM_OFF);
  /* 设定进度条颜色，提高可见性 */
  lv_obj_set_style_bg_color(bar_steps, lv_palette_lighten(LV_PALETTE_GREY, 1), 0);
  lv_obj_set_style_bg_color(bar_steps, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
  lv_obj_add_flag(bar_steps, LV_OBJ_FLAG_GESTURE_BUBBLE);

  // 右侧容器（详细数值文本区域）
  // 说明：该容器用于显示详细的传感器数值文本（HR、SpO2、温湿度、海拔、方向、电量等），
  //      主要作为阅读型数值展示；底部彩色圆形模块为视觉化快速查看（环形进度 + 图标），
  //      两者在部分数据上会存在形式上的重复（例如温度/心率），但职责不同：右侧为详细文本，底部为视觉组件。
  cont_right = lv_obj_create(scr);
  lv_obj_set_size(cont_right, 140, 140);
  /* 将右侧容器对齐到顶部右侧，避免与底部彩色圆形模块重叠 */
  lv_obj_align(cont_right, LV_ALIGN_TOP_RIGHT, -12, 36);
  lv_obj_set_style_bg_opa(cont_right, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cont_right, 0, 0);
  lv_obj_set_style_radius(cont_right, 0, 0);
  lv_obj_set_flex_flow(cont_right, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont_right, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(cont_right, 6, 0);
  /* 右侧容器不需要滚动条，关闭以避免显示滑块 */
  lv_obj_clear_flag(cont_right, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(cont_right, LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_flag(cont_right, LV_OBJ_FLAG_GESTURE_BUBBLE);

  //4.右侧保留详细数值：血氧、海拔、方向、电量（心率、温湿度等在底部视觉模块显示）
  lbl_spo2 = lv_label_create(cont_right);
  lv_label_set_text_fmt(lbl_spo2, "SpO2: %u%%", SENSOR_SPO2_INIT);
  lv_obj_set_style_text_font(lbl_spo2, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lbl_spo2, lv_color_white(), 0);
  lbl_alt = lv_label_create(cont_right);
  lv_label_set_text_fmt(lbl_alt, "Alt:%dm", SENSOR_ALTITUDE_INIT);
  lv_obj_set_style_text_font(lbl_alt, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lbl_alt, lv_color_white(), 0);
  lbl_compass = lv_label_create(cont_right);
  lv_label_set_text_fmt(lbl_compass, "Dir:%d°", SENSOR_COMPASS_INIT);
  lv_obj_set_style_text_font(lbl_compass, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lbl_compass, lv_color_white(), 0);

  lbl_batt = lv_label_create(cont_right);
  lv_label_set_text_fmt(lbl_batt, "Bat:%u%%", SENSOR_BATTERY_INIT);
  lv_obj_set_style_text_font(lbl_batt, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lbl_batt, lv_color_white(), 0);

  //5.底部：彩色圆形传感器模块（视觉化模块）
  // 说明：底部为三个彩色圆形视觉模块，展示温度/湿度/心率的环形进度与图标，
  //      侧重视觉效果与快速读数。
  lv_obj_t *cont_bottom = lv_obj_create(scr);
  lv_obj_set_size(cont_bottom, 320, 100);
  lv_obj_align(cont_bottom, LV_ALIGN_BOTTOM_MID, 0, 10);
  lv_obj_set_style_bg_opa(cont_bottom, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cont_bottom, 0, 0);
  lv_obj_set_style_radius(cont_bottom, 0, 0);
  lv_obj_set_flex_flow(cont_bottom, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(cont_bottom, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  /* 底部容器也关闭滚动条显示（视觉组件不应滚动） */
  lv_obj_clear_flag(cont_bottom, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(cont_bottom, LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_flag(cont_bottom, LV_OBJ_FLAG_GESTURE_BUBBLE);

  // 三个模块：橙色（温度），青色（水滴/湿度），红色（心率）
  char tmpbuf[32];
  snprintf(tmpbuf, sizeof(tmpbuf), "%d°", SENSOR_TEMP_INIT);
  create_sensor_widget(cont_bottom, lv_palette_main(LV_PALETTE_ORANGE), "°C", tmpbuf, 30);

  snprintf(tmpbuf, sizeof(tmpbuf), "%d%%", SENSOR_HUMIDITY_INIT);
  create_sensor_widget(cont_bottom, lv_palette_main(LV_PALETTE_BLUE), "H2O", tmpbuf, SENSOR_HUMIDITY_INIT);

  snprintf(tmpbuf, sizeof(tmpbuf), "%d", SENSOR_HEARTRATE_INIT);
  create_sensor_widget(cont_bottom, lv_palette_main(LV_PALETTE_RED), "HR", tmpbuf, 78);

  /* 6.使用封装函数创建顶部下拉面板 */
  create_dropdown_panel(scr);

  /* 7.创建左滑菜单面板（隐藏在左侧），用于应用列表 */
  create_left_menu(scr);

}




// 更新界面上的传感器显示（占位/接口）
// 这里使用常量作为占位值，后续请将真实传感器数据在观察者回调中写入并调用该函数
void myui_update_sensor_values(void)
{
  if(lbl_steps) lv_label_set_text_fmt(lbl_steps, "今日步数\n%u", SENSOR_STEPS_INIT);
  if(bar_steps) lv_bar_set_value(bar_steps, SENSOR_STEPS_INIT > 10000 ? 10000 : SENSOR_STEPS_INIT, LV_ANIM_OFF);
  if(lbl_spo2) lv_label_set_text_fmt(lbl_spo2, "SpO2: %u%%", SENSOR_SPO2_INIT);
  if(lbl_alt) lv_label_set_text_fmt(lbl_alt, "Alt: %dm", SENSOR_ALTITUDE_INIT);
  if(lbl_compass) lv_label_set_text_fmt(lbl_compass, "Dir:%d°", SENSOR_COMPASS_INIT);
  if(lbl_batt) lv_label_set_text_fmt(lbl_batt, "Bat:%u%%", SENSOR_BATTERY_INIT);
}


/*
 * 观察者绑定提示：
 * - 真实项目中可实现一个注册函数：myui_register_sensor_observer(callback)，
 *   在回调中更新相应传感器的全局变量或直接更新标签并调用 `myui_update_sensor_values()`。
 */
