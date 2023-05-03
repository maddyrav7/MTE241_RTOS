#include "_kernelCore.h"
#include "osConstants.h"
#include "_threadsCore.h"
#include "stdio.h"
#include "stdbool.h"
#include "_sysCall.h"

thread runnableThreadsHeap[MAX_THREADS]; //Priority max heap for running threads
uint8_t runnableThreads = 0; // Running thread count

blockedThread blockedThreadsList[MAX_THREADS]; //List of blocked threads, currently unused
uint8_t blockedThreads = 0; //Number of blocked threads

mutex mutexList[MAX_MUTEX]; // List of mutexes
uint8_t mutexes = 0; // Number of mutexes

uint32_t mspAddr;
uint64_t systemClock = 0;

uint8_t tickCounter = 0; //Ticks since time slice start, counting for preemption
bool startCounter = false; // A bool storing whether to start the preemption counter

void idle(void *args) {
    while (1) {
#if TEST_CASE
        static uint32_t printCounter = 0;
        if (printCounter == 0) {
            printf("Idling\n");
        }
        printCounter++;
        printCounter %= 1000000; // Only print once every 1000000 iterations
#endif
    }
}

/*
 * 1. Initializes PendSV, SysTick
 * 2. Creates the idle thread
*/
void kernelInit(void) {
    //set the priority of PendSV to almost the weakest
    SHPR3 |= 0xFE << 16;
    SHPR3 |= 0xFFU << 24; //Set the priority of SysTick to be the weakest
    SHPR2 |= 0xFDU << 24; //Set the priority of SVC the be the strongest
    //initialize the address of the MSP
    uint32_t *MSP_Original = 0;
    mspAddr = *MSP_Original;
    SysTick_Config(SYSTICK_CLOCK_CYCLES);
    // Create the idle thread of priority zero
    createThread(idle, IDLE_PERIOD);
}

/*
 * Called by the SysTick interrupt
 */
void SysTick_Handler(void) {
    // If the preemption tick counter is enabled count the ticks
    if (startCounter)
        tickCounter++;

    systemClock++; //System clock in ticks

    // Track whether any thread has unblocked
    bool hasUnblocked = false;

    // Loop through all blocked threads, unblock based on condition and clock
    for (uint8_t i = 0; i < blockedThreads; i++) {
        blockedThread bThread = blockedThreadsList[i];
        switch (bThread.condition) {
            case SLEEP:
                if (bThread.context.sleepContext.wakeTime <= systemClock) {
                    // If a thread has finished sleeping and has an earlier deadline, reschedule
                    if ((systemClock + bThread.baseThread.period) < activeThread.deadlineTime)
                        hasUnblocked = true;
                }
                break;
            case MUTEXED:
                break;
            case VOID_BLOCKED:
                break;
        }
    }

    // If any thread has unblocked with an earlier deadline, initiate a context switch
    // Also trigger a context switch if the time slice is fully consumed
    if ((tickCounter >= TIME_SLICE_TICKS) || hasUnblocked) {
        activeThread.stackPtr = (uint32_t * )(__get_PSP() - 8 * 4);
        ICSR |= 1 << 28; // Tell the chip that PendSV needs to run
        __asm("isb"); // Run the "isb" assembly instruction to flush the pipeline
    }
}

/*
 * Return: Never
 * Starts executing threads in the runnable heap if any exist
*/
bool osKernelStart(void) {
    if (runnableThreads > 0) {
        __set_CONTROL(0x02);
        __set_PSP((uint32_t)activeThread.stackPtr);
        startCounter = true;
        sysCall(SYS_YIELD, NULL);
    }
    return false;
}

/*
 * Return: 1
 * Determines the next thread to run and switches into it. Also tracks and alters bool polarity to keep
 * track of which tasks have not run this rotation.
 */
int task_switch(void) {
    if (activeThread.period != IDLE_PERIOD) {
        activeThread.deadlineTime = systemClock + activeThread.period;
    }
    heapifyDown(runnableThreadsHeap, runnableThreads, activeThreadIndex);

    // Loop through all blocked threads, unblock based on condition and clock
    // Note that this is slightly repeated code, but is necessary since heap operations should only take place in task_switch
    for (uint8_t i = 0; i < blockedThreads; i++) {
        blockedThread bThread = blockedThreadsList[i];
        switch (bThread.condition) {
            case SLEEP:
                if (bThread.context.sleepContext.wakeTime <= systemClock) {
                    thread rThread = popBlockedThread(i);
                    rThread.deadlineTime = systemClock + rThread.period;
                    pushRunnableThread(rThread);
                    i--;
                }
                break;
            case MUTEXED:
                if (bThread.context.mutexContext.waitingPosition == 0) {
                    thread rThread = popBlockedThread(i);
                    rThread.deadlineTime = systemClock + rThread.period;
                    pushRunnableThread(rThread);
                    i--;
                }
                break;
            case VOID_BLOCKED:
                break;
        }
    }

    //set the new PSP to the highest priority thread
    __set_PSP((uint32_t)activeThread.stackPtr);
    // Reset preemption tick counter
    tickCounter = 0;
    return 1;
}
