/* Week 1 / Ex2 — priorities, preemption, starvation.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "tasks.h"
void app_main(void)
{
    dwt_init();

    /* TODO: create the three tasks below with the priorities from params.h.
     *       Keep the stacks as small as you can still justify — ex4 will make
     *       you prove the number. */
    BaseType_t taskA = xTaskCreate( task_a, "taskA", 128,NULL,TASK_A_PRIO, NULL);
    BaseType_t taskB = xTaskCreate( task_b, "taskB", 128,NULL,TASK_B_PRIO, NULL);
    BaseType_t taskC = xTaskCreate( task_c, "taskC", 128,NULL,TASK_C_PRIO, NULL);
    configASSERT(taskA == pdPASS);
    configASSERT(taskB == pdPASS);
    configASSERT(taskC == pdPASS);
    vTaskStartScheduler();

    for (;;) {
    }
}
