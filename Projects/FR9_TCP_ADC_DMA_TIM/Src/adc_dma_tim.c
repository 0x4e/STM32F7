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
#include "semphr.h"
#include "UDPLoggingPrintf.h"

#include "SEGGER_SYSVIEW.h"
#include "SEGGER_SYSVIEW_FreeRTOS.h"



/* Private variables ---------------------------------------------------------*/
uint32_t g_ADCValue;
ADC_HandleTypeDef g_AdcHandle;
ADC_HandleTypeDef g_AdcHandle2;
SemaphoreHandle_t xSemaphore = NULL;
DMA_HandleTypeDef  g_DmaHandle;
TIM_HandleTypeDef htim4;

#define ADC_BUFFER_LENGTH 1024 //NOTE - for simplicity the buffer should be a power of 2. Buffer is 32 bit
#define MAX_TX_PACKETSIZE 1024
#define HALF_BUFFER (ADC_BUFFER_LENGTH/2)
#define QUATER_PACKET (MAX_TX_PACKETSIZE/4)

uint32_t g_ADCBuffer[ADC_BUFFER_LENGTH];

enum DMA_BUFF_STATUS {
	DMA_BUFF_HALF,
	DMA_BUFF_FULL
};

enum DMA_BUFF_STATUS Buff_status;


/* Private function prototypes -----------------------------------------------*/
static void ADC_configureADC();
static void ADC_configureDMA();
static void MX_TIM4_Init(void);
static void adc_task( void *pvParameters);

extern void Error_Handler(void);


void adc_create_task(void)
{
	xTaskCreate(adc_task, "adc_task", configMINIMAL_STACK_SIZE*2, NULL, 1, ( TaskHandle_t * )NULL);
}

static void ADC_configureADC()
{
    GPIO_InitTypeDef gpioInit;
    ADC_MultiModeTypeDef multimode;

    __GPIOC_CLK_ENABLE();
    __GPIOA_CLK_ENABLE();
    __ADC1_CLK_ENABLE();

    /**ADC1 GPIO Configuration
    PC2     ------> ADC1_IN12
    PA4     ------> ADC1_IN4
    */
    gpioInit.Pin = ARD_A2_Pin;
    gpioInit.Mode = GPIO_MODE_ANALOG;
    gpioInit.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ARD_A2_GPIO_Port, &gpioInit);

    gpioInit.Pin = ARD_A1_Pin;
    gpioInit.Mode = GPIO_MODE_ANALOG;
    gpioInit.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ARD_A1_GPIO_Port, &gpioInit);


    HAL_NVIC_SetPriority(ADC_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(ADC_IRQn);

    ADC_ChannelConfTypeDef adcChannel;

    g_AdcHandle.Instance = ADC1;
    g_AdcHandle.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV2;
    g_AdcHandle.Init.Resolution = ADC_RESOLUTION_12B;
    g_AdcHandle.Init.ScanConvMode = DISABLE;
    g_AdcHandle.Init.ContinuousConvMode = DISABLE;
    g_AdcHandle.Init.DiscontinuousConvMode = DISABLE;
    g_AdcHandle.Init.NbrOfDiscConversion = 0;
    g_AdcHandle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
    g_AdcHandle.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T4_TRGO;
    g_AdcHandle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    g_AdcHandle.Init.NbrOfConversion = 1;
    g_AdcHandle.Init.DMAContinuousRequests = ENABLE;
    g_AdcHandle.Init.EOCSelection = DISABLE;
    HAL_ADC_Init(&g_AdcHandle);

    /**Configure the ADC multi-mode
    */
	multimode.Mode = ADC_DUALMODE_REGSIMULT;
	multimode.DMAAccessMode = ADC_DMAACCESSMODE_2;
	multimode.TwoSamplingDelay = ADC_TWOSAMPLINGDELAY_5CYCLES;
	if (HAL_ADCEx_MultiModeConfigChannel(&g_AdcHandle, &multimode) != HAL_OK)
	{
	Error_Handler();
	}

    /**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
    */
    adcChannel.Channel = ADC_CHANNEL_4;
    adcChannel.Rank = 1;
    adcChannel.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    adcChannel.Offset = 0;

    if (HAL_ADC_ConfigChannel(&g_AdcHandle, &adcChannel) != HAL_OK)
    {
        asm("bkpt 255");
    }


    //ADC 2

    ADC_ChannelConfTypeDef adcChannel2;

    /* Peripheral clock enable */
    __HAL_RCC_ADC2_CLK_ENABLE();

    /**ADC2 GPIO Configuration
    PA6     ------> ADC2_IN6
    */
    gpioInit.Pin = GPIO_PIN_6;
    gpioInit.Mode = GPIO_MODE_ANALOG;
    gpioInit.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpioInit);


    g_AdcHandle2.Instance = ADC2;
    g_AdcHandle2.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV2;
    g_AdcHandle2.Init.Resolution = ADC_RESOLUTION_12B;
    g_AdcHandle2.Init.ScanConvMode = DISABLE;
    g_AdcHandle2.Init.ContinuousConvMode = DISABLE;
    g_AdcHandle2.Init.DiscontinuousConvMode = DISABLE;
    g_AdcHandle2.Init.NbrOfDiscConversion = 0;
    g_AdcHandle2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    g_AdcHandle2.Init.NbrOfConversion = 1;
    g_AdcHandle2.Init.DMAContinuousRequests = ENABLE;
    g_AdcHandle2.Init.EOCSelection = DISABLE;
    HAL_ADC_Init(&g_AdcHandle2);


    /**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
    */
    adcChannel2.Channel = ADC_CHANNEL_6;
    adcChannel2.Rank = 1;
    adcChannel2.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    adcChannel2.Offset = 0;

    if (HAL_ADC_ConfigChannel(&g_AdcHandle2, &adcChannel2) != HAL_OK)
    {
        asm("bkpt 255");
    }

}


static void ADC_configureDMA()
{
    __DMA2_CLK_ENABLE();
    g_DmaHandle.Instance = DMA2_Stream4;

    g_DmaHandle.Init.Channel  = DMA_CHANNEL_0;
    g_DmaHandle.Init.Direction = DMA_PERIPH_TO_MEMORY;
    g_DmaHandle.Init.PeriphInc = DMA_PINC_DISABLE;
    g_DmaHandle.Init.MemInc = DMA_MINC_ENABLE;
    g_DmaHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    g_DmaHandle.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    g_DmaHandle.Init.Mode = DMA_CIRCULAR;
    g_DmaHandle.Init.Priority = DMA_PRIORITY_HIGH;
    g_DmaHandle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    g_DmaHandle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
    g_DmaHandle.Init.MemBurst = DMA_MBURST_SINGLE;
    g_DmaHandle.Init.PeriphBurst = DMA_PBURST_SINGLE;

    HAL_DMA_Init(&g_DmaHandle);

    __HAL_LINKDMA(&g_AdcHandle, DMA_Handle, g_DmaHandle);

    HAL_NVIC_SetPriority(DMA2_Stream4_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream4_IRQn);
}


static void adc_task( void *pvParameters)
{
	/*just to remove compiler warning*/
	(void) pvParameters;

	Socket_t xSocket;
	struct freertos_sockaddr xDestinationAddress;
	struct freertos_sockaddr xLocalAddress;
	BaseType_t xSendTimeOut;
	uint8_t *pucUDPPayloadBuffer;
	TickType_t xBlockingTime = pdMS_TO_TICKS( 1000ul );
	int32_t iReturned;
	Buff_status = DMA_BUFF_HALF;
	uint8_t  numberofpackets;
	int i = 0;

	xSemaphore = xSemaphoreCreateBinary();

	SEGGER_SYSVIEW_Print("ADC Config");

	ADC_configureADC();

	MX_TIM4_Init(); // Config timer


	 /* Fill in the destination address and port number, which in this case is
	    port 10000 on IP address 192.168.1.110. */

	/* Use a fixed address to where the logging will be sent. */
    xDestinationAddress.sin_addr = FreeRTOS_inet_addr_quick( configUDP_LOGGING_ADDR0,
														configUDP_LOGGING_ADDR1,
														configUDP_LOGGING_ADDR2,
														configUDP_LOGGING_ADDR3 );

    xDestinationAddress.sin_port = FreeRTOS_htons( 9990 );


    /* Create the socket. */
    do
    {
    	vTaskDelay( xBlockingTime );
    	xSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP );
    } while( xSocket == FREERTOS_INVALID_SOCKET );


    SEGGER_SYSVIEW_Print("ADC Socket");

	xLocalAddress.sin_port = FreeRTOS_htons( 9991 );
	xLocalAddress.sin_addr = FreeRTOS_GetIPAddress();

	FreeRTOS_bind( xSocket, &xLocalAddress, sizeof( xLocalAddress ) );

	xSendTimeOut = xBlockingTime;
	FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_SNDTIMEO, &xSendTimeOut, sizeof( xSendTimeOut ) );



	//DMA Driven
	ADC_configureDMA();
	HAL_ADC_Start(&g_AdcHandle2); // might not be required
	HAL_ADCEx_MultiModeStart_DMA(&g_AdcHandle, g_ADCBuffer, ADC_BUFFER_LENGTH);

	while(1){
		xSemaphoreTake( xSemaphore, portMAX_DELAY);
		SEGGER_SYSVIEW_Print("ADC Semaphore taken");


		// Send the DMA buffer across multiple packets
		// UDP Payload cannot exceed Maximum MTU packet size (ipconfigNETWORK_MTU 1500)
		// For 2 channels a single uint32 is for each sample.

		// calculate iterations of packet
		numberofpackets = (ADC_BUFFER_LENGTH * sizeof(uint32_t) / 2)/MAX_TX_PACKETSIZE;

		for(i =0; i< numberofpackets; i++ ){

			/* Obtain a buffer from the TCP/IP stack that is large enough to hold the data
			being sent.  Although the maximum amount of time to wait for a buffer is passed
			into FreeRTOS_GetUDPPayloadBuffer() as portMAX_DELAY, the actual maximum time
			will be capped to ipconfigMAX_SEND_BLOCK_TIME_TICKS (defined in
			FreeRTOSIPConfig.h) */

			pucUDPPayloadBuffer = ( uint8_t * ) FreeRTOS_GetUDPPayloadBuffer( MAX_TX_PACKETSIZE,
																			  portMAX_DELAY );

			if (pucUDPPayloadBuffer !=NULL)
			{
				//Transmit from recently filled buffer
				if (Buff_status == DMA_BUFF_HALF){
					memcpy( pucUDPPayloadBuffer, g_ADCBuffer+(i*QUATER_PACKET), MAX_TX_PACKETSIZE);
				}
				else {
					memcpy( pucUDPPayloadBuffer, g_ADCBuffer+HALF_BUFFER+(i*QUATER_PACKET), MAX_TX_PACKETSIZE);
				}

				iReturned = FreeRTOS_sendto( xSocket,
						( const void * )pucUDPPayloadBuffer,
						//sizeof( pucUDPPayloadBuffer ),
						MAX_TX_PACKETSIZE,
						FREERTOS_ZERO_COPY,
						&xDestinationAddress,
						sizeof( xDestinationAddress ) );

			}
			if( iReturned == 0 )
			{
				/* The buffer pointed to by pucUDPPayloadBuffer was successfully
				passed (by reference) into the IP stack and is now queued for sending.
				The IP stack is responsible for returning the buffer after it has been
				sent, and pucUDPPayloadBuffer can be used safely in another call to
				FreeRTOS_GetUDPPayloadBuffer(). */
				//SEGGER_SYSVIEW_Print("buffer good");
				FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) pucUDPPayloadBuffer );
			}
			else
			{
				/* The buffer pointed to by pucUDPPayloadBuffer was not successfully
				passed (by reference) to the IP stack.  To prevent memory and network
				buffer leaks the buffer must be either reused or, as in this case,
				released back to the IP stack. */
				//FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) pucUDPPayloadBuffer );
				//SEGGER_SYSVIEW_Print("buffer fail");
			}
		}

	}
}



/* TIM4 init function */
static void MX_TIM4_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  __TIM4_CLK_ENABLE();
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 0;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 30000;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  //start timer
  HAL_TIM_Base_Start( &htim4 );

}


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* AdcHandle)
{

	Buff_status = DMA_BUFF_HALF;
}


void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* AdcHandle)
{
	Buff_status = DMA_BUFF_FULL;
}



void DMA2_Stream4_IRQHandler()
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	traceISR_ENTER();
    HAL_DMA_IRQHandler(&g_DmaHandle);
	xSemaphoreGiveFromISR( xSemaphore, &xHigherPriorityTaskWoken );
	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

void ADC_IRQHandler()
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	traceISR_ENTER();
	HAL_ADC_IRQHandler(&g_AdcHandle);
	xSemaphoreGiveFromISR( xSemaphore, &xHigherPriorityTaskWoken );
	//portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
