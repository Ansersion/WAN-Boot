#include "sched.h"


OS_TCB *OSTCBCur; // Pointer to the current running task(OS Task Control Block Current)
OS_TCB *OSTCBNext; // Pointer to the next running task(OS Task Control Block Next)

uint8_t OSTaskNext; // Index of the next task
uint8_t TaskDaemonNext; // Index of the next daemon

OS_TCB OSTCBTbl[OS_MAX_TASKS]; // (OS Task Control Block Table)
OS_STK TASK_IDLE_STK[TASK_STACK_SIZE]; //("TaskIdle" Stack)

INT32U TaskTickLeft; // Refer to the time ticks left for the current task


// Initialize the stack of a task, it is of much relationship with the specific CPU
OS_STK* OSTaskStkInit(void (*task)(void *p_arg),
	      void *p_arg,
	      OS_STK *p_tos)
{
	OS_STK *stk;
	stk = p_tos;

	*(stk)    = (INT32U)0x01000000L;             // xPSR                                               
	*(--stk)  = (INT32U)task;                    // Entry Point  

	// Don't be serious with the value below. They are of random
	*(--stk)  = (INT32U)0xFFFFFFFEL;             // R14 (LR) 
	*(--stk)  = (INT32U)0x12121212L;             // R12                                                
	*(--stk)  = (INT32U)0x03030303L;             // R3                                                 
	*(--stk)  = (INT32U)0x02020202L;             // R2                                                 
	*(--stk)  = (INT32U)0x01010101L;             // R1                                                 

	// pointer of the argument
	*(--stk)  = (INT32U)p_arg;                   // R0

	// Don't be serious with the value below. They are of random
	*(--stk)  = (INT32U)0x11111111L;             // R11 
	*(--stk)  = (INT32U)0x10101010L;             // R10 
	*(--stk)  = (INT32U)0x09090909L;             // R9  
	*(--stk)  = (INT32U)0x08080808L;             // R8  
	*(--stk)  = (INT32U)0x07070707L;             // R7  
	*(--stk)  = (INT32U)0x06060606L;             // R6  
	*(--stk)  = (INT32U)0x05050505L;             // R5  
	*(--stk)  = (INT32U)0x04040404L;             // R4  
	return stk;
}

void OSInitTaskIdle(void)
{
	OS_ENTER_CRITICAL();
	OSTCBTbl[0].OSTCBStkPtr = OSTaskStkInit(OS_TaskIdle, (void *)0, (OS_STK*)&TASK_IDLE_STK[TASK_STACK_SIZE - 1]);
	//OSTCBTbl[0].OSTCBStat = TASK_STATE_RUNNING;
	OSTCBTbl[0].IsDaemon = ~TASK_DAEMON;
	OSTCBTbl[0].Priority = LOWEST_PRIORITY; 
	OSTCBTbl[0].TaskState = TASK_STATE_IDLE;
	OS_EXIT_CRITICAL();
}

void OSTaskCreate(void (*task)(void *p_arg), 
		  void *p_arg, 
		  OS_STK *p_tos)
{
	OS_STK * tmp;
	INT8U i = 1;
	OS_ENTER_CRITICAL();
	while(OSTCBTbl[i].OSTCBStkPtr != (OS_STK*)0) {
		i++;
	}
	tmp = OSTaskStkInit(task, p_arg, p_tos);
	OSTCBSet(&OSTCBTbl[i], tmp, ~TASK_DAEMON, LOWEST_PRIORITY, TASK_STATE_PAUSING);
		// OSTCBSet(&OSTCBTbl[i], tmp, ~TASK_DAEMON, 1, TASK_STATE_PAUSING);
	OS_EXIT_CRITICAL();
}

void OSTCBSet(OS_TCB *p_tcb, OS_STK *p_tos, bool_t IsDaemon, uint8_t Priority, uint8_t TaskState)
{
	p_tcb->OSTCBStkPtr = p_tos;
	p_tcb->IsDaemon = IsDaemon;
	p_tcb->Priority = Priority;
	p_tcb->TaskState = TaskState;
}

void OS_TaskIdle(void *p_arg)
{
	p_arg = p_arg; // No use of p_arg, only for avoiding "warning" here.
	for(;;) {
		// OS_ENTER_CRITICAL();
		// Nothing to do
		// OS_EXIT_CRITICAL();
	}
}

void OSTaskInit(void)
{
	INT8U i;
	OSTaskNext = 0;
	TaskDaemonNext = 0;
	for(i = 0; i < OS_MAX_TASKS; i++) {
		OSTCBSet(&OSTCBTbl[i], (OS_STK *)0, ~TASK_DAEMON, LOWEST_PRIORITY, TASK_STATE_IDLE);
	}
	OSInitTaskIdle();
	OSTCBCur = &OSTCBTbl[0];
	OSTCBNext = &OSTCBTbl[0];
}

void OSTaskSchedule(void)
{
		OSTCBCur = OSTCBNext;
		OSCtxSw();
		OSTaskNext++;
		while(OSTCBTbl[OSTaskNext].OSTCBStkPtr == (OS_STK*)0) { 
			OSTaskNext++;
			if(OSTaskNext >= OS_MAX_TASKS) {
				OSTaskNext = 0;
			}
		}
		OSTCBNext = &OSTCBTbl[OSTaskNext];
//	TimeMS++;
//	OS_TCB TcbTmp;
//	uint32_t i;
//	uint8_t HighestPriority = LOWEST_PRIORITY;
	
	// OSTCBCur = OSTCBNext;
	// TcbTmp = *OSTCBCur;
	
	//OSTaskNext++;
	//for(i = 0; i < OS_MAX_TASKS; i++) {
		//if(OSTCBTbl[i].TaskState == TASK_STATE_PAUSING) {
			//HighestPriority = OSTCBTbl[i].Priority < HighestPriority ? OSTCBTbl[i].Priority : HighestPriority; 
		//}
//	}
	
//	for(i = OSTaskNext; i < OS_MAX_TASKS; i++) {
	//	if(OSTCBTbl[i].IsDaemon) {
		//	if(OSTCBTbl[i].TaskState == TASK_STATE_PAUSING) {
			//}
			
	//	} else if(OSTCBTbl[i].Priority == HighestPriority && HighestPriority != LOWEST_PRIORITY) {
		//	break;
		//}
		
//		if(OSTaskNext > 1 && i == OS_MAX_TASKS - 1) {
	//		i = 0;
		//}
		
		// No Task/Daemon left, just to run TaskIdle
//		if(i == OSTaskNext - 1) {
	//		i = 0;
		//	break;
		//}
//	}
//	OSTaskNext = i;
	
//	OSTCBNext = &OSTCBTbl[OSTaskNext];
	
//	OSCtxSw();
}

// For task attributes modification
bool_t TaskSetDaemonFlag(OS_TCB * tcb, bool_t IsDaemon)
{
	tcb->IsDaemon = IsDaemon;
	return tcb->IsDaemon;
}

uint8_t TaskSetPriority(OS_TCB * tcb, uint8_t Priority)
{
	tcb->Priority = Priority;
	return tcb->Priority;
}

uint8_t TaskSetState(OS_TCB * tcb, uint8_t TaskState)
{
	tcb->TaskState = TaskState;
	return tcb->TaskState;
}

