#ifndef IRQ_H
#define IRQ_H
#include "include.h"

// IRQ Rename
#define IRQ_HardFault 		HardFault_Handler
#define IRQ_MemManage			MemManage_Handler
#define IRQ_BusFault			BusFault_Handler
#define IRQ_UsageFault		UsageFault_Handler
#define IRQ_SysTick 			SysTick_Handler 
#define IRQ_PendSV 				PendSV_Handler
#define IRQ_Usart1 				USART1_IRQHandler
// #define IRQ_PendSV 			PendSV_Handler
#define IRQ_DMA1_Channel5 	DMA1_Channel5_IRQHandler

#define TASK_TIME_SLICE 	5 		// 5ms for every task to run every time

// assembling functions
void OS_ENTER_CRITICAL(void); 			// Enter Critical area, that is to disable interruptions
void OS_EXIT_CRITICAL(void);			// Exit Critical area, that is to enable interruptions

volatile void IRQ_HardFault(void);
volatile void IRQ_MemManage(void);
volatile void IRQ_BusFault(void);
volatile void UsageFault_Handler(void);
volatile void IRQ_SysTick(void); // The interrupt function
volatile void IRQ_Usart1(void);
void IRQ_DMA1_Channel5(void);
void IRQInit(void);

void IRQ_PendSV_C(void);

// PendSV interruption--asm
// void PendSV_Handler_Asm(void);	

INT32U GetTime_API(void);
bool_t IsMsgGotten(void);
void SetMsgGotten(void);
void ClrMsgGotten(void);

#endif
