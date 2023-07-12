
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diag/trace.h"
#include <time.h>
#include <math.h>

#include "FreeRTOS.h"

#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "task.h"


#define CCM_RAM __attribute__((section(".ccmram")))


static void SendAutoReloadTimerCallback();
static void SendAutoReloadTimerCallback2();
static void ReceiveAutoReloadTimerCallback();



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"



BaseType_t xTimer_1_Started;
BaseType_t xTimer_2_Started;
BaseType_t xTimer_3_Started;

TaskHandle_t Sender_Task_1_Handle=NULL;
TaskHandle_t Sender_Task_2_Handle=NULL;
TaskHandle_t Receiver_Task_Handle=NULL;

SemaphoreHandle_t Sender2Semaphore=NULL;
SemaphoreHandle_t Sender1Semaphore=NULL;
SemaphoreHandle_t ReceiveSemaphore=NULL;

static TimerHandle_t SendAutoReloadTimer=NULL;
static TimerHandle_t SendAutoReloadTimer2=NULL;
static TimerHandle_t ReceiveAutoReloadTimer=NULL;

QueueHandle_t my_Queue;
int Treceiver = 100;

//Random Period

//array of different periods of sender timer and a pointer pointing to the first location before it
int Tsender, Tsender1, Tsender2;

int indexs;

const int lower[6] = {50, 80, 110, 140, 170, 200};
const int upper[6] = {150, 200, 250, 300,350, 400};

int Random (int i)
{
   Tsender=((rand()%( upper[i] -lower[i] +1))+lower[i]);
   return (Tsender);
}
int SucessSend=0,BlockedMessages=0,MessagesNumber=0;
int ReceivedMessages=0;
void Sender_Task_1(void *p)
{
	char sendertxt[50];
	BaseType_t Status;
	while(1)
	{
		Tsender1 = Random(indexs);
		xTimerChangePeriod( SendAutoReloadTimer,pdMS_TO_TICKS(Tsender1),0);
		if ( xSemaphoreTake(Sender1Semaphore,portMAX_DELAY) )
		{
			sprintf(sendertxt,"Time is %ul",xTaskGetTickCount());
			Status=xQueueSend(my_Queue,sendertxt,0);
			if(Status==pdPASS)
			{
				SucessSend++;
			}
			else
			{
				BlockedMessages++;
			}
			MessagesNumber++;
			xSemaphoreGive(Sender1Semaphore);
		 }
		vTaskDelay(pdMS_TO_TICKS(Tsender1));
	}
}

void Sender_Task_2(void *p)
{
	char sendertxt[50];
	BaseType_t Status;
	Sender2Semaphore=xSemaphoreCreateBinary();
	while(1)
	{
		Tsender2 = Random(indexs);
		xTimerChangePeriod( SendAutoReloadTimer2,pdMS_TO_TICKS(Tsender2),0);
		if (xSemaphoreTake(Sender2Semaphore,portMAX_DELAY))
		 {
			sprintf(sendertxt,"Time is %ul",xTaskGetTickCount());
			Status=xQueueSend(my_Queue,sendertxt,0);
			if(Status==pdPASS)
				SucessSend++;
			else
				BlockedMessages++;
			MessagesNumber++;
			xSemaphoreGive(Sender2Semaphore);
		 }
		vTaskDelay(pdMS_TO_TICKS(Tsender2));
	}
}


void ReceiverTask (void *p)
{
	char Receivertxt[50];
	while(1)
	{
		 if ( xSemaphoreTake(ReceiveSemaphore,portMAX_DELAY) )
		 {
			if(my_Queue!=0)
			{
				if(xQueueReceive(my_Queue,Receivertxt,0))
				{
					ReceivedMessages++;
				}
				xSemaphoreGive(ReceiveSemaphore);
			}
			if (ReceivedMessages >=500)
			{
				Reset();
			}
		 }
			vTaskDelay(pdMS_TO_TICKS(100));
	}
}

int EndFlag=0;
int first_time=1;
void Reset ()
{
	printf("index=%d\n%d\n%d\n",indexs,lower[indexs],upper[indexs]);
	if(first_time)
	{
			#define RECEIVE_TIMER_PERIOD pdMS_TO_TICKS( 100 )
	}
	printf("total number of Received Messages: %d\n",ReceivedMessages);
	printf("total number of blocked message: %d\n",BlockedMessages);
	printf("total number of send message: %d\n",SucessSend);
	printf("--------------------\n");
		SucessSend=0; BlockedMessages=0; ReceivedMessages=0;
	if (indexs <5)
		{indexs ++;}
	else {
				//destroying timers and printing "Game Over"
				xTimerDelete( SendAutoReloadTimer,0 );
				xTimerDelete( SendAutoReloadTimer2,0 );
				xTimerDelete( ReceiveAutoReloadTimer,0);
				printf("Game Over\n");
				exit(0);
			}
	xQueueReset( my_Queue );
	}

int main(int argc, char *argv[])
{
	   srand(time(0));
   int queueSize=20;
	my_Queue=xQueueCreate(queueSize,sizeof(char[50]));

	xTaskCreate(Sender_Task_1,"send1",1000,0,0,&Sender_Task_1_Handle);
	xTaskCreate(Sender_Task_2,"send2",1000,0,0,&Sender_Task_2_Handle);
	xTaskCreate(ReceiverTask,"Receive",1000,0,0,&Receiver_Task_Handle);

	vSemaphoreCreateBinary(Sender1Semaphore);
	vSemaphoreCreateBinary(Sender2Semaphore);
	vSemaphoreCreateBinary(ReceiveSemaphore);

	SendAutoReloadTimer=xTimerCreate("SendTimer",pdMS_TO_TICKS(1000) ,pdTRUE,0,SendAutoReloadTimerCallback);
	SendAutoReloadTimer2=xTimerCreate("SendTimer",pdMS_TO_TICKS(1000) ,pdTRUE,1,SendAutoReloadTimerCallback2);
	ReceiveAutoReloadTimer=xTimerCreate("ReceiveTimer",pdMS_TO_TICKS(100),pdTRUE,1,ReceiveAutoReloadTimerCallback);

	if ((SendAutoReloadTimer != NULL) && (SendAutoReloadTimer2 != NULL) && (ReceiveAutoReloadTimer != NULL))
	 {
		   xTimer_1_Started = xTimerStart(SendAutoReloadTimer, 0);
		   xTimer_2_Started = xTimerStart(SendAutoReloadTimer2, 0);
		   xTimer_3_Started = xTimerStart(ReceiveAutoReloadTimer, 0);
	 }

	    if (xTimer_1_Started == pdPASS && xTimer_2_Started == pdPASS && xTimer_3_Started == pdPASS)
	        vTaskStartScheduler();
    return 0;
}


#pragma GCC diagnostic pop

static void SendAutoReloadTimerCallback()
{

	xSemaphoreGive(Sender1Semaphore);
}

static void SendAutoReloadTimerCallback2()
{

	xSemaphoreGive(Sender2Semaphore);
}

static void ReceiveAutoReloadTimerCallback()
{
	xSemaphoreGive(ReceiveSemaphore);
}

void vApplicationMallocFailedHook(void)
{
    /* Called if a call to pvPortMalloc() fails because there is insufficient
    free memory available in the FreeRTOS heap.  pvPortMalloc() is called
    internally by FreeRTOS API functions that create tasks, queues, software
    timers, and semaphores.  The size of the FreeRTOS heap is set by the
    configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
    for (;;)
        ;
}
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    (void)pcTaskName;
    (void)pxTask;

    /* Run time stack overflow checking is performed if
    configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected. */
    for (;;)
        ;
}
void vApplicationIdleHook(void)
{
    volatile size_t xFreeStackSpace;

    /* This function is called on each cycle of the idle task.  In this case it
    does nothing useful, other than report the amout of FreeRTOS heap that
    remains unallocated. */
    xFreeStackSpace = xPortGetFreeHeapSize();

    if (xFreeStackSpace > 100)
    {
        /* By now, the kernel has allocated everything it is going to, so
        if there is a lot of heap remaining unallocated then
        the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
        reduced accordingly. */
    }
}
void vApplicationTickHook(void)
{
}
StaticTask_t xIdleTaskTCB CCM_RAM;
StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE] CCM_RAM;
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
{
    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
static StaticTask_t xTimerTaskTCB CCM_RAM;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH] CCM_RAM;
/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
