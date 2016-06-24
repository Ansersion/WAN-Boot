#include "include.h"
#include "stm32f10x.h"
#include "irq.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_gpio.h"

/**********************************************************/
/******************WAN Boot configure**********************/
/*
* MAX_KERNEL_SIZE:
* Description: The max size of the OS kernel that
	WAN-Boot supported. The value size is 
	determined by the flash/disk space and 
	the memory size. For example, the 
	STM32F103RCT6 has 512k flash and 64k memory.
	On one hand, we the enough space of flash/disk
	to store the OS kernel, on another hand we also 
	enough memory to run the OS kernel. So this value
	must be smaller than both of the flash space and 
	memory.
* Warning: This must be complied with: 
	"MAX_KERNEL_SIZE < min(flash size, memory size) / 2".
*	ToDo: Burn the kernel with slice buffer.
* Default: 16k
*/
#define MAX_KERNEL_SIZE 			16384 // 16k

/*
* TICK_FREQUENCY:
* Description: The frequency of the microchip.For example the 
	STM32F103RCT6 support the frequency 8-72MHz.
* Warning: The default value(72MHz) is not supported to be 
	modified.
* ToDo: Make this value changeable 
* Default: 72MHz
*/
#define TICK_FREQUENCY				72000000 // 72MHz

/*
* SYS_TICK_UNIT:
* Description: The heart beat of the WAN-Boot.
* Default: 1 millisecond
*/
#define SYS_TICK_UNIT 				1 // 1 millisecond

/*
* USART1_BAUD_RATE:
* Description: The baud rate of the USART.
	Because STM32F103RCT6 has 2 USART, 
	and we configure the USART1 only.
* Default: 9600
*/
#define USART1_BAUD_RATE			9600

/*
* RAM_ADDR_FOR_KERNEL:
* Description: RAM start address for kernel to run.
* Default: 0x20000000
*/
#define RAM_ADDR_FOR_KERNEL 	0x20000000


/**********************************************************/
/****************Hardware configure************************/

/*
* SYSTICK_PRIORITY_PTR:
* Description: Systick interruption priority
*/
#define SYSTICK_PRIORITY_PTR 	((char *)0xE000ED23)		

/*
* GPIO
*/
#define LEN_RED_TURN() (GPIOA->ODR ^= 1<<8)
#define LEN_GREEN_TURN() (GPIOD->ODR ^= 1<<2)

/*
* USART1 sends data 
*/
#define USART1_SEND(ch) (USART1->DR = (ch) & (uint16_t)0x01FF)

/*
* USART1 receive data
*/
#define USART1_RECEIVE(ch) ((ch) = USART1->DR & (uint16_t)0x01FF)

/**********************************************************/
/**********************************************************/

uint8_t * GetKernelBuffer(void);
volatile void InitClk(INT32U frequency);
volatile void InitBuffers(void);
volatile void SysTickInit(INT8U Nms); // (System Tick Initialization)
volatile void USART1_Configuration(INT32U bound);
volatile void LedInit(void);
volatile void DMA_Configuration(void);
volatile void RCC_EnablePhripheralInit(void);
