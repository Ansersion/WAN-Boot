#include "init.h"

// Only to initialize the Task Control Block Table
void OSInit(void)
{
	
	OS_ENTER_CRITICAL();
	InitClk(TICK_FREQUENCY);

	OSTaskInit();
	SysTickInit(SYS_TICK_UNIT);
	// SysTickInit(1);
	// SysTickInit(1);
	RCC_EnablePhripheralInit();
	InitBuffers();
	LedInit();
	USART1_Configuration(USART1_BAUD_RATE);
	DMA_Configuration();
	IRQInit();
	OS_EXIT_CRITICAL();
}


