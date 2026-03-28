#ifndef MYUI_H
#define MYUI_H

#include "lvgl.h"

#include <stdlib.h>
#include <stdio.h>

// 初始化 UI 界面
// 模块说明：
//  该模块负责创建智能手表的横屏开机主题页面（启动界面），
//  页面包含各传感器数据展示的占位控件（步数、心率、血氧、气压/海拔、指南针、温湿度、电量等），
//  传感器数据当前使用常量占位，后续可通过观察者模式将真实传感器数据绑定到 `myui_update_sensor_values()`。
//  函数均带中文注释说明模块用途和绑定点。
void myui_init(void);

// 更新界面上的传感器显示（占位/接口）
// 说明：实际项目中请在观察者回调中调用该函数或实现具体的绑定函数。
void myui_update_sensor_values(void);

// 切换顶部下拉快捷面板（show = true 显示，false 隐藏）
// 说明：面板创建于 `myui_init()`，该函数用于在运行时显示或隐藏面板并带有简单过渡。
void myui_toggle_dropdown(bool show);


#endif
