/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  * Author: Maltsev Yuriy
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "utils.h"
#include "LCD_LPH8731.h"
#include "uart.h"
#include "buttons.h"
#include "testspeedcopy.h"
#include "i2c.h"
#include "bmp280.h"
#include "ATH25.h"
// #include "mt6701.h"
#include "parse_cmd.h"

// то что хотим видеть в глобальной области видимости, подключаем непосредственно в "main.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticSemaphore_t osStaticMutexDef_t;
typedef StaticSemaphore_t osStaticSemaphoreDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define INCLUDE_vTaskSuspend 1
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

CRC_HandleTypeDef hcrc;

DAC_HandleTypeDef hdac;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

IWDG_HandleTypeDef hiwdg;

RNG_HandleTypeDef hrng;

TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart2;

DMA_HandleTypeDef hdma_memtomem_dma2_stream1;
/* Definitions for keyReadTask */
osThreadId_t keyReadTaskHandle;
uint32_t keyReadTaskBuffer[ 512 ];
osStaticThreadDef_t keyReadTaskControlBlock;
const osThreadAttr_t keyReadTask_attributes = {
  .name = "keyReadTask",
  .cb_mem = &keyReadTaskControlBlock,
  .cb_size = sizeof(keyReadTaskControlBlock),
  .stack_mem = &keyReadTaskBuffer[0],
  .stack_size = sizeof(keyReadTaskBuffer),
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for uartReadTask */
osThreadId_t uartReadTaskHandle;
uint32_t uartReadTaskBuffer[ 512 ];
osStaticThreadDef_t uartReadTaskControlBlock;
const osThreadAttr_t uartReadTask_attributes = {
  .name = "uartReadTask",
  .cb_mem = &uartReadTaskControlBlock,
  .cb_size = sizeof(uartReadTaskControlBlock),
  .stack_mem = &uartReadTaskBuffer[0],
  .stack_size = sizeof(uartReadTaskBuffer),
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for ADC1ReadTask */
osThreadId_t ADC1ReadTaskHandle;
uint32_t myTaskADC1Buffer[ 512 ];
osStaticThreadDef_t myTaskADC1ControlBlock;
const osThreadAttr_t ADC1ReadTask_attributes = {
  .name = "ADC1ReadTask",
  .cb_mem = &myTaskADC1ControlBlock,
  .cb_size = sizeof(myTaskADC1ControlBlock),
  .stack_mem = &myTaskADC1Buffer[0],
  .stack_size = sizeof(myTaskADC1Buffer),
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for BMP280Task */
osThreadId_t BMP280TaskHandle;
uint32_t BMP280TaskBuffer[ 512 ];
osStaticThreadDef_t BMP280TaskControlBlock;
const osThreadAttr_t BMP280Task_attributes = {
  .name = "BMP280Task",
  .cb_mem = &BMP280TaskControlBlock,
  .cb_size = sizeof(BMP280TaskControlBlock),
  .stack_mem = &BMP280TaskBuffer[0],
  .stack_size = sizeof(BMP280TaskBuffer),
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for myMutex01 */
osMutexId_t myMutex01Handle;
osStaticMutexDef_t myMutex01ControlBlock;
const osMutexAttr_t myMutex01_attributes = {
  .name = "myMutex01",
  .cb_mem = &myMutex01ControlBlock,
  .cb_size = sizeof(myMutex01ControlBlock),
};
/* Definitions for myBinarySemAdcReady */
osSemaphoreId_t myBinarySemAdcReadyHandle;
osStaticSemaphoreDef_t myBinarySemAdcReadyControlBlock;
const osSemaphoreAttr_t myBinarySemAdcReady_attributes = {
  .name = "myBinarySemAdcReady",
  .cb_mem = &myBinarySemAdcReadyControlBlock,
  .cb_size = sizeof(myBinarySemAdcReadyControlBlock),
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_CRC_Init(void);
static void MX_DAC_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_RNG_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_IWDG_Init(void);
void StartTaskKeyRead(void *argument);
void StartTaskUartRead(void *argument);
void StartTaskADC(void *argument);
void StartTaskReadBMP280(void *argument);

/* USER CODE BEGIN PFP */
// область для объявления приватных функций
float get_vdd_from_adc(uint16_t val);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

uint16_t adc[4] = {0}; // ADC: pin A0, pin A1, Tmp_int, VDD_int

// Преобразует raw величину с внутреннего
// канала ADC в напряжение питания, если внутреннее опорное ровно 1.2В
float get_vdd_from_adc(uint16_t val){
	return (1 * 1.2f * 4096) / val;
}

/* callback на завершение преобразования ADC */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
	if (hadc->Instance == ADC1) {
		// через семафор разрешим задаче обработать данные
		osSemaphoreRelease(myBinarySemAdcReadyHandle);

		// если режим не continues и запуск не от таймера, то нужно перезапускать
		// HAL_ADC_Start_DMA(&hadc1, (uint32_t*) adc, _SIZE_ARRAY(adc));
	}
}

// висит на пинах поворотный энкодер
// TO DO: реализовать защиту от дребезга на энкодере
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	printf("GPIO %d\r\n", GPIO_Pin);
}

// Опрос BMP280 и ATH25 и вывод результатов в терминал и на LCD
void read_bmp280_ath25() {
#ifdef INC_BMP280_H_
	float temperature, pressure;
	bmp280_ReadData(&temperature, &pressure);
#ifdef INC_ATH25_H_
	float ATH25_temperature, ATH25_humidity;
	HAL_StatusTypeDef status;

	status = ATH25_Read_Data(&ATH25_temperature, &ATH25_humidity);
	if (status == HAL_OK) {
		printf("T: %.2f; P1: %.2f; P2: %.4f; H: %.2f; T2: %.2f\r\n",
				temperature, pressure, pressure / mmHg, ATH25_humidity,
				ATH25_temperature);
	} else {
		printf("T: %.2f; P1: %.2f; P2: %.4f;\r\n", temperature, pressure,
				pressure / mmHg);
	}
#else
	printf("T: %.2f; P1: %.2f; P2: %.4f;\r\n",
			temperature, pressure, pressure/mmHg);
#endif /* INC_ATH25_H_ */

#ifdef INC_MT6701_H_
	 uint16_t rez_raw = mt6701_read();
	 float deg = rez_raw * (360.0f / 16384.0f);
	 printf("MT6701 = raw: %4d, deg: %.3f", rez_raw, deg);
#endif /* INC_MT6701_H_ */

	// print to LCD
	uint16_t last_color_ink = color_ink;
	uint16_t last_color_back = color_back;
	color_ink = RED;
	color_back = WHITE;
	// memset(buffLcd, 0, sizeof(buffLcd));
#ifdef INC_ATH25_H_
	sprintf(buffLcd, "T: %.2f %d \nP: %.3f mmhg\nH: %.2f T2:%.2f", temperature,
			(int) pressure, pressure / mmHg, ATH25_humidity, ATH25_temperature);
#else
	sprintf(buffLcd, "T: %.2f %d \nP: %.3f mmhg",
			temperature, (int)pressure, pressure/mmHg);
#endif /* INC_ATH25_H_ */
	lcd_print_str(buffLcd, 0, 4, 1, 1);
	color_ink = last_color_ink;
	color_back = last_color_back;
#endif /* INC_BMP280_H_ */
}

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
  MX_ADC1_Init();
  MX_CRC_Init();
  MX_DAC_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_RNG_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */

	printf("Run on controller: ");
#if defined(STM32F103xB)
	printf("'STM32F103xB'\r\n");
#elif defined(STM32F407xx)
	printf("'STM32F407xx'\r\n");
#else
	printf("'unknown'\r\n");
#endif

	lcd_init();

	UART_Start();

	/* меняем настройки прескалера TIM4 */
	htim4.Init.Prescaler = 42000 - 1; // 42 MHz -> 1 KHz
	htim4.Init.Period = 5000; // msec
	TIM_Base_SetConfig(htim4.Instance, &htim4.Init); // Set the Time Base configuration
	HAL_TIM_OC_Start(&htim4, TIM_CHANNEL_4); // запускаем таймер

	// HAL_ADCEx_Calibration_Start(&hadc1); // отсутствует в F4, но есть в F1
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*) adc, _SIZE_ARRAY(adc));

#ifdef INC_BMP280_H_
	bmp280_init();
#endif
#ifdef INC_ATH25_H_
	ATH25_init();
#endif
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();
  /* Create the mutex(es) */
  /* creation of myMutex01 */
  myMutex01Handle = osMutexNew(&myMutex01_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of myBinarySemAdcReady */
  myBinarySemAdcReadyHandle = osSemaphoreNew(1, 0, &myBinarySemAdcReady_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of keyReadTask */
  keyReadTaskHandle = osThreadNew(StartTaskKeyRead, NULL, &keyReadTask_attributes);

  /* creation of uartReadTask */
  uartReadTaskHandle = osThreadNew(StartTaskUartRead, NULL, &uartReadTask_attributes);

  /* creation of ADC1ReadTask */
  ADC1ReadTaskHandle = osThreadNew(StartTaskADC, NULL, &ADC1ReadTask_attributes);

  /* creation of BMP280Task */
  BMP280TaskHandle = osThreadNew(StartTaskReadBMP280, NULL, &BMP280Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T4_CC4;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 4;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_144CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = 3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_VREFINT;
  sConfig.Rank = 4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief DAC Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC_Init(void)
{

  /* USER CODE BEGIN DAC_Init 0 */

  /* USER CODE END DAC_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC_Init 1 */

  /* USER CODE END DAC_Init 1 */

  /** DAC Initialization
  */
  hdac.Instance = DAC;
  if (HAL_DAC_Init(&hdac) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT1 config
  */
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC_Init 2 */

  /* USER CODE END DAC_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 400000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
  hiwdg.Init.Reload = 4095;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */

}

/**
  * @brief RNG Initialization Function
  * @param None
  * @retval None
  */
static void MX_RNG_Init(void)
{

  /* USER CODE BEGIN RNG_Init 0 */

  /* USER CODE END RNG_Init 0 */

  /* USER CODE BEGIN RNG_Init 1 */

  /* USER CODE END RNG_Init 1 */
  hrng.Instance = RNG;
  if (HAL_RNG_Init(&hrng) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RNG_Init 2 */

  /* USER CODE END RNG_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 41999;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 500;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV2;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TOGGLE;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 1000000;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  * Configure DMA for memory to memory transfers
  *   hdma_memtomem_dma2_stream1
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* Configure DMA request hdma_memtomem_dma2_stream1 on DMA2_Stream1 */
  hdma_memtomem_dma2_stream1.Instance = DMA2_Stream1;
  hdma_memtomem_dma2_stream1.Init.Channel = DMA_CHANNEL_0;
  hdma_memtomem_dma2_stream1.Init.Direction = DMA_MEMORY_TO_MEMORY;
  hdma_memtomem_dma2_stream1.Init.PeriphInc = DMA_PINC_ENABLE;
  hdma_memtomem_dma2_stream1.Init.MemInc = DMA_MINC_ENABLE;
  hdma_memtomem_dma2_stream1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdma_memtomem_dma2_stream1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  hdma_memtomem_dma2_stream1.Init.Mode = DMA_NORMAL;
  hdma_memtomem_dma2_stream1.Init.Priority = DMA_PRIORITY_LOW;
  hdma_memtomem_dma2_stream1.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
  hdma_memtomem_dma2_stream1.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
  hdma_memtomem_dma2_stream1.Init.MemBurst = DMA_MBURST_SINGLE;
  hdma_memtomem_dma2_stream1.Init.PeriphBurst = DMA_PBURST_SINGLE;
  if (HAL_DMA_Init(&hdma_memtomem_dma2_stream1) != HAL_OK)
  {
    Error_Handler( );
  }

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, LCD_CS_Pin|LCD_RES_Pin|LCD_RS_Pin|LCD_CLK_Pin
                          |LCD_DAT_Pin|LED1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PE2 PE3 PE4 PE5
                           PE6 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PE8 PE10 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_CS_Pin LCD_RS_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin|LCD_RS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_RES_Pin LED1_Pin */
  GPIO_InitStruct.Pin = LCD_RES_Pin|LED1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_CLK_Pin LCD_DAT_Pin */
  GPIO_InitStruct.Pin = LCD_CLK_Pin|LCD_DAT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : BT_U_Pin BT_SET_Pin BT_D_Pin BT_R_Pin
                           BT_M_Pin BT_L_Pin */
  GPIO_InitStruct.Pin = BT_U_Pin|BT_SET_Pin|BT_D_Pin|BT_R_Pin
                          |BT_M_Pin|BT_L_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartTaskKeyRead */
/**
  * @brief  Опрос клавиатуры и выполнение команд.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartTaskKeyRead */
void StartTaskKeyRead(void *argument)
{
  /* USER CODE BEGIN 5 */
	/* Infinite loop */
	for (;;) {
		uint16_t kb = buttons_read();
		buttons_test_msg(kb);

		if (kb & KB_UP) {
			printf("Scanning I2C buses\r\n");
			scanI2C();
		}
		if (kb & KB_DOWN) {
			printf("Lcd test\r\n");
			lcd_test();
		}
		if (kb & KB_RIGHT) {
			_PRNFAST("Min free stack:\r\n");
			printf("BMP280TaskHandle   %d\r\n", (uint16_t)uxTaskGetStackHighWaterMark(BMP280TaskHandle));
			printf("ADC1ReadTaskHandle %d\r\n", (uint16_t)uxTaskGetStackHighWaterMark(ADC1ReadTaskHandle));
			printf("uartReadTaskHandle %d\r\n", (uint16_t)uxTaskGetStackHighWaterMark(uartReadTaskHandle));
			printf("keyReadTaskHandle  %d\r\n", (uint16_t)uxTaskGetStackHighWaterMark(keyReadTaskHandle));
		}
#ifdef INC_BMP280_H_
		if (kb & KB_MID) {
			bmp280_Read_All();
		}
#endif
		osDelay(20 / portTICK_RATE_MS);
	}
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartTaskUartRead */
/**
* @brief Обрабатываем команды принятые по uart.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskUartRead */
void StartTaskUartRead(void *argument)
{
  /* USER CODE BEGIN StartTaskUartRead */
	/* Infinite loop */
	stParseFunc_t stParseFunc = {
		.parseChr = command_chr,
		.parseStr = command_str,
		.parseBin = command_bin,
	};

	for (;;)
	{
		if (dataRxIsReady() == 1) {
			dataParse(&stParseFunc);
		}
		osDelay(pdMS_TO_TICKS(5));
	}
  /* USER CODE END StartTaskUartRead */
}

/* USER CODE BEGIN Header_StartTaskADC */
/**
* @brief Function implementing the myTaskADC1 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskADC */
void StartTaskADC(void *argument)
{
  /* USER CODE BEGIN StartTaskADC */
	/* Infinite loop */
	for (;;) {
		osSemaphoreAcquire(myBinarySemAdcReadyHandle, osWaitForever);
		HAL_IWDG_Refresh(&hiwdg);

		float vdd = get_vdd_from_adc(adc[3]);

		printf("ADC: A0:%4d, A1:%4d, Tint=%4d, vdd=%4d (%.3f V)\r\n",
				adc[0], adc[1], adc[2], adc[3], vdd);

		// LCD print
		color_ink = BLACK;
		color_back = WHITE;
		sprintf(buffLcd, " Vdd:  %.3f V\n ADC1: %4d\n ADC2: %4d", vdd, adc[0], adc[1]);
		lcd_print_str(buffLcd, 0, 0, 1, 1);

		// osDelay(pdMS_TO_TICKS(1000));
	}
  /* USER CODE END StartTaskADC */
}

/* USER CODE BEGIN Header_StartTaskReadBMP280 */
/**
* @brief Function implementing the BMP280Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskReadBMP280 */
void StartTaskReadBMP280(void *argument)
{
  /* USER CODE BEGIN StartTaskReadBMP280 */
	/* Infinite loop */
	TickType_t tcnt = xTaskGetTickCount();
	for (;;)
	{
		LED_Toggle;
		read_bmp280_ath25();
		HAL_IWDG_Refresh(&hiwdg);
		vTaskDelayUntil( &tcnt, pdMS_TO_TICKS(3000) ); // более точный период
	}
  /* USER CODE END StartTaskReadBMP280 */
}

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
