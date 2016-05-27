#ifndef SCHED_H
#define SCHED_H
#include "include.h"
#include "irq.h"

// Number of Task & Daemon
#define OS_MAX_TASKS	16

// Task state
#define TASK_STATE_CREATING 	0
#define TASK_STATE_RUNNING	1
#define TASK_STATE_PAUSING	2
#define TASK_STATE_DEAD 	3
#define TASK_STATE_IDLE 	4

// Task/Daemon stack size
#define TASK_STACK_SIZE 	256

// Daemon label
#define TASK_DAEMON 	1

// Priority level
#define HIGHEST_PRIORITY 	0
#define LOWEST_PRIORITY		254

typedef struct os_tcb {
	OS_STK	*OSTCBStkPtr; 	// (OS Task Control Block Stack Pointer)
	//INT8U 	OSTCBStat; 	// (OS Task Control Block Status)
	bool_t  IsDaemon;  // Daemon is always on work
	uint8_t Priority; // 0: highest; 1,2,...: lower and lower;
	uint8_t TaskState;
} OS_TCB; // (OS Task Control Block)

void OSTaskInit(void);
void OS_TaskIdle(void *p_arg);
void OSInitTaskIdle(void); // (OS Initialization of "TaskIdle")
void OSTaskCreate(void (*task)(void *p_arg), // task function
		  void *p_arg, 	// (pointer of arguments)
		  OS_STK *p_tos); // (pointer to the top of stack)
void OSTCBSet(OS_TCB *p_tcb, OS_STK *p_tos, bool_t IsDaemon, uint8_t Priority, uint8_t TaskState);

// For task attributes modification
bool_t TaskSetDaemonFlag(OS_TCB * tcb, bool_t IsDaemon);	 		  
uint8_t TaskSetPriority(OS_TCB * tcb, uint8_t Priority);
uint8_t TaskSetState(OS_TCB * tcb, uint8_t TaskState);		
		  
// For tasks Schdule
void OSTaskSchedule(void);

// Task Switching Function(OS Context Switch)--asm
void OSCtxSw(void);				

		  
OS_STK* OSTaskStkInit(void (*task)(void *p_arg), // task function
	          void *p_arg, // (pointer of arguments)
	      	  OS_STK *p_tos); // (pointer to the top of stack)
		  
#endif
