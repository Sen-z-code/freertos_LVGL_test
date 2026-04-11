/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "st7789.h"
#include "MS5611Task.h"
#include "i2c.h"
#include "spi.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for myLVGLTask */
osThreadId_t myLVGLTaskHandle;
const osThreadAttr_t myLVGLTask_attributes = {
  .name = "myLVGLTask",
  .stack_size = 2048 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for myMS5611Task */
osThreadId_t myMS5611TaskHandle;
const osThreadAttr_t myMS5611Task_attributes = {
  .name = "myMS5611Task",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for myDHT11Task */
osThreadId_t myDHT11TaskHandle;
const osThreadAttr_t myDHT11Task_attributes = {
  .name = "myDHT11Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for myMPU6050Task */
osThreadId_t myMPU6050TaskHandle;
const osThreadAttr_t myMPU6050Task_attributes = {
  .name = "myMPU6050Task",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for mympuprintfTask */
osThreadId_t mympuprintfTaskHandle;
const osThreadAttr_t mympuprintfTask_attributes = {
  .name = "mympuprintfTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for myRTCTask */
osThreadId_t myRTCTaskHandle;
const osThreadAttr_t myRTCTask_attributes = {
  .name = "myRTCTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for myTouchTask */
osThreadId_t myTouchTaskHandle;
const osThreadAttr_t myTouchTask_attributes = {
  .name = "myTouchTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for mympu6050Queue */
osMessageQueueId_t mympu6050QueueHandle;
const osMessageQueueAttr_t mympu6050Queue_attributes = {
  .name = "mympu6050Queue"
};
/* Definitions for spi1Mutex */
osMutexId_t spi1MutexHandle;
const osMutexAttr_t spi1Mutex_attributes = {
  .name = "spi1Mutex"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
extern void StartLVGLTask(void *argument);
extern void StartMS5611Task(void *argument);
extern void StartDHT11Task(void *argument);
extern void StartMPU6050Task(void *argument);
extern void StartmpuprintfTask(void *argument);
extern void StartRTCTask(void *argument);
extern void StartTouchTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of spi1Mutex */
  spi1MutexHandle = osMutexNew(&spi1Mutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of mympu6050Queue */
  mympu6050QueueHandle = osMessageQueueNew (16, 32, &mympu6050Queue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of myLVGLTask */
  myLVGLTaskHandle = osThreadNew(StartLVGLTask, NULL, &myLVGLTask_attributes);

  /* creation of myMS5611Task */
  myMS5611TaskHandle = osThreadNew(StartMS5611Task, NULL, &myMS5611Task_attributes);

  /* creation of myDHT11Task */
  myDHT11TaskHandle = osThreadNew(StartDHT11Task, NULL, &myDHT11Task_attributes);

  /* creation of myMPU6050Task */
  myMPU6050TaskHandle = osThreadNew(StartMPU6050Task, NULL, &myMPU6050Task_attributes);

  /* creation of mympuprintfTask */
  mympuprintfTaskHandle = osThreadNew(StartmpuprintfTask, NULL, &mympuprintfTask_attributes);

  /* creation of myRTCTask */
  myRTCTaskHandle = osThreadNew(StartRTCTask, NULL, &myRTCTask_attributes);

  /* creation of myTouchTask */
  myTouchTaskHandle = osThreadNew(StartTouchTask, NULL, &myTouchTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

