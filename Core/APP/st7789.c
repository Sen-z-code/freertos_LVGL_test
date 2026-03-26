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
#define ST7789_WIDTH_LANDSCAPE 320U
#define ST7789_HEIGHT_LANDSCAPE 240U
#define ST7789_BURST_PIXELS    64U

/* Runtime resolution follows panel orientation (portrait/landscape). */
static uint16_t g_width = ST7789_WIDTH_PORTRAIT;
static uint16_t g_height = ST7789_HEIGHT_PORTRAIT;

static void st7789_select(void);
static void st7789_unselect(void);

// 按 RGB565 缓冲连续写像素，内部做高低字节重排后通过 SPI 发送。
static void st7789_stream_color_buffer(const uint16_t *pixels, uint32_t pixel_count)
{
  uint8_t burst[ST7789_BURST_PIXELS * 2U];
  uint16_t chunk;
  uint16_t i;
  uint32_t remaining;

  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  st7789_select();

  remaining = pixel_count;
  while (remaining > 0U) {
    chunk = (remaining > ST7789_BURST_PIXELS) ? ST7789_BURST_PIXELS : (uint16_t)remaining;
    for (i = 0U; i < chunk; i++) {
      burst[2U * i] = (uint8_t)(pixels[i] >> 8);
      burst[(2U * i) + 1U] = (uint8_t)(pixels[i] & 0xFFU);
    }
    (void)HAL_SPI_Transmit(&hspi1, burst, (uint16_t)(chunk * 2U), HAL_MAX_DELAY);
    pixels += chunk;
    remaining -= chunk;
  }

  st7789_unselect();
}

// 连续写入同一颜色像素块，供全屏填充和局部矩形复用。
static void st7789_stream_solid_color(uint32_t pixel_count, uint16_t color)
{
  uint8_t burst[ST7789_BURST_PIXELS * 2U];
  uint32_t i;
  uint32_t remaining;
  uint16_t chunk;

  for (i = 0U; i < ST7789_BURST_PIXELS; i++) {
    burst[2U * i] = (uint8_t)(color >> 8);
    burst[(2U * i) + 1U] = (uint8_t)(color & 0xFFU);
  }

  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  st7789_select();

  remaining = pixel_count;
  while (remaining > 0U) {
    chunk = (remaining > ST7789_BURST_PIXELS) ? ST7789_BURST_PIXELS : (uint16_t)remaining;
    (void)HAL_SPI_Transmit(&hspi1, burst, (uint16_t)(chunk * 2U), HAL_MAX_DELAY);
    remaining -= chunk;
  }

  st7789_unselect();
}

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
 * 设置为竖屏方向，并同步软件侧分辨率。
 * 用途：后续所有窗口和全屏填充都按竖屏 240x320 计算。
 */
static void st7789_set_rotation_portrait(void)
{
  /*
   * MADCTL(0x36) 控制显示方向与颜色顺序。
   * 0x08 是当前屏子常见可用的竖屏参数之一（BGR 组合）。
   * 若出现上下颠倒、左右镜像、颜色红蓝互换，优先改这里。
   */
  uint8_t madctl = 0x08U;

  st7789_write_cmd(0x36U);
  st7789_write_data(&madctl, 1U);

  g_width = ST7789_WIDTH_PORTRAIT;
  g_height = ST7789_HEIGHT_PORTRAIT;
}

/*
 * 设置为横屏方向，并同步软件侧分辨率。
 * 用途：后续所有窗口和全屏填充都按横屏 320x240 计算。
 */
static void st7789_set_rotation_landscape(void)
{
  /*
   * 这里选择一个常见的横屏 MADCTL 值（MV + MX + BGR），若显示方向或颜色异常，
   * 可调整为 0x28/0xA8/0xE8 等合适值以匹配面板硬件。
   */
  uint8_t madctl = 0x68U;

  st7789_write_cmd(0x36U);
  st7789_write_data(&madctl, 1U);

  g_width = ST7789_WIDTH_LANDSCAPE;
  g_height = ST7789_HEIGHT_LANDSCAPE;
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
   * 0x20: 显示反相 OFF（与 LVGL 默认颜色语义一致）
   * 0x13: Normal Display Mode
   * 0x29: Display ON
   */
  st7789_write_cmd(0x20U);
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
  uint32_t total_pixels;

  /* Target whole frame. */
  st7789_set_window(0U, 0U, (uint16_t)(g_width - 1U), (uint16_t)(g_height - 1U));

  total_pixels = (uint32_t)g_width * (uint32_t)g_height;
  st7789_stream_solid_color(total_pixels, color);
}

void ST7789_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
  uint16_t x1;
  uint16_t y1;
  uint32_t pixels;

  if ((w == 0U) || (h == 0U)) {
    return;
  }

  if ((x >= g_width) || (y >= g_height)) {
    return;
  }

  x1 = (uint16_t)(x + w - 1U);
  y1 = (uint16_t)(y + h - 1U);

  if (x1 >= g_width) {
    x1 = (uint16_t)(g_width - 1U);
  }
  if (y1 >= g_height) {
    y1 = (uint16_t)(g_height - 1U);
  }

  st7789_set_window(x, y, x1, y1);
  pixels = (uint32_t)(x1 - x + 1U) * (uint32_t)(y1 - y + 1U);
  st7789_stream_solid_color(pixels, color);
}

bool ST7789_WritePixels(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *pixels)
{
  uint16_t x1;
  uint16_t y1;
  uint32_t count;

  if ((pixels == NULL) || (w == 0U) || (h == 0U)) {
    return false;
  }

  if ((x >= g_width) || (y >= g_height)) {
    return false;
  }

  x1 = (uint16_t)(x + w - 1U);
  y1 = (uint16_t)(y + h - 1U);

  // LVGL flush 正常不会越界；这里做保护，避免错误参数破坏显示。
  if ((x1 >= g_width) || (y1 >= g_height)) {
    return false;
  }

  st7789_set_window(x, y, x1, y1);
  count = (uint32_t)w * (uint32_t)h;
  st7789_stream_color_buffer(pixels, count);
  return true;
}

uint16_t ST7789_GetWidth(void)
{
  return g_width;
}

uint16_t ST7789_GetHeight(void)
{
  return g_height;
}
