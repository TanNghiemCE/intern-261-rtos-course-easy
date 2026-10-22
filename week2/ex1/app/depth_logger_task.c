#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "main.h"
#include "params.h"
#include "pipeline.h"

extern UART_HandleTypeDef huart1;

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