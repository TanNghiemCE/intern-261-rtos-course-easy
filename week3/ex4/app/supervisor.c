/* Week 3 / Ex4 -- supervisor task: waits for all monitored tasks to check in
 * via an event group every cycle; refreshes IWDG only if they all did.
 * Native FreeRTOS API only.
 */
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#include "main.h"
#include "params.h"
#include "supervisor.h"

extern UART_HandleTypeDef huart1;

EventGroupHandle_t supervisor_events;
static IWDG_HandleTypeDef hiwdg;
#define ALL_BITS ((1u << SUPERVISOR_BITS) - 1u)

volatile uint32_t g_last_iwdg_refresh_tick = 0;

void supervisor_init(void)
{
    /* TODO 1: supervisor_events = xEventGroupCreate(); check non-NULL.
     *
     * TODO 2: HAL IWDG init here (IWDG_TIMEOUT_MS -> prescaler + reload per
     *         RM0008 IWDG chapter; IWDG runs off LSI, ~40 kHz, compute the
     *         reload value and show the computation in REPORT.md). */
    supervisor_events = xEventGroupCreate();
    configASSERT(supervisor_events != NULL);

    hiwdg.Instance       = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_4;
    hiwdg.Init.Reload    = 2499;
    HAL_StatusTypeDef st = HAL_IWDG_Init(&hiwdg);
    configASSERT(st == HAL_OK);
    (void)st;
}

void heartbeat_task(void *argument)
{
    (void)argument;

    for (;;) {
        supervisor_checkin(SUPERVISOR_BIT_HEARTBEAT);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void supervisor_task(void *argument)
{
    (void)argument;
    static char msg[160];
    uint8_t logged_slow_refresh = 0;

    for (;;) {
        /* TODO 3: xEventGroupWaitBits(supervisor_events, ALL_BITS, pdTRUE,
         *         pdTRUE, <timeout>) -- clear-on-exit, wait-for-all.
         *
         * TODO 4: if the wait actually collected ALL_BITS,
         *         HAL_IWDG_Refresh(&hiwdg). Otherwise do NOTHING. */
        EventBits_t bits = xEventGroupWaitBits(
            supervisor_events,
            ALL_BITS,
            pdTRUE,
            pdTRUE,
            pdMS_TO_TICKS(100)
        );
        if ((bits & ALL_BITS) == ALL_BITS) {
            uint32_t now = (uint32_t)xTaskGetTickCount();
            uint32_t dt_refresh = now - g_last_iwdg_refresh_tick;
            HAL_IWDG_Refresh(&hiwdg);
            g_last_iwdg_refresh_tick = now;

            if (!logged_slow_refresh && now >= 2200 && dt_refresh >= 40) {
                logged_slow_refresh = 1;
                int len = snprintf(msg, sizeof(msg),
                                   "[SUPERVISOR-Q6] IWDG Refreshed OK at tick=%lu ms | dt_refresh=%lu ms (< 100 ms wait, < 250 ms IWDG) -> NO FALSE-TRIP!\r\n",
                                   (unsigned long)now,
                                   (unsigned long)dt_refresh);
                if (len > 0) {
                    HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)len, 50);
                }
            }
        } else {
            uint32_t now = (uint32_t)xTaskGetTickCount();
            int len = snprintf(msg, sizeof(msg),
                               "[SUPERVISOR] Timeout! bits=0x%lX (expected 0x%X) at tick=%lu ms (last_refresh=%lu ms) -> NO IWDG REFRESH\r\n",
                               (unsigned long)bits, (unsigned int)ALL_BITS,
                               (unsigned long)now,
                               (unsigned long)g_last_iwdg_refresh_tick);
            if (len > 0) {
                HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)len, 50);
            }
        }
    }
}
