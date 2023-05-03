//This file contains relevant pin and other settings 
#include <LPC17xx.h>

//This file is for printf and other IO functions
#include "stdio.h"

//this file sets up the UART
#include "uart.h"

// Include our new thread and kernel files
#include "_threadsCore.h"
#include "_kernelCore.h"
#include "_sysCall.h"

#if TEST_CASE
int x = 0;
int y = 0;
int z = 0;
#endif

#if TEST_CASE == 1
uint8_t mutex1;
void task1(void *args) {
    while (1) {
        x++;
        sysCall(SYS_MUTEX_LOCK, &mutex1);
        printf("In task 1. x is: %d\n", x);
        sysCall(SYS_MUTEX_UNLOCK, &mutex1);
        sysCall(SYS_YIELD, NULL);
    }
}

void task2(void *args) {
    while (1) {
        y++;
        sysCall(SYS_MUTEX_LOCK, &mutex1);
        printf("In task 2. y is: %d\n", y);
        sysCall(SYS_MUTEX_UNLOCK, &mutex1);
        sysCall(SYS_YIELD, NULL);
    }
}

void task3(void *args) {
    while (1) {
        z++;
        sysCall(SYS_MUTEX_LOCK, &mutex1);
        printf("In task 3. z is: %d\n", z);
        sysCall(SYS_MUTEX_UNLOCK, &mutex1);
        sysCall(SYS_YIELD, NULL);
    }
}
#endif

#if TEST_CASE == 2

// LED CODE FROM PRELIMIARY PROJECT
#define GPIO1_LEDS_SIZE 3
#define GPIO2_LEDS_SIZE 5
#define GPIO1_LED_NUMS (int[]){28, 29, 31}
#define GPIO2_LED_NUMS (int[]){2, 3, 4, 5, 6}

void Turn_On_LEDS(unsigned int led_nums) {
    for (int i = 0; i < GPIO1_LEDS_SIZE; i++) {
        int is_led_set = led_nums & (1 << i);
        LPC_GPIO1->FIOCLR |= 1 << GPIO1_LED_NUMS[i];
        if (is_led_set != 0)
            LPC_GPIO1->FIOSET |= 1 << GPIO1_LED_NUMS[i];
    }

    for (int i = 0; i < GPIO2_LEDS_SIZE; i++) {
        int is_led_set = led_nums & (1 << (GPIO1_LEDS_SIZE + i));
        LPC_GPIO2->FIOCLR |= 1 << GPIO2_LED_NUMS[i];
        if (is_led_set != 0)
            LPC_GPIO2->FIOSET |= 1 << GPIO2_LED_NUMS[i];
    }
}

uint8_t mutex1;
uint8_t mutex2;

void task1(void *args) {
    while (1) {
        sysCall(SYS_MUTEX_LOCK, &mutex1);
        x++;
        sysCall(SYS_MUTEX_UNLOCK, &mutex1);
        printf("In task 1. x is: %d\n", x);
        sysCall(SYS_YIELD, NULL);
    }
}

void task2(void *args) {
    while (1) {
        sysCall(SYS_MUTEX_LOCK, &mutex1);
        sysCall(SYS_MUTEX_LOCK, &mutex2);
        Turn_On_LEDS(x % 47);
        printf("In task 2. x %% 47 is: %d\n", x % 47);
        sysCall(SYS_MUTEX_UNLOCK, &mutex1);
        sysCall(SYS_MUTEX_UNLOCK, &mutex2);
        sysCall(SYS_YIELD, NULL);
    }
}

void task3(void *args) {
    while (1) {
        sysCall(SYS_MUTEX_LOCK, &mutex2);
        Turn_On_LEDS(0x71);
        printf("In task 3. Switching LEDs\n");
        sysCall(SYS_MUTEX_UNLOCK, &mutex2);
        sysCall(SYS_YIELD, NULL);
    }
}

#endif

int main(void) {
    //Always call this function at the start. It sets up various peripherals, the clock etc. If you don't call this
    //you may see some weird behaviour
    SystemInit();

    //Printf now goes to the UART, so be sure to have PuTTY open and connected
    printf("Hello, world!\r\n");
    uint32_t *initMSP = getMSPInitialLocation();
    //Let's see what it is
    printf("MSP Initially: %x\n", (uint32_t) initMSP);

    kernelInit();

#if TEST_CASE == 1
    mutex1 = sysCall(SYS_MUTEX_CREATE, NULL);
    createThread(task1, 7);
    createThread(task2, 7);
    createThread(task3, 7);
#endif

#if TEST_CASE == 2
    //Set GPIO1 LED directions
    for (int i = 0; i < GPIO1_LEDS_SIZE; i++)
        LPC_GPIO1->FIODIR |= (1 << GPIO1_LED_NUMS[i]);

    for (int i = 0; i < GPIO2_LEDS_SIZE; i++)
        LPC_GPIO2->FIODIR |= (1 << GPIO2_LED_NUMS[i]);

    mutex1 = sysCall(SYS_MUTEX_CREATE, NULL);
    mutex2 = sysCall(SYS_MUTEX_CREATE, NULL);

    createThread(task1, 7);
    createThread(task2, 7);
    createThread(task3, 7);
#endif

    //Now start the kernel, which will run our first thread
    osKernelStart();

    while (1);
}
