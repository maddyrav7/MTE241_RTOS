#include "_sysCall.h"

int8_t SVC_Handler_Main(uint32_t *svc_args) {
    char call = ((char *) svc_args[6])[-2];
    if (call == SYS_SLEEP) {
        ICSR |= 1 << 28; // Tell the chip that PendSV needs to run
        __asm("isb");
        return 0;
    }
    else if (call == SYS_YIELD) {
        /*
     * Switches to and runs the next thread defined by the priority heap.
     */
        static bool firstCall = true;
        if (firstCall) {
            firstCall = false;
        }
        else {
            activeThread.stackPtr = (uint32_t * )(__get_PSP() - 8 * 4);
        }
        ICSR |= 1 << 28; // Tell the chip that PendSV needs to run
        __asm("isb"); // Run the "isb" assembly instruction to flush the pipeline
    }
    else
        return -1;

    return 0;
}


int8_t sysCall(uint8_t code, void *args) {
    if (code == SYS_YIELD) {
        __ASM("SVC #0");
    }
    else if (code == SYS_SLEEP) {
        uint64_t ms = ((sleepParams *) args)->sleepTime;
        uint64_t ticksToSleep = ms / (1000 * SYSTICK_RESOLUTION_S);
        // Create blocked thread
        blockedThread bThread = {.baseThread=activeThread, .condition=SLEEP,
                .context.sleepContext={.wakeTime=systemClock + ticksToSleep}};

        // Push thread into blocked threads array, remove from priority queue
        if (pushBlockedThread(bThread) == -1)
            return -1;
        popRunnableThread(activeThreadIndex);
        // Offset the stackPtr stored in the blocked threads array
        blockedThreadsList[blockedThreads - 1].baseThread.stackPtr = (uint32_t * )(
                __get_PSP() - 16 * 4);
        __ASM("SVC #1");
    }
    else if (code == SYS_MUTEX_CREATE) {
        // Create a new mutex and store it in the mutex list
        // Create the new mutex
        mutex newMutex = (mutex) {.ownerID = VOID_THREAD.threadID, .numWaiting = 0};

        // If the list of mutexes if full, return an error
        if (mutexes >= MAX_MUTEX)
            return -1;

        mutexList[mutexes] = newMutex;
        mutexes++;
        return mutexes-1;
    }
    else if (code == SYS_MUTEX_LOCK) {
        int8_t index = *((int8_t *) args);

        // If the mutex does not have an owner (is unlocked), lock it
        if (mutexList[index].ownerID == VOID_THREAD.threadID)
            mutexList[index].ownerID = activeThread.threadID;
        else {
            mutexList[index].numWaiting++;
            blockedThread bThread = {.baseThread=activeThread, .condition=MUTEXED,
                    .context.mutexContext = {.mutexIndex = index, .waitingPosition = mutexList[index].numWaiting}};
            // Push thread into blocked threads array, remove from priority queue
            if (pushBlockedThread(bThread) == -1)
                return -1;
            popRunnableThread(activeThreadIndex);
            // Offset the stackPtr stored in the blocked threads array
            blockedThreadsList[blockedThreads - 1].baseThread.stackPtr = (uint32_t * )(
                    __get_PSP() - 16 * 4);
            __ASM("SVC #1");
        }
    }
    else if (code == SYS_MUTEX_UNLOCK) {
        int8_t index = *((int8_t *) args);

        // If the mutex is already unlocked, or the owner does not match, return an error
        if (mutexList[index].ownerID != activeThread.threadID)
            return -1;
        else if (mutexList[index].numWaiting == 0) {
            mutexList[index].ownerID = VOID_THREAD.threadID;
            return 0;
        }
        else {
            for (uint8_t i = 0; i < blockedThreads; i++) {
                blockedThread bThread = blockedThreadsList[i];

                if (bThread.condition == MUTEXED &&
                    bThread.context.mutexContext.mutexIndex == index) {
                    bThread.context.mutexContext.waitingPosition--;
                    if (bThread.context.mutexContext.waitingPosition == 0) {
                        mutexList[index].ownerID = bThread.baseThread.threadID;
                        mutexList[index].numWaiting--;
                    }
                }
            }
        }
    }
    else
        return -1;

    return 0;
}
