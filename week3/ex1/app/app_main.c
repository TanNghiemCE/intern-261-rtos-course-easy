/* Week 3 / Ex1 -- ISR -> task wakeup, NVIC priority.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "exti_button.h"

void app_main(void)
{
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    exti_button_init();
    BaseType_t check = xTaskCreate(
        ack_task,"ack_task",256,NULL,4,NULL
    );
    configASSERT(check == pdPASS);
    vTaskStartScheduler();

    for (;;) {
    }
}
