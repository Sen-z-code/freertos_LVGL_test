/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "rtc.h"

/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/*
 * 说明（中文）：
 * - 本文件由 CubeMX 生成，负责 HAL 层 RTC 的初始化（MX_RTC_Init）和
 *   与时钟源（LSE）相关的 MSP 初始化。默认生成代码会在首次初始化
 *   时写入一个默认的日历时间（下文的 HAL_RTC_SetTime/HAL_RTC_SetDate）。
 * - 为了避免每次固件重新生成或上电时用默认时间覆盖已保存的时间，
 *   我们把实际的持久化/恢复逻辑放在非生成区的文件：
 *   Core/APP/rtc_driver.c（不会被 CubeMX 覆盖）。
 * - 这里的 USER CODE 区（尤其是 Check_RTC_BKUP）用于在 HAL 初始化后
 *   检查备份寄存器（BKP_DR0 的魔数），若备份存在则直接返回，跳过
 *   生成器写入默认时间的操作，从而保留 RTC/备份寄存器的一致性。
 * - 重要提示：持久化依赖备份域（VBAT）与低速晶振 LSE，请确保硬件
 *   已正确接线并且 VBAT 由电池（或超级电容）供电以实现掉电持续计时。
 */

/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

    /* USER CODE BEGIN Check_RTC_BKUP */
    /*
     * 中文说明：
     * - 真实的备份/恢复逻辑在 Core/APP/rtc_driver.c 中实现；该文件不会被
     *   CubeMX 覆盖。为了避免下面生成的默认日历写入覆盖已有备份，
     *   在此检查备份寄存器的魔数（BKP_DR0）。若发现魔数存在，说明
     *   备份有效并已由 rtc_driver 管理，则直接返回，跳过后续生成的
     *   HAL_RTC_SetTime/HAL_RTC_SetDate 调用。
     * - 该检查位于 USER CODE 区，CubeMX 不会覆盖该段注释与逻辑，所
     *   以可以安全地在这里放置保护性早退逻辑。
     */
    HAL_PWR_EnableBkUpAccess();
    {
      const uint32_t RTC_BKP_MAGIC_LOCAL = 0xA5A55A5AU;
      if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) == RTC_BKP_MAGIC_LOCAL)
      {
        /* 发现有效备份：跳过生成器的默认时间写入，保留现有 RTC/备份 */
        return;
      }
    }
    /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x21;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_THURSDAY;
  sDate.Month = RTC_MONTH_APRIL;
  sDate.Date = 0x9;
  sDate.Year = 0x26;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();
  /* USER CODE BEGIN RTC_MspInit 1 */

  /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
