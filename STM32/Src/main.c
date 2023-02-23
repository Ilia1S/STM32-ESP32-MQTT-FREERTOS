/* USER CODE BEGIN Header */

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "DHT.h"
#include "stdio.h"
#include "string.h"
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

/* USER CODE BEGIN PV */
uint16_t lightArr[1];
uint8_t light = 0, lightNew = 0, humNew = 0, tempNew = 0, threshold = 30, manualLight = 0;
char sendBufUart2[40], sendBufUart1[10], readBufUart1[2], stmToEsp[2], espToStm[40];
volatile uint8_t lightConvCompleted = 0, dhtConvCompleted = 0, dataReceived = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void sleepMode(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_TIM5_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_OC_Start(&htim2, TIM_CHANNEL_1);
  HAL_TIM_Base_Start_IT(&htim5);
  HAL_ADC_Start_DMA(&hadc1, (uint32_t *)lightArr, 1);
  static DHT_sensor livingRoom = {GPIOC, GPIO_PIN_2, DHT11};
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if (dhtConvCompleted)
	  {
		  DHT_data d = DHT_getData(&livingRoom);
		  //if temperature or humidity has changed more than 5% - transmit data to ESP32
		  if (d.temp <= tempNew*0.95 || d.temp >= tempNew*1.05 || d.hum <= humNew*0.95 || d.hum >= humNew*1.05)
		  {
			  sprintf(sendBufUart2, "\n\rTemperature %d°C, Humidity %d%%", (uint8_t)d.temp, (uint8_t)d.hum);
			  HAL_UART_Transmit(&huart2, (uint8_t*) sendBufUart2, strlen(sendBufUart2), HAL_MAX_DELAY);
			  stmToEsp[0] = 1; //type of sensor data, 1 - temperature, 2 - humidity, 3 - light
			  stmToEsp[1] = d.temp; //value
			  HAL_UART_Transmit(&huart1, (uint8_t*) stmToEsp, 2, HAL_MAX_DELAY);
			  stmToEsp[0] = 2;
			  stmToEsp[1] = d.hum;
			  HAL_UART_Transmit(&huart1, (uint8_t*) stmToEsp, 2, HAL_MAX_DELAY);
			  tempNew = d.temp;
			  humNew = d.hum;
		  }
		  dhtConvCompleted = 0;
		  sleepMode();
	  }
	  if (lightConvCompleted)
	  {
		  light = 100 - (((float)lightArr[0]-700)/3380)*100;
		  if (light <= threshold && manualLight == 0)
		  {
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, SET);
		  }
		  else if (light > threshold && manualLight == 0)
		  {
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, RESET);
		  }
		  //if outside light has changed more than 5% - transmit data to ESP32
		  if (light <= lightNew*0.9 || light >= lightNew*1.1)
		  {
			  sprintf(sendBufUart2, "\n\rLight = %u%%", light);
			  HAL_UART_Transmit(&huart2, (uint8_t*) sendBufUart2, strlen(sendBufUart2), HAL_MAX_DELAY);
			  stmToEsp[0] = 3;
			  stmToEsp[1] = light;
			  HAL_UART_Transmit(&huart1, (uint8_t*) stmToEsp, 2, HAL_MAX_DELAY);
			  lightNew = light;
		  }
		  lightConvCompleted = 0;
		  sleepMode();
	  }
	  HAL_UART_Receive_IT(&huart1, (uint8_t*) readBufUart1, 2);
	  if (dataReceived)
	  {
		  HAL_UART_Receive_IT(&huart1, (uint8_t*) readBufUart1, 2);
		  sprintf(espToStm, "\n\rCommand type - %u, value - %u", readBufUart1[0], readBufUart1[1]);
		  HAL_UART_Transmit(&huart2, (uint8_t*) espToStm, strlen(espToStm), HAL_MAX_DELAY);
		  if(readBufUart1[0] == 1) //command type: 1 - LED state, 2 - thresholds, 3 - manual mode
		  {
			  if (readBufUart1[1] == 1)
			  {
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, SET);
				  manualLight = 1;
			  }
			  else if (readBufUart1[1] == 0)
			  {
				  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, RESET);
				  manualLight = 1;
			  }
		  }
		  else if (readBufUart1[0] == 2)
		  {
			  threshold = readBufUart1[1];
		  }
		  else if (readBufUart1[0] == 3)
		  {
			  manualLight = readBufUart1[1];
		  }
		  dataReceived = 0;
		  sleepMode();
	  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	lightConvCompleted = 1;
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	dhtConvCompleted = 1;
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	dataReceived = 1;
}
void sleepMode(void)
{
	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
	HAL_SuspendTick();
	HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
	HAL_ResumeTick();
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
