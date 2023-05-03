#ifndef _THREADS_CORE_H
#define _THREADS_CORE_H

#include <stdint.h>
#include <LPC17xx.h>
#include "type.h"
#include "osConstants.h"

extern thread runnableThreadsHeap[MAX_THREADS];
extern uint8_t runnableThreads;
extern blockedThread blockedThreadsList[MAX_THREADS];
extern uint8_t blockedThreads;

void idle(void *args);

/*
 * Heapify from the node at index i downwards
 */
void heapifyDown(thread *heap, uint8_t heapSize, uint8_t i);

/*
 * Heapify from the node at index i upwards
 */
void heapifyUp(thread *heap, uint8_t i);

/*
 * Push a runnable thread into the heap
 */
int8_t pushRunnableThread(thread th);

/*
 * Pop the top thread from the heap, returning it
 */
thread popRunnableThread(uint8_t i);

int8_t pushBlockedThread(blockedThread bThread);

thread popBlockedThread(uint8_t index);


/*
 * Obtains the initial location of MSP by looking it up in the vector table
 */
uint32_t *getMSPInitialLocation(void);

/*
 * Non-reentrant.
 * Todo: implement locking.
 * Returns the address of the next available PSP.
 */
uint32_t *getNewThreadStack(void);

/*
 * Sets the value of PSP to threadStack.
 * Ensures that the microcontroller is using that value by changing the CONTROL register
 */
void setThreadingWithPSP(uint32_t *threadStack);

/*
 * Creates a new thread and pushes it into the heap of running threads, returning index of new thread
 */
int8_t createThread(void (*threadFunc)(void *args), uint64_t period);

#endif
