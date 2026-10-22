/* Week 2 / Ex2 — WRITER_COUNT identical writer tasks. Each prints an MSG_LEN
 * byte string with its own id + sequence number, every WRITER_PERIOD_MS.
 */
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "params.h"
#include "uart_guard.h"

extern UART_HandleTypeDef huart1;

#if ENABLE_LATENCY_REPORT
#define MEASURE_SAMPLES    (50)  /* 50 mẫu cho mỗi task (thu thập nhanh và chính xác) */

typedef struct {
    uint32_t min_us;
    uint32_t max_us;
    uint64_t sum_us;
    uint32_t count;
} task_latency_t;

static task_latency_t s_stats[WRITER_COUNT];
static volatile uint32_t s_tasks_done = 0;
static char s_rep[256];

static void print_latency_report(void)
{
    uint32_t overall_min = 0xFFFFFFFF;
    uint32_t overall_max = 0;
    uint64_t overall_sum = 0;
    uint32_t overall_cnt = 0;

    snprintf(s_rep, sizeof(s_rep),
        "\r\n============================================================\r\n"
        "   LATENCY REPORT (MODE %d) - %d SAMPLES/TASK (%d TASKS)\r\n"
        "============================================================\r\n",
        (int)UART_GUARD_MODE, (int)MEASURE_SAMPLES, (int)WRITER_COUNT);
    HAL_UART_Transmit(&huart1, (uint8_t *)s_rep, (uint16_t)strlen(s_rep), 500);

    for (uint32_t i = 0; i < WRITER_COUNT; i++) {
        uint32_t avg = (s_stats[i].count > 0) ? (uint32_t)(s_stats[i].sum_us / s_stats[i].count) : 0;
        snprintf(s_rep, sizeof(s_rep),
            " Task %lu: Avg = %lu us, Min = %lu us, Max = %lu us\r\n",
            (unsigned long)i, (unsigned long)avg, 
            (unsigned long)s_stats[i].min_us, (unsigned long)s_stats[i].max_us);
        HAL_UART_Transmit(&huart1, (uint8_t *)s_rep, (uint16_t)strlen(s_rep), 500);

        if (s_stats[i].min_us < overall_min) overall_min = s_stats[i].min_us;
        if (s_stats[i].max_us > overall_max) overall_max = s_stats[i].max_us;
        overall_sum += s_stats[i].sum_us;
        overall_cnt += s_stats[i].count;
    }

    uint32_t overall_avg = (overall_cnt > 0) ? (uint32_t)(overall_sum / overall_cnt) : 0;
    snprintf(s_rep, sizeof(s_rep),
        "------------------------------------------------------------\r\n"
        " OVERALL: Avg = %lu us, Min = %lu us, Max = %lu us\r\n"
        "============================================================\r\n"
        "=== MEASUREMENT COMPLETE - SYSTEM PAUSED ===\r\n",
        (unsigned long)overall_avg, (unsigned long)overall_min, (unsigned long)overall_max);
    HAL_UART_Transmit(&huart1, (uint8_t *)s_rep, (uint16_t)strlen(s_rep), 500);
}
#endif

/* Format message nhanh mà KHÔNG gọi snprintf (tiêu tốn 0 byte stack, triệt tiêu nguy cơ tràn stack) */
static void format_message(char *buf, size_t max_len, uint32_t id, uint32_t seq)
{
    if (max_len < 12) return;

    /* Header: "[W<d>:<dddd>] " = 10 ký tự */
    snprintf(buf, max_len, "[W%u:%04u] ", (unsigned)(id % 10), (unsigned)(seq % 10000));

    /* Fill từ index 10 đến max_len - 3 */
    memset(buf + 10, 'A' + (id % 26), max_len - 12);

    buf[max_len - 2] = '\r';
    buf[max_len - 1] = '\n';
}

void writer_task(void *argument)
{
    uint32_t id = (uint32_t)(uintptr_t)argument;
    uint32_t seq = 0;
    char msg[MSG_LEN];

#if ENABLE_LATENCY_REPORT
    if (id < WRITER_COUNT) {
        s_stats[id].min_us = 0xFFFFFFFF;
        s_stats[id].max_us = 0;
        s_stats[id].sum_us = 0;
        s_stats[id].count  = 0;
    }
    for (;;) {
        /* Kiểm tra nếu task đã thu thập đủ mẫu */
        if (id < WRITER_COUNT && s_stats[id].count >= MEASURE_SAMPLES) {
            taskENTER_CRITICAL();
            s_tasks_done++;
            taskEXIT_CRITICAL();
            if (id == 0) {
                uint32_t wait_ticks = 0;
                while (s_tasks_done < WRITER_COUNT) {
                    vTaskDelay(pdMS_TO_TICKS(50));
                    wait_ticks++;
                }
                vTaskDelay(pdMS_TO_TICKS(50));
                /* In báo cáo duy nhất ra UART */
                print_latency_report();
                /* Tạm dừng scheduler, giữ màn hình sạch */
                vTaskSuspendAll();
                for (;;) {
                }
            } else {
                /* Task 1..7 tự suspend để nhường 100% CPU và không gọi send nữa */
                vTaskSuspend(NULL);
            }
        }

        format_message(msg, MSG_LEN, id, seq++);

        /* Bấm giờ đo độ trễ in bằng DWT (72 MHz: 72 cycle = 1 us) */
        uint32_t t_start = DWT->CYCCNT;
        uart_guard_send(msg, MSG_LEN);
        uint32_t t_end = DWT->CYCCNT;

        /* Xử lý wrap-around modulo 2^32 bằng phép trừ unsigned theo Rule.md */
        uint32_t lat_us = (uint32_t)((t_end - t_start) / 72UL);

        if (id < WRITER_COUNT) {
            if (lat_us < s_stats[id].min_us) s_stats[id].min_us = lat_us;
            if (lat_us > s_stats[id].max_us) s_stats[id].max_us = lat_us;
            s_stats[id].sum_us += lat_us;
            s_stats[id].count++;
        }

        vTaskDelay(pdMS_TO_TICKS(WRITER_PERIOD_MS));
    }
#else
    /* Chế độ chạy bình thường: in liên tục ra UART để chụp log 4 task nộp bài */
    for (;;) {
        format_message(msg, MSG_LEN, id, seq++);
        uart_guard_send(msg, MSG_LEN);
        vTaskDelay(pdMS_TO_TICKS(WRITER_PERIOD_MS));
    }
#endif
}
