/* step_counter.c
 * 简易步数检测实现：基于加速度幅值的自适应峰值检测，辅以陀螺阈值增强
 * 目标：在资源受限的手表上实现稳健、轻量的计步逻辑
 */
#include "step_counter.h"
#include <math.h>

// 配置参数（可按需调优）
static const uint32_t STEP_MIN_INTERVAL_MS = 300; // 最小步间隔（防抖）
static const float GRAVITY_TAU = 0.5f; // 低通估计重力时间常数（s）
static const float SMOOTH_ALPHA = 0.2f; // 峰值包络平滑系数
static const float BASE_THRESH = 0.20f; // 初始峰值阈值（g）
static const float GYRO_AID_THRESH = 25.0f; // 陀螺阈值（°/s），作为辅助条件

static uint32_t step_count;
static uint32_t last_step_ts;
static float gravity_lp;      // 低通估计的重力模长（g）
static float envelope_lp;     // 峰值包络平滑值
static float adapt_thresh;    // 自适应阈值
static uint32_t last_ts;

void StepCounter_Init(void)
{
    step_count = 0;
    last_step_ts = 0;
    gravity_lp = 1.0f;
    envelope_lp = 0.0f;
    adapt_thresh = BASE_THRESH;
    last_ts = 0;
}

void StepCounter_Reset(void)
{
    StepCounter_Init();
}

uint32_t StepCounter_GetCount(void)
{
    return step_count;
}

bool StepCounter_ProcessSample(float ax_g, float ay_g, float az_g,
                               float gx_dps, float gy_dps, float gz_dps,
                               uint32_t timestamp_ms)
{
    // 计算采样间隔（秒），保护 first-sample
    float dt_s = 0.02f; // 默认 20ms
    if (last_ts != 0) {
        uint32_t dms = (timestamp_ms >= last_ts) ? (timestamp_ms - last_ts) : 0;
        dt_s = (dms > 0) ? (dms / 1000.0f) : dt_s;
    }
    last_ts = timestamp_ms;

    // 1) 计算加速度幅值
    float mag = sqrtf(ax_g*ax_g + ay_g*ay_g + az_g*az_g);

    // 2) 低通估计重力分量（可随 dt 自适应）
    float alpha = dt_s / (GRAVITY_TAU + dt_s);
    gravity_lp += alpha * (mag - gravity_lp);

    // 3) 高通(去重力)信号与包络平滑
    float mag_hp = mag - gravity_lp;
    if (mag_hp < 0.0f) mag_hp = 0.0f; // 只关心正向脉冲

    envelope_lp = SMOOTH_ALPHA * envelope_lp + (1.0f - SMOOTH_ALPHA) * mag_hp;

    // 4) 自适应阈值（简单指数移动平均）
    adapt_thresh = 0.995f * adapt_thresh + 0.005f * envelope_lp; // 慢变的基线
    float detect_thresh = fmaxf(BASE_THRESH, adapt_thresh + 0.08f);

    // 5) 陀螺辅助：计算角速度幅值
    float gyro_mag = sqrtf(gx_dps*gx_dps + gy_dps*gy_dps + gz_dps*gz_dps);

    // 6) 峰值检测+去抖
    bool step = false;
    if (envelope_lp > detect_thresh) {
        // 确保时间间隔足够
        if (timestamp_ms - last_step_ts > STEP_MIN_INTERVAL_MS) {
            // 如果陀螺也有明显变化，则更可靠
            if (gyro_mag > GYRO_AID_THRESH || envelope_lp > detect_thresh * 1.6f) {
                step = true;
            }
        }
    }

    if (step) {
        step_count++;
        last_step_ts = timestamp_ms;
        return true;
    }

    return false;
}
