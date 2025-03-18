/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

extern CRC_HandleTypeDef hcrc;

extern DAC_HandleTypeDef hdac;

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;

extern RNG_HandleTypeDef hrng;

extern TIM_HandleTypeDef htim4;

extern UART_HandleTypeDef huart2;

extern DMA_HandleTypeDef hdma_memtomem_dma2_stream1;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define LED_Port 	LED1_GPIO_Port
#define LED_Pin 	LED1_Pin
#define LED_On		HAL_GPIO_WritePin(LED_Port, LED_Pin, GPIO_PIN_SET);   // GPIO_PIN_RESET
#define LED_Off		HAL_GPIO_WritePin(LED_Port, LED_Pin, GPIO_PIN_RESET); // GPIO_PIN_SET
#define LED_Toggle	HAL_GPIO_TogglePin(LED_Port, LED_Pin);

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LCD_CS_Pin GPIO_PIN_11
#define LCD_CS_GPIO_Port GPIOE
#define LCD_RES_Pin GPIO_PIN_12
#define LCD_RES_GPIO_Port GPIOE
#define LCD_RS_Pin GPIO_PIN_13
#define LCD_RS_GPIO_Port GPIOE
#define LCD_CLK_Pin GPIO_PIN_14
#define LCD_CLK_GPIO_Port GPIOE
#define LCD_DAT_Pin GPIO_PIN_15
#define LCD_DAT_GPIO_Port GPIOE
#define BT_U_Pin GPIO_PIN_10
#define BT_U_GPIO_Port GPIOD
#define BT_SET_Pin GPIO_PIN_11
#define BT_SET_GPIO_Port GPIOD
#define BT_D_Pin GPIO_PIN_12
#define BT_D_GPIO_Port GPIOD
#define BT_R_Pin GPIO_PIN_13
#define BT_R_GPIO_Port GPIOD
#define BT_M_Pin GPIO_PIN_14
#define BT_M_GPIO_Port GPIOD
#define BT_L_Pin GPIO_PIN_15
#define BT_L_GPIO_Port GPIOD
#define LED1_Pin GPIO_PIN_0
#define LED1_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
