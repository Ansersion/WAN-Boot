#include "hardware.h"
#include <absacc.h>

// u8  __attribute__((aligned(32)))SRAM_Kernel_Buffer[MAX_KERNEL_SIZE]; // For loading the kernel
uint8_t SRAM_Kernel_Buffer[MAX_KERNEL_SIZE]; // For loading the kernel

uint8_t * GetKernelBuffer(void)
{
	return SRAM_Kernel_Buffer;
}

volatile void InitBuffers(void)
{
	memset(SRAM_Kernel_Buffer, 0, sizeof(SRAM_Kernel_Buffer));
}

volatile void InitClk(INT32U frequency)
{
	frequency = frequency; // not used now
	
	RCC_DeInit();
	RCC_HSEConfig(RCC_HSE_ON);
	if(RCC_WaitForHSEStartUp() != SUCCESS) {
		RCC_DeInit();
		return;
	}
	
	RCC_HCLKConfig(RCC_SYSCLK_Div1);
	RCC_PCLK2Config(RCC_HCLK_Div1);
	RCC_PCLK1Config(RCC_HCLK_Div2);
	FLASH_SetLatency(FLASH_Latency_2);  
  FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
	// RCC_PREDIV1Config(RCC_PREDIV1_Source_HSE, RCC_PREDIV1_Div1);
	RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9); 
	RCC_PLLCmd(ENABLE); 
	while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET){
	}
	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
	while (RCC_GetSYSCLKSource() != 0x08) {
	}
	
}

volatile void SysTickInit(INT8U Nms)
{
    OS_ENTER_CRITICAL();                          
	
	SysTick_Config(Nms * TICK_FREQUENCY / 1000);
	NVIC_SetPriority(SysTick_IRQn, 0x0); 
  OS_EXIT_CRITICAL();
}

volatile void RCC_EnablePhripheralInit(void)
{
	// Enable clock of GPIO-A and GPIO-D
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOD, ENABLE);
	
	//Enable clock of CRC unit
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);
}

volatile void LedInit(void)
{
	GPIO_InitTypeDef GPIO_InitType;
	GPIO_TypeDef * GPIO_Type;
	
	// GPIO output initialization;
	GPIO_InitType.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitType.GPIO_Speed = GPIO_Speed_50MHz;
	
	// Initialize PA.8
	GPIO_Type = GPIOA;
	GPIO_InitType.GPIO_Pin = GPIO_Pin_8;
	GPIO_Init(GPIO_Type, &GPIO_InitType);
	
	// Initialize PD.2
	GPIO_Type = GPIOD;
	GPIO_InitType.GPIO_Pin = GPIO_Pin_2;
	GPIO_Init(GPIO_Type, &GPIO_InitType);
	
}

volatile void USART1_Configuration(INT32U bound)
{
	
	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure; 
	NVIC_InitTypeDef NVIC_InitStructure;
	
	/* Enable GPIO clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	
	/* USART clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE); 

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; 
	GPIO_Init(GPIOA,&GPIO_InitStructure); 

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; 
	GPIO_Init(GPIOA,&GPIO_InitStructure); 
	
	USART_DeInit(USART1);
	
	/* Usart init£¬9600£¬8bit data bit,1 stop bit, No Parity and flow control, rx tx enable */
	USART_InitStructure.USART_BaudRate               = bound;
	USART_InitStructure.USART_WordLength             = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits               = USART_StopBits_1;
	USART_InitStructure.USART_Parity                 = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl    = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode                   = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
		
	USART_Cmd(USART1, ENABLE);
}

volatile void DMA_Configuration(void)
{
		
	NVIC_InitTypeDef NVIC_InitStructure;
	DMA_InitTypeDef DMA_InitStructure;
	
	/* DMA clock enable (USART RX using dma1) */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 
		
	DMA_DeInit(DMA1_Channel5);
	
	DMA_InitStructure.DMA_PeripheralBaseAddr    = (u32)(&USART1->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr        = (uint32_t)SRAM_Kernel_Buffer;
	DMA_InitStructure.DMA_BufferSize            = 0;
    
	DMA_InitStructure.DMA_PeripheralInc         = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc             = DMA_MemoryInc_Enable;
	
	DMA_InitStructure.DMA_PeripheralDataSize    = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize        = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode                  = DMA_Mode_Normal;  

	DMA_InitStructure.DMA_Priority              = DMA_Priority_High;
	DMA_InitStructure.DMA_DIR                   = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_M2M                   = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);	
	
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	DMA_ITConfig(DMA1_Channel5,DMA_IT_TC, ENABLE); 
}

