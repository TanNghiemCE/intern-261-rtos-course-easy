/* Week 3 / Ex3 -- command parser: rx_stream -> change demo_task's
 * period/priority/suspend state at runtime.
 *
 * Commands (each terminated by '\r', max MAX_CMD_LEN bytes):
 *   P<ms>\r   -- set demo_task period to <ms>
 *   S<prio>\r -- vTaskPrioritySet(demo_task, <prio>)
 *   T<0|1>\r  -- 0 = vTaskSuspend(demo_task), 1 = vTaskResume(demo_task)
 */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include "main.h"
#include "params.h"
#include "uart_rx.h"
#include "parser_task.h"

extern UART_HandleTypeDef huart1;

static TaskHandle_t s_demo_task_handle;
static volatile uint32_t s_demo_period_ms = 200;
static volatile uint32_t s_cmd_ok;
static volatile uint32_t s_cmd_overflow;

static void apply_command(const char *cmd, size_t len)
{
    /* TODO 1: switch on cmd[0]: 'P' -> parse decimal into s_demo_period_ms
     *         (demo_task reads this every loop, no restart needed);
     *         'S' -> parse decimal, vTaskPrioritySet(s_demo_task_handle, n);
     *         'T' -> cmd[1]=='0' ? vTaskSuspend(...) : vTaskResume(...).
     *         Ignore anything malformed -- do not crash on bad input. */
    (void)cmd;
    (void)len;
    if (cmd == NULL || len < 1) {
        return;
    }
    switch (cmd[0]) {
        case 'P': {
            uint32_t period = 0;
            size_t i = 1;
            while (i < len && cmd[i] >= '0' && cmd[i] <= '9') {
                period = period * 10 + (uint32_t)(cmd[i] - '0');
                i++;
            }
            if (i > 1 && period > 0) {
                s_demo_period_ms = period;
                s_cmd_ok++;
            }
            break;
        }
        case 'S': {
            UBaseType_t prio = 0;
            size_t i = 1;
            while (i < len && cmd[i] >= '0' && cmd[i] <= '9') {
                prio = prio * 10 + (UBaseType_t)(cmd[i] - '0');
                i++;
            }
            if (i > 1 && prio < configMAX_PRIORITIES && s_demo_task_handle != NULL) {
                vTaskPrioritySet(s_demo_task_handle, prio);
                s_cmd_ok++;
            }
            break;
        }
        case 'T': {
            if (len >= 2 && s_demo_task_handle != NULL) {
                if (cmd[1] == '0') {
                    vTaskSuspend(s_demo_task_handle);
                    s_cmd_ok++;
                } else if (cmd[1] == '1') {
                    vTaskResume(s_demo_task_handle);
                    s_cmd_ok++;
                }
            }
            break;
        }
        case 'Q': {
            if (s_demo_task_handle != NULL) {
                char buf[48];
                int n = snprintf(buf, sizeof(buf), "OK:%lu P:%lu S:%lu\r\n",
                                 (unsigned long)s_cmd_ok,
                                 (unsigned long)s_demo_period_ms,
                                 (unsigned long)uxTaskPriorityGet(s_demo_task_handle));
                if (n > 0) {
                    (void)HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)n, 100);
                }
                s_cmd_ok = 0;
            }
            break;
        }
        default:
            break;
    }
}

void parser_task(void *argument)
{
    (void)argument;
    char cmd[MAX_CMD_LEN];
    size_t idx = 0;
    bool discarding = false; 
    for (;;) {
        uint8_t byte;
        if (xStreamBufferReceive(rx_stream, &byte, 1, portMAX_DELAY) == 0) {
        continue;
        }
        if (byte == '\n') {
            continue;
        }
        /* TODO 2: xStreamBufferReceive(rx_stream, &byte, 1, portMAX_DELAY). */

        /* TODO 3: if byte == '\r', call apply_command(cmd, idx), idx = 0.
         *         else if idx < MAX_CMD_LEN - 1, cmd[idx++] = byte.
         *         else (overflow): discard the whole partial command, idx = 0
         *         -- explain in REPORT.md why silently truncating instead
         *         would be worse. */
        (void)byte;
        (void)cmd;
        (void)idx;
        if (byte == '\r') {
        if (!discarding && idx > 0) {
            apply_command(cmd, idx);
        }
        idx = 0;
        discarding = false;
        } 
        else if (discarding) {
        } 
        else if (idx < MAX_CMD_LEN - 1) {
                cmd[idx++] = (char)byte;
        } 
        else {                   
            idx = 0;
            discarding = true;
            s_cmd_overflow++;
        }
    }
}

void demo_task(void *argument)
{
    (void)argument;
    s_demo_task_handle = xTaskGetCurrentTaskHandle();

    for (;;) {
        /* TODO 4: toggle a GPIO pin here. */
        HAL_GPIO_TogglePin(GPIOA, (1U<<7));
        vTaskDelay(pdMS_TO_TICKS(s_demo_period_ms));
    }
}
