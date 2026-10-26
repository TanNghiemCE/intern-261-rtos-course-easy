/* Week 2 / Ex3 — priority inversion.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "tasks_lmh.h"
void dwt_init(void);
void app_main(void)
{
    dwt_init();
    resource_init();
    /* TODO: xTaskCreate task_low, task_mid, task_high with three DISTINCT
     *       priorities, low < mid < high. Justify the exact numbers in
     *       REPORT.md — they must leave room for configMAX_PRIORITIES and not
     *       collide with any other task in the system. */
    BaseType_t check_task_low = xTaskCreate(
        task_low, "task_low",128,NULL,tskIDLE_PRIORITY + 1,NULL);
    BaseType_t check_task_mid = xTaskCreate(
        task_mid, "task_mid",128,NULL,tskIDLE_PRIORITY + 2,NULL);
    BaseType_t check_task_high = xTaskCreate(
        task_high, "task_high",128,NULL,tskIDLE_PRIORITY + 3,NULL);
    configASSERT(check_task_low == pdTRUE);
    configASSERT(check_task_mid == pdTRUE);
    configASSERT(check_task_high == pdTRUE);
    vTaskStartScheduler();
    for (;;) {
    }
}
