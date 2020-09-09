/* System headers. */
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "croutine.h"
#include "partest.h"
#include "semphr.h"

/* Demo file headers. */
#include "BlockQ.h"
#include "PollQ.h"
#include "death.h"
#include "crflash.h"
#include "flop.h"
#include "print.h"
#include "fileIO.h"
#include "semtest.h"
#include "integer.h"
#include "dynamic.h"
#include "mevents.h"
#include "crhook.h"
#include "blocktim.h"
#include "GenQTest.h"
#include "QPeek.h"
#include "countsem.h"
#include "recmutex.h"

#include "AsyncIO/AsyncIO.h"
#include "AsyncIO/AsyncIOSocket.h"
#include "AsyncIO/PosixMessageQueueIPC.h"
#include "AsyncIO/AsyncIOSerial.h"


/*----------------------- METHODS ---------------------------*/
static void configAll();
static void xTaskHandlerUDP_Rx(void *params);
static void xPrint(void *params);

/*------------------------  UDP  ----------------------------*/

#define mainUDP_SEND_ADDRESS		"127.0.0.1"
#define mainUDP_PORT				( 5000 )

struct sockaddr_in xReceiveAddress; /* UDP Packet structure */

/*------------------------  TASK'S  -------------------------*/
xTaskHandle xTaskHandleUDP_Rx = NULL;
xTaskHandle xTaskHandleUDP_Tx = NULL;

xTaskHandle xPrintHandle = NULL;

/*------------------------  QUEUES  -------------------------*/
xQueueHandle xUDPReceiveQueue = NULL;
xQueueHandle xUDPTransmissionQueue = NULL;

/*------------------------  MUTEXES  ------------------------*/
xSemaphoreHandle xSemaphore = NULL;

/*----------------------  PRIORITIES  -----------------------*/
#define xTaskHandleUDP_PRIO_Rx 		(tskIDLE_PRIORITY + 2)
#define xTaskHandleUDP_PRIO_Tx 		(tskIDLE_PRIORITY + 2)

#define xPrintHandle_PRIO 			(tskIDLE_PRIORITY + 1)

int main(void) {

	configAll();

	xTaskCreate(xTaskHandlerUDP_Rx, "UDPRx", configMINIMAL_STACK_SIZE, NULL, xTaskHandleUDP_PRIO_Rx, &xTaskHandleUDP_Rx);
	xTaskCreate(xPrint, "xPrint", configMINIMAL_STACK_SIZE, NULL, xPrintHandle_PRIO, &xPrintHandle);

	vTaskStartScheduler();
	return 1;
}


static void xTaskHandlerUDP_Rx(void *params){

	/* Variables */
	char msg[1024];

	/* Socket aux's */
	int iSocketSend = 0, iReturn = 0;
	static xUDPPacket xPacket, xPacketReceive;
	struct sockaddr_in xSendAddress;

	/* Task Frequency */
	portTickType xLastWakeTime;
	const portTickType xFrequency = 5000; // 1000 milliseconds.
	xLastWakeTime = xTaskGetTickCount();

	 /* Open a socket for sending. */
	 iSocketSend = iSocketOpenUDP( NULL, NULL, NULL );

	 if (iSocketSend != 0) {

		/* Set the UDP main address to reflect your local subnet. */
		xSendAddress.sin_family = AF_INET;
		iReturn = !inet_aton( mainUDP_SEND_ADDRESS, (struct in_addr *) &( xSendAddress.sin_addr.s_addr ) );
		xSendAddress.sin_port = htons( mainUDP_PORT );

		for (;;){
			vTaskDelayUntil(&xLastWakeTime, xFrequency);

			/* Copy command to package */
			strcpy(xPacket.ucPacket, "sta0");

			/* Critical section Initialized */
			xSemaphoreTake(xSemaphore, portMAX_DELAY);

			/* Send package to boiler */
			iReturn = iSocketUDPSendTo(iSocketSend, &xPacket, &xSendAddress);

			/* Package was corrupted ? */
			if (iReturn != sizeof(xUDPPacket)) {
				puts("[Fail] - SEND PACKAGE - Task UDPRx");
			} else {
				iSocketUDPReceiveFrom(iSocketSend, &xPacketReceive, &xSendAddress);
				xPacketReceive.ucNull = 0;
			}
			/* Critical section Finished */
			xSemaphoreGive(xSemaphore);

			/* Sending the data to respective queue */
			if(iReturn == sizeof(xUDPPacket)){
				strcpy(msg, xPacketReceive.ucPacket);
				xQueueSend(xUDPReceiveQueue, &msg, (portTickType) 0);
			}
		}
	}

}

static void xPrint(void *params){
	char msg[1024];

	for(;;){
		xQueueReceive(xUDPReceiveQueue, &msg, portMAX_DELAY);
		puts(msg);
	}
}

static void configAll()
{
	/* Initialise hardware and utilities. */
	vParTestInitialise();
	vPrintInitialise();

	/* Creating mutexes */
	xSemaphore = xSemaphoreCreateMutex();

	/* Creating queues */
	xUDPReceiveQueue = xQueueCreate(10, (sizeof(unsigned char) * 1024));

}
