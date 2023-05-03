#ifndef KERNEL_CORE_H
#define KERNEL_CORE_H

#include <stdint.h>
#include <LPC17xx.h>
#include <stdbool.h>
#include "_threadsCore.h"

extern uint32_t mspAddr;
extern uint64_t systemClock;

#define activeThreadIndex 0
#define activeThread (runnableThreadsHeap[activeThreadIndex])

/*
 * Initializes memory structures and interrupts necessary to run the kernel
*/
void kernelInit(void);

/*
 * Starts executing threads in the runnable heap if any exist
*/
bool osKernelStart(void);

int8_t findPriorityThread(uint8_t i, bool polarity);

/*
 * Called by the interrupt as part of context switching.
 * Sets the new PSP to the next thread to run.
 */
int task_switch(void);

#endif
