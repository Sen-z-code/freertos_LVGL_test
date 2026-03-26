#include "myui.h"

//将温度变量添加到一个主题 要么静态要么全局 这里选择全局
lv_subject_t  temperature_subj;

//外部参数的观察者回调函数
//observer:同一个主题的回调函数可以有多个观察者，通过这个参数可以用来确定是哪个观察者触发的回调函数
//subject: 添加观察者的主题对象，即对应观察的对象
void temperature_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    //获取上一次的温度值
    int32_t prev_temp = lv_subject_get_previous_int(subject);
    //获取温度值
    int32_t temp = lv_subject_get_int(subject);
    printf("Temperature changed: %d -> %d\n", prev_temp, temp);
}

//温度标签观察者回调：温度变化时按当前语言刷新文本并保持居中
static void temperature_label_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * label = lv_observer_get_target_obj(observer);
    lv_obj_t * bar = lv_observer_get_user_data(observer);
    int32_t temp = lv_subject_get_int(subject);

    lv_label_set_text_fmt(label, lv_tr("Temperature: %d °C"), temp);
    if(bar) {
        lv_obj_align_to(label, bar, LV_ALIGN_CENTER, 0, 0);
    }
}

//按钮的事件回调函数
static void increase_temperature_cb(lv_event_t * e)
{
    //获取事件类型
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        //增加温度值
        int32_t temp = lv_subject_get_int(&temperature_subj);
        temp += 1;
        //更新主题的温度值
        lv_subject_set_int(&temperature_subj, temp);
    }
}
static void decrease_temperature_cb(lv_event_t * e)
{
    //获取事件类型
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        //减少温度值
        int32_t temp = lv_subject_get_int(&temperature_subj);
        temp -= 1;
        //更新主题的温度值
        lv_subject_set_int(&temperature_subj, temp);
    }
}

//创建用于翻译的静态数组
static const char *const languages[] = {"en", "Chinese", NULL};
static const char *const tags[] = {"Tab1", "Tab2", "Temperature: %d °C", NULL};
static const char *const translations[] = {
    "Tab1", "设置",
    "Tab2", "返回",
    "Temperature: %d °C", "温度: %d °C"
};
//下拉菜单事件回调函数
static void language_selection_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED){
        lv_obj_t * dropdown = lv_event_get_current_target(e);
        //获取下拉菜单的选中项索引下标
        uint16_t selected = lv_dropdown_get_selected(dropdown);
        //根据索引下标执行相应的操作
        if(selected == 0){
            lv_translation_set_language("en");
        }else if(selected == 1){
            lv_translation_set_language("Chinese");
        }
    }
}
//翻译标签的事件回调函数
static void language_change_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_TRANSLATION_LANGUAGE_CHANGED){
        //获取标签对象
        lv_obj_t * label = lv_event_get_current_target_obj(e);
        //获取标签的文本标识符
        char *tag = lv_event_get_user_data(e);
        lv_label_set_text(label, lv_tr(tag));
    }
}
//翻译标签的事件回调函数2
static void language_change_cb2(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_TRANSLATION_LANGUAGE_CHANGED){
        //语言切换后通知一次主题，复用观察者回调刷新文本，避免重复绑定
        lv_subject_notify(&temperature_subj);
    }
}

//旋转框的按钮事件回调函数
static void add_btn_event_cb(lv_event_t * e)
{
    // 处理加号按钮点击事件
    //点击一次就让旋转框的的数值加1
    //获取按钮的父对象从而获取旋转框对象
    lv_obj_t * parent = lv_obj_get_parent(lv_event_get_target(e));
    //获取旋转框对象
    lv_obj_t * spinbox = lv_obj_get_child(parent, 0);
    //增加旋转框的数值
    lv_spinbox_increment(spinbox);
}
static void sub_btn_event_cb(lv_event_t * e)
{
    // 处理减号按钮点击事件
    //点击一次就让旋转框的的数值减1
    //获取按钮的父对象从而获取旋转框对象
    lv_obj_t * parent = lv_obj_get_parent(lv_event_get_target(e));
    //获取旋转框对象
    lv_obj_t * spinbox = lv_obj_get_child(parent, 0);
    //减少旋转框的数值
    lv_spinbox_decrement(spinbox);
}

//滚动条事件回调函数
static void roller_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        //获取滚轮对象内容
        char buf[32];
        lv_roller_get_selected_str(obj, buf, sizeof(buf));
        LV_LOG_USER("Selected month: %s\n", buf);
    }
}

//下拉菜单事件回调函数
static void my_dropdown_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED){
        lv_obj_t * dropdown = lv_event_get_current_target(e);
        //获取下拉菜单的选中项索引下标
        uint16_t selected = lv_dropdown_get_selected(dropdown);
        //根据索引下标执行相应的操作
        if((selected+1) == 1){
            printf("option 1 selected\n");
        }else if((selected+1) == 2){
            printf("option 2 selected\n");
        }else if((selected+1) == 3){
            printf("option 3 selected\n");
        }else if((selected+1) == 4){
            printf("option 4 selected\n");
        }
    }
}


//消息框事件回调函数
static void yes_cb(lv_event_t * e)
{
    // 处理"是"按钮点击事件
    printf("yes button clicked\n");
    // 从当前回调对象向上查找所属的 msgbox，然后安全关闭
    lv_obj_t * obj = lv_event_get_current_target(e);
    if(!obj) obj = lv_event_get_target(e);
    while(obj && !lv_obj_check_type(obj, &lv_msgbox_class)) {
        obj = lv_obj_get_parent(obj);
    }
    if(obj) lv_msgbox_close(obj);
}
static void no_cb(lv_event_t * e)
{
    // 处理"否"按钮点击事件
    printf("no button clicked\n");
    // 从当前回调对象向上查找所属的 msgbox，然后安全关闭
    lv_obj_t * obj = lv_event_get_current_target(e);
    if(!obj) obj = lv_event_get_target(e);
    while(obj && !lv_obj_check_type(obj, &lv_msgbox_class)) {
        obj = lv_obj_get_parent(obj);
    }
    if(obj) lv_msgbox_close(obj);
}

//开关事件处理函数
static void my_switch_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED){
        //获取开关对象
        lv_obj_t * sw = lv_event_get_current_target(e);
        //获取开关的状态
        bool state = lv_obj_has_state(sw, LV_STATE_CHECKED);
        if(state){
            printf("switch is ON\n");
        }else{
            printf("switch is OFF\n");
        }
    }
}



static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
        lv_obj_t * win = lv_obj_get_parent(btn);
        while(win && !lv_obj_check_type(win, &lv_win_class)) {
            win = lv_obj_get_parent(win);
        }
        if(win) lv_obj_del(win);
    }
}

//表格事件回调函数
static void my_table_event_cb(lv_event_t * event){
    if(lv_event_get_code(event) == LV_EVENT_VALUE_CHANGED){
        lv_obj_t * table = lv_event_get_current_target(event);
        //获取被点击的单元格的行列索引
        uint32_t row,col;
        lv_table_get_selected_cell(table, &row, &col);
        printf("clicked cell: row=%d, col=%d\n", row, col);
        //根据行列索引获取单元格的值
        const char * cell_value = lv_table_get_cell_value(table, row, col);
        printf("cell value: %s\n", cell_value);
    }
}

//日历事件回调函数
static void my_calendar_event_cb(lv_event_t * event){
    if(lv_event_get_code(event) == LV_EVENT_VALUE_CHANGED){
        lv_obj_t * calendar = lv_event_get_current_target(event);
        static lv_calendar_date_t date;
        //获取被点击的日期
        lv_calendar_get_pressed_date(calendar, &date);
        printf("selected date: %04d-%02d-%02d\n", date.year, date.month, date.day);
        //点击日期后，高亮显示被点击的日期
        lv_calendar_set_highlighted_dates(calendar, &date, 1);

    }
}

//1.创建动画
static lv_anim_t   anim_template;//动画模板
static lv_anim_t * running_anim;//当前正在运行的动画
//8.动画执行回调函数
static void anim_exec_cb(void * var, int32_t value){
    lv_obj_t * obj = (lv_obj_t *)var;
    //修改label的x坐标
    lv_obj_set_x(obj, value);
}
//7.按钮事件回调函数
static void my_button_event_cb(lv_event_t * event){
    if(lv_event_get_code(event) == LV_EVENT_PRESSED){
        printf("button pressed\n");

        //按钮点击一次使滑块的值增加1
        lv_obj_t * slider = lv_obj_get_child(lv_obj_get_parent(lv_event_get_target(event)), 1);//获取滑块对象
        int value = lv_slider_get_value(slider);
        lv_slider_set_value(slider, value + 1, LV_ANIM_OFF);

        //手动触发滑块的事件回调函数
        lv_obj_send_event(slider, LV_EVENT_VALUE_CHANGED, NULL);

        static uint8_t counter = 0;
        counter++;
        if(counter % 2 == 0){
            //设置动画的起始值和结束值
            lv_anim_set_values(&anim_template, 150,-150);
            counter=0;
        }else{
            lv_anim_set_values(&anim_template, -150,150);
        }
        // 按下按钮触
        running_anim=lv_anim_start(&anim_template);
    }
}

//滑块事件回调函数
static void my_slider_event_cb(lv_event_t * event){
    if(lv_event_get_code(event) == LV_EVENT_VALUE_CHANGED){
        lv_obj_t * slider = lv_event_get_target(event);
        int value = lv_slider_get_value(slider);
        printf("slider value: %d\n", value);
    }
}

//初始化ui界面
void myui_init(void)
{
  //1.获取当前硬件整个屏幕
  lv_obj_t * scr = lv_screen_active();//屏幕为240*320

//   //2.创建一个新的屏幕--->有个默认大小100*100
//   lv_obj_t * new_scr = lv_obj_create(scr);

//   //2.1修改新屏幕的大小和位置
//   lv_obj_set_size(new_scr, 80, 80);
//   lv_obj_set_pos(new_scr, 80, 120);


//   //3.创建一个按钮
//   lv_obj_t * btn = lv_button_create(new_scr);

//   //3.1修改按钮的大小和位置
//   lv_obj_set_size(btn, LV_PCT(80), LV_PCT(80));
//   lv_obj_set_pos(btn, LV_PCT(10), LV_PCT(10));

//   //4.修改新屏幕的样式
//   lv_obj_set_style_border_width(new_scr, 0, LV_PART_MAIN);
//   lv_obj_set_style_pad_all(new_scr, 0, LV_PART_MAIN);
//   lv_obj_set_style_radius(new_scr, 0, LV_PART_MAIN);

//   //5.相对位置
// //   lv_obj_set_align(btn, LV_ALIGN_CENTER);//相对于父对象居中
// //   lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 10);//相对于父对象顶部居中，向下偏移10像素
// //   lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);//相对于父对象底部左对齐，向右偏移10像素，向上偏移10像素

//    //按钮未被按下时的样式
//    lv_obj_set_style_bg_color(btn, lv_color_lighten(lv_palette_main(LV_PALETTE_GREEN), 100), LV_PART_MAIN);
//    //按钮被按下时的样式
//    lv_obj_set_style_bg_color(btn, lv_color_darken(lv_palette_main(LV_PALETTE_PINK), 255), LV_PART_MAIN | LV_STATE_PRESSED);

// /* Only its pointer is saved so must static, global or dynamically allocated */
// static const lv_style_prop_t trans_props[] = {
//                                             LV_STYLE_BG_OPA,//要过渡的属性：背景不透明度
//                                             LV_STYLE_BG_COLOR,//要过渡的属性：背景颜色
//                                             0, /* End marker */
// };

// static lv_style_transition_dsc_t trans1;
// //（1）存储过渡效果的结构体（2）存储过渡效果的数组（3）动画缓动函数（缓出）（4）持续时间（5）过渡前的延迟时间（6）用户数据，动画缓动函数的参数
// lv_style_transition_dsc_init(&trans1, trans_props, lv_anim_path_ease_out, 800, 10,NULL);
// //定义并初始化一个样式对象
// static lv_style_t style1;
// lv_style_init(&style1);
// //添加过渡效果给样式
// lv_style_set_transition(&style1, &trans1);
// //将样式应用到按钮上=>进入到按钮的主状态（LV_PART_MAIN）时会有过渡效果
// lv_obj_add_style(btn, &style1, LV_PART_MAIN);

// //添加按钮的事件回调函数 （1）按钮对象（2）事件回调函数（名称自定义）（3）事件类型：按下事件（4）用户数据）
// lv_obj_add_event_cb(btn, my_button_event_cb, LV_EVENT_PRESSED, NULL);

// //添加滑块
// lv_obj_t * slider = lv_slider_create(new_scr);
// lv_obj_set_size(slider, LV_PCT(80), LV_PCT(10));
// lv_obj_set_pos(slider, LV_PCT(10), LV_PCT(60));

// //添加滑块值变化的事件回调函数
// lv_obj_add_event_cb(slider, my_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

// //禁止水平和垂直方向的滚动链传递，避免滑块被父对象的滚动事件干扰
// lv_obj_remove_flag(slider, LV_OBJ_FLAG_SCROLL_CHAIN);


// //创建折线
// lv_obj_t * line = lv_line_create(scr);
// //添加折线的样式
// lv_obj_set_style_line_color(line, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);//颜色
// lv_obj_set_style_line_width(line, 3, LV_PART_MAIN);//线宽
// lv_obj_set_style_arc_rounded(line, true, LV_PART_MAIN);//圆角
// //定义折线的点坐标
// static const lv_point_precise_t line_points[] = {{0, 0}, {20, 30}, {40, 10}, {60, 40}, {80, 20}};
// //设置折线的点坐标
// lv_line_set_points(line, line_points,5);

// //创建一个标签并设置文本
// lv_obj_t * label = lv_label_create(btn);
// //文本颜色为蓝色
// lv_label_set_text_fmt(label, "#0000ff Hello LVGL!#");
// //长模式设置为滚动
// lv_obj_set_size(label, LV_PCT(80), LV_PCT(80));
// lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
// //启用文本重色功能
// lv_label_set_recolor(label, true);

// lv_obj_t * label_2 = lv_label_create(scr);
// lv_label_set_text_fmt(label_2, "#ff0000 anim test#");
// //label_2的初始位置
// lv_obj_set_pos(label_2, -100, 0);
// lv_label_set_recolor(label_2, true);



// //2.初始化动画模板
// lv_anim_init(&anim_template);
// //3.创建动画的执行回调函数为anim_exec_cb，动画的起始值为0，结束值为200
// lv_anim_set_exec_cb(&anim_template, (lv_anim_exec_xcb_t)anim_exec_cb);
// //4.添加动画控制对象
// lv_anim_set_var(&anim_template, label_2);
// //5.动画运行时间为500ms
// lv_anim_set_duration(&anim_template, 500);
// //6.设置动画的起始值和结束值
// lv_anim_set_values(&anim_template, -150,150);

// lv_anim_set_reverse_duration(&anim_template,1000);//设置动画的反向运行时间为1000ms
// lv_anim_set_reverse_delay(&anim_template,500);//设置动画的反向运行前的延迟时间为500毫秒

//创建矢量动画示例
// extern const uint8_t lv_example_lottie_approve[];
// extern const size_t lv_example_lottie_approve_size;
// //1，创建一个矢量动画对象
// lv_obj_t * lottie = lv_lottie_create(scr);
// //2.设置矢量动画的数据源
// //绝对路径方式
// lv_lottie_set_src_file(lottie, "E:/LVGL_PC_kaifa/LVGL/lv_port_pc_vscode-master/lvgl/examples/widgets/lottie/Loader cat.json");
// //设置矢量动画需要的缓冲区大小=>根据具体文件大小调整
// static uint8_t lottie_buf[280 * 200*4];//4字节一个像素
// lv_lottie_set_buffer(lottie, 280 ,200, lottie_buf);
// //3.使矢量动画居中显示
// lv_obj_center(lottie);


// //1.创建日历（默认日历）
// lv_obj_t * calendar = lv_calendar_create(scr);
// //2.设置日历的大小和位置
// lv_obj_set_size(calendar, 300, 300);
// lv_obj_align(calendar, LV_ALIGN_CENTER, 0, 0);
// //3.设置日历的日期
// lv_calendar_set_today_date(calendar, 2026, 03, 23);
// //4.设置日历的月份
// lv_calendar_set_month_shown(calendar, 2026,03);
// //5.添加日历年和月份的选择
// // lv_calendar_add_header_dropdown(calendar);
// //6.添加日历的左右箭头
// lv_calendar_add_header_arrow(calendar);
// //7.设置日历为中国农历模式
// lv_calendar_set_chinese_mode(calendar, true);
// //8.修改字体类型=>使用包含中文的字体
// lv_obj_set_style_text_font(calendar, &lv_font_source_han_sans_sc_14_cjk, LV_PART_MAIN);
// //9.添加点击日期的事件回调函数
// lv_obj_add_event_cb(calendar, my_calendar_event_cb, LV_EVENT_VALUE_CHANGED, NULL);


// //创建表格
// lv_obj_t * table = lv_table_create(scr);
// //设置表格大小和位置
// lv_obj_set_size(table, LV_PCT(80), LV_PCT(80));
// lv_obj_align(table, LV_ALIGN_CENTER, 0, 0);
// //设置表格行数和列数
// lv_table_set_col_cnt(table, 3);
// lv_table_set_row_cnt(table, 4);
// //设置表格列宽
// lv_table_set_column_width(table, 0, 100);
// lv_table_set_column_width(table, 1, 100);
// lv_table_set_column_width(table, 2, 100);
// //设置表格内容
// lv_table_set_cell_value(table, 0, 0, "Name");
// lv_table_set_cell_value(table, 0, 1, "Age");
// lv_table_set_cell_value(table, 0, 2, "City");
// lv_table_set_cell_value(table, 1, 0, "Alice");
// lv_table_set_cell_value(table, 1, 1, "25");
// lv_table_set_cell_value(table, 1, 2, "New York");
// lv_table_set_cell_value(table, 2, 0, "Bob");
// lv_table_set_cell_value(table, 2, 1, "30");
// lv_table_set_cell_value(table, 2, 2, "Los Angeles");
// lv_table_set_cell_value(table, 3, 0, "Charlie");
// lv_table_set_cell_value(table, 3, 1, "35");
// lv_table_set_cell_value(table, 3, 2, "Chicago");
// //添加表格的事件回调函数
// lv_obj_add_event_cb(table, my_table_event_cb, LV_EVENT_VALUE_CHANGED, NULL);


// //创建一个不带刻度的简单图表
// lv_obj_t * chart = lv_chart_create(scr);
// //设置图表大小和位置
// lv_obj_set_size(chart, 200, 150);
// lv_obj_align(chart, LV_ALIGN_CENTER, 0, 0);
// //设置为折线图表
// lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
// //设置y轴范围
// lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);//设置第一个y轴范围
// lv_chart_set_range(chart, LV_CHART_AXIS_SECONDARY_Y, 0, 200);//设置第二个y轴范围
// //设置点的个数
// lv_chart_set_point_count(chart, 10);
// //添加数据系列
// lv_chart_series_t * ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);//添加第一个数据系列，颜色为红色，使用第一个y轴
// lv_chart_series_t * ser2 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_SECONDARY_Y);//添加第二个数据系列，颜色为蓝色，使用第二个y轴
// //添加数据=>保证数据点个数不少于lv_chart_set_point_count设置的个数
// //使用数组获取数据点
// int32_t *y_array=lv_chart_get_series_y_array(chart, ser1);
// //修改数据点的值 有两种方式
// for(int i = 0; i < 10; i++) {
//     //第一种方式：直接修改数据点的值
//     y_array[i] = lv_rand(0, 100);//使用lv_rand函数赋随机值
//     //第二种方式：使用函数修改数据点的值
//     lv_chart_set_next_value(chart, ser2, lv_rand(0, 200));
// }
// //刷新图表显示
// lv_chart_refresh(chart);


// //1.创建一个新屏幕放置图表
// lv_obj_t * chart_scr = lv_obj_create(scr);
// lv_obj_set_size(chart_scr, LV_PCT(100), LV_PCT(100));
// lv_obj_align(chart_scr, LV_ALIGN_CENTER, 0, 0);
// //创建一个新容器放置图表和刻度
// lv_obj_t * chart_cont = lv_obj_create(chart_scr);
// //先清空容器的样式再设置容器大小为屏幕的300%宽度和100%高度
// lv_obj_remove_style_all(chart_cont);
// lv_obj_set_size(chart_cont, LV_PCT(300), LV_PCT(100));
// //设置容器的布局为弹性布局
// lv_obj_set_layout(chart_cont, LV_LAYOUT_FLEX);
// //设置容器的布局形式为排成一列但不换列
// lv_obj_set_flex_flow(chart_cont, LV_FLEX_FLOW_COLUMN);

// //2.创建一个图表
// lv_obj_t * chart2 = lv_chart_create(chart_cont);
// //设置为柱状图表
// lv_chart_set_type(chart2, LV_CHART_TYPE_BAR);
// //设置图表宽度为容器的100%，使其占满整个容器
// lv_obj_set_width(chart2, LV_PCT(100));
// // 设置chart2对象在弹性布局中按比例扩展，占据剩余空间
// lv_obj_set_flex_grow(chart2, 1);
// //设置点的个数
// lv_chart_set_point_count(chart2, 12);

// //3.添加刻度
// lv_obj_t * scale = lv_scale_create(chart_cont);
// //刻度的显示位置为横轴下方
// lv_scale_set_mode(scale, LV_SCALE_MODE_HORIZONTAL_BOTTOM);
// lv_obj_set_size(scale, LV_PCT(100), 25);
// //设置刻度的刻度数为12
// lv_scale_set_total_tick_count(scale, 12);
// //设置每个刻度为主刻度
// lv_scale_set_major_tick_every(scale, 1);
// static const char * tick_labels[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
// //设置刻度标签的文本源为tick_labels数组
// lv_scale_set_text_src(scale, tick_labels);
// //修改刻度间隔=>使之对齐图表的柱状图
// lv_obj_set_style_pad_hor(scale, lv_chart_get_first_point_center_offset(chart2), LV_PART_MAIN);

// //设置y轴范围
// lv_chart_set_range(chart2, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
// lv_chart_set_range(chart2, LV_CHART_AXIS_SECONDARY_Y, 0, 200);
// //4.创建系列
// lv_chart_series_t * ser3 = lv_chart_add_series(chart2, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
// lv_chart_series_t * ser4 = lv_chart_add_series(chart2, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_SECONDARY_Y);
// for(int i = 0; i < 12; i++) {
//     lv_chart_set_next_value(chart2, ser3, lv_rand(0, 100));
//     lv_chart_set_next_value(chart2, ser4, lv_rand(0, 200));
// }
// //刷新图表显示
// lv_chart_refresh(chart2);

// //创建tabview
// lv_obj_t * tabview = lv_tabview_create(scr);
// //设置tabview的背景颜色
// lv_obj_set_style_bg_color(tabview, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
// //设置tabview的标签栏大小和位置
// lv_tabview_set_tab_bar_position(tabview, LV_DIR_LEFT);
// lv_tabview_set_tab_bar_size(tabview, LV_PCT(20));
// //获取标签栏再修改颜色
// lv_obj_t * tab_bar = lv_tabview_get_tab_bar(tabview);
// lv_obj_set_style_bg_color(tab_bar, lv_palette_main(LV_PALETTE_PINK), LV_PART_MAIN);

// //添加标签页
// lv_obj_t * tab1 = lv_tabview_add_tab(tabview, "Tab 1");
// lv_obj_t * tab2 = lv_tabview_add_tab(tabview, "Tab 2");
// lv_obj_t * tab3 = lv_tabview_add_tab(tabview, "Tab 3");
// //修改每一页的颜色
// lv_obj_set_style_bg_color(tab1, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
// lv_obj_set_style_bg_color(tab2, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
// lv_obj_set_style_bg_color(tab3, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN);
// //不能直接覆盖tabview颜色，需要手动设置bg_opa来覆盖tabview的颜色
// lv_obj_set_style_bg_opa(tab1, LV_OPA_COVER, LV_PART_MAIN);
// lv_obj_set_style_bg_opa(tab2, LV_OPA_COVER, LV_PART_MAIN);
// lv_obj_set_style_bg_opa(tab3, LV_OPA_COVER, LV_PART_MAIN);

// //标签页内添加内容
// lv_obj_t * label1 = lv_label_create(tab1);
// lv_label_set_text(label1, "This is Tab 1");
// lv_obj_center(label1);
// lv_obj_t * label2 = lv_label_create(tab2);
// lv_label_set_text(label2, "This is Tab 2");
// lv_obj_center(label2);
// lv_obj_t * label3 = lv_label_create(tab3);
// lv_label_set_text(label3, "This is Tab 3");
// lv_obj_center(label3);


    // lv_obj_t * tv = lv_tileview_create(lv_screen_active());
    // /*Tile1: just a label*/
    // lv_obj_t * tile1 = lv_tileview_add_tile(tv, 0, 0, LV_DIR_BOTTOM);
    // lv_obj_t * label = lv_label_create(tile1);
    // lv_label_set_text(label, "Scroll down");
    // lv_obj_center(label);
    // /*Tile2: a button*/
    // lv_obj_t * tile2 = lv_tileview_add_tile(tv, 0, 1, (lv_dir_t)(LV_DIR_TOP | LV_DIR_RIGHT));
    // lv_obj_t * btn = lv_button_create(tile2);
    // label = lv_label_create(btn);
    // lv_label_set_text(label, "Scroll up or right");
    // lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    // lv_obj_center(btn);
    // /*Tile3: a list*/
    // lv_obj_t * tile3 = lv_tileview_add_tile(tv, 1, 1, LV_DIR_LEFT);
    // lv_obj_t * list = lv_list_create(tile3);
    // lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    // lv_list_add_button(list, NULL, "One");
    // lv_list_add_button(list, NULL, "Two");
    // lv_list_add_button(list, NULL, "Three");
    // lv_list_add_button(list, NULL, "Four");
    // lv_list_add_button(list, NULL, "Five");
    // lv_list_add_button(list, NULL, "Six");
    // lv_list_add_button(list, NULL, "Seven");
    // lv_list_add_button(list, NULL, "Eight");
    // lv_list_add_button(list, NULL, "Nine");
    // lv_list_add_button(list, NULL, "Ten");

    // lv_obj_t * win = lv_win_create(lv_screen_active());
    // lv_obj_t * btn;
    // lv_win_add_title(win, "A title");
    // //创建一个关闭按钮
    // btn = lv_win_add_button(win, LV_SYMBOL_CLOSE, 60);
    // lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, NULL);
    // //显示窗口内容
    // lv_obj_t * cont = lv_win_get_content(win);
    // lv_obj_t * label = lv_label_create(cont);
    // lv_label_set_text(label, "This is\n"
    //                   "a pretty\n"
    //                   "long text\n"
    //                   "to see how\n"
    //                   "the window\n"
    //                   "becomes\n"
    //                   "scrollable.\n"
    //                   "\n"
    //                   "\n"
    //                   "Some more\n"
    //                   "text to be\n"
    //                   "sure it\n"
    //                   "overflows. :)");


// //创建一个开关
// lv_obj_t * sw = lv_switch_create(scr);
// //设置开关的位置 位于屏幕中心
// lv_obj_align(sw, LV_ALIGN_CENTER, 0, 0);
// //设置开关的方向为竖直方向
// lv_switch_set_orientation(sw, LV_SWITCH_ORIENTATION_VERTICAL);
// //设置开关的状态为关闭
// lv_obj_add_state(sw, LV_STATE_FOCUSED);
// //设置开关的大小
// lv_obj_set_size(sw, 60, 120);
// //设置开关的事件回调函数
// lv_obj_add_event_cb(sw, my_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

// //创建一个消息框
// lv_obj_t * mbox = lv_msgbox_create(scr);
// //设置消息框大小和位置，大小为屏幕的80%，位置居中
// lv_obj_set_size(mbox, LV_PCT(80), LV_PCT(80));
// lv_obj_align(mbox, LV_ALIGN_CENTER, 0, 0);
// //消息框的标题
// lv_msgbox_add_title(mbox, "Message Box");
// //消息框的文本内容
// lv_msgbox_add_text(mbox, "This is a message box.\nDo you want to continue?");
// //消息框的关闭按钮
// lv_msgbox_add_close_button(mbox);
// //添加用户选项按钮
// lv_obj_t * yes_btn = lv_msgbox_add_footer_button(mbox, "Yes");
// lv_obj_t * no_btn = lv_msgbox_add_footer_button(mbox, "No");
// //为用户选项按钮添加事件回调函数
// lv_obj_add_event_cb(yes_btn, yes_cb, LV_EVENT_CLICKED, NULL);
// lv_obj_add_event_cb(no_btn, no_cb, LV_EVENT_CLICKED, NULL);


// //创建下拉菜单
// lv_obj_t * ddlist = lv_dropdown_create(scr);
// //设置菜单的选项
// lv_dropdown_set_options(ddlist, "Option 1\nOption 2\nOption 3\nOption 4");
// //设置菜单的默认选项为第二个选项（索引从0开始）
// lv_dropdown_set_selected(ddlist, 1);
// //设置下拉菜单的方向为向右
// lv_dropdown_set_dir(ddlist, LV_DIR_RIGHT);
// //修改三角形图标的方向使之与下拉菜单的方向一致
// lv_dropdown_set_symbol(ddlist, LV_SYMBOL_RIGHT);
// //设置下拉菜单的事件回调函数
// lv_obj_add_event_cb(ddlist, my_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, NULL);



// lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
// //创建复选框
// lv_obj_t * checkbox = lv_checkbox_create(scr);
// //设置复选框的文本
// lv_checkbox_set_text(checkbox, "Check me");
// //将文本设置为收音机按钮
// lv_obj_set_radio_button(checkbox, true);

// lv_obj_t * checkbox2 = lv_checkbox_create(scr);
// lv_checkbox_set_text(checkbox2, "Check me too");
// lv_obj_set_radio_button(checkbox2, true);

// lv_obj_t * roller1 = lv_roller_create(lv_screen_active());
//     lv_roller_set_options(roller1,
//                           "January\n"
//                           "February\n"
//                           "March\n"
//                           "April\n"
//                           "May\n"
//                           "June\n"
//                           "July\n"
//                           "August\n"
//                           "September\n"
//                           "October\n"
//                           "November\n"
//                           "December",
//                         //   LV_ROLLER_MODE_INFINITE);//循环模式
//                           LV_ROLLER_MODE_NORMAL);//非循环模式

//     lv_roller_set_visible_row_count(roller1, 4);//设置可见行数为4
//     lv_obj_center(roller1);
//     lv_obj_add_event_cb(roller1, roller_cb, LV_EVENT_ALL, NULL);


// //添加旋转框
// lv_obj_t * spinbox = lv_spinbox_create(scr);
// //设置旋转框的大小和位置
// lv_obj_set_size(spinbox, 80, 50);
// lv_obj_align(spinbox, LV_ALIGN_CENTER, 0, 0);
// //设置旋转框的范围为0-10000
// lv_spinbox_set_range(spinbox, 0, 10000);
// //设置旋转框的初始值为50
// lv_spinbox_set_value(spinbox, 50);
// //设置小数点位置为3，即显示两位小数
// lv_spinbox_set_dec_point_pos(spinbox, 3);
// //获取旋转框的高度，用于设置按钮的大小
// int32_t  spinbox_height = lv_obj_get_height(spinbox);
// //添加两个按钮实现加减功能
// lv_obj_t * add_btn = lv_btn_create(scr);
// lv_obj_t * sub_btn = lv_btn_create(scr);
// //在按钮上表示加/减
// lv_obj_set_style_bg_image_src(add_btn, LV_SYMBOL_PLUS, LV_PART_MAIN);
// lv_obj_set_style_bg_image_src(sub_btn, LV_SYMBOL_MINUS, LV_PART_MAIN);
// //设置按钮的大小与旋转框高度
// lv_obj_set_size(add_btn, spinbox_height, spinbox_height);
// lv_obj_set_size(sub_btn, spinbox_height, spinbox_height);
// //设置按钮的位置,加号按钮在旋转框右侧，减号按钮在旋转框左侧
// lv_obj_align_to(add_btn, spinbox, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
// lv_obj_align_to(sub_btn, spinbox, LV_ALIGN_OUT_LEFT_MID, -10, 0);
// //添加按钮的事件回调函数
// lv_obj_add_event_cb(add_btn, add_btn_event_cb, LV_EVENT_PRESSED, NULL);
// lv_obj_add_event_cb(sub_btn, sub_btn_event_cb, LV_EVENT_PRESSED, NULL);


// //创建列表
// lv_obj_t * list = lv_list_create(scr);
// //设置列表大小和位置
// lv_obj_set_size(list, LV_PCT(80), LV_PCT(80));
// lv_obj_align(list, LV_ALIGN_CENTER, 0, 0);
// //添加列表项
// lv_list_add_button(list, LV_SYMBOL_FILE, "File");
// lv_list_add_button(list, LV_SYMBOL_DIRECTORY, "Directory");
// lv_list_add_button(list, LV_SYMBOL_CLOSE, "Close");
// //添加列表的事件回调函数
// // lv_obj_add_event_cb(list, my_list_File_cb, LV_EVENT_VALUE_CHANGED, NULL);
// // lv_obj_add_event_cb(list, my_list_Directory_cb, LV_EVENT_VALUE_CHANGED, NULL);
// // lv_obj_add_event_cb(list, my_list_Close_cb, LV_EVENT_VALUE_CHANGED, NULL);


// //拼音输入法
// //1.创建拼音模块
// lv_obj_t * pinyin = lv_ime_pinyin_create(scr);
// //1.1.设置合理的字体
// lv_obj_set_style_text_font(pinyin, &Sen_notosanssc_16, LV_PART_MAIN);
// //1.2.可以自定义拼音写出来的字
// //如若不显示字体，则是编码集不对 需要使用UTF-8编码的中文字符
// //如若拼音出来的字显示成方块 即表示当前字体不支持显示这些中文字符 需要使用包含这些中文字符的字体
// //如若程序崩溃 即卡死 则是字典不完整 缺少基础的单个字母 起码要把26个字母的拼音都写上
// static const lv_pinyin_dict_t lv_ime_pinyin_def_dict[] = {
//     //默认繁体字，可自己修改为简体中文
//     { "a", "" },
//     { "b", "" },
//     { "c", "" },
//     { "d", "" },
//     { "ding", "定" },
//     { "e", "" },
//     { "f", "" },
//     { "fan", "返" },
//     { "g", "" },
//     { "h", "" },
//     { "hui", "回" },
//     { "i", "" },
//     { "j", "" },
//     { "k", "" },
//     { "l", "" },
//     { "m", "" },
//     { "n", "" },
//     { "o", "" },
//     { "p", "" },
//     { "q", "" },
//     { "qu", "取" },
//     { "que", "确" },
//     { "r", "" },
//     { "s", "" },
//     { "she", "设" },
//     { "t", "" },
//     { "u", "" },
//     { "v", "" },
//     { "w", "" },
//     { "x", "" },
//     { "xiao", "消" },
//     { "xue", "血" },
//     { "y", "" },
//     { "yang", "氧" },
//     { "z", "" },
//     { "zhi", "置" },
//         {NULL, NULL}};
// //将自定义的字典设置给拼音模块
// lv_ime_pinyin_set_dict(pinyin, lv_ime_pinyin_def_dict);
// //2.创建一个文本输入框 用来拼音输入的显示
// lv_obj_t * ta = lv_textarea_create(scr);
// //2.1设置文本输入框为单行模式
// lv_textarea_set_one_line(ta, true);
// //2.2设置文本输入框的字体 与拼音模块一致
// lv_obj_set_style_text_font(ta, &Sen_notosanssc_16, LV_PART_MAIN);
// //3.创建键盘
// lv_obj_t * kb = lv_keyboard_create(scr);
// //4.将拼音模块和键盘关联
// lv_ime_pinyin_set_keyboard(pinyin, kb);
// //5.将键盘和文本输入框关联
// lv_keyboard_set_textarea(kb, ta);
// //6.修改控制板的格式
// lv_obj_t * cand_panel = lv_ime_pinyin_get_cand_panel(pinyin);
// lv_obj_set_size(cand_panel, LV_PCT(50), LV_PCT(10));
// lv_obj_align_to(cand_panel, kb, LV_ALIGN_OUT_TOP_MID, 0, 0);


//开启静态翻译功能
lv_translation_add_static(languages,tags,translations);
//设置语言 先用简体中文试试
lv_translation_set_language("Chinese");

//添加一个标签视图
lv_obj_t * tabview = lv_tabview_create(scr);
//设置标签视图的大小
lv_obj_set_size(tabview, LV_PCT(100), LV_PCT(100));
//添加标签页
lv_obj_t * tab1 = lv_tabview_add_tab(tabview, lv_tr("Tab1"));
lv_obj_t * tab2 = lv_tabview_add_tab(tabview, lv_tr("Tab2"));
//添加接收翻译修改的事件回调函数 =>tab bar子对象是按钮btn，按钮btn的子对象是标签label，标签label的文本就是需要翻译的内容，需要一级一级获取到标签对象才能修改文本
//首先先获取tab bar
lv_obj_t * tab_bar = lv_tabview_get_tab_bar(tabview);
//再获取到tab bar的按钮 有两个按钮可以采用for循环遍历获取
for(int i = 0; i < 2; i++) {
    lv_obj_t * btn = lv_obj_get_child(tab_bar, i);
    //然后获取按钮的标签
    lv_obj_t * label = lv_obj_get_child(btn, 0);
    //最后为标签添加事件回调函数
    lv_obj_add_event_cb(label, language_change_cb, LV_EVENT_TRANSLATION_LANGUAGE_CHANGED, (void *)tags[i]);
}


//外部参数的观察者模式
//初始化一个主题 =>选择正确的类型和初始值
lv_subject_init_int(&temperature_subj, 10);
//添加外部参数的观察者 =>选择正确的事件类型和回调函数
lv_observer_t * temperature_observer = lv_subject_add_observer(&temperature_subj, temperature_observer_cb, NULL);

//创建两个按钮来模拟温度的变化
lv_obj_t * increase_btn = lv_button_create(tab1);
lv_obj_t * decrease_btn = lv_button_create(tab1);
//设置按钮的大小和位置
lv_obj_set_size(increase_btn, LV_PCT(15), LV_PCT(15));
lv_obj_set_size(decrease_btn, LV_PCT(15), LV_PCT(15));
lv_obj_align(increase_btn, LV_ALIGN_CENTER, -50, 0);
lv_obj_align(decrease_btn, LV_ALIGN_CENTER, 50, 0);
//设置按钮的标签 显示+-号
lv_obj_t * inc_label = lv_label_create(increase_btn);
lv_label_set_text(inc_label, LV_SYMBOL_PLUS);
lv_obj_t * dec_label = lv_label_create(decrease_btn);
lv_label_set_text(dec_label, LV_SYMBOL_MINUS);
//按钮的事件回调函数
lv_obj_add_event_cb(increase_btn, increase_temperature_cb, LV_EVENT_CLICKED, &temperature_subj);
lv_obj_add_event_cb(decrease_btn, decrease_temperature_cb, LV_EVENT_CLICKED, &temperature_subj);

//添加一个条形图来显示温度的数值
lv_obj_t * bar = lv_bar_create(tab1);
lv_obj_set_size(bar, LV_PCT(80), LV_PCT(10));
lv_obj_align(bar, LV_ALIGN_CENTER, 0, -50);
//设置条形图的范围为-100到100
lv_bar_set_range(bar, -100, 100);
//直接绑定组件，使用观察者模式更新条形图显示的温度数值 =>在观察者回调函数中更新条形图的值
lv_bar_bind_value(bar, &temperature_subj);

//用标签显示温度的数值
lv_obj_t * label = lv_label_create(tab1);
//标签换颜色
lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
//使用自定义观察者更新温度文本（支持语言切换后直接刷新）
lv_subject_add_observer_obj(&temperature_subj, temperature_label_observer_cb, label, bar);
//触发一次刷新，初始化标签文本
lv_subject_notify(&temperature_subj);
//将标签显示在条形图内中间处
lv_obj_align_to(label, bar, LV_ALIGN_CENTER, 0, 0);
//添加接收翻译修改的事件回调函数
lv_obj_add_event_cb(label, language_change_cb2, LV_EVENT_TRANSLATION_LANGUAGE_CHANGED, (void *)tags[0]);

//创建一个滑块来模拟温度的变化
lv_obj_t * slider = lv_slider_create(tab1);
lv_obj_set_size(slider, LV_PCT(80), LV_PCT(10));
lv_obj_align(slider, LV_ALIGN_CENTER, 0, 50);
//设置滑块的范围为-100到100
lv_slider_set_range(slider, -100, 100);
//绑定滑块的值到外部参数 =>双向绑定，滑块的值变化会更新外部参数，外部参数的变化也会更新滑块的值
lv_slider_bind_value(slider, &temperature_subj);

//使用交互组件来切换语言 这里使用下拉菜单
lv_obj_t * ddlist = lv_dropdown_create(tab2);
lv_obj_set_size(ddlist, LV_PCT(50), LV_PCT(10));
lv_obj_align(ddlist, LV_ALIGN_CENTER, 0, 0);
//设置下拉菜单的选项为支持的语言列表
lv_dropdown_set_options(ddlist, "English\nChinese");
//设置下拉菜单的默认选项为当前语言
lv_dropdown_set_selected(ddlist, 1);
//下拉菜单的事件回调函数
lv_obj_add_event_cb(ddlist, language_selection_cb, LV_EVENT_VALUE_CHANGED, ddlist);
}
