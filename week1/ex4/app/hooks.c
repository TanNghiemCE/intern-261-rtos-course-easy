/* Week 1 / Ex4 — FreeRTOS hooks.
 *
 * Both hooks must be observable from outside the chip. A breakpoint is not a
 * deliverable: you need a UART line or a distinct LED pattern you can photograph.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include <string.h>

extern UART_HandleTypeDef huart1;

static void uart_print(const char *message){
    while (*message) {
        while (!(USART1->SR & USART_SR_TXE)) {
        }
        USART1->DR = (uint8_t)(*message++);
    }
}
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;

    uart_print("\r\n!!! STACK OVERFLOW: ");
    if (pcTaskName != NULL) {
        uart_print(pcTaskName);
    }
    uart_print(" !!!\r\n");

    for (;;) {
    }
}

void vApplicationMallocFailedHook(void)
{
    /* TODO 2: report, then stop. */
    uart_print("\r\n Malloc Failed!\r\n");
    for (;;) {
    }
}

/* configSUPPORT_STATIC_ALLOCATION = 1 makes these two callbacks mandatory.
 * Provide the memory for the idle task and the timer task here. */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
