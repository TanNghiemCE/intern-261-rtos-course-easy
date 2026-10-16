/* Week 1 / Ex2 — the three competing tasks.
 *
 * Each task: raise its pin, burn exactly TASK_x_WORK_US of CPU, lower its pin,
 * and loop again WITHOUT blocking. The missing block is the point of the
 * exercise, not a bug to fix in step 2.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include <stdint.h>
#include "tasks.h"

/* TODO: one helper that takes a port/pin and a work time, then three thin
 *       wrappers. Do not copy-paste the body three times. */
#define GPIO_PIN0 (uint16_t)(1UL<<TASK_A_PIN)
#define GPIO_PIN4 (uint16_t)(1UL<<TASK_B_PIN)
#define GPIO_PIN6 (uint16_t)(1UL<<TASK_C_PIN)

static void task_body(uint16_t pin, uint32_t work_hours){
        HAL_GPIO_WritePin(GPIOA, pin, GPIO_PIN_SET);
        dwt_busy_wait_us(work_hours);
        HAL_GPIO_WritePin(GPIOA, pin, GPIO_PIN_RESET);
}
void task_a(void *argument)
{
    (void)argument;
    for (;;) {
        task_body(GPIO_PIN0, TASK_A_WORK_US);
        // vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void task_b(void *argument)
{
    (void)argument;
    for (;;) {
       task_body(GPIO_PIN4, TASK_B_WORK_US); 
    }
}

void task_c(void *argument)
{
    (void)argument;
    for (;;) {
        task_body(GPIO_PIN6, TASK_C_WORK_US);
        // vTaskDelay(pdMS_TO_TICKS(5));
    }
}
