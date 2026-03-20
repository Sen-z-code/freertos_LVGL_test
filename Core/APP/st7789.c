#include "st7789.h"

#include "main.h"
#include "spi.h"
#include "tim.h"

/*
 * 说明：
 * 1) 本文件按 ST7789 指令集最小流程初始化，目标是先稳定点亮。
 * 2) 关键命令来源是 ST7789 数据手册的 Command Table：
 *    - 0x2A/0x2B/0x2C：列地址/行地址/GRAM 写入
 *    - 0x36：MADCTL（扫描方向、RGB/BGR）
 *    - 0x3A：COLMOD（像素格式）
 *    - 0x11/0x29：Sleep Out / Display ON
 * 3) 关键延时来源是手册时序约束：Sleep Out 后需要等待 >=120ms。
 */

#define ST7789_WIDTH_PORTRAIT  240U
#define ST7789_HEIGHT_PORTRAIT 320U
#define ST7789_BURST_PIXELS    64U

/* Runtime resolution follows panel orientation (portrait/landscape). */
static uint16_t g_width = ST7789_WIDTH_PORTRAIT;
static uint16_t g_height = ST7789_HEIGHT_PORTRAIT;

/* 拉低 CS，选中 LCD，从机开始接收本次 SPI 传输。 */
static void st7789_select(void)
{
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
}

/* 拉高 CS，结束本次 SPI 传输并释放总线片选。 */
static void st7789_unselect(void)
{
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

/*
 * 发送 1 字节命令。
 * 用途：写控制寄存器入口命令，例如 0x2A/0x2B/0x2C/0x36/0x3A。
 */
static void st7789_write_cmd(uint8_t cmd)
{
  /* D/C=0 means the byte is interpreted as a register command. */
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);
  st7789_select();
  (void)HAL_SPI_Transmit(&hspi1, &cmd, 1U, HAL_MAX_DELAY);
  st7789_unselect();
}

/*
 * 发送命令对应的数据区。
 * 用途：在 st7789_write_cmd 后跟随参数数据，长度可变。
 */
static void st7789_write_data(const uint8_t *data, uint16_t size)
{
  /* D/C=1 means payload data for the previously selected command. */
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  st7789_select();
  (void)HAL_SPI_Transmit(&hspi1, (uint8_t *)data, size, HAL_MAX_DELAY);
  st7789_unselect();
}

/*
 * 通过 RST 引脚做硬复位。
 * 用途：上电初始化前清状态，避免控制器停留在未知模式。
 */
static void st7789_hard_reset(void)
{
  /*
   * 硬件复位：先拉低再拉高，让控制器回到已知状态。
   * 这里用 10ms 低电平、120ms 释放等待，明显大于手册最小值，
   * 目的是兼容上电斜率慢/线缆较长等实际情况，提升一次点亮成功率。
   */
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(10U);
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(120U);
}

/*
 * 设置为横屏方向，并同步软件侧分辨率。
 * 用途：后续所有窗口和全屏填充都按横屏 320x240 计算。
 */
static void st7789_set_rotation_landscape(void)
{
  /*
   * MADCTL(0x36) 控制显示方向与颜色顺序。
   * 0x68 是当前屏子可用的横屏参数之一（MY/MX/MV 与 BGR 组合）。
   * 若出现上下颠倒、左右镜像、颜色红蓝互换，优先改这里。
   */
  uint8_t madctl = 0x68U;

  st7789_write_cmd(0x36U);
  st7789_write_data(&madctl, 1U);

  g_width = 320U;
  g_height = 240U;
}

/*
 * 设置显存写入窗口。
 * 用途：限定后续 0x2C 像素流的落点区域，可用于全屏或局部刷新。
 */
static void st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  uint8_t data[4];

  /*
   * 0x2A: 列地址（X 起止），0x2B: 行地址（Y 起止），0x2C: 开始写显存。
   * 这三个命令就是“地址从哪里来”的核心：来自 ST7789 指令表。
   * 地址按高字节在前发送（big-endian）：[MSB, LSB]。
   */
  st7789_write_cmd(0x2AU);
  data[0] = (uint8_t)(x0 >> 8);
  data[1] = (uint8_t)(x0 & 0xFFU);
  data[2] = (uint8_t)(x1 >> 8);
  data[3] = (uint8_t)(x1 & 0xFFU);
  st7789_write_data(data, 4U);

  st7789_write_cmd(0x2BU);
  data[0] = (uint8_t)(y0 >> 8);
  data[1] = (uint8_t)(y0 & 0xFFU);
  data[2] = (uint8_t)(y1 >> 8);
  data[3] = (uint8_t)(y1 & 0xFFU);
  st7789_write_data(data, 4U);

  st7789_write_cmd(0x2CU);
}

/*
 * 设置背光亮度百分比（0~100）。
 * 用途：通过 TIM4 PWM 调节 LED 背光，0 为灭，100 为最亮。
 */
void ST7789_SetBacklight(uint8_t percent)
{
  uint32_t pulse;

  if (percent > 100U) {
    percent = 100U;
  }

  /* Convert percentage to CCR value based on timer period. */
  (void)HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
  pulse = ((uint32_t)htim4.Init.Period * percent) / 100U;
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, pulse);
}

/*
 * ST7789 最小初始化流程。
 * 用途：完成复位、退出休眠、像素格式和方向配置，并最终开显。
 */
void ST7789_Init(void)
{
  uint8_t data;

  /* 初始化期间先关背光，避免屏在未配置完成前出现随机闪烁。 */
  ST7789_SetBacklight(0U);
  st7789_hard_reset();

  /* 软件复位（可进一步清内部状态）。 */
  st7789_write_cmd(0x01U);
  HAL_Delay(120U);

  /*
   * Sleep Out：退出休眠。
   * 手册要求该命令后等待 >=120ms，再进行后续寄存器配置。
   */
  st7789_write_cmd(0x11U);
  HAL_Delay(120U);

  /*
   * COLMOD(0x3A)=0x55 -> 16bit/pixel (RGB565)。
   * 之所以选 565：带宽与色彩效果在 MCU SPI 场景下更平衡。
   */
  data = 0x55U;
  st7789_write_cmd(0x3AU);
  st7789_write_data(&data, 1U);

  st7789_set_rotation_landscape();

  /*
   * 0x21: 显示反相 ON（很多面板观感更好，可按需关闭）
   * 0x13: Normal Display Mode
   * 0x29: Display ON
   */
  st7789_write_cmd(0x21U);
  st7789_write_cmd(0x13U);
  st7789_write_cmd(0x29U);
  HAL_Delay(20U);

  /* 参数写完再开背光，避免用户看到上电中间态。 */
  ST7789_SetBacklight(50U);
}

/*
 * 全屏填充单一颜色。
 * 用途：用于硬件自检、清屏、以及后续 UI 框架接入前的基础显示验证。
 */
void ST7789_FillColor(uint16_t color)
{
  uint8_t burst[ST7789_BURST_PIXELS * 2U];
  uint32_t total_pixels;
  uint32_t remaining;
  uint32_t i;
  uint16_t chunk;

  /* Prebuild a small repeated RGB565 chunk to reduce CPU overhead. */
  for (i = 0U; i < ST7789_BURST_PIXELS; i++) {
    burst[2U * i] = (uint8_t)(color >> 8);
    burst[(2U * i) + 1U] = (uint8_t)(color & 0xFFU);
  }

  /* Target whole frame. */
  st7789_set_window(0U, 0U, (uint16_t)(g_width - 1U), (uint16_t)(g_height - 1U));

  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  st7789_select();

  /* Stream frame data in fixed-size bursts over SPI. */
  total_pixels = (uint32_t)g_width * (uint32_t)g_height;
  remaining = total_pixels;
  while (remaining > 0U) {
    chunk = (remaining > ST7789_BURST_PIXELS) ? ST7789_BURST_PIXELS : (uint16_t)remaining;
    (void)HAL_SPI_Transmit(&hspi1, burst, (uint16_t)(chunk * 2U), HAL_MAX_DELAY);
    remaining -= chunk;
  }

  st7789_unselect();
}
