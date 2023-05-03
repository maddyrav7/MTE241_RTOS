#include <stdint.h>
#include <LPC17xx.h>
#include "_threadsCore.h"
#include "_kernelCore.h"
#include "osConstants.h"

// Create a global variable pointer to 0x00
uint32_t **MSP_PTR = (uint32_t **) MSP_ADDR;

void swapThreads(thread *a, thread *b) {
    thread temp = *a;
    *a = *b;
    *b = temp;
}

/*
 * Heapify from the node at index i downwards
 */
void heapifyDown(thread *heap, uint8_t heapSize, uint8_t i) {
    uint8_t left = 2 * i + 1;
    uint8_t right = left + 1;
    uint8_t soonest = i;
    if (left < heapSize && heap[left].deadlineTime < heap[i].deadlineTime) {
        soonest = left;
    }
    if (right < heapSize && heap[right].deadlineTime < heap[soonest].deadlineTime) {
        soonest = right;
    }
    if (soonest != i) {
        swapThreads(&heap[soonest], &heap[i]);
        heapifyDown(heap, heapSize, soonest);
    }
}

/*
 * Heapify from the node at index i upwards
 */
void heapifyUp(thread *heap, uint8_t i) {
    uint8_t parent = (i - 1) / 2;
    if (i != 0 && heap[parent].deadlineTime > heap[i].deadlineTime) {
        swapThreads(&heap[parent], &heap[i]);
        heapifyUp(heap, parent);
    }
}

/*
 * Push a runnable thread into the heap
 */
int8_t pushRunnableThread(thread th) {
    if (runnableThreads + blockedThreads >= MAX_THREADS) {
        return -1;
    }
    runnableThreadsHeap[runnableThreads] = th;
    heapifyUp(runnableThreadsHeap, runnableThreads);
    runnableThreads++;
    return 0;
}

/*
 * Pop the thread with index i from the heap, returning it
 */
thread popRunnableThread(uint8_t i) {
    if (!runnableThreads) {
        return VOID_THREAD;
    }
    // Swap the thread to pop with the last running thread, then heapify down
    uint8_t lastThreadIndex = runnableThreads - 1;
    thread threadToPop = runnableThreadsHeap[i];
    runnableThreadsHeap[i] = runnableThreadsHeap[lastThreadIndex];
    runnableThreadsHeap[lastThreadIndex] = VOID_THREAD;
    runnableThreads--;
    heapifyDown(runnableThreadsHeap, runnableThreads, i);
    return threadToPop;
}

/*
 * Push a new blocked thread to the blocked thread array
 */
int8_t pushBlockedThread(blockedThread bThread) {
    if (blockedThreads == MAX_THREADS)
        return -1;

    blockedThreadsList[blockedThreads] = bThread;
    blockedThreads++;
    return 0;
}

/*
 * Pop a blocked thread of the given index
 */
thread popBlockedThread(uint8_t index) {
    if (blockedThreads == 0)
        return VOID_THREAD;
    // Swap blocked thread with last thread, then delete last thread
    thread bThread = blockedThreadsList[index].baseThread;
    blockedThreadsList[index] = blockedThreadsList[blockedThreads - 1];
    blockedThreadsList[blockedThreads - 1] = VOID_BLOCKED_THREAD;
    blockedThreads--;
    return bThread;
}

/*
 * Obtains the initial location of MSP by looking it up in the vector table
 */
uint32_t *getMSPInitialLocation(void) {
    return *MSP_PTR;
}

/*
 * Non-reentrant.
 * Returns the address of the next available PSP.
 */
uint32_t *getNewThreadStack() {
    static uint32_t newThreadLocation = 0; // Tracks current offset
    if (newThreadLocation) {
        newThreadLocation -= THREAD_STACK_SIZE;
    }
    else {
        newThreadLocation = (uint32_t) getMSPInitialLocation() - MSR_STACK_SIZE;
    }
    // Round for stack alignment
    if (newThreadLocation % 8)
        newThreadLocation += sizeof(uint32_t);

    // Return the address after offset
    return (uint32_t *) newThreadLocation;
}

/*Sets the value of
PSP to threadStack and ensures that the microcontroller is using that
value by changing the CONTROL register*/
void setThreadingWithPSP(uint32_t *threadStack) {

    // Set the PSP pointer to the given stack
    __set_PSP((uint32_t) threadStack);

    // Set the CONTROL register to go to thread mode
    __set_CONTROL(0x02);
}

/*
 * Creates a new thread and pushes it into the heap of running threads, returning index of new thread
 */
int8_t createThread(void (*threadFunc)(void *args), uint64_t period) {
    // Track thread IDs
    static uint8_t threadID = 1;

    // Setting up the thread stack
    uint32_t *threadStack = getNewThreadStack();
    if (!threadStack) {
        return -1;
    }
    *--threadStack = 1 << 24; // Store Xspr
    *--threadStack = (uint32_t) threadFunc; //Setting Program counter
    //Setting LR, R12, R3, R2, R1, R0, R11, R10, R9, R8, R7, R6, R5, R4 in that order with arbitrary values
    for (uint8_t i = 0; i < 14; i++) {
        *--threadStack = i;
    }

    // Creates a newThread with the given function pointer and newly initialized thread stack
    // All threads have a priority of 1 for now
    thread newThread = {.threadFunc=threadFunc, .threadID = threadID, .stackPtr=threadStack, .period = period, .deadlineTime =
    systemClock + period};
    threadID++;
    if (pushRunnableThread(newThread) == -1) {
        return -1;
    }
    return runnableThreads - 1;
}
