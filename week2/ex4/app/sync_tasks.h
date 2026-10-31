#ifndef SYNC_TASKS_H
#define SYNC_TASKS_H

#include <stdint.h>

/* Bit i of the event group corresponds to producer_i.
 * producer_0 is the one swapped to task notification in step 4 — guard it
 * with PRODUCER0_USE_NOTIFY so the two configurations build from the same
 * source. */
#ifndef PRODUCER0_USE_NOTIFY
#define PRODUCER0_USE_NOTIFY 1
#endif
#ifndef BUSY_LOOP
#define BUSY_LOOP 0
#endif

/* Chế độ kiểm chứng Broadcast (1-to-N) cho mục 3 của REPORT:
 * BCAST_TEST = 1: chạy broadcaster_task & listener_task
 * BCAST_TEST = 0: chạy bài gốc (producer_task & sync_task)
 */
#ifndef BCAST_TEST
#define BCAST_TEST 0
#endif

/* Cấu hình so sánh cho thí nghiệm Broadcast:
 * BCAST_USE_NOTIFY = 0: dùng xEventGroupSetBits (1 lần gọi)
 * BCAST_USE_NOTIFY = 1: dùng xTaskNotifyGive loop (N lần gọi)
 */
#ifndef BCAST_USE_NOTIFY
#define BCAST_USE_NOTIFY 1
#endif

#define BCAST_N         3u
#define BCAST_BIT       (1u << EVENT_BIT_COUNT)
#define BCAST_PERIOD_MS 100u

void sync_init(void);
void dwt_init(void);
void dwt_busy_wait_us(uint32_t microseconds);

/* Bài tập chính (Ex4) */
void producer_task(void *argument); /* argument = producer index 0..EVENT_BIT_COUNT-1 */
void sync_task(void *argument);
extern void *sync_task_handle;

/* Thí nghiệm Broadcast (Mục 3 REPORT) */
void broadcaster_task(void *arg);
void listener_task(void *arg);
extern void *s_listener_h[BCAST_N];

#endif /* SYNC_TASKS_H */
