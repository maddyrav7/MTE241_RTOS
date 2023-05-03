#ifndef MTE241__SYSCALL_H
#define MTE241__SYSCALL_H

#include <LPC17xx.h>
#include "_kernelCore.h"

#define SYS_YIELD 0
#define SYS_SLEEP 1
#define SYS_MUTEX_CREATE 2
#define SYS_MUTEX_LOCK 3
#define SYS_MUTEX_UNLOCK 4

extern mutex mutexList[MAX_MUTEX];
extern uint8_t mutexes;

int8_t SVC_Handler_Main(uint32_t *svc_args);

typedef struct sleepParams_t {
    uint64_t sleepTime;
} sleepParams;

int8_t sysCall(uint8_t code, void *args);

#endif //MTE241__SYSCALL_H
