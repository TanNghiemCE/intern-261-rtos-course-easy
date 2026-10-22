/* Week 2 / Ex2 — race condition & mutex.
 * Native FreeRTOS API only.
 */
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "uart_guard.h"

extern UART_HandleTypeDef huart1;
void writer_task(void *argument);

void app_main(void)
{
    /* Kích hoạt DWT Cycle Counter ở xung nhịp 72 MHz (72 cycles = 1 us) */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    uart_guard_init();
    char prompt[] = "Press Enter";
    HAL_UART_Transmit(&huart1, (uint8_t *)prompt, sizeof(prompt) - 1, 100);
    uint8_t rx_char = 0;
    HAL_UART_Receive(&huart1, &rx_char, 1, 2000);

    /* xTaskCreate WRITER_COUNT instances of writer_task */
    for (uint32_t i = 0; i < WRITER_COUNT; i++) {
        char task_name[16];
        snprintf(task_name, sizeof(task_name), "w_%lu", (unsigned long)i);
        BaseType_t check = xTaskCreate(writer_task, task_name, 128, (void *)(uintptr_t)i, tskIDLE_PRIORITY + 1, NULL);
        configASSERT(check == pdPASS);
    }

    vTaskStartScheduler();

    for (;;) {
    }
}
