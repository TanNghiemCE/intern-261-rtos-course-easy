/* Week 2 / Ex3 — priority inversion vs priority inheritance.
 *
 * task_low  (lowest prio)  holds the shared resource for LOW_HOLD_MS.
 * task_mid  (middle prio)  never touches the resource; pure CPU-bound work,
 *                          its only job is to be ready and preemptible so it
 *                          can steal CPU from task_low while task_low holds
 *                          the resource task_high is waiting for.
 * task_high (highest prio) needs the resource every HIGH_PERIOD_MS and is
 *                          the one whose block time we measure.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "main.h"
#include "params.h"
#include "tasks_lmh.h"
void dwt_init(void);
static SemaphoreHandle_t s_resource;
void resource_init(void)
{
#if RESOURCE_MODE == 0
    /* TODO 1: xSemaphoreCreateBinary(), then give it once so it starts
     *         "available" (binary semaphores start empty). */
     s_resource = xSemaphoreCreateBinary();
     configASSERT(s_resource != NULL);
     configASSERT(xSemaphoreGive(s_resource)==pdTRUE);

    
#else
    /* TODO 2: xSemaphoreCreateMutex(). */
    s_resource = xSemaphoreCreateMutex();
    configASSERT(s_resource != NULL);
#endif
}

void task_low(void *argument)
{
    (void)argument;
    for (;;) {
        /* TODO 3: take s_resource (portMAX_DELAY), toggle a GPIO marking
         *         "resource held", busy-wait LOW_HOLD_MS using the ex2 DWT
         *         helper or a HAL tick-based wait, lower the GPIO, give
         *         s_resource back. Then yield/delay briefly before looping —
         *         this task must not monopolize the CPU outside the hold. */
        xSemaphoreTake(s_resource,portMAX_DELAY);
        HAL_GPIO_WritePin(GPIOA, (uint16_t)(1UL<<0),GPIO_PIN_SET);
        dwt_busy_wait_us(LOW_HOLD_MS * 1000UL); 
        HAL_GPIO_WritePin(GPIOA, (uint16_t)(1UL<<0),GPIO_PIN_RESET);
        xSemaphoreGive(s_resource);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void task_mid(void *argument)
{
    (void)argument;
    for (;;) {
        /* TODO 4: toggle a GPIO, busy-wait MID_WORK_MS, lower the GPIO, then
         *         vTaskDelay a short period so it keeps becoming Ready
         *         periodically rather than running once and blocking
         *         forever. This task never touches s_resource. */
        HAL_GPIO_WritePin(GPIOA, (uint16_t)(1UL<<4),GPIO_PIN_SET);
        dwt_busy_wait_us(MID_WORK_MS * 1000UL);
        HAL_GPIO_WritePin(GPIOA, (uint16_t)(1UL<<4),GPIO_PIN_RESET);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void task_high(void *argument)
{
    (void)argument;
    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        /* TODO 5: toggle a GPIO high (marks "task_high wants the resource"),
         *         take s_resource (portMAX_DELAY), toggle a second GPIO
         *         transition (marks "task_high got it"), do a small amount
         *         of work, give s_resource back, lower the first GPIO.
         *         The gap between the two GPIO edges on the logic analyzer
         *         is your block time t1 - t0. */
        HAL_GPIO_WritePin(GPIOA, (uint16_t)(1UL<<6),GPIO_PIN_SET);
        xSemaphoreTake(s_resource,portMAX_DELAY);
        dwt_busy_wait_us(1000); 
        xSemaphoreGive(s_resource);
        HAL_GPIO_WritePin(GPIOA, (uint16_t)(1UL<<6),GPIO_PIN_RESET);
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(HIGH_PERIOD_MS));
    }
}
