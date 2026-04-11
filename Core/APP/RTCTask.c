#include "RTCTask.h"
#include "gpio.h"


/* 任务入口：初始化 RTC 驱动并每秒读取一次时间到缓存 */
void StartRTCTask(void *argument)
{
	rtc_driver_init();

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000);

	for(;;)
	{
		rtc_time_t t;
		rtc_driver_get_time(&t);

		/* 发布时间到 UI（LVGL 在其线程的 1Hz 定时器中刷新显示） */
		int year_full = 2000 + (int)t.year;
		myui_set_datetime(year_full, t.month, t.day, t.hours, t.minutes, t.seconds);

		vTaskDelayUntil(&xLastWakeTime, xFrequency);
	}

}
