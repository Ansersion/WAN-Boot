#include "include.h"
#include "init.h"
#include "sched.h"
#include "time.h"
#include "printf_to_serial.h"
#include "stm32f10x_flash.h"
#include "wanp.h"
#include "checksum.h"
#include "cmd.h"
#include "crc32.h"
#include "flash_read.h"

#define OS_STORED_ADDRESS 	0x08020000
#define OS_INFO_ADDRESS 		0x08018000

extern OS_TCB OSTCBTbl[OS_MAX_TASKS]; // (OS Task Control Block Table)
extern OS_STK TASK_IDLE_STK[TASK_STACK_SIZE]; //("TaskIdle" Stack)
extern OS_TCB *OSTCBCur; // Pointer to the current running task(OS Task Control Block Current)
extern OS_TCB *OSTCBNext; // Pointer to the next running task(OS Task Control Block Next)
extern INT8U OSTaskNext; // Index of the next task
extern INT32U TaskTickLeft; // Refer to the time ticks left for the current task
extern INT32U TimeMS;       // For system time record                             
extern INT32U TaskTimeSlice; // For system time record
extern u8 SRAM_Kernel_Buffer[MAX_KERNEL_SIZE]; // For store the kernel

// For usart1 receiving message
extern unsigned char RecvBuffer[BUFSIZ];
extern unsigned char SendBuffer[BUFSIZ];
extern bool_t MsgGotten;

/* cmd.c */
extern uint8_t cmd_buf[64];

extern const Cmd Hello;
extern const Cmd Burn;
extern const Cmd Startos;

char tmp_buf[BUFSIZ];

typedef struct KernelPmt {
	uint32_t MoveKernelTo; // also the kernel running start address
	uint32_t MoveKernelFrom;
	uint32_t KernelSize;
} KernelPmt;

extern void MoveAndStartKernel_Asm(KernelPmt * KP); // asm function in boot_cpu_a.asm

/*
uint8_t kernel_name[64];
uint8_t addr[32];
uint8_t kernel_size[32];
uint8_t crc[32];
*/

//uint32_t u_kernel_size;
//uint32_t u_addr;
// uint32_t u_crc;
// u8 * kernel_start_address;


// Task stack creating
OS_STK TaskLedStk[TASK_STACK_SIZE];
OS_STK TaskBHStk[TASK_STACK_SIZE];

// Task functions
void TaskLed(void *p_arg);
void TaskBH(void *p_arg);

// unsigned long Addr = 0;
bool_t ReadDMAFlag = 0;
// bool_t StartOSFlag = 0;

uint8_t WaitBurning(uint32_t addr, uint32_t size, uint32_t crc);
bool_t EndBurningFlag;

uint8_t WaitBurning(uint32_t addr, uint32_t size, uint32_t crc)
{
	uint32_t count;
	if(addr < 0x08000000 || addr > 0x08020000) {
		printf("address is over crossed\r\n");
		return 1; // over cross
	}		
	if(size > MAX_KERNEL_SIZE) {
		printf("kernel size is too large: %d > %d\r\n", size, MAX_KERNEL_SIZE);
		return 2;
	}
	
		USART_Cmd(USART1, DISABLE);
    USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
    USART_DMACmd(USART1,USART_DMAReq_Rx,ENABLE); 
    DMA1_Channel5->CNDTR = size;//load
    DMA_Cmd(DMA1_Channel5, ENABLE);//open DMA
    USART_Cmd(USART1, ENABLE);
	  EndBurningFlag = FALSE;
    LEN_RED_TURN();
		while(!EndBurningFlag)
			;

		if(crc32_byte(SRAM_Kernel_Buffer, size) != crc) {
			printf("Warning: CRC is not identical\r\n");
		} 
		OS_ENTER_CRITICAL();
		count = 0;
		FLASH_Unlock();
		FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
		FLASH_ErasePage(addr);
		printf("Start to burn flash\r\n"); //test
		while(count < size / 2 + 1)
		{
			FLASH_ProgramHalfWord((addr +count*2),*((uint16_t *)(SRAM_Kernel_Buffer+count * 2)));  //flash  为一个字节存储，16位数据必须地址加2
			count++;
		}

		
		FLASH_Lock();
		
		FLASH_Unlock();
		FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
		// OS Info	
		FLASH_ErasePage(OS_INFO_ADDRESS);
		printf("start to burn OS info\r\n");
		FLASH_ProgramWord(OS_INFO_ADDRESS, addr);
		FLASH_ProgramWord(OS_INFO_ADDRESS + 4, size);
		FLASH_Lock();
		OS_EXIT_CRITICAL();
//		kernel_start_address = (u8 *)addr;	
		LEN_RED_TURN();
		return 0;
}

int main(void)
{
	OSInit();
	
	OSTaskCreate(TaskLed, (void*)0, (OS_STK*)&TaskLedStk[TASK_STACK_SIZE-1]);
	OSTaskCreate(TaskBH, (void*)0, (OS_STK*)&TaskBHStk[TASK_STACK_SIZE-1]);
	
	BootStart();
}


void TaskLed(void *p_arg)
{
	while(1) {
		delayMs(1000);
		LEN_GREEN_TURN();
	}
}

void TaskBH(void *p_arg)
{
	uint8_t kernel_name[64];
	uint8_t addr[32];
	uint8_t kernel_size[32];
	uint8_t crc[32];
	
	uint32_t u_crc;
	
	char * endptr;
	char * tmp_buf_pointer;


	// uint8_t k;
	int ret;
//	int i;
	uint16_t msg_size;
	
	KernelPmt KP;
	
	KP.MoveKernelTo = 0x20000000;
	
	#ifdef WANP

	uint16_t msg_size_net;
	uint16_t csum;
	#endif

	while(1) {
		if(!MsgGotten) {
			continue;
		}
		
	#ifdef WANP	
		Wan_GetSize(RecvBuffer, &msg_size);
		if(Wan_CheckMsg(RecvBuffer, msg_size) < 0) {
						// printf("wan message check error\r\n");
						break;
					}
		#endif
										// buf[msg_size] = '\0';
					GetCmd(RecvBuffer, cmd_buf);
					// printf("cmd: %s\r\n", cmd_buf);
					if(strcmp(cmd_buf, Hello.Name) == 0) {
						ret = DoHello(RecvBuffer);
						if(0 == ret) {
							RespOK(SendBuffer, cmd_buf, NULL);
						} else if(HELP_HELLO == ret) {
							tmp_buf_pointer = tmp_buf;
							tmp_buf_pointer += sprintf(tmp_buf_pointer, "hello: \r\n");
							tmp_buf_pointer += sprintf(tmp_buf_pointer, "	a test message to confirm connection.\r\n");
							RespOK(SendBuffer, cmd_buf, tmp_buf);
						} else {
							RespErr(SendBuffer, cmd_buf, "hello err");
						}
					} else if(strcmp(cmd_buf, Burn.Name) == 0) {
						ret = DoBurn(RecvBuffer, kernel_name, addr, kernel_size, crc);
						if(ret == 0) {
							if('0' == addr[0] && ('x' == addr[1] || 'X' == addr[1])) {
								KP.MoveKernelFrom = strtol((const char *)addr, &endptr, 16);
							} else {			
								KP.MoveKernelFrom = atoi((const char *)addr);
							}
							if('0' == kernel_size[0] && ('x' == kernel_size[1] || 'X' == kernel_size[1])) {
								KP.KernelSize = strtol((const char *)kernel_size, &endptr, 16);
							} else {
								KP.KernelSize = atoi((const char *)kernel_size);
							}
							if('0' == crc[0] && ('x' == crc[1] || 'X' == crc[1])) {
								u_crc = strtol((const char *)crc, &endptr, 16);
							} else {
								u_crc = atoi((const char *)crc);
							}
							WaitBurning(KP.MoveKernelFrom, KP.KernelSize, u_crc);
							printf("Addr:KernelSize:Crc=%08x:%08x:%08x\r\n", KP.MoveKernelFrom, KP.KernelSize, u_crc);
							RespOK(SendBuffer, cmd_buf, NULL);
						} else if(HELP_BURN == ret) {
							tmp_buf_pointer = tmp_buf;
							tmp_buf_pointer += sprintf(tmp_buf_pointer, "burn: \r\n");
							tmp_buf_pointer += sprintf(tmp_buf_pointer, "	burn kernel to specific address.\r\n");
							tmp_buf_pointer += sprintf(tmp_buf_pointer, "	-f <kernel binary>\r\n");
							tmp_buf_pointer += sprintf(tmp_buf_pointer, "	-a <address to burn>\r\n");
							tmp_buf_pointer += sprintf(tmp_buf_pointer, "	-s <kernel size>(in byte)\r\n");
							tmp_buf_pointer += sprintf(tmp_buf_pointer, "	-c <CRC32>\r\n");
							RespOK(SendBuffer, cmd_buf, tmp_buf);
						} else {
							RespErr(SendBuffer, cmd_buf, "paramter err");
						}
					} else if(strcmp(cmd_buf, Startos.Name) == 0) {
						ret = DoStartos(RecvBuffer);
						if(0 == ret) {
							// OS Info
							/*
							flash_read_l(OS_INFO_ADDRESS, &u_addr);
							flash_read_l(OS_INFO_ADDRESS + 4, &u_kernel_size);
							KP.MoveKernelFrom = u_addr;
							KP.KernelSize = u_kernel_size;
							KP.MoveKernelTo = 0x20000000;
							*/
							flash_read_l(OS_INFO_ADDRESS, &(KP.MoveKernelFrom));
							flash_read_l(OS_INFO_ADDRESS + 4, &(KP.KernelSize));
							printf("Addr:KernelSize:RunAddr=%x:%d:%x\r\n", KP.MoveKernelFrom, KP.KernelSize, KP.MoveKernelTo);
							RespOK(SendBuffer, cmd_buf, NULL);
							
							// void StartKernel_2(uint32_t addr);
							OS_ENTER_CRITICAL();
							MoveAndStartKernel_Asm(&KP);
							// StartKernel_2(0x20008000);
							OS_EXIT_CRITICAL();

						} else if(HELP_STARTOS == ret) {
							tmp_buf_pointer = tmp_buf;
							tmp_buf_pointer += sprintf(tmp_buf_pointer, "startos: \r\n");
							tmp_buf_pointer += sprintf(tmp_buf_pointer, "	launch the OS kernel.\r\n");
							RespOK(SendBuffer, cmd_buf, tmp_buf);
						} else {
							RespErr(SendBuffer, cmd_buf, "startos err");
						}
					} else {
						RespErr(SendBuffer, cmd_buf, "Command not found");
					}
					if((msg_size = SealPacket(SendBuffer)) < 0) {
						// printf("SealPacket error\r\n");
						break;
					}
					printf("%s\r\n", SendBuffer);
		
		OS_ENTER_CRITICAL();
		MsgGotten = FALSE;
		OS_EXIT_CRITICAL();
	}
}

