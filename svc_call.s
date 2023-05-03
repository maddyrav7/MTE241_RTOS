	AREA	handle_pend,CODE,READONLY
	EXTERN task_switch ;I am going to call a C function to handle the switching
	GLOBAL PendSV_Handler
	GLOBAL SVC_Handler
	PRESERVE8
SVC_Handler
    EXTERN SVC_Handler_Main
    TST LR,#4
    ITE EQ
    MRSEQ r0, MSP
    MRSNE r0, PSP
    B SVC_Handler_Main

PendSV_Handler
    MRS r0,PSP

    ;Store the registers
    STMDB r0!,{r4-r11}

    ;call kernel task switch
    BL task_switch

    MRS r0,PSP ;this is the new task stack
    MOV LR,#0xFFFFFFFD ;magic return value to get us back to Thread mode

    ;LoaD Multiple Increment After, basically undo the stack pushes we did before
    LDMIA r0!,{r4-r11}

    ;Reload PSP. Now that we've popped a bunch, PSP has to be updated
    MSR PSP,r0

    ;return
    BX LR

    END