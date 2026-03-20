#include "xpt2046.h"

#include "main.h"
#include "spi.h"

#define XPT2046_CMD_READ_X 0xD0U
#define XPT2046_CMD_READ_Y 0x90U
#define XPT2046_SAMPLES    5U

/* 运行时校准参数：把原始 ADC 值线性映射到屏幕像素坐标。 */
static uint16_t g_x_min = XPT2046_RAW_X_MIN;
static uint16_t g_x_max = XPT2046_RAW_X_MAX;
static uint16_t g_y_min = XPT2046_RAW_Y_MIN;
static uint16_t g_y_max = XPT2046_RAW_Y_MAX;

/* 片选拉低，开始一次触摸 SPI 事务。 */
static void xpt2046_select(void)
{
  HAL_GPIO_WritePin(T_CS_GPIO_Port, T_CS_Pin, GPIO_PIN_RESET);
}

/* 片选拉高，结束一次触摸 SPI 事务。 */
static void xpt2046_unselect(void)
{
  HAL_GPIO_WritePin(T_CS_GPIO_Port, T_CS_Pin, GPIO_PIN_SET);
}

/*
 * 发送通道读取命令并取回 12bit 原始值。
 * XPT2046 返回值左对齐在 16bit 数据里，因此需要右移 3 位。
 */
static uint16_t xpt2046_read_channel(uint8_t cmd)
{
  uint8_t tx[3] = {cmd, 0x00U, 0x00U};
  uint8_t rx[3] = {0U, 0U, 0U};
  uint16_t value;

  (void)HAL_SPI_TransmitReceive(&hspi2, tx, rx, 3U, HAL_MAX_DELAY);
  value = (uint16_t)(((uint16_t)rx[1] << 8) | rx[2]);
  return (uint16_t)(value >> 3);
}

/* 对小样本做升序排序，供中值滤波使用。 */
static void xpt2046_sort_u16(uint16_t *buf, uint8_t n)
{
  uint8_t i;
  uint8_t j;

  for (i = 0U; i < n; i++) {
    for (j = (uint8_t)(i + 1U); j < n; j++) {
      if (buf[j] < buf[i]) {
        uint16_t tmp = buf[i];
        buf[i] = buf[j];
        buf[j] = tmp;
      }
    }
  }
}

/* 多次采样后取中值，降低抖动和尖峰噪声。 */
static uint16_t xpt2046_read_filtered(uint8_t cmd)
{
  uint16_t samples[XPT2046_SAMPLES];
  uint8_t i;

  for (i = 0U; i < XPT2046_SAMPLES; i++) {
    samples[i] = xpt2046_read_channel(cmd);
  }

  xpt2046_sort_u16(samples, XPT2046_SAMPLES);
  return samples[XPT2046_SAMPLES / 2U];
}

/* 把输入值限制在[min_v, max_v]内，避免后续映射越界。 */
static uint16_t xpt2046_clamp_u16(uint16_t v, uint16_t min_v, uint16_t max_v)
{
  if (v < min_v) {
    return min_v;
  }
  if (v > max_v) {
    return max_v;
  }
  return v;
}

/* 初始化时保持触摸芯片不被选中，避免误通信。 */
void XPT2046_Init(void)
{
  xpt2046_unselect();
}

bool XPT2046_IsTouched(void)
{
  /* T_IRQ 为低电平表示被按下 */
  return (HAL_GPIO_ReadPin(T_IRQ_GPIO_Port, T_IRQ_Pin) == GPIO_PIN_RESET);
}

/*
 * 读取原始坐标：先判断是否按下，再读取 X/Y 通道并做中值滤波。
 * 若未按下，直接返回 false。
 */
bool XPT2046_ReadRaw(uint16_t *x_raw, uint16_t *y_raw)
{
  uint16_t xr;
  uint16_t yr;

  if (!XPT2046_IsTouched()) {
    return false;
  }

  xpt2046_select();
  xr = xpt2046_read_filtered(XPT2046_CMD_READ_X);
  yr = xpt2046_read_filtered(XPT2046_CMD_READ_Y);
  xpt2046_unselect();

  if (x_raw != NULL) {
    *x_raw = xr;
  }
  if (y_raw != NULL) {
    *y_raw = yr;
  }

  return true;
}

/*
 * 读取并输出像素坐标：
 * 1) 按安装方向做轴交换；2) 按校准范围截断；3) 线性映射到 LCD 分辨率；
 * 4) 按配置执行 X/Y 反向。
 */
bool XPT2046_ReadPoint(uint16_t *x, uint16_t *y)
{
  uint16_t xr;
  uint16_t yr;
  uint16_t raw_x;
  uint16_t raw_y;
  uint32_t px;
  uint32_t py;

  if (!XPT2046_ReadRaw(&xr, &yr)) {
    return false;
  }

#if XPT2046_SWAP_XY
  raw_x = yr;
  raw_y = xr;
#else
  raw_x = xr;
  raw_y = yr;
#endif

  raw_x = xpt2046_clamp_u16(raw_x, g_x_min, g_x_max);
  raw_y = xpt2046_clamp_u16(raw_y, g_y_min, g_y_max);

  px = ((uint32_t)(raw_x - g_x_min) * (uint32_t)(XPT2046_LCD_WIDTH - 1U)) /
       (uint32_t)(g_x_max - g_x_min);
  py = ((uint32_t)(raw_y - g_y_min) * (uint32_t)(XPT2046_LCD_HEIGHT - 1U)) /
       (uint32_t)(g_y_max - g_y_min);

#if XPT2046_INVERT_X
  px = (uint32_t)(XPT2046_LCD_WIDTH - 1U) - px;
#endif

#if XPT2046_INVERT_Y
  py = (uint32_t)(XPT2046_LCD_HEIGHT - 1U) - py;
#endif

  if (x != NULL) {
    *x = (uint16_t)px;
  }
  if (y != NULL) {
    *y = (uint16_t)py;
  }

  return true;
}

/* 更新校准参数，只有 min < max 时才生效，避免非法区间。 */
void XPT2046_SetCalibration(uint16_t x_min, uint16_t x_max, uint16_t y_min, uint16_t y_max)
{
  if (x_min < x_max) {
    g_x_min = x_min;
    g_x_max = x_max;
  }

  if (y_min < y_max) {
    g_y_min = y_min;
    g_y_max = y_max;
  }
}
