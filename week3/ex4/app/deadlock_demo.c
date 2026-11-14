/* Week 3 / Ex4 -- task_alpha and task_beta share two resources, protected by
 * two mutexes, mimicking a sensor register + a shared UART buffer that both
 * tasks occasionally need at once. Native FreeRTOS API only.
 */
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "main.h"
#include "params.h"
#include "deadlock_demo.h"
#include "supervisor.h"

extern UART_HandleTypeDef huart1;
extern volatile uint32_t g_last_iwdg_refresh_tick;

static SemaphoreHandle_t s_mutex_a;
static SemaphoreHandle_t s_mutex_b;

/* Step 5 demo: Set to 1 to recreate AB-BA deadlock after 2200 ms. */
#define DEMO_INJECT_DEADLOCK 0

/* Step 6 (Question 6) test: Set to 1 to make task_beta run slow by 1 full
 * cycle (+25 ms) once at tick >= 2200 ms and log the real check-in period. */
#define DEMO_SLOW_ONE_CYCLE  0

volatile uint32_t g_alpha_last_checkin = 0;
volatile uint32_t g_alpha_period_ms    = 0;
volatile uint32_t g_beta_last_checkin  = 0;
volatile uint32_t g_beta_period_ms     = 0;

void deadlock_demo_init(void)
{
    s_mutex_a = xSemaphoreCreateMutex();
    s_mutex_b = xSemaphoreCreateMutex();
    configASSERT(s_mutex_a != NULL);
    configASSERT(s_mutex_b != NULL);
}

void task_alpha(void *argument)
{
    (void)argument;

    for (;;) {
        if (xSemaphoreTake(s_mutex_a, portMAX_DELAY) == pdTRUE) {
            vTaskDelay(pdMS_TO_TICKS(5)); /* simulated register access time */

            if (xSemaphoreTake(s_mutex_b, portMAX_DELAY) == pdTRUE) {
                /* ... shared work using both resources ... */

                xSemaphoreGive(s_mutex_b);
            }
            xSemaphoreGive(s_mutex_a);
        }

        uint32_t now = (uint32_t)xTaskGetTickCount();
        if (g_alpha_last_checkin != 0) {
            g_alpha_period_ms = now - g_alpha_last_checkin;
        }
        g_alpha_last_checkin = now;
        supervisor_checkin(SUPERVISOR_BIT_ALPHA);

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void task_beta(void *argument)
{
    (void)argument;
    uint8_t injected_logged = 0;
    uint8_t slow_done = 0;
    static char msg[160];

    for (;;) {
#if DEMO_INJECT_DEADLOCK
        if (xTaskGetTickCount() >= pdMS_TO_TICKS(2200)) {
            if (!injected_logged) {
                uint32_t now = (uint32_t)xTaskGetTickCount();
                int len = snprintf(msg, sizeof(msg),
                                   "\r\n[DEMO] Injecting AB-BA deadlock at tick=%lu ms (last_refresh=%lu ms)\r\n",
                                   (unsigned long)now,
                                   (unsigned long)g_last_iwdg_refresh_tick);
                if (len > 0) {
                    HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)len, 50);
                }
                injected_logged = 1;
            }
            if (xSemaphoreTake(s_mutex_b, portMAX_DELAY) == pdTRUE) {
                vTaskDelay(pdMS_TO_TICKS(25));

                if (xSemaphoreTake(s_mutex_a, portMAX_DELAY) == pdTRUE) {
                    xSemaphoreGive(s_mutex_a);
                }
                xSemaphoreGive(s_mutex_b);
            }
            supervisor_checkin(SUPERVISOR_BIT_BETA);
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
#else
        (void)injected_logged;
#endif

        /* Fixed lock order: always acquire s_mutex_a before s_mutex_b (same as
         * task_alpha) to eliminate the AB-BA circular wait. */
        if (xSemaphoreTake(s_mutex_a, portMAX_DELAY) == pdTRUE) {
            vTaskDelay(pdMS_TO_TICKS(5)); /* simulated UART buffer access time */

            if (xSemaphoreTake(s_mutex_b, portMAX_DELAY) == pdTRUE) {
                /* ... shared work using both resources ... */

                xSemaphoreGive(s_mutex_b);
            }
            xSemaphoreGive(s_mutex_a);
        }

#if DEMO_SLOW_ONE_CYCLE
        if (!slow_done && xTaskGetTickCount() >= pdMS_TO_TICKS(2200)) {
            slow_done = 1;
            /* Simulate task_beta running slow by 1 full cycle (+25 ms)
             * WITHOUT deadlocking. */
            vTaskDelay(pdMS_TO_TICKS(25));
            uint32_t now_slow = (uint32_t)xTaskGetTickCount();
            uint32_t slow_period = now_slow - g_beta_last_checkin;
            int len = snprintf(msg, sizeof(msg),
                               "\r\n[Q6-SLOW-1-CYCLE] beta check-in delayed at tick=%lu ms: beta_period=%lu ms (normal: alpha=%lu ms, beta=25 ms, hbeat=20 ms)\r\n",
                               (unsigned long)now_slow,
                               (unsigned long)slow_period,
                               (unsigned long)g_alpha_period_ms);
            if (len > 0) {
                HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)len, 50);
            }
        }
#else
        (void)slow_done;
        (void)msg;
#endif

        uint32_t now = (uint32_t)xTaskGetTickCount();
        if (g_beta_last_checkin != 0) {
            g_beta_period_ms = now - g_beta_last_checkin;
        }
        g_beta_last_checkin = now;
        supervisor_checkin(SUPERVISOR_BIT_BETA);

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
