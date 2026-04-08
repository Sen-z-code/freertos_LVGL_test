#include "rtc_driver.h"
#include "rtc.h"
#include "stm32f4xx_hal.h"
#include <string.h>

/* 内部缓存当前时间，RTCTask 周期性更新 */
static volatile rtc_time_t s_current_time;

/* 初始化：启用备份访问并调用生成的 MX_RTC_Init() */
void rtc_driver_init(void)
{
  /* 确保备份域可写（MX_RTC_Init 内有备份寄存器判断） */
  HAL_PWR_EnableBkUpAccess();
  MX_RTC_Init();

  /* 读取一次当前时间到缓存 */
  rtc_time_t t = {0};
  rtc_driver_get_time(&t);
  memcpy((void *)&s_current_time, &t, sizeof(rtc_time_t));
}

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

  /* 更新内部缓存（非原子但为近似最新值） */
  memcpy((void *)&s_current_time, t, sizeof(rtc_time_t));
}

rtc_time_t rtc_driver_get_current_time(void)
{
  rtc_time_t tmp;
  memcpy(&tmp, (const void *)&s_current_time, sizeof(rtc_time_t));
  return tmp;
}
