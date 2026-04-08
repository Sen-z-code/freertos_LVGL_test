/* rtc_driver.h -- simple RTC driver wrapper for HAL RTC */
#ifndef __RTC_DRIVER_H
#define __RTC_DRIVER_H

#include <stdint.h>

typedef struct {
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;
  uint8_t day;
  uint8_t month;
  uint8_t year; /* two-digit year */
  uint8_t weekday;
} rtc_time_t;

/* 初始化驱动（会调用生成的 MX_RTC_Init，并保证备份寄存器逻辑已处理） */
void rtc_driver_init(void);

/* 读取当前时间（线程安全：返回的是复制的结构） */
void rtc_driver_get_time(rtc_time_t *t);

/* 直接返回当前时间副本（便捷函数） */
rtc_time_t rtc_driver_get_current_time(void);

#endif
