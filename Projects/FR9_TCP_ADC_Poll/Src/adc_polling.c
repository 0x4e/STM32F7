/*
 * adc_polling.c
 *
 *  Created on: 4 Jul 2017
 *      Author: nick
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"
#include "main.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "UDPLoggingPrintf.h"

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
uint32_t g_ADCValue;

/* Private function prototypes -----------------------------------------------*/
static void MX_ADC1_Init(void);
static void MX_ADC2_Init(void);
static void adc_task( void *pvParameters);


extern void Error_Handler(void);

/* ADC1 init function */
static void MX_ADC1_Init(void)
{

  //ADC_MultiModeTypeDef multimode;
  ADC_ChannelConfTypeDef sConfig;

    /**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
    */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

    /**Configure the ADC multi-mode
    */
  /*
  multimode.Mode = ADC_DUALMODE_REGSIMULT;
  multimode.DMAAccessMode = ADC_DMAACCESSMODE_DISABLED;
  multimode.TwoSamplingDelay = ADC_TWOSAMPLINGDELAY_5CYCLES;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }
  */

    /**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
    */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

}

/* ADC2 init function */
static void MX_ADC2_Init(void)
{

  ADC_ChannelConfTypeDef sConfig;

    /**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
    */
  hadc2.Instance = ADC2;
  hadc2.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc2.Init.Resolution = ADC_RESOLUTION_12B;
  hadc2.Init.ScanConvMode = DISABLE;
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = 1;
  hadc2.Init.DMAContinuousRequests = DISABLE;
  hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }

    /**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
    */
  sConfig.Channel = ADC_CHANNEL_6;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

}

void adc_init(void)
{
	MX_ADC1_Init();
	MX_ADC2_Init();
}

void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(hadc->Instance==ADC1)
  {
    /* Peripheral clock enable */
    __HAL_RCC_ADC1_CLK_ENABLE();

    /**ADC1 GPIO Configuration
    PC2     ------> ADC1_IN12
    PA4     ------> ADC1_IN4
    */
    GPIO_InitStruct.Pin = ARD_A2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ARD_A2_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = ARD_A1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ARD_A1_GPIO_Port, &GPIO_InitStruct);
  }
  else if(hadc->Instance==ADC2)
  {
    /* Peripheral clock enable */
    __HAL_RCC_ADC2_CLK_ENABLE();

    /**ADC2 GPIO Configuration
    PA6     ------> ADC2_IN6
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc)
{

  if(hadc->Instance==ADC1)
  {
    /* Peripheral clock disable */
    __HAL_RCC_ADC1_CLK_DISABLE();

    /**ADC1 GPIO Configuration
    PC2     ------> ADC1_IN12
    PA4     ------> ADC1_IN4
    */
    HAL_GPIO_DeInit(ARD_A2_GPIO_Port, ARD_A2_Pin);
    HAL_GPIO_DeInit(ARD_A1_GPIO_Port, ARD_A1_Pin);
  }
  else if(hadc->Instance==ADC2)
  {
    /* Peripheral clock disable */
    __HAL_RCC_ADC2_CLK_DISABLE();

    /**ADC2 GPIO Configuration
    PA6     ------> ADC2_IN6
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_6);
  }

}

void adc_create_task(void)
{
	xTaskCreate(adc_task, "adc_task", configMINIMAL_STACK_SIZE, NULL, 1, ( TaskHandle_t * )NULL);
}

static void adc_task( void *pvParameters)
{
	/*just to remove compiler warning*/
	(void) pvParameters;



	while(1){
		//read ADC
		if (HAL_ADC_Start(&hadc1) != HAL_OK)
		{
		/* Start Conversation Error */
		Error_Handler();
		}

		if (HAL_ADC_Start(&hadc2) != HAL_OK)
		{
		/* Start Conversation Error */
		Error_Handler();
		}

		if (HAL_ADC_PollForConversion(&hadc1, 10)== HAL_OK)
		{

			g_ADCValue = HAL_ADC_GetValue(&hadc1);
			FreeRTOS_printf( ( "ADC = %d\n", g_ADCValue) );
		}
		else
		{
			FreeRTOS_printf( ( "error ADC" ) );
		}
		vTaskDelay(500);

	}
}
