/*
 * coms_packet.c
 *
 *  Created on: 23 Oct 2017
 *      Author: Nick
 */

#include "coms_packet.h"
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>


#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"


#include "SEGGER_SYSVIEW.h"
#include "SEGGER_SYSVIEW_FreeRTOS.h"

#define PACKET_MAX_PL_LEN		1024

/* Coms port */
#define COMS_PORT_NUMBER 		7001

#define READ_IO32( addr ) (*(volatile int*)(addr))

//#define COMM_FW_VERSION 0x1
#define FW_VERSION_MAJOR 0x2
#define FW_VERSION_MINOR 0x3


// Private variables
static uint8_t send_buffer[PACKET_MAX_PL_LEN];
static uint8_t RxBuffer[PACKET_MAX_PL_LEN];
static uint16_t RxState = 0;
static uint16_t RxTimer = 0;
//static SemaphoreHandle_t xComs_TX_Semaphore = NULL;


static void coms_packet_tx(Socket_t xSocket, struct freertos_sockaddr *pxClient, uint16_t data_length);
static void coms_packet_process_rx(uint8_t *data, int32_t len, Socket_t xSocket, struct freertos_sockaddr *pxClient);
static void coms_packet_rx_task( void *pvParameters );
static void coms_packet_timeout_task( void *pvParameters );
static uint32_t buffer_get_uint32(const uint8_t *buffer, int32_t *index);
static void buffer_append_uint32(uint8_t* buffer, uint32_t number, int32_t *index);

void coms_packet_create_task(void)
{
	xTaskCreate(coms_packet_rx_task, "coms_rx_task", configMINIMAL_STACK_SIZE*2, NULL, 1, ( TaskHandle_t * )NULL);
	xTaskCreate(coms_packet_timeout_task, "coms_time_out", configMINIMAL_STACK_SIZE, NULL, 1, ( TaskHandle_t * )NULL);
}

/**
 * Send a packet
 *
 *
 */
static void coms_packet_tx(Socket_t xSocket, struct freertos_sockaddr *pxClient, uint16_t data_length)
{
uint8_t *pucUDPPayloadBuffer = NULL;
int32_t lReturned;

	/* This task is going to send using the zero copy interface.  The data
	being sent is therefore written directly into a buffer that is passed
	into, rather than copied into, the FreeRTOS_sendto() function.  First
	obtain a buffer of adequate length from the IP stack into which the
	error packet will be written.  Although a max delay is used, the actual
	delay will be capped to ipconfigMAX_SEND_BLOCK_TIME_TICKS. */
	pucUDPPayloadBuffer = ( uint8_t * ) FreeRTOS_GetUDPPayloadBuffer( data_length, portMAX_DELAY );

	if( pucUDPPayloadBuffer != NULL )
	{

		memcpy( pucUDPPayloadBuffer, send_buffer, data_length); // THIS SEEMS REDUNDANT. WHY COPY WHEN ITS SUPPOSED TO BE "ZERO COPY"??
		/* Pass the buffer into the send function.  ulFlags has the
		FREERTOS_ZERO_COPY bit set so the IP stack will take control of the
		buffer rather than copy data out of the buffer. */
		lReturned = FreeRTOS_sendto( xSocket,  						/* The socket to which the error frame is sent. */
									( const void * )pucUDPPayloadBuffer, /* A pointer to the the data being sent. */
									data_length, 					/* The length of the data being sent. */
									FREERTOS_ZERO_COPY, 			/* ulFlags with the FREERTOS_ZERO_COPY bit set. */
									pxClient, 			/* Where the data is being sent. */
									sizeof( *pxClient ) );

		if( lReturned == 0 )
		{
			/* The send operation failed, so this task is still responsible
			for the buffer obtained from the IP stack.  To ensure the buffer
			is not lost it must either be used again, or, as in this case,
			returned to the IP stack using FreeRTOS_ReleaseUDPPayloadBuffer(). */
			FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) pucUDPPayloadBuffer );
		}
		else
		{
			/* The send was successful so the IP stack is now managing the
			buffer pointed to by pucUDPPayloadBuffer, and the IP stack will
			return the buffer once it has been sent. */
		}
	}
}

//move to helper function
static uint32_t buffer_get_uint32(const uint8_t *buffer, int32_t *index) {
	uint32_t res =	((uint32_t) buffer[*index]) << 24 |
					((uint32_t) buffer[*index + 1]) << 16 |
					((uint32_t) buffer[*index + 2]) << 8 |
					((uint32_t) buffer[*index + 3]);
	*index += 4;
	return res;
}

static void buffer_append_uint32(uint8_t* buffer, uint32_t number, int32_t *index) {
	buffer[(*index)++] = number >> 24;
	buffer[(*index)++] = number >> 16;
	buffer[(*index)++] = number >> 8;
	buffer[(*index)++] = number;
}


/**
 * Process a received buffer with commands and data.
 *
 * @param data
 * The buffer to process.
 *
 * @param len
 * The length of the buffer.
 */
static void coms_packet_process_rx(uint8_t *data, int32_t len, Socket_t xSocket, struct freertos_sockaddr *pxClient){

	COMM_PACKET_ID packet_id;
	int32_t ind = 0;
	volatile uint32_t address_32 = 0;
	uint32_t result = 0;

	packet_id = data[0];
	data++;
	len--;

	switch (packet_id) {
	case COMM_FW_VERSION:
		ind = 0;
		send_buffer[ind++] = COMM_FW_VERSION;
		send_buffer[ind++] = FW_VERSION_MAJOR;
		send_buffer[ind++] = FW_VERSION_MINOR;
		coms_packet_tx(xSocket,pxClient, ind);
		break;

	case COMM_READ_REG32:
		ind = 0;
		address_32 = buffer_get_uint32(data, &ind);

		//read register
		result = READ_IO32(address_32);

		ind = 0;
		send_buffer[ind++] = COMM_READ_REG32;

		buffer_append_uint32(send_buffer, result, &ind);
		coms_packet_tx(xSocket,pxClient,ind);
		break;


	default:
		break;
	}
}


static void coms_packet_rx_task( void *pvParameters )
{
	int32_t lBytes;
	uint8_t *pucUDPPayloadBuffer;
	struct freertos_sockaddr xClient, xBindAddress;
	uint32_t xClientLength = sizeof( xClient ), ulIPAddress;
	Socket_t xComPortListeningSocket;


	uint32_t PayloadLength;
	uint32_t RxDataPtr;

	/*just to remove compiler warning*/
	(void) pvParameters;

    SEGGER_SYSVIEW_Print("Setup RX Task");

	/* Attempt to open the socket.  The receive block time defaults to the max
	delay, so there is no need to set that separately. */
	xComPortListeningSocket = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, FREERTOS_IPPROTO_UDP );
	configASSERT( xComPortListeningSocket != FREERTOS_INVALID_SOCKET );

	/* Bind to the standard TFTP port. */
	FreeRTOS_GetAddressConfiguration( &ulIPAddress, NULL, NULL, NULL );
	xBindAddress.sin_addr = FreeRTOS_inet_addr_quick( configUDP_LOGGING_ADDR0,
			configUDP_LOGGING_ADDR1,
			configUDP_LOGGING_ADDR2,
			configUDP_LOGGING_ADDR3 );
	xBindAddress.sin_port = FreeRTOS_htons( COMS_PORT_NUMBER );
	FreeRTOS_bind( xComPortListeningSocket, &xBindAddress, sizeof( xBindAddress ) );

   for( ;; )
   {
       /* Receive data from the socket.  ulFlags has the zero copy bit set
       (FREERTOS_ZERO_COPY) indicating to the stack that a reference to the
       received data should be passed out to this RTOS task using the second
       parameter to the FreeRTOS_recvfrom() call.  When this is done the
       IP stack is no longer responsible for releasing the buffer, and
       the RTOS task must return the buffer to the stack when it is no longer
       needed.  By default the block time is portMAX_DELAY but it can be
       changed using FreeRTOS_setsockopt(). */
       lBytes = FreeRTOS_recvfrom(
    		   xComPortListeningSocket,
    		   ( void * ) &pucUDPPayloadBuffer,
			   0,
			   FREERTOS_ZERO_COPY,
			   &xClient,
			   &xClientLength);

       if( lBytes > 0 )
       {
           /* Data was received and can be processed here. */
    	   SEGGER_SYSVIEW_Print("got some data COMS");

    		unsigned char rx_data;
    		const int rx_timeout = 50;

    		for (int i = 0; i < lBytes; i++){

    			rx_data = pucUDPPayloadBuffer[i];

    			switch(RxState){
    			case 0:
    				if (rx_data == 2){

    				}
    				else {
    					RxState = 0;
    				}
    				break;
    			case 1:
    	            PayloadLength = (unsigned int)rx_data << 8;
    	            RxState++;
    	            RxTimer = rx_timeout;
    	            RxDataPtr = 0;
    	            PayloadLength = 0;
    	            break;
    			}
    	        case 2:
    	            PayloadLength |= (unsigned int)rx_data;
    	            if (PayloadLength <= PACKET_MAX_PL_LEN && PayloadLength > 0) {
    	                RxState++;
    	                RxTimer = rx_timeout;
    	            } else {
    	                RxState = 0;
    	            }
    	            break;
    	        case 3:
    	            RxBuffer[RxDataPtr++] = rx_data;
    	            if (RxDataPtr == PayloadLength) {
    	                RxState++;
    	            }
    	            RxTimer = rx_timeout;
    	            break;
    	        case 4:
    	        	coms_packet_process_rx(RxBuffer, PayloadLength, xComPortListeningSocket, &xClient);
    	            RxState = 0;
    	            break;

    	        default:
    	            RxState = 0;
    	            break;
    		}

    		FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) pucUDPPayloadBuffer );
       }

   }
}

static void coms_packet_timeout_task( void *pvParameters )
{
	/*just to remove compiler warning*/
	(void) pvParameters;
	if (0 < RxTimer){
		RxTimer--;

	}
	else{
		RxState = 0;
		RxTimer = 0;
	}
	vTaskDelay(10);
}



