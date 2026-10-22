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

#define DEPTH_LOG_MAX 300
typedef struct {
    uint32_t tick_ms;
    uint8_t  raw;
    uint8_t  filt;
} depth_entry_t;

static depth_entry_t s_depth_log[DEPTH_LOG_MAX];
static uint16_t s_depth_count = 0;
static uint8_t s_dumped = 0;

void depth_logger_task(void *argument)
{
    (void)argument;
    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        UBaseType_t raw  = uxQueueMessagesWaiting(queue_raw);
        UBaseType_t filt = uxQueueMessagesWaiting(queue_filtered);

        /* Giai đoạn thu thập: Lưu độ đầy vào RAM mỗi 50ms */
        if (s_depth_count < DEPTH_LOG_MAX) {
            s_depth_log[s_depth_count].tick_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
            s_depth_log[s_depth_count].raw     = (uint8_t)raw;
            s_depth_log[s_depth_count].filt    = (uint8_t)filt;
            s_depth_count++;
        } 
        /* Sau 15 giây (thu thập đủ 300 mẫu): Tạm dừng pipeline và xuất CSV ra UART1 */
        else if (!s_dumped) {
            s_dumped = 1;

            /* Tạm dừng sensor_task và uart_task để tránh xung đột dữ liệu trên UART1 */
            if (s_sensor_handle != NULL) {
                vTaskSuspend(s_sensor_handle);
            }
            if (s_uart_handle != NULL) {
                vTaskSuspend(s_uart_handle);
            }
            vTaskDelay(pdMS_TO_TICKS(50));
            
            char line[64];
            int n = snprintf(line, sizeof(line), "\r\n=== DEPTH LOG DUMP START ===\r\n");
            HAL_UART_Transmit(&huart1, (uint8_t *)line, (uint16_t)n, 200);
            
            n = snprintf(line, sizeof(line), "tick_ms,raw_depth,filt_depth\r\n");
            HAL_UART_Transmit(&huart1, (uint8_t *)line, (uint16_t)n, 200);
            for (uint16_t i = 0; i < DEPTH_LOG_MAX; i++) {
                n = snprintf(line, sizeof(line), "%lu,%u,%u\r\n",
                             (unsigned long)s_depth_log[i].tick_ms,
                             (unsigned int)s_depth_log[i].raw,
                             (unsigned int)s_depth_log[i].filt);
                HAL_UART_Transmit(&huart1, (uint8_t *)line, (uint16_t)n, 200);
            }
            n = snprintf(line, sizeof(line), "=== DEPTH LOG DUMP END ===\r\n");
            HAL_UART_Transmit(&huart1, (uint8_t *)line, (uint16_t)n, 200);

            /* Kết thúc nhiệm vụ ghi log */
            vTaskDelete(NULL);
        }
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(50));
    }
}


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

