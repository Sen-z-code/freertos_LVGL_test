#include "DHT11Task.h"
#include "DHT11.h"
#include "myui.h"
#include <stdio.h>


void StartDHT11Task(void *argument)
{
	(void)argument;

	/* 初始化 DHT11 驱动 */
	DHT11_Init();

	for(;;) {
		int temp = 0, hum = 0;
		bool ok = DHT11_Read(&temp, &hum);
		if (ok) {
			/* 发布到 UI（线程安全：myui_set_temp_hum 会在 LVGL 线程更新） */
			myui_set_temp_hum((float)temp, (float)hum);
			vTaskDelay(pdMS_TO_TICKS(2000)); /* 每 2 秒采样一次 */
		}
		
	}
}