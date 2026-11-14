/* Week 3 / Ex4 -- run-time stats on DWT CYCCNT.
 * Native FreeRTOS API only.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "runtime_stats.h"

extern UART_HandleTypeDef huart1;

void runtime_stats_timer_init(void)
{
    /* TODO 1: same DWT enable sequence as week1/ex2 dwt_init() (CoreDebug
     *         DEMCR TRCENA bit, zero DWT->CYCCNT, DWT->CTRL CYCCNTENA bit).
     *         If you already have week1/ex2's dwt_init(), the sequence is
     *         identical -- do not invent a second mechanism. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t runtime_stats_timer_value(void)
{
    /* TODO 2: return DWT->CYCCNT. This is portGET_RUN_TIME_COUNTER_VALUE()'s
     *         implementation -- wire it up in FreeRTOSConfig.deltas. */
    return DWT->CYCCNT;
}

void runtime_stats_task(void *argument)
{
    (void)argument;
    static char buf[512];
    const char *header = "\r\nTask          Abs Time        % Time\r\n"
                         "--------------------------------------\r\n";

    for (;;) {
        /* TODO 3: vTaskGetRunTimeStats(buf) and print buf over UART.
         *         Note: this walks every task's TCB -- it is itself not free,
         *         factor that into why STATS_WINDOW_MS should not be too
         *         short. */
        vTaskGetRunTimeStats(buf);
        TaskStatus_t statuses[8];
        UBaseType_t n = uxTaskGetSystemState(statuses, 8, NULL);
        for (UBaseType_t i = 0; i < n; i++) {
        char line[64];
        int len = snprintf(line, sizeof(line), "%s: state=%d\r\n",
                        statuses[i].pcTaskName, (int)statuses[i].eCurrentState);
        if (len > 0) {
            HAL_UART_Transmit(&huart1, (uint8_t *)line, (uint16_t)len, 100);
        }
        }
        HAL_UART_Transmit(&huart1, (uint8_t *)header, (uint16_t)strlen(header), 200);
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)strlen(buf), 500);
        vTaskDelay(pdMS_TO_TICKS(STATS_WINDOW_MS));
    }
}
