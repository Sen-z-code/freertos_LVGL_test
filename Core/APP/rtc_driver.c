/*
 * rtc_driver.c
 *
 * RTC 持久化驱动
 * 功能：
 * - 提供对 HAL RTC 的包装，维护一个进程内缓存 `s_current_time`；
 * - 使用备份寄存器（BKP_DRx）保存年月日/时分秒以及一个魔数标记，
 *   实现在主电源断电后通过 VBAT 供电仍能恢复时间或在上电时恢复备份；
 * - 启动时比较硬件 RTC（RTC 寄存器）与备份寄存器的时间，选择较新的
 *   时间作为系统时间（避免用陈旧备份覆盖仍在走时的硬件 RTC）；
 * - 在读取到有效时间时将时间写回备份寄存器以便下次恢复。
 *
 * 重要说明：实现依赖硬件支持（VBAT 与 LSE）。如果目标板没有把 VBAT
 * 接到电池/电容，RTC 在断电期不会继续计时，备份寄存器也可能在断电
 * 后丢失。
 */

#include "rtc_driver.h"
#include "rtc.h"
#include "stm32f4xx_hal.h"
#include <string.h>

/* 内部缓存当前时间，RTCTask 周期性更新 */
static volatile rtc_time_t s_current_time;

/* 备份寄存器布局（DR0 用作魔数，DR1/DR2 存放时间） */
#define RTC_APP_BKP_MAGIC 0xA5A55A5AU
#define RTC_APP_BKP_MAGIC_REG RTC_BKP_DR0
#define RTC_APP_BKP_DATE_REG  RTC_BKP_DR1
#define RTC_APP_BKP_TIME_REG  RTC_BKP_DR2

/*
 * rtc_time_is_valid
 * 简单的字段范围校验：用于判断从 RTC 寄存器或备份寄存器读出的时间是否
 * 在可接受的范围内，避免把非法值写回备份或覆盖 RTC。
 * 返回：1 表示合法，0 表示非法。
 */
static int rtc_time_is_valid(const rtc_time_t *t)
{
  if (!t) return 0;
  if (t->hours > 23) return 0;
  if (t->minutes > 59) return 0;
  if (t->seconds > 59) return 0;
  if (t->month < 1 || t->month > 12) return 0;
  if (t->day < 1 || t->day > 31) return 0;
  if (t->year > 99) return 0;     /* two-digit year: 0..99 */
  if (t->weekday > 7) return 0;   /* weekday 允许 0..7 */
  return 1;
}

/*
 * rtc_time_compare
 * 按年/月/日/时/分/秒的顺序比较两个时间结构：
 * - 返回 1 表示 a 晚于 b（a > b）
 * - 返回 0 表示相等
 * - 返回 -1 表示 a 早于 b（a < b）
 * 该比较忽略 weekday 字段，仅用于判断哪个时间更“新”。
 */
static int rtc_time_compare(const rtc_time_t *a, const rtc_time_t *b)
{
  if (!a || !b) return 0;
  if (a->year != b->year) return (a->year > b->year) ? 1 : -1;
  if (a->month != b->month) return (a->month > b->month) ? 1 : -1;
  if (a->day != b->day) return (a->day > b->day) ? 1 : -1;
  if (a->hours != b->hours) return (a->hours > b->hours) ? 1 : -1;
  if (a->minutes != b->minutes) return (a->minutes > b->minutes) ? 1 : -1;
  if (a->seconds != b->seconds) return (a->seconds > b->seconds) ? 1 : -1;
  return 0;
}

/*
 * rtc_driver_backup_write
 * 将 `rtc_time_t` 按照预定义布局打包并写入备份寄存器：
 * - DR1 = [year(8) | month(8) | date(8) | weekday(8)]
 * - DR2 = [hours(8) | minutes(8) | seconds(8)]
 * - DR0 = magic 标志 (RTC_APP_BKP_MAGIC)
 *
 * 注意：调用该函数前必须先调用 `HAL_PWR_EnableBkUpAccess()` 开启备份域写权限；
 * 写入备份寄存器不是原子操作（分三次写入），但对本应用而言足够。
 */
static void rtc_driver_backup_write(const volatile rtc_time_t *t)
{
  uint32_t dr1 = ((uint32_t)(t->year & 0xFF) << 24) |
                 ((uint32_t)(t->month & 0xFF) << 16) |
                 ((uint32_t)(t->day & 0xFF) << 8) |
                 ((uint32_t)(t->weekday & 0xFF));

  uint32_t dr2 = ((uint32_t)(t->hours & 0xFF) << 16) |
                 ((uint32_t)(t->minutes & 0xFF) << 8) |
                 ((uint32_t)(t->seconds & 0xFF));

  HAL_RTCEx_BKUPWrite(&hrtc, RTC_APP_BKP_DATE_REG, dr1);
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_APP_BKP_TIME_REG, dr2);
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_APP_BKP_MAGIC_REG, RTC_APP_BKP_MAGIC);
}

/*
 * rtc_driver_backup_read
 * 从备份寄存器按布局读取时间并填充到 `rtc_time_t` 中：
 * - 若 DR0 的 magic 不匹配则表示备份不存在或不可信，返回 0；
 * - 否则解包 DR1/DR2 并返回 1。
 */
static int rtc_driver_backup_read(rtc_time_t *t)
{
  if (!t) return 0;
  uint32_t magic = HAL_RTCEx_BKUPRead(&hrtc, RTC_APP_BKP_MAGIC_REG);
  if (magic != RTC_APP_BKP_MAGIC) return 0;

  uint32_t dr1 = HAL_RTCEx_BKUPRead(&hrtc, RTC_APP_BKP_DATE_REG);
  uint32_t dr2 = HAL_RTCEx_BKUPRead(&hrtc, RTC_APP_BKP_TIME_REG);

  t->year = (uint8_t)((dr1 >> 24) & 0xFF);
  t->month = (uint8_t)((dr1 >> 16) & 0xFF);
  t->day = (uint8_t)((dr1 >> 8) & 0xFF);
  t->weekday = (uint8_t)(dr1 & 0xFF);

  t->hours = (uint8_t)((dr2 >> 16) & 0xFF);
  t->minutes = (uint8_t)((dr2 >> 8) & 0xFF);
  t->seconds = (uint8_t)(dr2 & 0xFF);

  return 1;
}

/*
 * rtc_driver_init
 * 初始化驱动：
 * 1) 打开备份域写权限；2) 调用生成的 MX_RTC_Init() 完成 HAL 层初始化；
 * 3) 读取硬件 RTC（HRRTC）与备份寄存器并比较；
 * 4) 决策策略：
 *    - 如果 HW 比 BKP 新：以 HW 为准，并把 HW 写回备份（更新备份）；
 *    - 如果 BKP 比 HW 新：以 BKP 恢复到 HW（写回 RTC 寄存器）；
 *    - 如果只有一方有效，则使用有效一方并同步到另一方；
 *    - 否则不作更改（保守策略）。
 *
 * 注意：该函数在引导时调用一次以同步时间，之后由 RTCTask 周期性读取/发布时间。
 */
void rtc_driver_init(void)
{
  /* 确保备份域可写（MX_RTC_Init 内已有备份寄存器判断） */
  HAL_PWR_EnableBkUpAccess();
  MX_RTC_Init();

  /* 读取硬件 RTC（真实寄存器值） */
  rtc_time_t hw = {0};
  {
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK &&
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) == HAL_OK)
    {
      hw.hours = (uint8_t)sTime.Hours;
      hw.minutes = (uint8_t)sTime.Minutes;
      hw.seconds = (uint8_t)sTime.Seconds;
      hw.day = (uint8_t)sDate.Date;
      hw.month = (uint8_t)sDate.Month;
      hw.year = (uint8_t)sDate.Year;
      hw.weekday = (uint8_t)sDate.WeekDay;
    }
  }

  /* 读取备份 */
  rtc_time_t stored = {0};
  int has_stored = rtc_driver_backup_read(&stored);

  /* 决策并同步：以更“新”的时间为准 */
  if (has_stored && rtc_time_is_valid(&stored) && rtc_time_is_valid(&hw))
  {
    int cmp = rtc_time_compare(&hw, &stored);
    if (cmp > 0)
    {
      /* 硬件时间比备份新：保留硬件时间并更新备份 */
      memcpy((void *)&s_current_time, &hw, sizeof(rtc_time_t));
      rtc_driver_backup_write(&s_current_time);
    }
    else if (cmp < 0)
    {
      /* 备份比硬件新：把备份恢复到 RTC */
      RTC_TimeTypeDef sTime = {0};
      RTC_DateTypeDef sDate = {0};
      sTime.Hours = stored.hours;
      sTime.Minutes = stored.minutes;
      sTime.Seconds = stored.seconds;
      sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
      sTime.StoreOperation = RTC_STOREOPERATION_RESET;
      if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) Error_Handler();
      sDate.WeekDay = stored.weekday;
      sDate.Month = stored.month;
      sDate.Date = stored.day;
      sDate.Year = stored.year;
      if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) Error_Handler();
      memcpy((void *)&s_current_time, &stored, sizeof(rtc_time_t));
    }
    else
    {
      /* 相等：使用硬件时间并同步缓存 */
      memcpy((void *)&s_current_time, &hw, sizeof(rtc_time_t));
    }
  }
  else if (has_stored && rtc_time_is_valid(&stored))
  {
    /* 只有备份有效，恢复备份到 RTC 并更新缓存 */
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    sTime.Hours = stored.hours;
    sTime.Minutes = stored.minutes;
    sTime.Seconds = stored.seconds;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) Error_Handler();
    sDate.WeekDay = stored.weekday;
    sDate.Month = stored.month;
    sDate.Date = stored.day;
    sDate.Year = stored.year;
    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) Error_Handler();
    memcpy((void *)&s_current_time, &stored, sizeof(rtc_time_t));
  }
  else if (rtc_time_is_valid(&hw))
  {
    /* 只有硬件有效：使用硬件并写入备份 */
    memcpy((void *)&s_current_time, &hw, sizeof(rtc_time_t));
    rtc_driver_backup_write(&s_current_time);
  }
  else
  {
    /* 都无效：保持默认行为（不覆盖），缓存清零 */
    memset((void *)&s_current_time, 0, sizeof(rtc_time_t));
  }
}

/*
 * rtc_driver_get_time
 * 从硬件 RTC 读取当前时间并填充到传入结构，随后更新内部缓存并在合法时
 * 将时间持久化到备份寄存器。注意：读取顺序必须是 GetTime 后 GetDate（HAL 要求）。
 */
void rtc_driver_get_time(rtc_time_t *t)
{
  if(!t) return;

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* 先读取 time，再读取 date（这是 HAL 的要求以锁存寄存器） */
  if(HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
    return;
  }
  if(HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
    return;
  }

  t->hours = (uint8_t)sTime.Hours;
  t->minutes = (uint8_t)sTime.Minutes;
  t->seconds = (uint8_t)sTime.Seconds;
  t->day = (uint8_t)sDate.Date;
  t->month = (uint8_t)sDate.Month;
  t->year = (uint8_t)sDate.Year; /* two-digit year */
  t->weekday = (uint8_t)sDate.WeekDay;

  /* 更新内部缓存（非原子但为近似最新值）并在合法时持久化到备份寄存器 */
  memcpy((void *)&s_current_time, t, sizeof(rtc_time_t));
  if (rtc_time_is_valid(t))
  {
    rtc_driver_backup_write(t);
  }
}

/* 返回当前内存缓存的时间（线程安全性取决于调用方） */
rtc_time_t rtc_driver_get_current_time(void)
{
  rtc_time_t tmp;
  memcpy(&tmp, (const void *)&s_current_time, sizeof(rtc_time_t));
  return tmp;
}
