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

#define LONG_TIME 0xffff

/* Private variables ---------------------------------------------------------*/
uint32_t g_ADCValue;
ADC_HandleTypeDef g_AdcHandle;
SemaphoreHandle_t xSemaphore = NULL;
DMA_HandleTypeDef  g_DmaHandle;

enum{ ADC_BUFFER_LENGTH = 128 };
uint32_t g_ADCBuffer[ADC_BUFFER_LENGTH];


/* Private function prototypes -----------------------------------------------*/
static void ADC_configureADC();
static void ADC_configureDMA();
static void adc_task( void *pvParameters);

//extern void Error_Handler(void);

void adc_create_task(void)
{
	xTaskCreate(adc_task, "adc_task", configMINIMAL_STACK_SIZE, NULL, 1, ( TaskHandle_t * )NULL);
}

static void ADC_configureADC()
{
    GPIO_InitTypeDef gpioInit;

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
    g_AdcHandle.Init.ContinuousConvMode = ENABLE;
    g_AdcHandle.Init.DiscontinuousConvMode = DISABLE;
    g_AdcHandle.Init.NbrOfDiscConversion = 0;
    g_AdcHandle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    g_AdcHandle.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1;
    g_AdcHandle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    g_AdcHandle.Init.NbrOfConversion = 1;
    g_AdcHandle.Init.DMAContinuousRequests = ENABLE;
    g_AdcHandle.Init.EOCSelection = DISABLE;

    HAL_ADC_Init(&g_AdcHandle);


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
	int g_MeasurementNumber;

	Socket_t xSocket;
	struct freertos_sockaddr xDestinationAddress;
	struct freertos_sockaddr xLocalAddress;
	BaseType_t xSendTimeOut;
	uint8_t *pucUDPPayloadBuffer;
	TickType_t xBlockingTime = pdMS_TO_TICKS( 1000ul );
	int32_t iReturned;

	//TEMP
	uint8_t cString[ 50 ];
	uint32_t ulCount = 0UL;
	//END TEMP

	xSemaphore = xSemaphoreCreateBinary();
	ADC_configureADC();


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

	xLocalAddress.sin_port = FreeRTOS_htons( 9991 );
	xLocalAddress.sin_addr = FreeRTOS_GetIPAddress();

	FreeRTOS_bind( xSocket, &xLocalAddress, sizeof( xLocalAddress ) );

	xSendTimeOut = xBlockingTime;
	FreeRTOS_setsockopt( xSocket, 0, FREERTOS_SO_SNDTIMEO, &xSendTimeOut, sizeof( xSendTimeOut ) );


	//Polling
//    HAL_ADC_Start(&g_AdcHandle);
//    for (;;)
//    {
//        if (HAL_ADC_PollForConversion(&g_AdcHandle, 1000000) == HAL_OK)
//        {
//            g_ADCValue = HAL_ADC_GetValue(&g_AdcHandle);
//            g_MeasurementNumber++;
//        }
//        vTaskDelay(500);
//    }

	//interrupt driven
//	HAL_ADC_Start_IT(&g_AdcHandle);

	//DMA Driven
	ADC_configureDMA();
	HAL_ADC_Start_DMA(&g_AdcHandle, g_ADCBuffer, ADC_BUFFER_LENGTH);

	while(1){
		xSemaphoreTake( xSemaphore, portMAX_DELAY);
		//buffer full transmit
//		iReturned = FreeRTOS_sendto( xSocket, ( void * )&g_ADCBuffer, sizeof( g_ADCBuffer ), 0, &xDestinationAddress, sizeof( xDestinationAddress ) );
//
//		        if( iReturned != 0 )
//		        {
//		            /* The buffer pointed to by pucUDPPayloadBuffer was successfully
//		            passed (by reference) into the IP stack and is now queued for sending.
//		            The IP stack is responsible for returning the buffer after it has been
//		            sent, and pucUDPPayloadBuffer can be used safely in another call to
//		            FreeRTOS_GetUDPPayloadBuffer(). */
//		        	asm("nop");
//		        }
//		        else
//		        {
//		            /* The buffer pointed to by pucUDPPayloadBuffer was not successfully
//		            passed (by reference) to the IP stack.  To prevent memory and network
//		            buffer leaks the buffer must be either reused or, as in this case,
//		            released back to the IP stack. */
//		        	asm("nop");
//		        }


	    /* Obtain a buffer from the TCP/IP stack that is large enough to hold the data
	    being sent.  Although the maximum amount of time to wait for a buffer is passed
	    into FreeRTOS_GetUDPPayloadBuffer() as portMAX_DELAY, the actual maximum time
	    will be capped to ipconfigMAX_SEND_BLOCK_TIME_TICKS (defined in
	    FreeRTOSIPConfig.h) */

	    pucUDPPayloadBuffer = ( uint8_t * ) FreeRTOS_GetUDPPayloadBuffer( sizeof(g_ADCBuffer),
	                                                                      portMAX_DELAY );

	    if (pucUDPPayloadBuffer !=NULL)
	    {
	    	memcpy( pucUDPPayloadBuffer, g_ADCBuffer, sizeof(g_ADCBuffer) );

	    	iReturned = FreeRTOS_sendto( xSocket,
	    			( void * )&pucUDPPayloadBuffer,
					sizeof( pucUDPPayloadBuffer ),
					0,
					&xDestinationAddress,
					sizeof( xDestinationAddress ) );

		}
        if( iReturned != 0 )
        {
            /* The buffer pointed to by pucUDPPayloadBuffer was successfully
            passed (by reference) into the IP stack and is now queued for sending.
            The IP stack is responsible for returning the buffer after it has been
            sent, and pucUDPPayloadBuffer can be used safely in another call to
            FreeRTOS_GetUDPPayloadBuffer(). */
        }
        else
        {
            /* The buffer pointed to by pucUDPPayloadBuffer was not successfully
            passed (by reference) to the IP stack.  To prevent memory and network
            buffer leaks the buffer must be either reused or, as in this case,
            released back to the IP stack. */
            FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) pucUDPPayloadBuffer );
        }

//	       /* Create the string that is sent. */
//	       sprintf( cString,
//	                "Standard send message number %lu\r\n",
//	                ulCount );
//
//	       /* Send the string to the UDP socket.  ulFlags is set to 0, so the standard
//	       semantics are used.  That means the data from cString[] is copied
//	       into a network buffer inside FreeRTOS_sendto(), and cString[] can be
//	       reused as soon as FreeRTOS_sendto() has returned. */
//	       FreeRTOS_sendto( xSocket,
//	                        cString,
//	                        strlen( cString ),
//	                        0,
//	                        &xDestinationAddress,
//	                        sizeof( xDestinationAddress ) );
//
//	       ulCount++;


        vTaskDelay(pdMS_TO_TICKS(1000UL));
	}
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* AdcHandle)
{
    //g_ADCValue = HAL_ADC_GetValue(AdcHandle);

}

void DMA2_Stream4_IRQHandler()
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    HAL_DMA_IRQHandler(&g_DmaHandle);
	xSemaphoreGiveFromISR( xSemaphore, &xHigherPriorityTaskWoken );
	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

void ADC_IRQHandler()
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	HAL_ADC_IRQHandler(&g_AdcHandle);
	xSemaphoreGiveFromISR( xSemaphore, &xHigherPriorityTaskWoken );
	//portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
