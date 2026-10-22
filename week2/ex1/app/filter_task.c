/* Week 2 / Ex1 — moving-average filter, window = FILTER_WINDOW samples.
 * queue_raw -> queue_filtered.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "main.h"
#include "params.h"
#include "pipeline.h"

void filter_task(void *argument)
{
    (void)argument;

    /* TODO 1: ring buffers for ax/ay/az, size FILTER_WINDOW, plus running sums.
     *         Do NOT recompute the full sum every sample — that is the whole
     *         point of a moving-average window. */
    int16_t ring_ax[FILTER_WINDOW] = {0};
    int16_t ring_ay[FILTER_WINDOW] = {0};
    int16_t ring_az[FILTER_WINDOW] = {0};
    int32_t sum_ax = 0;
    int32_t sum_ay = 0;
    int32_t sum_az = 0;
    size_t head  = 0;
    size_t count = 0;
    for (;;) {
        raw_sample_t in;

        /* TODO 2: xQueueReceive(queue_raw, &in, portMAX_DELAY). */
        if (xQueueReceive(queue_raw, &in, portMAX_DELAY) == pdTRUE) {
        /* TODO 3: push into the ring buffer, update running sums, compute the
         *         averaged ax/ay/az. */

        filtered_sample_t out;
        out.seq = in.seq;
        out.raw_ay = in.ay;   /* <-- THÊM: Lưu lại giá trị thô hiện tại của ay */
         if (count < FILTER_WINDOW) {
                /* Giai đoạn khởi động (warm-up): Chưa đủ 4 mẫu */
                ring_ax[head] = in.ax;
                ring_ay[head] = in.ay;
                ring_az[head] = in.az;
                sum_ax += in.ax;
                sum_ay += in.ay;
                sum_az += in.az;
                head = (head + 1) % FILTER_WINDOW;
                count++;
                out.ax = (int16_t)(sum_ax / (int32_t)count);
                out.ay = (int16_t)(sum_ay / (int32_t)count);
                out.az = (int16_t)(sum_az / (int32_t)count);
            } else {
                /* Trạng thái xác lập: Trừ phần tử cũ nhất chuẩn bị rời khỏi cửa sổ */
                sum_ax -= ring_ax[head];
                sum_ay -= ring_ay[head];
                sum_az -= ring_az[head];
                /* Ghi đè phần tử mới vào ring buffer */
                ring_ax[head] = in.ax;
                ring_ay[head] = in.ay;
                ring_az[head] = in.az;
                /* Cộng phần tử mới vào running sum */
                sum_ax += in.ax;
                sum_ay += in.ay;
                sum_az += in.az;
                head = (head + 1) % FILTER_WINDOW;
                out.ax = (int16_t)(sum_ax / FILTER_WINDOW);
                out.ay = (int16_t)(sum_ay / FILTER_WINDOW);
                out.az = (int16_t)(sum_az / FILTER_WINDOW);
            }
            /* TODO 4: fill out.ax/ay/az from the running average (done above). */

            /* TODO 5: xQueueSend(queue_filtered, &out, <timeout>) — same question
             *         as sensor_task: what should happen here if uart_task is
             *         slow and queue_filtered is full? */
            xQueueSend(queue_filtered, &out, 0);
        }
    }
}

