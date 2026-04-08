/*
 * DHT11.c
 * 基于 HAL 的 DHT11 位操作驱动
 * 使用 DWT(CYCCNT) 提供微秒延时，不需要额外配置定时器。
 */
#include "DHT11.h"
#include "main.h"

#include <string.h>
#ifdef FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#endif

/* DWT 微秒延时初始化/实现 */
static void DWT_Delay_Init(void)
{
  /* 启用 DWT_CYCCNT */
  if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) == 0) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  }
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void delay_us(uint32_t us)
{
  uint32_t clk = HAL_RCC_GetHCLKFreq();
  uint32_t cycles = (clk / 1000000UL) * us;
  uint32_t start = DWT->CYCCNT;
  while ((DWT->CYCCNT - start) < cycles) {
    __NOP();
  }
}

static void DHT11_Pin_Output(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = DHT11_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

static void DHT11_Pin_Input(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = DHT11_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStruct);
}

bool DHT11_Init(void)
{
  DWT_Delay_Init();
  /* 将引脚设为输出并拉高（总线空闲为高） */
  DHT11_Pin_Output();
  HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);
  return true;
}

/* 发送起始信号：拉低 >= 18 ms，然后拉高 20-40 us，切换为输入等待应答 */
static void dht11_start(void)
{
  DHT11_Pin_Output();
  HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_RESET);
  vTaskDelay(pdMS_TO_TICKS(20)); /* >=18ms */
  HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);
  delay_us(30); /* 20~40us */
  DHT11_Pin_Input();
}

static int dht11_wait_response(void)
{
  uint32_t timeout = 0;
  /* 等待主机拉高后传感器拉低（约 80us） */
  while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET) {
    delay_us(1);
    if (++timeout > 100) return -1; /* 超时 */
  }
  timeout = 0;
  while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_RESET) {
    delay_us(1);
    if (++timeout > 100) return -1;
  }
  timeout = 0;
  while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET) {
    delay_us(1);
    if (++timeout > 100) return -1;
  }
  return 0;
}

static int dht11_read_bit(void)
{
  uint32_t timeout = 0;
  /* 等待拉低结束（低电平 ~50us） */
  while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_RESET) {
    delay_us(1);
    if (++timeout > 100) return -1;
  }
  /* 开始计时高电平长度 */
  uint32_t start = DWT->CYCCNT;
  while (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET) {
    if ((DWT->CYCCNT - start) > (HAL_RCC_GetHCLKFreq() / 1000000UL) * 200) {
      /* 超过 200us 视为异常 */
      return -1;
    }
  }
  uint32_t cycles = DWT->CYCCNT - start;
  uint32_t us = cycles / (HAL_RCC_GetHCLKFreq() / 1000000UL);
  /* 高电平 26~28us -> 0 ; ~70us -> 1 */
  return (us > 50) ? 1 : 0;
}

static int dht11_read_byte(uint8_t *byte)
{
  uint8_t i;
  uint8_t val = 0;
  for (i = 0; i < 8; i++) {
    int b = dht11_read_bit();
    if (b < 0) return -1;
    val <<= 1;
    if (b) val |= 1;
  }
  *byte = val;
  return 0;
}

bool DHT11_Read(int *temperature_c, int *humidity)
{
  uint8_t data[5] = {0};
  dht11_start();

#ifdef FREERTOS
  taskENTER_CRITICAL(); /* 保护微秒级采样，禁止任务切换 */
#endif

  if (dht11_wait_response() != 0) {
#ifdef FREERTOS
    taskEXIT_CRITICAL();
#endif
    return false;
  }

  for (int i = 0; i < 5; i++) {
    if (dht11_read_byte(&data[i]) != 0) {
#ifdef FREERTOS
      taskEXIT_CRITICAL();
#endif
      return false;
    }
  }

#ifdef FREERTOS
  taskEXIT_CRITICAL();
#endif
  uint8_t sum = data[0] + data[1] + data[2] + data[3];
  if (sum != data[4]) return false;
  if (humidity) *humidity = data[0];
  if (temperature_c) *temperature_c = data[2];
  /* 读完后将引脚设为输出并拉高，回到空闲状态 */
  DHT11_Pin_Output();
  HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);
  return true;
}
