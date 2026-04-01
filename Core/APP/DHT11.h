#ifndef DHT11_H
#define DHT11_H

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

/* 如果 CubeMX 生成了 DHT11_DATA_GPIO_Port/DHT11_DATA_Pin，就用它们 */
#if defined(DHT11_DATA_GPIO_Port) && defined(DHT11_DATA_Pin)
#define DHT11_GPIO_PORT DHT11_DATA_GPIO_Port
#define DHT11_GPIO_PIN  DHT11_DATA_Pin
#else
/* 回退到 PA8 */
#define DHT11_GPIO_PORT GPIOA
#define DHT11_GPIO_PIN  GPIO_PIN_8
#endif

/* 初始化驱动（启用微秒延时支持），返回 true 表示成功 */
bool DHT11_Init(void);

/* 读取一次数据，temperature_c 与 humidity 为输出（整数部分），
 * 成功返回 true，失败返回 false（超时或校验和错误）。
 * 建议调用间隔 >= 2000 ms（DHT11 典型数据表要求）。
 */
bool DHT11_Read(int *temperature_c, int *humidity);

#endif
