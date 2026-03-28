#include "myui.h"


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
static lv_obj_t *dropdown_grab_area; // 屏幕顶部抓取区（面板隐藏时也能被按到）

// 亮度滑块阻止事件冒泡：避免与 dropdown_panel 的拖拽逻辑冲突（否则可能拖不动/卡死）
static void brightness_slider_block_bubble_cb(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);

  /* 这些事件在拖动时会高频触发；若冒泡到 dropdown_panel，会触发面板跟随拖动 */
  if(code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING || code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST ||
     code == LV_EVENT_CLICKED || code == LV_EVENT_GESTURE) {
    lv_event_stop_bubbling(e);
  }
}
// 亮度滑块事件：改变遮罩透明度，并按规则切换按钮颜色
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

/* 一次性定时器：在布局完成后初始化 brightness_fill 的高度（避免初始高度为 0） */
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


// 顶部抓取区事件回调：处理按下、按住移动和释放，移动 dropdown_panel 的 Y
static void dropdown_drag_event_cb(lv_event_t * e)
{
  if(!dropdown_panel) return; // 如果面板未创建，直接返回（保护性判断）
  lv_event_code_t code = lv_event_get_code(e); // 获取当前事件类型（按下/移动/释放等）
  lv_indev_t * indev = lv_indev_get_act(); // 获取当前活动的输入设备（触摸/鼠标）
  if(!indev) return; // 如果没有有效输入设备，返回
  lv_point_t p;
  /* lv_indev_get_point 返回 void（直接填充 p），不要在表达式中比较其返回值 */
  lv_indev_get_point(indev, &p); // 读取当前输入点坐标到 p（以屏幕坐标表示）

  static lv_coord_t start_y_touch;   // 记录按下时触点的 Y（用于计算拖动位移）
  static lv_coord_t start_panel_y;   // 记录按下时面板的 Y（用于计算拖动基准）
  const lv_coord_t panel_h = lv_obj_get_height(dropdown_panel); // 面板高度（用于边界约束与吸附计算）

  if(code == LV_EVENT_PRESSED) {
    // 事件：刚按下（触控按下或鼠标按下）
    start_y_touch = p.y; // 记录当前触点 Y，用作后续拖动的参考起点
    start_panel_y = lv_obj_get_y(dropdown_panel); // 记录面板当前 Y 位置作为拖动基准
  } else if(code == LV_EVENT_PRESSING) {
    // 事件：持续按压并移动（拖动进行中）
    lv_coord_t dy = p.y - start_y_touch; // 计算触点位移（相对于初始按下点）
    lv_coord_t newy = start_panel_y + dy; // 计算面板的新 Y = 面板起始 Y + 触点位移

    // 限制 newy，避免面板移动到屏幕下方或过度上移
    if(newy > 0) newy = 0;           // 不允许面板低于屏幕顶端（Y>0 表示向下突出）
    if(newy < -panel_h) newy = -panel_h; // 不允许面板完全上移超过自身高度（完全隐藏）

    lv_obj_set_y(dropdown_panel, newy); // 实时设置面板 Y，产生跟随拖动的效果
  } else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    /* 事件：释放或按压丢失（例如手指抬起或触控被外部中断）
       改进：优先根据拖动方向决定吸附结果（向上收起、向下展开），
       若拖动距离很小则回退到基于位置的阈值判断（更容易收起） */
    lv_coord_t cur_y = lv_obj_get_y(dropdown_panel); // 当前面板 Y
    lv_coord_t dy = p.y - start_y_touch; // 相对按下点的位移（正为向下，负为向上）
    lv_coord_t target_y;

    /* 优先使用方向判断——向上（dy < 0）意图收起，向下（dy > 0）意图展开 */
    const lv_coord_t dir_move_thresh = 8; /* 像素阈值：移动小于该值视为未明确意图 */
    if(dy < -dir_move_thresh) {
      target_y = -panel_h*1.5; /* 向上移动 -> 收起 */
    } else if(dy > dir_move_thresh) {
      target_y = 0; /* 向下移动 -> 展开 */
    } else {
      /* 未明显移动：使用位置阈值判断，阈值放宽为 panel_h/3，
         使得在不大幅露出的情况下更容易收起（用户体验更自然） */
      const lv_coord_t snap_thresh = panel_h / 3;
      if(cur_y > -snap_thresh) target_y = 0; else target_y = -panel_h*1.5;
    }

    /* 取消可能正在运行的动画，避免与新的吸附动画冲突 */
    lv_anim_del(dropdown_panel, (lv_anim_exec_xcb_t)lv_obj_set_y);

    /* 创建并启动一个短时动画，把面板平滑移动到目标位置 */
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, dropdown_panel); // 指定要动画的对象
    lv_anim_set_values(&a, cur_y, target_y); // 动画起始值和目标值
    lv_anim_set_time(&a, 150); // 动画时长 150ms（短平滑过渡）
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y); // 动画回调：设置对象 Y
    lv_anim_start(&a);
  }
}


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


// 初始化并创建横屏启动主题界面
void myui_init(void)
{
  lv_obj_t * scr = lv_screen_active(); // 屏幕假定为横屏
  lv_obj_clean(scr);
  //创建底色与基本样式
  create_base_style(scr);
  /* 关闭屏幕的滚动与滚动条，避免意外显示全局滚动条 */
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

  // 左侧：大时间显示
  lbl_time = lv_label_create(scr);
  lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_28, 0);
  lv_label_set_text(lbl_time, "11:59");
  lv_obj_set_size(lbl_time, 160, 40);
  lv_obj_align(lbl_time, LV_ALIGN_LEFT_MID, 16, -50);
  lv_obj_set_style_text_color(lbl_time, lv_color_white(), 0);

  // 右上：日期/星期
  lbl_date = lv_label_create(scr);
  lv_obj_set_style_text_font(lbl_date, &lv_font_montserrat_14, 0);
  lv_label_set_text(lbl_date, "11-08 Tue.");
  lv_obj_align(lbl_date, LV_ALIGN_TOP_RIGHT, -12, 8);
  lv_obj_set_style_text_color(lbl_date, lv_color_white(), 0);

  // 中部：步数与进度条
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

  // 右侧保留详细数值：血氧、海拔、方向、电量（心率、温湿度等在底部视觉模块显示）
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

  // 底部：彩色圆形传感器模块（视觉化模块）
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

  // 三个模块：橙色（温度），青色（水滴/湿度），红色（心率）
  char tmpbuf[32];
  snprintf(tmpbuf, sizeof(tmpbuf), "%d°", SENSOR_TEMP_INIT);
  create_sensor_widget(cont_bottom, lv_palette_main(LV_PALETTE_ORANGE), "°C", tmpbuf, 30);

  snprintf(tmpbuf, sizeof(tmpbuf), "%d%%", SENSOR_HUMIDITY_INIT);
  create_sensor_widget(cont_bottom, lv_palette_main(LV_PALETTE_BLUE), "H2O", tmpbuf, SENSOR_HUMIDITY_INIT);

  snprintf(tmpbuf, sizeof(tmpbuf), "%d", SENSOR_HEARTRATE_INIT);
  create_sensor_widget(cont_bottom, lv_palette_main(LV_PALETTE_RED), "HR", tmpbuf, 78);

  // 最终刷新一次（占位）
  myui_update_sensor_values();

  // 创建“屏幕顶部抓取区”（透明条）：用于在 dropdown_panel 完全隐藏到屏幕外时仍可触发拖拽
  // 说明：抓取区放在普通内容之上，但放在 dropdown_panel 之下（因为面板稍后创建，Z序更高），
  //      这样面板展开后按钮仍可正常点击，不会被抓取区遮挡。
  {
    const lv_coord_t grab_h = 20; // 抓取区高度（越大越容易按到，但不要太大以免误触）
    dropdown_grab_area = lv_obj_create(scr);
    lv_obj_set_size(dropdown_grab_area, lv_obj_get_width(scr), grab_h);
    lv_obj_align(dropdown_grab_area, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_flag(dropdown_grab_area, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(dropdown_grab_area, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_clear_flag(dropdown_grab_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(dropdown_grab_area, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(dropdown_grab_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dropdown_grab_area, 0, 0);
    lv_obj_set_style_outline_width(dropdown_grab_area, 0, 0);
    lv_obj_set_style_pad_all(dropdown_grab_area, 0, 0);
    lv_obj_add_event_cb(dropdown_grab_area, dropdown_drag_event_cb, LV_EVENT_ALL, NULL);
  }

  // 创建顶部下拉面板（默认隐藏）
  {
    const lv_coord_t panel_h = 64;
    dropdown_panel = lv_obj_create(scr);
    lv_obj_set_size(dropdown_panel, lv_obj_get_width(scr), panel_h);
    lv_obj_align(dropdown_panel, LV_ALIGN_TOP_MID, 0, -panel_h*1.5); // 隐藏到屏幕上方
    /* 使面板支持吸附行为（snap） */
    lv_obj_add_flag(dropdown_panel, LV_OBJ_FLAG_SNAPPABLE);
    /* 关闭滚动行为并隐藏滚动条，避免显示白色滑块 */
    lv_obj_clear_flag(dropdown_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(dropdown_panel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(dropdown_panel, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(dropdown_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dropdown_panel, 0, 0);
    lv_obj_set_style_radius(dropdown_panel, 6, 0);
    lv_obj_set_flex_flow(dropdown_panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dropdown_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(dropdown_panel, 8, 0);
    /* 按钮之间的间距（横向），增大以减少触摸误触 */
    lv_obj_set_style_pad_gap(dropdown_panel, 25, 0);

    // 前三个按钮：默认灰色，每次按下在“灰色 <-> 蓝色”之间切换
    {
      const char *btn_labels[] = {"Torch", "DND", "Air"};
      for(int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_btn_create(dropdown_panel);
        /* 让 3 个按钮横向均分空间，铺满 dropdown_panel */
        lv_obj_set_width(btn, 0);
        lv_obj_set_height(btn, 48);
        lv_obj_set_style_flex_grow(btn, 1, 0);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_set_style_border_width(btn, 0, 0);

        /* 关键：设置为可切换（checked），并用状态样式控制颜色 */
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_GREY), 0);
        lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);

        /* 文本占位（可替换为图标/自定义字体） */
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btn_labels[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
        lv_obj_center(lbl);
      }
    }

    // 第四个“按钮”：亮度调节（竖向滑块）
    {
      brightness_btn = lv_btn_create(dropdown_panel);
      /* 与其它按钮一致：占 1/4 宽度 */
      lv_obj_set_width(brightness_btn, 0);
      lv_obj_set_height(brightness_btn, 48);
      lv_obj_set_style_flex_grow(brightness_btn, 1, 0);
      lv_obj_clear_flag(brightness_btn, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_set_style_radius(brightness_btn, 6, 0);
      lv_obj_set_style_border_width(brightness_btn, 0, 0);
      /* 按钮背景使用灰色，蓝色填充使用子对象实现（底部增长） */
      lv_obj_set_style_bg_color(brightness_btn, lv_palette_main(LV_PALETTE_GREY), 0);
      lv_obj_set_style_bg_opa(brightness_btn, LV_OPA_COVER, 0);
      /* 让滑块可覆盖整个按钮区域（更像“滑动按钮”） */
      lv_obj_set_style_pad_all(brightness_btn, 0, 0);

      /* 亮度滑块：拖到底=0（最暗），拖到顶=100（最亮） */
      brightness_slider = lv_slider_create(brightness_btn);
      lv_slider_set_range(brightness_slider, 0, 100);
      lv_slider_set_value(brightness_slider, 100, LV_ANIM_OFF);
      lv_slider_set_orientation(brightness_slider, LV_SLIDER_ORIENTATION_VERTICAL);
      /* 关键：让滑块铺满按钮，但把滑块的“外观痕迹”全部隐藏 */
      /* 注意：brightness_btn 在 flex 布局生效前宽度可能为 0，所以这里用百分比铺满 */
      lv_obj_set_size(brightness_slider, LV_PCT(100), LV_PCT(100));
      lv_obj_center(brightness_slider);
      /* 先阻止冒泡，再处理数值变化 */
      lv_obj_add_event_cb(brightness_slider, brightness_slider_block_bubble_cb, LV_EVENT_ALL, NULL);
      lv_obj_add_event_cb(brightness_slider, brightness_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

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

      /* 文本提示（不影响滑块拖动） */
      lv_obj_t *lbl = lv_label_create(brightness_btn);
      lv_label_set_text(lbl, "BRI "); // lv_font_montserrat_12亮度调节
      lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
      lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
      lv_obj_center(lbl);
      /* 防止标签抢占点击导致滑块收不到拖动 */
      lv_obj_clear_flag(lbl, LV_OBJ_FLAG_CLICKABLE);

      /* 亮度按钮内部的蓝色填充：从底部增长，高度由滑块回调控制 */
      brightness_fill = lv_obj_create(brightness_btn);
      lv_obj_set_size(brightness_fill, LV_PCT(100), 0); /* 初始高度为0，稍后由回调设置 */
      lv_obj_align(brightness_fill, LV_ALIGN_BOTTOM_MID, 0, 0);
      lv_obj_set_style_bg_color(brightness_fill, lv_palette_main(LV_PALETTE_BLUE), 0);
      lv_obj_set_style_bg_opa(brightness_fill, LV_OPA_COVER, 0);
      lv_obj_set_style_radius(brightness_fill, 6, 0);
      lv_obj_clear_flag(brightness_fill, LV_OBJ_FLAG_CLICKABLE);
      /* 去掉可能的边框或描边，避免出现白色轮廓 */
      lv_obj_set_style_border_width(brightness_fill, 0, 0);
      lv_obj_set_style_outline_width(brightness_fill, 0, 0);
      lv_obj_set_style_pad_all(brightness_fill, 0, 0);
      /* 将填充放到按钮背景之上但在文本/滑块之下：先移动到背景，再把标签置前 */
      lv_obj_move_background(brightness_fill);
      lv_obj_move_foreground(lbl);
    }

    // 允许直接在面板上拖拽（更自然），确保同一回调绑定到面板
    lv_obj_add_flag(dropdown_panel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_border_width(dropdown_panel, 0, 0);
    lv_obj_set_style_outline_width(dropdown_panel, 0, 0);
    lv_obj_add_event_cb(dropdown_panel, dropdown_drag_event_cb, LV_EVENT_ALL, NULL);
  }

  /* 亮度遮罩：放到最前面以模拟整体亮度（不响应点击，不影响交互） */
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
