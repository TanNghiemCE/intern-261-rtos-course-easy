/* Week 2 / Ex1 — sensor pipeline: sensor_task -> queue_raw -> filter_task ->
 * queue_filtered -> uart_task.
 *
 * Native FreeRTOS API only.
 */
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "main.h"
#include "params.h"
#include "pipeline.h"

extern UART_HandleTypeDef huart1;

QueueHandle_t queue_raw;
QueueHandle_t queue_filtered;

static TaskHandle_t s_sensor_handle = NULL;
static TaskHandle_t s_uart_handle   = NULL;

void app_main(void)
{
    /* TODO 1: xQueueCreate(QUEUE_LENGTH, sizeof(raw_sample_t)) for queue_raw,
     *         and the equivalent for queue_filtered with filtered_sample_t.
     *         Check both handles are non-NULL before continuing. */
    queue_raw      = xQueueCreate(QUEUE_LENGTH, sizeof(raw_sample_t));
    queue_filtered = xQueueCreate(QUEUE_LENGTH, sizeof(filtered_sample_t));
    configASSERT(queue_raw != NULL);
    configASSERT(queue_filtered != NULL);

    /* --- CÁCH A: Chờ PC sẵn sàng ---
     * Bật terminal, bật ghi log trên PC xong -> Gõ phím bất kỳ (hoặc Enter) để bắt đầu.
     */
    char prompt[] = "Press ENTER to start logging...\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t *)prompt, sizeof(prompt) - 1, 100);
    uint8_t rx_char = 0;
    HAL_UART_Receive(&huart1, &rx_char, 1, HAL_MAX_DELAY);
    
    /* TODO 2: xTaskCreate sensor_task, filter_task, uart_task.
     *         Pick priorities you can justify in REPORT.md: who must never be
     *         starved by whom? */
    xTaskCreate(sensor_task, "sensor", 256, NULL, 3, &s_sensor_handle);
    xTaskCreate(filter_task, "filter", 256, NULL, 2, NULL);
    xTaskCreate(uart_task,   "uart",   256, NULL, 1, &s_uart_handle);

    /* TODO 3: xTaskCreate depth_logger_task — must not perturb the pipeline it
     *         is measuring (think about its own priority and period). */
    xTaskCreate(depth_logger_task, "depth", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    for (;;) {
    }
}

