/* Week 1 / Ex3 — entry point. */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include <stdint.h>

void periodic_task(void *argument);
void dwt_init(void);
//task nho
volatile uint32_t idle_count = 0;
volatile uint8_t done = 0;
void idle_measure_task(void *argument){
    (void)argument;
    uint32_t start = DWT->CYCCNT;
    uint32_t count = 0;
    while ((DWT->CYCCNT - start)< 72000000UL) {
        count++;
    }
    idle_count = count;
    done = 1;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
void app_main(void)
{
    dwt_init();

    /* TODO: create periodic_task. Its priority must be the highest in the system
     *       — justify why in REPORT.md, and say what the measurement would look
     *       like if it were not. */
     BaseType_t check = xTaskCreate(
        periodic_task, "periodic_task", 128 , NULL , 4 , NULL
     );
     BaseType_t check_tiny_task = xTaskCreate(
        idle_measure_task,"idle_measure_task", 128, NULL, tskIDLE_PRIORITY,NULL
     );
     configASSERT(check==pdPASS);
     configASSERT(check_tiny_task==pdPASS);
    vTaskStartScheduler();
    for (;;) {
    }
}
