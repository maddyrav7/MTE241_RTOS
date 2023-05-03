#ifndef MTE241_OSCONSTANTS_H
#define MTE241_OSCONSTANTS_H

#include <LPC17xx.h>
#include "stdbool.h"

// Interrupt registers
#define SHPR2 *(uint32_t*)0xE000ED1C
#define SHPR3 *(uint32_t*)0xE000ED20
#define ICSR *(uint32_t*)0xE000ED04

// Define stack constants
#define MSP_ADDR 0x00
#define TOTAL_STACK_SIZE 0x2000
#define THREAD_STACK_SIZE 0x200
#define MSR_STACK_SIZE 0x400

// Threading constructs
#define MAX_THREADS 20 // Sum of running threads and sum of blocked threads
#define SYSTICK_RESOLUTION_S 0.001 // The period of a sysTick tick in seconds
#define SYSTICK_CLOCK_CYCLES (SYSTICK_RESOLUTION_S*SystemCoreClock) // The number of clock cycles in one sysTick tick
#define TIME_SLICE_S 0.005 // The length of the time slice (time till preemption) in seconds
#define TIME_SLICE_TICKS (TIME_SLICE_S*SystemCoreClock/SYSTICK_CLOCK_CYCLES) // The number of sysTick ticks in a time slice

#define IDLE_PERIOD ((0xFFFFFFFFFFFFFFFF - 1)/2) // Set idle deadline as largest possible uint64_t

// Maximum number of mutexes
#define MAX_MUTEX 20

// Conditional compile for switching between test cases
#define TEST_CASE 2

enum threadStatus {
    RUNNABLE, BLOCKED, VOID
};

typedef struct thread_t {
    void (*threadFunc)(void *args); // Pointer to function to execute
    uint8_t threadID;
    uint32_t *stackPtr; // Pointer to stack to use
    uint64_t period;
    uint64_t deadlineTime;
} thread;

enum blockedCondition {
    VOID_BLOCKED, SLEEP, MUTEXED
};

typedef struct blocked_thread_t {
    thread baseThread;
    enum blockedCondition condition;
    union Context {
        uint8_t inUse;
        struct {
            uint64_t wakeTime;
        } sleepContext;
        struct {
            uint8_t mutexIndex;
            uint8_t waitingPosition;
        } mutexContext;
    } context;
} blockedThread;

//VOID thread definition used as a placeholder or to indicate an error
#define VOID_THREAD ((thread) {.threadFunc=0, .threadID = 0, .stackPtr=0, .period=0, .deadlineTime=0})
#define VOID_BLOCKED_THREAD ((blockedThread) {.baseThread = VOID_THREAD, .condition = VOID_BLOCKED, .context.inUse = false})

typedef struct mutex_t {
    uint8_t ownerID;
    uint8_t numWaiting;
} mutex;

#endif //MTE241_OSCONSTANTS_H
