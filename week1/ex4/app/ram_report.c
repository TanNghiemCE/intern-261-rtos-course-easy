/* Week 1 / Ex4 — print the live RAM picture over USART1.
 *
 * This is your measurement instrument. It must be honest: read the values from
 * FreeRTOS, never from a constant you typed in.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include <stdio.h>
#include <string.h>
#include "timers.h"
/* TODO 1: a blocking UART print helper.+so it may
 *         block — but say in REPORT.md what would break if you called it from
 *         the stack overflow hook instead. */
extern UART_HandleTypeDef huart1;
extern TaskHandle_t dyn_task;
extern TaskHandle_t stat_task;
extern TaskHandle_t report_task;
extern TaskHandle_t extra_task;

static void uart_print(const char *message){
    HAL_UART_Transmit(&huart1, (uint8_t *)message , strlen(message) , 200);
}

void ram_report_task(void *argument)
{
    (void)argument;
    char buf[128];
    for (;;) {
        /* TODO 2: print, every 2 seconds:
         *   - xPortGetFreeHeapSize()
         *   - xPortGetMinimumEverFreeHeapSize()
         *   - uxTaskGetStackHighWaterMark() for: dyn_task, stat_task,
         *     ram_report_task, the idle task (xTaskGetIdleTaskHandle()),
         *     and the timer task (xTimerGetTimerDaemonTaskHandle()).
         *
         * High water mark is returned in WORDS. Print both words and bytes so
         * your report cannot silently mix the two units. */
         size_t free_heap = xPortGetFreeHeapSize();
         size_t min_ever_free_heap = xPortGetMinimumEverFreeHeapSize();
         uart_print("\r\n--- FREE RTOS RAM REPORT ---\r\n");
         snprintf(buf, sizeof(buf), "Free Heap: %u bytes (Target >= %u bytes: %s)\r\n",
                  (unsigned int)free_heap,
                  (unsigned int)TARGET_FREE_HEAP_BYTES,
                  (free_heap >= TARGET_FREE_HEAP_BYTES) ? "PASS" : "FAIL");
         uart_print(buf);
         snprintf(buf, sizeof(buf), "Min Ever Free Heap: %u bytes\r\n", (unsigned int)min_ever_free_heap);
         uart_print(buf);
         /* In High Water Mark cho từng task (in cả WORDS và BYTES) */
         TaskHandle_t idle_handle = xTaskGetIdleTaskHandle();
         TaskHandle_t timer_handle = xTimerGetTimerDaemonTaskHandle();
         UBaseType_t hwm_dyn    = uxTaskGetStackHighWaterMark(dyn_task);
         UBaseType_t hwm_stat   = uxTaskGetStackHighWaterMark(stat_task);
         UBaseType_t hwm_report = uxTaskGetStackHighWaterMark(report_task);
         UBaseType_t hwm_idle   = idle_handle ? uxTaskGetStackHighWaterMark(idle_handle) : 0;
         UBaseType_t hwm_timer  = timer_handle ? uxTaskGetStackHighWaterMark(timer_handle) : 0;
         if (extra_task != NULL) {
             UBaseType_t hwm_extra = uxTaskGetStackHighWaterMark(extra_task);
             snprintf(buf, sizeof(buf), "HWM extra_task:      %3u words (%4u bytes)\r\n", 
             (unsigned int)hwm_extra, (unsigned int)(hwm_extra * 4));
             uart_print(buf);
         }
         snprintf(buf, sizeof(buf), "HWM dyn_task:        %3u words (%4u bytes)\r\n", (unsigned int)hwm_dyn, (unsigned int)(hwm_dyn * 4));
         uart_print(buf);
         snprintf(buf, sizeof(buf), "HWM stat_task:       %3u words (%4u bytes)\r\n", (unsigned int)hwm_stat, (unsigned int)(hwm_stat * 4));
         uart_print(buf);
         snprintf(buf, sizeof(buf), "HWM ram_report_task: %3u words (%4u bytes)\r\n", (unsigned int)hwm_report, (unsigned int)(hwm_report * 4));
         uart_print(buf);
         snprintf(buf, sizeof(buf), "HWM idle_task:       %3u words (%4u bytes)\r\n", (unsigned int)hwm_idle, (unsigned int)(hwm_idle * 4));
         uart_print(buf);
         snprintf(buf, sizeof(buf), "HWM timer_task:      %3u words (%4u bytes)\r\n", (unsigned int)hwm_timer, (unsigned int)(hwm_timer * 4));
         uart_print(buf);
         vTaskDelay(pdMS_TO_TICKS(2000));
    }
}