#ifndef MYUI_H
#define MYUI_H

#include "lvgl.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

// 初始化 UI 界面
// 模块说明：
//  该模块负责创建智能手表的横屏开机主题页面（启动界面），
//  页面包含各传感器数据展示的占位控件（步数、心率、血氧、气压/海拔、指南针、温湿度、电量等），
//  传感器数据当前使用常量占位，后续可通过观察者模式将真实传感器数据绑定到 `myui_update_sensor_values()`。
//  函数均带中文注释说明模块用途和绑定点。
void myui_init(void);

// 切换顶部下拉快捷面板（show = true 显示，false 隐藏）
// 说明：面板创建于 `myui_init()`，该函数用于在运行时显示或隐藏面板并带有简单过渡。
void myui_toggle_dropdown(bool show);


// ============================
// 海拔数据绑定（观察者模式）
// ============================

// 海拔数据观察者回调
// 说明：
//  - `altitude_m`：海拔（米），float
//  - 注意：回调可能在“数据发布者线程/任务”中被调用，请不要在回调里直接操作 LVGL 控件。
typedef void (*myui_altitude_observer_cb_t)(float altitude_m, void * user_data);

// 注册/注销海拔观察者
// 返回：注册成功返回 true；列表满或参数非法返回 false。
bool myui_altitude_register_observer(myui_altitude_observer_cb_t cb, void * user_data);
void myui_altitude_unregister_observer(myui_altitude_observer_cb_t cb, void * user_data);

// 发布海拔数据（通知所有观察者）
// 说明：传感器任务以 100Hz 调用即可；UI 会在 LVGL 线程里以 100Hz 刷新显示。
void myui_altitude_publish(float altitude_m);

// 传感器侧使用的便捷接口：设置海拔（米，float）
// 说明：内部等价于 `myui_altitude_publish()`。
void myui_set_altitude_m(float altitude_m);


// ============================
// 温湿度数据绑定（观察者模式）
// ============================

// 温湿度回调（temp: 摄氏度, hum: 相对湿度百分比）
// 注意：回调可能在数据发布者线程/任务中被调用，请不要在回调里直接操作 LVGL 控件。
typedef void (*myui_temp_hum_observer_cb_t)(float temp_c, float hum_pct, void * user_data);

// 注册/注销温湿度观察者
bool myui_temp_hum_register_observer(myui_temp_hum_observer_cb_t cb, void * user_data);
void myui_temp_hum_unregister_observer(myui_temp_hum_observer_cb_t cb, void * user_data);

// 发布温湿度（通知所有观察者）
void myui_temp_hum_publish(float temp_c, float hum_pct);

// 便捷设置接口（等价于 publish）
void myui_set_temp_hum(float temp_c, float hum_pct);

#endif
