/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "stdio.h"
#include "string.h"
#include "DHT.h"
#include "usart.h"
#include "adc.h"
#include "tim.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TASK_DEBUG

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static uint8_t threshold = 10, manualLight = 0, bitMask1 = 0x9, bitMask2 = 0x1, bitMask3 = 0xC, bitMask4 = 0;
static EventBits_t totalBits = 0x0;
char sendBufUart2[40], sendBufUart1[10], readBufUart1[2], stmToEsp[2], espToStm[40];
SemaphoreHandle_t xSemaphoreEspToStm;
EventGroupHandle_t myGroupHandle1;

/* USER CODE END Variables */
osThreadId DHTTaskHandle;
osThreadId ADCTaskHandle;
osThreadId DataFromESPTaskHandle;
osThreadId DefaultTaskHandle;
osThreadId InputsTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
void TaskSwitchedIn(int tag);
void TaskSwitchedOut(int tag);

/* USER CODE END FunctionPrototypes */

void StartDHTTask(void const * argument);
void StartADCTask(void const * argument);
void StartDataFromESPTask(void const * argument);
void StartDefaultTask(void const * argument);
void StartInputsTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationIdleHook(void);
void vApplicationTickHook(void);

/* USER CODE BEGIN 2 */
void vApplicationIdleHook( void )
{
#ifdef TASK_DEBUG
	GPIOC->BSRR |= GPIO_BSRR_BS12;
	GPIOC->BSRR |= GPIO_BSRR_BR12;
#endif
}
/* USER CODE END 2 */

/* USER CODE BEGIN 3 */
void vApplicationTickHook( void )
{
#ifdef TASK_DEBUG
	GPIOC->BSRR |= GPIO_BSRR_BS11;
	GPIOC->BSRR |= GPIO_BSRR_BR11;
#endif
}
/* USER CODE END 3 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
	xSemaphoreEspToStm = xSemaphoreCreateBinary();
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
	myGroupHandle1 = xEventGroupCreate();
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of DHTTask */
  osThreadDef(DHTTask, StartDHTTask, osPriorityNormal, 0, 128);
  DHTTaskHandle = osThreadCreate(osThread(DHTTask), NULL);

  /* definition and creation of ADCTask */
  osThreadDef(ADCTask, StartADCTask, osPriorityNormal, 0, 128);
  ADCTaskHandle = osThreadCreate(osThread(ADCTask), NULL);

  /* definition and creation of DataFromESPTask */
  osThreadDef(DataFromESPTask, StartDataFromESPTask, osPriorityAboveNormal, 0, 128);
  DataFromESPTaskHandle = osThreadCreate(osThread(DataFromESPTask), NULL);

  /* definition and creation of DefaultTask */
  osThreadDef(DefaultTask, StartDefaultTask, osPriorityHigh, 0, 128);
  DefaultTaskHandle = osThreadCreate(osThread(DefaultTask), NULL);

  /* definition and creation of InputsTask */
  osThreadDef(InputsTask, StartInputsTask, osPriorityHigh, 0, 128);
  InputsTaskHandle = osThreadCreate(osThread(InputsTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDHTTask */
/**
  * @brief  Function implementing the DHTTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDHTTask */
void StartDHTTask(void const * argument)
{
  /* USER CODE BEGIN StartDHTTask */
#ifdef TASK_DEBUG
	vTaskSetApplicationTaskTag(NULL, (void*)1);
#endif
	uint8_t humNew = 0;
	DHT_sensor livingRoom = { GPIOC, GPIO_PIN_6, DHT11 };
	/* Infinite loop */
	for (;;)
	{
		DHT_data d = DHT_getData(&livingRoom);
		//if temperature or humidity has changed more than 10% - transmit data to ESP32
		sprintf(sendBufUart2, "\n\rTemperature %d°C, Humidity %d%%", (uint8_t) d.temp, (uint8_t) d.hum);
		HAL_UART_Transmit(&huart2, (uint8_t*) sendBufUart2, strlen(sendBufUart2), HAL_MAX_DELAY);
		stmToEsp[0] = 1; //type of sensor data, 1 - temperature, 2 - humidity, 3 - light
		stmToEsp[1] = d.temp; //value
		HAL_UART_Transmit(&huart1, (uint8_t*) stmToEsp, 2, HAL_MAX_DELAY);
		if (d.hum <= humNew * 0.9 || d.hum >= humNew * 1.1)
		{
			stmToEsp[0] = 2;
			stmToEsp[1] = d.hum;
			HAL_UART_Transmit(&huart1, (uint8_t*) stmToEsp, 2, HAL_MAX_DELAY);
		}
		humNew = d.hum;
		osDelay(10000);
	}
  /* USER CODE END StartDHTTask */
}

/* USER CODE BEGIN Header_StartADCTask */
/**
* @brief Function implementing the ADCTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartADCTask */
void StartADCTask(void const * argument)
{
  /* USER CODE BEGIN StartADCTask */
#ifdef TASK_DEBUG
	vTaskSetApplicationTaskTag(NULL, (void*)2);
#endif
	uint16_t lightArr[1];
	uint8_t light = 0, lightNew = 0;
  /* Infinite loop */
	for (;;)
	{
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
		lightArr[0] = HAL_ADC_GetValue(&hadc1);
		HAL_ADC_Stop(&hadc1);
		light = 100 - ((float) lightArr[0] / 255) * 100;
		if (light <= threshold && manualLight == 0)
		{
			GPIOC->BSRR |= GPIO_BSRR_BS8;
		}
		else if (light > threshold && manualLight == 0)
		{
			GPIOC->BSRR |= GPIO_BSRR_BR8;
		}
		//if outside light has changed more than 10% - transmit data to ESP32
		if (light <= lightNew * 0.9 || light >= lightNew * 1.1)
		{
			sprintf(sendBufUart2, "\n\rLight = %u%%", light);
			HAL_UART_Transmit(&huart2, (uint8_t*) sendBufUart2, strlen(sendBufUart2), HAL_MAX_DELAY);
			stmToEsp[0] = 3;
			stmToEsp[1] = light;
			HAL_UART_Transmit(&huart1, (uint8_t*) stmToEsp, 2, HAL_MAX_DELAY);
			lightNew = light;
		}
		osDelay(10000);
	}
  /* USER CODE END StartADCTask */
}

/* USER CODE BEGIN Header_StartDataFromESPTask */
/**
* @brief Function implementing the DataFromESPTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDataFromESPTask */
void StartDataFromESPTask(void const * argument)
{
  /* USER CODE BEGIN StartDataFromESPTask */
#ifdef TASK_DEBUG
	vTaskSetApplicationTaskTag(NULL, (void*)3);
#endif
  /* Infinite loop */
	for (;;)
	{
		HAL_UART_Receive_IT(&huart1, (uint8_t*) readBufUart1, 2);
		if (xSemaphoreTake(xSemaphoreEspToStm, portMAX_DELAY) == pdTRUE)
		{
//			sprintf(espToStm, "\n\rCommand type - %u, value - %u", readBufUart1[0], readBufUart1[1]);
//			HAL_UART_Transmit(&huart2, (uint8_t*) espToStm, strlen(espToStm), HAL_MAX_DELAY);
			switch (readBufUart1[0]) //command type: 1 - LED state, 2 - thresholds, 3 - manual mode,
									 //4, 5, 6, 7 - bitmasks
			{
				case 1:
					if (readBufUart1[1] == 1)
					{
						GPIOC->BSRR |= GPIO_BSRR_BS8;
						manualLight = 1;
					}
					else if (readBufUart1[1] == 0)
					{
						GPIOC->BSRR |= GPIO_BSRR_BR8;
						manualLight = 1;
					}
					break;
				case 2:
					threshold = readBufUart1[1];
					break;
				case 3:
					manualLight = readBufUart1[1];
					break;
				case 4:
					bitMask1 = readBufUart1[1];
					break;
				case 5:
					bitMask2 = readBufUart1[1];
					break;
				case 6:
					bitMask3 = readBufUart1[1];
					break;
				case 7:
					bitMask4 = readBufUart1[1];
					break;
			}
		}
	}
  /* USER CODE END StartDataFromESPTask */
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
* @brief Function implementing the DefaultTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
	HAL_UART_Transmit(&huart2, (uint8_t*)"\033[0;0H", strlen("\033[0;0H"), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t*)"\033[2J", strlen("\033[2J"), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t*) "Welcome to MeteoStation X 2.0!\n",
			strlen("Welcome to MeteoStation X 2.0!\n\n"), HAL_MAX_DELAY);
  /* Infinite loop */
	for (;;)
	{
		GPIOA->BSRR |= GPIO_BSRR_BS5;
		vTaskSuspend(DefaultTaskHandle);
	}
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartInputsTask */
/**
* @brief Function implementing the InputsTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartInputsTask */
void StartInputsTask(void const * argument)
{
  /* USER CODE BEGIN StartInputsTask */
#ifdef TASK_DEBUG
	vTaskSetApplicationTaskTag(NULL, (void*)4);
#endif
  /* Infinite loop */
	for (;;)
	{
		EventBits_t uxBits = xEventGroupWaitBits(myGroupHandle1, 0xF, pdTRUE, pdFALSE, portMAX_DELAY);
		totalBits = uxBits | totalBits;
		if ((totalBits & bitMask1) == bitMask1)
		{
			GPIOA->BSRR = GPIO_BSRR_BS6;
		}
		if ((totalBits & bitMask2) == bitMask2)
		{
			GPIOA->BSRR = GPIO_BSRR_BS7;
		}
		if ((totalBits & bitMask3) == bitMask3)
		{
			GPIOB->BSRR = GPIO_BSRR_BS1;
		}
		if ((totalBits & bitMask4) == bitMask4)
		{
			GPIOB->BSRR = GPIO_BSRR_BS2;
		}
	}
  /* USER CODE END StartInputsTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	SysTick->CTRL  |= SysTick_CTRL_TICKINT_Msk;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE, xResult;
	xResult = xSemaphoreGiveFromISR(xSemaphoreEspToStm, &xHigherPriorityTaskWoken);
	if (xResult != errQUEUE_FULL)
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE, xSetBitsResult = pdFAIL;
	switch (GPIO_Pin)
	{
		case GPIO_PIN_0:
			xSetBitsResult = xEventGroupSetBitsFromISR(myGroupHandle1, 0x1, &xHigherPriorityTaskWoken);
			break;
		case GPIO_PIN_1:
			xSetBitsResult = xEventGroupSetBitsFromISR(myGroupHandle1, 0x2, &xHigherPriorityTaskWoken);
			break;
		case GPIO_PIN_2:
			xSetBitsResult = xEventGroupSetBitsFromISR(myGroupHandle1, 0x4, &xHigherPriorityTaskWoken);
			break;
		case GPIO_PIN_3:
			xSetBitsResult = xEventGroupSetBitsFromISR(myGroupHandle1, 0x8, &xHigherPriorityTaskWoken);
			break;
		case GPIO_PIN_13:
			xEventGroupClearBitsFromISR(myGroupHandle1, 0xF);
			totalBits = 0x0;
			GPIOA->BSRR = GPIO_BSRR_BR6;
			GPIOA->BSRR = GPIO_BSRR_BR7;
			GPIOB->BSRR = GPIO_BSRR_BR1;
			GPIOB->BSRR = GPIO_BSRR_BR2;
			break;
	}
	if (xSetBitsResult != pdFAIL)
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
void TaskSwitchedIn(int tag)
{
#ifdef TASK_DEBUG
	switch(tag)
	{
		case 1:
			GPIOH->BSRR |= GPIO_BSRR_BS1;
			break;
		case 2:
			GPIOB->BSRR |= GPIO_BSRR_BS7;
			break;
		case 3:
			GPIOA->BSRR |= GPIO_BSRR_BS15;
			break;
		case 4:
			GPIOC->BSRR |= GPIO_BSRR_BS10;
			break;
	}
#endif
}
void TaskSwitchedOut(int tag)
{
#ifdef TASK_DEBUG
	switch(tag)
	{
		case 1:
			GPIOH->BSRR |= GPIO_BSRR_BR1;
			break;
		case 2:
			GPIOB->BSRR |= GPIO_BSRR_BR7;
			break;
		case 3:
			GPIOA->BSRR |= GPIO_BSRR_BR15;
			break;
		case 4:
			GPIOC->BSRR |= GPIO_BSRR_BR10;
			break;
	}
#endif
}
void vPortSuppressTicksAndSleep(TickType_t xExpectedIdleTime)
{
	unsigned long ulLowPowerTimeBeforeSleep = 0, ulLowPowerTimeAfterSleep;
	eSleepModeStatus eSleepStatus;
	htim5.Init.Period = 2 * xExpectedIdleTime - 1;
	HAL_TIM_Base_Init(&htim5);
	HAL_TIM_Base_Start_IT(&htim5);
	SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
	__disable_irq();
	eSleepStatus = eTaskConfirmSleepModeStatus();
	if (eSleepStatus == eAbortSleep)
	{
		SysTick->CTRL  |= SysTick_CTRL_TICKINT_Msk;
		__enable_irq();
		HAL_TIM_Base_Stop_IT(&htim5);
		return;
	}
	else if (eSleepStatus == eStandardSleep)
	{
		GPIOA->BSRR |= GPIO_BSRR_BR5;
		HAL_PWR_EnterSLEEPMode(0, PWR_STOPENTRY_WFI);
		ulLowPowerTimeAfterSleep = __HAL_TIM_GET_COUNTER(&htim5);
		vTaskStepTick(ulLowPowerTimeAfterSleep - ulLowPowerTimeBeforeSleep);
		HAL_TIM_Base_Stop_IT(&htim5);
		__enable_irq();
		SysTick->CTRL  |= SysTick_CTRL_TICKINT_Msk;
		GPIOA->BSRR |= GPIO_BSRR_BS5;
	}
}
/* USER CODE END Application */
