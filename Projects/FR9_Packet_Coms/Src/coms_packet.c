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
#define COMS_PORT_NUMBER 				( ( uint16_t ) 7001 )

#define COMM_FW_VERSION 0x1
#define FW_VERSION_MAJOR 0x2
#define FW_VERSION_MINOR 0x3


// Private variables
static uint8_t send_buffer[PACKET_MAX_PL_LEN];
//static SemaphoreHandle_t xComs_TX_Semaphore = NULL;

/* Standard TFTP opcodes. */
typedef enum
{
	eReadRequest = 1,
	eWriteRequest,
	eData,
	eAck,
	eError
} eTFTPOpcode_t;



/* The index for the error string below MUST match the value of the applicable
eTFTPErrorCode_t error code value. */
static const char *cErrorStrings[] =
{
	NULL, /* Not valid. */
	"File not found.",
	"Access violation.",
	"Disk full or allocation exceeded.",
	"Illegal TFTP operation.",
	"Unknown transfer ID.",
	"File already exists.",
	"No such user."
};

/* Error codes from the RFC. */
typedef enum
{
	eFileNotFound = 1,
	eAccessViolation,
	eDiskFull,
	eIllegalTFTPOperation,
	eUnknownTransferID,
	eFileAlreadyExists
} eTFTPErrorCode_t;


static void coms_packet_tx(Socket_t xSocket, struct freertos_sockaddr *pxClient, eTFTPErrorCode_t eErrorCode);
static void coms_packet_process_rx(uint8_t *data, int32_t len, Socket_t xSocket, struct freertos_sockaddr *pxClient);

static void coms_packet_rx_task( void *pvParameters );


void coms_packet_create_task(void)
{
	xTaskCreate(coms_packet_rx_task, "coms_rx_task", configMINIMAL_STACK_SIZE*2, NULL, 1, ( TaskHandle_t * )NULL);

}

/**
 * Send a packet
 *
 *
 */
static void coms_packet_tx(Socket_t xSocket, struct freertos_sockaddr *pxClient, eTFTPErrorCode_t eErrorCode)
{
uint8_t *pucUDPPayloadBuffer = NULL;
const size_t xFixedSizePart = ( size_t ) 5; /* 2 byte opcode, plus two byte error code, plus string terminating 0. */
const size_t xNumberOfErrorStrings = sizeof( cErrorStrings ) / sizeof( char * );
size_t xErrorCode = ( size_t ) eErrorCode, xTotalLength = 0; /* Only initialised to keep compiler quiet. */
const char *pcErrorString = NULL;
int32_t lReturned;

	/* The total size of the packet to be sent depends on the length of the
	error string. */
	if( xErrorCode < xNumberOfErrorStrings )
	{
		pcErrorString = cErrorStrings[ xErrorCode ];

		/* This task is going to send using the zero copy interface.  The data
		being sent is therefore written directly into a buffer that is passed
		into, rather than copied into, the FreeRTOS_sendto() function.  First
		obtain a buffer of adequate length from the IP stack into which the
		error packet will be written.  Although a max delay is used, the actual
		delay will be capped to ipconfigMAX_SEND_BLOCK_TIME_TICKS. */
		xTotalLength = strlen( pcErrorString ) + xFixedSizePart;
		pucUDPPayloadBuffer = ( uint8_t * ) FreeRTOS_GetUDPPayloadBuffer( xTotalLength, portMAX_DELAY );
	}

	if( pucUDPPayloadBuffer != NULL )
	{
		FreeRTOS_printf( ( "Error: %s\n", pcErrorString ) );

		/* Create error packet: Opcode. */
		pucUDPPayloadBuffer[ 0 ] = 0;
		pucUDPPayloadBuffer[ 1 ] = ( uint8_t ) eError;

		/* Create error packet: Error code. */
		pucUDPPayloadBuffer[ 2 ] = 0;
		pucUDPPayloadBuffer[ 3 ] = ( uint8_t ) eErrorCode;

		/* Create error packet: Error string. */
		strcpy( ( ( char * ) &( pucUDPPayloadBuffer[ 4 ] ) ), pcErrorString );

		/* Pass the buffer into the send function.  ulFlags has the
		FREERTOS_ZERO_COPY bit set so the IP stack will take control of the
		buffer rather than copy data out of the buffer. */
		lReturned = FreeRTOS_sendto( xSocket,  						/* The socket to which the error frame is sent. */
									( void * ) pucUDPPayloadBuffer, /* A pointer to the the data being sent. */
									xTotalLength, 					/* The length of the data being sent. */
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



/**
 * Process a received buffer with commands and data.
 *
 * @param data
 * The buffer to process.
 *
 * @param len
 * The length of the buffer.
 */
static void coms_packet_process_rx(uint8_t *data, int32_t len, Socket_t xSocket, struct freertos_sockaddr *pxClient) {
	if (!len) {
		return;
	}

	COMM_PACKET_ID packet_id;
	int32_t ind = 0;


	packet_id = data[0];
	data++;
	len--;

	switch (packet_id) {
	case COMM_FW_VERSION:
		ind = 0;
		send_buffer[ind++] = COMM_FW_VERSION;
		send_buffer[ind++] = FW_VERSION_MAJOR;
		send_buffer[ind++] = FW_VERSION_MINOR;
		coms_packet_tx(xSocket,pxClient, eIllegalTFTPOperation);
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
    	   coms_packet_process_rx(pucUDPPayloadBuffer, lBytes, xComPortListeningSocket, &xClient);
    	   FreeRTOS_ReleaseUDPPayloadBuffer( ( void * ) pucUDPPayloadBuffer );
       }

   }
}
