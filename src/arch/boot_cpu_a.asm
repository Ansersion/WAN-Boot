	IMPORT 	OSTCBCur
	IMPORT	OSTCBNext
	
	EXPORT	OS_ENTER_CRITICAL
	EXPORT 	OS_EXIT_CRITICAL
	EXPORT	BootStart
	EXPORT	PendSV_Handler
	EXPORT 	OSCtxSw
	EXPORT 	MoveAndStartKernel_Asm
		
	; EXPORT StartKernel_2
	
NVIC_INT_CTRL	EQU			0xE000ED04	; Address of NVIC Interruptions Control Register
NVIC_PENDSVSET	EQU			0x10000000	; Enable PendSV
NVIC_SYSPRI14   EQU     	0xE000ED22  ; System priority register (priority 14).
NVIC_PENDSV_PRI EQU         0xFF        ; PendSV priority value (lowest).
	
	PRESERVE8 ; align 8

	AREA    |.text|, CODE, READONLY 
	THUMB 

;/******************OS_ENTER_CRITICAL************/
OS_ENTER_CRITICAL
	CPSID	I	; Enable interruptions(Change Processor States: Interrupts Disable)
	BX	LR	; Return

;/******************OS_EXIT_CRITICAL************/
OS_EXIT_CRITICAL
	CPSIE	I	; Disable interruptions
	BX	LR 	; Return

;/******************BootStart************/
BootStart
	; disable interruptions
	CPSID	I							; OS_ENTER_CRITICAL();
	; initialize PendSV
	; Set the PendSV exception priority
	LDR     R0, =NVIC_SYSPRI14              ; R0 = NVIC_SYSPRI14;
	LDR     R1, =NVIC_PENDSV_PRI			; R1 = NVIC_PENDSV_PRI;
	STRB    R1, [R0]						; *R0 = R1;
	
	; initialize PSP as 0
	; MOV	R4, #0
	LDR R4,  =0x0						; R4 = 0;
	MSR	PSP, R4							; PSP = R4;
	
	; trigger PendSV
	LDR	R4, =NVIC_INT_CTRL				; R4 = NVIC_INT_CTRL;
	LDR	R5, =NVIC_PENDSVSET				; R5 = NVIC_PENDSVSET;
	STR	R5, [R4]						; *R4 = R5;
	
	; enable interruptions
	CPSIE	I							; OS_EXIT_CRITICAL();

; should never get here
; a endless loop
BootStartHang									
	B	BootStartHang

;/******************PendSV_Handler************/
PendSV_Handler
	CPSID	I							; OS_ENTER_CRITICAL();
	; judge if PSP is 0 which means the task is first invoked
	MRS 	R0, PSP						; R0 = PSP;
	CBZ 	R0, PendSV_Handler_NoSave	; if(R0 == 0) goto PendSV_Handler_NoSave;
	
	; store R4-R11 to *R0
	SUB 	R0, R0, #0x20
	STM 	R0, {R4-R11}
	
	LDR 	R1, =OSTCBCur			; R1 = OSTCBCur;
	LDR 	R1, [R1] 				; R1 = *R1;(R1 = OSTCBCur->OSTCBStkPtr)
	STR 	R0, [R1] 				; *R1 = R0;(*(OSTCBCur->OSTCBStkPrt) = R0)

PendSV_Handler_NoSave
	LDR 	R0, =OSTCBCur 			; R0 = OSTCBCur;
	LDR 	R1, =OSTCBNext			; R1 = OSTCBNext;
	LDR 	R2, [R1] 				; R2 = OSTCBNext->OSTCBStkPtr;
	STR 	R2, [R0]				; *R0 = R2;(OSTCBCur->OSTCBStkPtr = OSTCBNext->OSTCBStkPtr)
	
	LDR 	R0, [R2] 				; R0 = *R2;(R0 = OSTCBNext->OSTCBStkPtr)

	; restore *R0 to R4-R11 
	LDM 	R0!, {R4-R11}
	
	MSR 	PSP, R0 				; PSP = R0;(PSP = OSTCBNext->OSTCBStkPtr)
	ORR 	LR, LR, #0x04 		; LR = LR | 0x04; use PSP
	CPSIE 	I 					; OS_EXIT_CRITICAL();
	BX	LR 						; return;

OSCtxSw ;OS context switch
	PUSH	{R4, R5}				
	LDR 	R4, =NVIC_INT_CTRL 			; R4 = NVIC_INT_CTRL
	LDR 	R5, =NVIC_PENDSVSET			; R5 = NVIC_PENDSVSET
	STR 	R5, [R4] 					; *R4 = R5
	POP 	{R4, R5}
	BX 	LR 								; return;
	
	align 4
;void MoveAndStartKernel_Asm(KernelPmt * KP)
;Warning:
; 	This function will destroy the stack of the boot system.
; 	So we must go to running kernel without any withdrawal.
MoveAndStartKernel_Asm
	MOV 	R7, R0
	
	; R0 = KP->MoveKernelTo
	LDR 	R6, [R7] 
	MOV 	R0, R6   
	ADD 	R7, #4
	
	; R1 = KP->MoveKernelFrom
	LDR 	R6, [R7] 
	MOV		R1, R6	 
	ADD 	R7, #4
	
	; R2 = KP->KernelSize
	LDR 	R6, [R7]
	MOV 	R2, R6
	
	; R5 = KP->MoveKernelTo; backup for "StartKernel"
	MOV 	R5, R0
	
MovingLoop
	LDR 	R7, [R1]
	STR 	R7, [R0]
	ADD 	R0, #4
	ADD		R1, #4
	SUB 	R2, #4
	CMP 	R2, #0
	BLE 	MovingComplete
	B 		MovingLoop
MovingComplete

StartKernel
	MOV 	R0, R5
	ORR 	R0, R0, #0x1 ; To avoid UsageFault, LSB must be 1
	MOV 	PC, R0
; Never come here
DeadLoop
	B 		DeadLoop
	
;	align 4
; StartKernel_2
; 	ADD 	R0, #1 
; 	MOV 	PC, R0
	; Never come here
; DeadLoop_2
; 	B 		DeadLoop_2
	
	align 4
	end
