/* step_counter.h
 * 简单的手腕步数计数器接口（使用加速度幅值峰值+陀螺辅助）
 */
#ifndef STEPCOUNTER_H
#define STEPCOUNTER_H

#include <stdint.h>
#include <stdbool.h>

// 初始化步数计数器（必须调用一次）
void StepCounter_Init(void);

// 处理一次采样，参数：加速度 g、角速度 °/s、时间戳 ms
// 返回：若本次采样检测到步则返回 true
bool StepCounter_ProcessSample(float ax_g, float ay_g, float az_g,
                               float gx_dps, float gy_dps, float gz_dps,
                               uint32_t timestamp_ms);

// 获取当前累计步数
uint32_t StepCounter_GetCount(void);

// 重置步数计数器
void StepCounter_Reset(void);

#endif /* STEPCOUNTER_H */
