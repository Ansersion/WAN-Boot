#include "include.h"
#include "irq.h"
#include "sched.h"
#include "hardware.h"
#include "wanp.h"

INT32U TimeMS = 0;
INT32U TaskTimeSlice = TASK_TIME_SLICE;

// uint32_t Index;
// uint32_t MsgSize;
// extern bool_t StartOSFlag;
extern bool_t EndBurningFlag;
// extern uint32_t * kernel_start_address;

/* Buffers for USART1 */
unsigned char RecvBuffer[BUFSIZ];
unsigned char SendBuffer[BUFSIZ];

/* Flags for USART1*/
// bool_t MagicGotten;
// bool_t SizeGotten;
// bool_t ChecksumGotten;
static bool_t MsgGotten;

#define HFSR 	((uint32_t *)0xE000ED2C)
#define MFSR ((char *)0xE000ED28)
#define BFSR ((char *)0xE000ED29)
#define UFSR ((char *)0xE000ED2A)
	
#define BFAR ((uint32_t *)0xE000ED38)
	
volatile void IRQ_HardFault(void)
{

	printf("HardFault:%x\r\n", *HFSR);
	printf("MemManage:%x\r\n", *MFSR);
	printf("BusFault:%x\r\n", *BFSR);
	printf("BFAR:%x\r\n", *BFAR);
	printf("UsageFault:%x\r\n", *UFSR);
	for(;;);
}

volatile void IRQ_MemManage(void)
{

	printf("MemManage:%x\r\n", *MFSR);
	for(;;);
}

volatile void IRQ_BusFault(void)
{

	printf("BusFault:%x\r\n", *BFSR);
	printf("BFAR:%x\r\n", *BFAR);
	for(;;);
}

volatile void UsageFault_Handler(void)
{

	printf("UsageFault:%x\r\n", *UFSR);
	for(;;);
}

void IRQInit(void)
{
	// Usart1 configuration;
	memset(RecvBuffer, 0, BUFSIZ);
	memset(SendBuffer, 0, BUFSIZ);
	// MagicGotten = FALSE;
	// SizeGotten = FALSE;
	// ChecksumGotten = FALSE;
	ClrMsgGotten();
}

volatile void IRQ_SysTick(void)
{
	 OS_ENTER_CRITICAL();
	 // if(StartOSFlag) ModifyPC();
	 if((--TaskTimeSlice) == 0){
		TaskTimeSlice = TASK_TIME_SLICE;
		OSTaskSchedule();
	}
	TimeMS++;
	
	OS_EXIT_CRITICAL();
}

volatile void IRQ_Usart1(void)
{
	static uint32_t Index = 0;
	uint16_t RxData=0;
	
	if(USART_GetITStatus(USART1, USART_IT_RXNE) == RESET) {
		return;
	}
	
	if(!IsMsgGotten()) {
		// RxData = USART_ReceiveData(USART1);
		USART1_RECEIVE(RxData);
		RecvBuffer[Index++] = (uint8_t)RxData;// serial_1();
	}
	else {
		// RxData = USART_ReceiveData(USART1); // ignore the data
		RxData = USART1_RECEIVE(RxData);
		return;
	}
	 
	#ifdef WANP
	if(Index > WAN_HEADER_SIZE &&
		RecvBuffer[Index-1] == '\n' &&
		RecvBuffer[Index-2] == '\r' &&
		RecvBuffer[Index-3] == '\n' &&
		RecvBuffer[Index-4] == '\r') {
			// OS_ENTER_CRITICAL();
			SetMsgGotten();
			// MsgSize = Index;
			// RecvBuffer[Index] = '\0';
			// OS_EXIT_CRITICAL();
			RecvBuffer[Index] = '\0';
			Index = 0;
	
		}
		#else

		
		if(Index >= BUFSIZ - 1 || RecvBuffer[Index-1] == '\r' || RecvBuffer[Index-1] == '\n') {
//			OS_ENTER_CRITICAL();
// 			MsgGotten = TRUE;
//			MsgSize = Index;
//			RecvBuffer[Index] = '\0';
//			OS_EXIT_CRITICAL();
			SetMsgGotten();
			RecvBuffer[Index] = '\0';
			
			Index = 0;
		}
		#endif
}

void IRQ_DMA1_Channel5(void)
{
	 DMA_ClearITPendingBit(DMA1_IT_TC5);
	 DMA_ClearITPendingBit(DMA1_IT_TE5);
	 USART_Cmd(USART1, DISABLE);
	 DMA_Cmd(DMA1_Channel5, DISABLE);//close DMA
	 USART_DMACmd(USART1,USART_DMAReq_Rx,DISABLE); 
	 USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	 USART_Cmd(USART1, ENABLE);
	 EndBurningFlag = TRUE;
	 // LED1TURN(); // For indication
}

INT32U GetTime_API(void)
{
	return TimeMS;
}

bool_t IsMsgGotten(void)
{
	return MsgGotten;
}

void SetMsgGotten(void)
{
	OS_ENTER_CRITICAL();
	MsgGotten = 1;
	OS_EXIT_CRITICAL();
}

void ClrMsgGotten(void)
{
	OS_ENTER_CRITICAL();
	MsgGotten = 0;
	OS_EXIT_CRITICAL();
}
