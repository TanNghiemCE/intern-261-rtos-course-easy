/* Week 2 / Ex4 — event group sync across EVENT_BIT_COUNT producers, with
 * producer_0's signal optionally replaced by a task notification.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "stdio.h"

#include "main.h"
#include "params.h"
#include <stddef.h>
#include <stdint.h>
#include "sync_tasks.h"

#define ALL_BITS_MASK ((1u << EVENT_BIT_COUNT) - 1u)
#define CYCLES_PER_US   (72u)
#define MAX_SYNC_LOGS   (30u)

static EventGroupHandle_t s_events;
static size_t s_eg_heap_bytes;
extern UART_HandleTypeDef huart1;
void *sync_task_handle;

void sync_init(void)
{
    /* TODO 1: xEventGroupCreate(). Check for NULL.
     *         Measure and record in REPORT.md the sizeof the structure this
     *         allocates (see event_groups.c in the kernel sources) to answer
     *         PREDICTION.md question 1. */
    void *tmp = pvPortMalloc(1); // khởi tạo heap ngay lập tức
    vPortFree(tmp); // trả lại 1bytes vừa mượn
    size_t free_before = xPortGetFreeHeapSize();
    s_events = xEventGroupCreate();
    configASSERT(s_events != NULL);
    size_t free_after = xPortGetFreeHeapSize();
    s_eg_heap_bytes = free_before - free_after;
}

#if !BCAST_TEST

typedef struct {
    uint32_t sync_tick;
    uint32_t period_ticks;
    uint32_t period_us;
    uint32_t d_notify;
    uint32_t d_eg;
} sync_log_t;

static volatile uint32_t t0_eg;      // event-group path (producer 1,2)
static volatile uint32_t t0_notify;  // notification path (producer 0)

static volatile size_t s_ntf_heap_bytes;
static sync_log_t s_logs[MAX_SYNC_LOGS];

void producer_task(void *argument)
{
    uint32_t idx = (uint32_t)(uintptr_t)argument;
    uint32_t rand_state = 0x12345678u ^ ((idx + 1u) * 0x9E3779B9u);
    for (;;) {
        /* TODO 2: random small period (reuse a simple xorshift like ex3's
         *         periodic task, or vTaskDelay with a bounded random jitter). */
        rand_state ^= rand_state << 13;
        rand_state ^= rand_state >> 17;
        rand_state ^= rand_state << 5;
        #if BUSY_LOOP
        if (idx == 1) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        #else
        vTaskDelay(pdMS_TO_TICKS(NOTIFY_PERIOD_MS + (rand_state % 4u)));
        #endif

#if PRODUCER0_USE_NOTIFY
        if (idx == 0) {
            /* TODO 3: xTaskNotifyGive((TaskHandle_t)sync_task_handle).
             *         Toggle a DWT-timestamped GPIO edge right before this
             *         call — this is your t0 for the wake-up latency
             *         measurement. */
            size_t ntf_before = xPortGetFreeHeapSize();
            HAL_GPIO_TogglePin(GPIOA, (uint16_t)(1u << 0));
            t0_notify = DWT->CYCCNT;
            xTaskNotifyGive((TaskHandle_t)(sync_task_handle));
            s_ntf_heap_bytes = ntf_before - xPortGetFreeHeapSize();
        } else
#endif
        {
            /* TODO 4: xEventGroupSetBits(s_events, 1u << idx).
             *         Same timestamp discipline as TODO 3 for producers that
             *         still use the event group. */
            HAL_GPIO_TogglePin(GPIOA, (uint16_t)(1u << 4));
            t0_eg = DWT->CYCCNT;
            xEventGroupSetBits(s_events, 1u << idx);
        }
    }
}

void sync_task(void *argument)
{
    (void)argument;
    TickType_t last_sync_tick = xTaskGetTickCount();
    uint32_t last_sync_cyc = DWT->CYCCNT;
    uint32_t log_count = 0;
    char uart_buf[256];
    for (;;) {
        BaseType_t synced = pdFALSE;
        uint32_t d_notify = 0;
        uint32_t d_eg = 0;
        EventBits_t uxBits = 0;
#if PRODUCER0_USE_NOTIFY
        /* TODO 5: ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(SYNC_TIMEOUT_MS)) for
         *         producer_0's contribution, THEN xEventGroupWaitBits for the
         *         remaining producers' bits (mask without bit 0), clear-on-exit
         *         = pdTRUE, wait-for-all = pdTRUE, same timeout.
         *         Toggle a GPIO edge the moment both are satisfied — your t1
         *         for the latency measurement. */
        const EventBits_t rem_mask = ALL_BITS_MASK & ~1u;
        uint32_t wait_n = DWT->CYCCNT;
        uint32_t notified = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(SYNC_TIMEOUT_MS));
        uint32_t t1_notify = DWT->CYCCNT;
        if ((notified > 0) && ((int32_t)(t0_notify - wait_n) > 0)) {
            d_notify = t1_notify - t0_notify;
        }

        uint32_t wait_eg = DWT->CYCCNT;
        uxBits = xEventGroupWaitBits(
            s_events,
            rem_mask,
            pdTRUE,
            pdTRUE,
            pdMS_TO_TICKS(SYNC_TIMEOUT_MS)
        );
        uint32_t t1_eg = DWT->CYCCNT;
        if ((notified > 0) && ((uxBits & rem_mask) == rem_mask)) {
            if ((int32_t)(t0_eg - wait_eg) > 0) {
                d_eg = t1_eg - t0_eg;
            }
            HAL_GPIO_TogglePin(GPIOA, (uint16_t)(1u << 5)); // PA5
            (void)ulTaskNotifyTake(pdTRUE, 0);
            synced = pdTRUE;
        }
#else
        /* TODO 6: xEventGroupWaitBits(s_events, ALL_BITS_MASK, pdTRUE, pdTRUE,
         *         pdMS_TO_TICKS(SYNC_TIMEOUT_MS)).
         *         Check the return value against ALL_BITS_MASK to tell
         *         "all bits arrived" apart from "timed out with some bits
         *         missing" — do not assume a non-zero return means success. */
        uint32_t wait_eg = DWT->CYCCNT;
        uxBits = xEventGroupWaitBits(
            s_events,
            ALL_BITS_MASK,
            pdTRUE,                       
            pdTRUE,                        
            pdMS_TO_TICKS(SYNC_TIMEOUT_MS)
        );
        uint32_t t1_eg = DWT->CYCCNT;
        if ((uxBits & ALL_BITS_MASK) == ALL_BITS_MASK) {
            if ((int32_t)(t0_eg - wait_eg) > 0) {
                d_eg = t1_eg - t0_eg;
            }
            HAL_GPIO_TogglePin(GPIOA, (uint16_t)(1u << 6)); // PA6
            synced = pdTRUE;
        }
#endif

        /* TODO 7: print the sync instant and measured period to UART. */
        if (synced == pdTRUE) {
            TickType_t now_tick = xTaskGetTickCount();
            uint32_t now_cyc = DWT->CYCCNT;
            TickType_t period_ticks = now_tick - last_sync_tick;
            uint32_t period_us = (now_cyc - last_sync_cyc) / 72UL;
            last_sync_tick = now_tick;
            last_sync_cyc = now_cyc;

            if (log_count < MAX_SYNC_LOGS) {
                s_logs[log_count].sync_tick    = (uint32_t)now_tick;
                s_logs[log_count].period_ticks = (uint32_t)period_ticks;
                s_logs[log_count].period_us    = period_us;
                s_logs[log_count].d_notify     = d_notify;
                s_logs[log_count].d_eg         = d_eg;
                log_count++;
            }
            
            if (log_count >= MAX_SYNC_LOGS) {
                uint64_t sum_ntf_cyc = 0, sum_eg_cyc = 0, sum_per_us = 0, sum_per_ticks = 0;
                uint32_t cnt_ntf = 0, cnt_eg = 0;
                for (uint32_t i = 0; i < MAX_SYNC_LOGS; i++) {
                    int len = snprintf(uart_buf, sizeof(uart_buf),
                        "[%02lu] sync_tick=%lu, period_ticks=%lu (%lu us), d_notify=%lu cyc, d_eg=%lu cyc\r\n",
                        (unsigned long)(i + 1u),
                        (unsigned long)s_logs[i].sync_tick,
                        (unsigned long)s_logs[i].period_ticks,
                        (unsigned long)s_logs[i].period_us,
                        (unsigned long)s_logs[i].d_notify,
                        (unsigned long)s_logs[i].d_eg);
                    if (len > 0) {
                        HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, (uint16_t)len, 100);
                    }
                    sum_per_ticks += s_logs[i].period_ticks;
                    sum_per_us    += s_logs[i].period_us;
                    if (s_logs[i].d_notify > 0u) {
                        sum_ntf_cyc += s_logs[i].d_notify;
                        cnt_ntf++;
                    }
                    if (s_logs[i].d_eg > 0u) {
                        sum_eg_cyc += s_logs[i].d_eg;
                        cnt_eg++;
                    }
                }
                // tính trung bình 30 mẫu
                uint32_t avg_ntf_cyc = (cnt_ntf > 0u) ? (uint32_t)(sum_ntf_cyc / cnt_ntf) : 0u;
                uint32_t avg_eg_cyc  = (cnt_eg > 0u)  ? (uint32_t)(sum_eg_cyc / cnt_eg)   : 0u;
                uint32_t avg_per_us  = (uint32_t)(sum_per_us / MAX_SYNC_LOGS);
                int len = snprintf(uart_buf, sizeof(uart_buf),
                    "=== SUMMARY (%u LOGS, BITS=%u, NOTIFY=%u) ===\r\n"
                    "RAM EventGroup : struct=%u B, heap=%u B\r\n"
                    "RAM Notify     : heap=%u B, TCB_fields=%u B (TCB=%u B, free_heap=%u B)\r\n",
                    (unsigned)MAX_SYNC_LOGS, (unsigned)EVENT_BIT_COUNT, (unsigned)PRODUCER0_USE_NOTIFY,
                    (unsigned)sizeof(StaticEventGroup_t), (unsigned)s_eg_heap_bytes,
                    (unsigned)s_ntf_heap_bytes, (unsigned)(sizeof(uint32_t) + sizeof(uint8_t)),
                    (unsigned)sizeof(StaticTask_t), (unsigned)xPortGetFreeHeapSize());
                if (len > 0) {
                    HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, (uint16_t)len, 200);
                }
                len = snprintf(uart_buf, sizeof(uart_buf),
                    "Wakeup Notify  : avg=%lu cyc (%lu.%02lu us) [n=%lu]\r\n"
                    "Wakeup EventGrp: avg=%lu cyc (%lu.%02lu us) [n=%lu]\r\n"
                    "Sync Period Avg: %lu.%02lu ticks (%lu us)\r\n",
                    (unsigned long)avg_ntf_cyc,
                    (unsigned long)(avg_ntf_cyc / CYCLES_PER_US),
                    (unsigned long)(((avg_ntf_cyc % CYCLES_PER_US) * 100u) / CYCLES_PER_US),
                    (unsigned long)cnt_ntf,
                    (unsigned long)avg_eg_cyc,
                    (unsigned long)(avg_eg_cyc / CYCLES_PER_US),
                    (unsigned long)(((avg_eg_cyc % CYCLES_PER_US) * 100u) / CYCLES_PER_US),
                    (unsigned long)cnt_eg,
                    (unsigned long)(sum_per_ticks / MAX_SYNC_LOGS),
                    (unsigned long)(((sum_per_ticks % MAX_SYNC_LOGS) * 100u) / MAX_SYNC_LOGS),
                    (unsigned long)avg_per_us);
                if (len > 0) {
                    HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, (uint16_t)len, 200);
                }
                vTaskSuspendAll();
                for (;;) {
                }
            }
        } else {
            int len = snprintf(uart_buf, sizeof(uart_buf),
                               "[TIMEOUT] sync timeout! uxBits=0x%02lx (expected=0x%02lx)\r\n",
                               (unsigned long)uxBits,
                               (unsigned long)ALL_BITS_MASK);
            if (len > 0) {
                HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, (uint16_t)len, 100);
            }
            log_count++;
            if (log_count >= MAX_SYNC_LOGS) {
                vTaskSuspendAll();
                for (;;) {
                }
            }
        }
    }
}

#else /* BCAST_TEST == 1 */

void *s_listener_h[BCAST_N];
static volatile uint32_t s_t0;
static volatile uint32_t s_lat_cyc[BCAST_N];
static volatile uint32_t s_send_cost_cyc;

void listener_task(void *arg)
{
    uint32_t idx = (uint32_t)(uintptr_t)arg;
    for (;;) {
#if BCAST_USE_NOTIFY
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
#else
        xEventGroupWaitBits(s_events, BCAST_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
#endif
        s_lat_cyc[idx] = DWT->CYCCNT - s_t0; /* t_i - t0 */
    }
}

void broadcaster_task(void *arg)
{
    (void)arg;
    char uart_buf[256];
    uint32_t sample = 0;
    uint64_t sum_send = 0, sum_l[BCAST_N] = {0};

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(BCAST_PERIOD_MS));

        for (uint32_t i = 0; i < BCAST_N; i++) {
            s_lat_cyc[i] = 0;
        }

        HAL_GPIO_TogglePin(GPIOA, (uint16_t)(1u << 0));
        uint32_t a = DWT->CYCCNT;
        s_t0 = a;
#if BCAST_USE_NOTIFY
        for (uint32_t i = 0; i < BCAST_N; i++) {
            xTaskNotifyGive((TaskHandle_t)s_listener_h[i]); /* N lần gọi, cần N handle */
        }
#else
        xEventGroupSetBits(s_events, BCAST_BIT);            /* 1 lần gọi, không cần handle */
#endif
        s_send_cost_cyc = DWT->CYCCNT - a;

        /* Lúc này cả N listener (priority 3 > 2) đã chạy xong và quay lại block */
        sum_send += s_send_cost_cyc;
        for (uint32_t i = 0; i < BCAST_N; i++) {
            sum_l[i] += s_lat_cyc[i];
        }
        sample++;

        int len = snprintf(uart_buf, sizeof(uart_buf),
            "[BCAST %02lu | NOTIFY=%u] send_cost=%lu cyc | L0=%lu, L1=%lu, L2=%lu cyc\r\n",
            (unsigned long)sample, (unsigned)BCAST_USE_NOTIFY,
            (unsigned long)s_send_cost_cyc,
            (unsigned long)s_lat_cyc[0],
            (unsigned long)s_lat_cyc[1],
            (unsigned long)s_lat_cyc[2]);
        if (len > 0) {
            HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, (uint16_t)len, 100);
        }

        if (sample >= MAX_SYNC_LOGS) {
            len = snprintf(uart_buf, sizeof(uart_buf),
                "=== BCAST SUMMARY (N=%u, NOTIFY=%u, %u samples) ===\r\n"
                "avg send_cost = %lu cyc | avg L0 = %lu, L1 = %lu, L2 = %lu cyc\r\n",
                (unsigned)BCAST_N, (unsigned)BCAST_USE_NOTIFY, (unsigned)MAX_SYNC_LOGS,
                (unsigned long)(sum_send / MAX_SYNC_LOGS),
                (unsigned long)(sum_l[0] / MAX_SYNC_LOGS),
                (unsigned long)(sum_l[1] / MAX_SYNC_LOGS),
                (unsigned long)(sum_l[2] / MAX_SYNC_LOGS));
            if (len > 0) {
                HAL_UART_Transmit(&huart1, (uint8_t *)uart_buf, (uint16_t)len, 200);
            }
            vTaskSuspendAll();
            for (;;) {
            }
        }
    }
}

#endif /* BCAST_TEST */


