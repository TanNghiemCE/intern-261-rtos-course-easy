/* Week 1 / Ex4 — entry point. */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"

#ifndef ENABLE_HOG
#define ENABLE_HOG 0
#endif

/* Cấu hình stack cho dyn_task để test Mục 6 (Stack nhỏ nhất an toàn):
 * - Lần 1: 128 words (Ban đầu)
 * - Lần 2: 48 words (HWM < 32 words - không đạt biên an toàn)
 * - Lần 3: 64 words (HWM >= 32 words - ĐẠT CHUẨN)
 * Bạn có thể đổi giá trị này thành 48 hoặc 64 để test. Mặc định là 64.
 */
#ifndef DYN_TEST_STACK_WORDS
#define DYN_TEST_STACK_WORDS 48
#endif

/* Tắt extra_task của Mục 8 khi test Mục 6 */
#define ENABLE_EXTRA_TASK 0

void ram_report_task(void *argument);

/* TODO 1: static storage for stat_task — TCB and stack.
 *         The stack array type matters; it is not char[]. */
TaskHandle_t dyn_task = NULL;
TaskHandle_t stat_task = NULL;
TaskHandle_t report_task = NULL;
TaskHandle_t extra_task = NULL;

static StaticTask_t stat_tcb;
static StackType_t stat_stack[STATIC_TASK_STACK_WORDS];

/* Thêm __attribute__((noinline)) để trình biên dịch (-O2) không tự ý gộp 
 * hàm này vào dyn_work, giữ đúng lời gọi hàm lồng nhau để đẩy SP chạm Canary */
__attribute__((noinline)) static void shared_work(void *argument)
{
    (void)argument;
    for (;;) {
        /* TODO 2: identical trivial work in both tasks, so that any difference
         *         in high water mark comes from how they were created, not from
         *         what they do. */
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void extra_work(void *argument)
{
    shared_work(argument);
}

static void dyn_work(void *argument)
{
#if ENABLE_HOG
    /* Khai báo mảng hog 384 bytes */
    volatile uint8_t hog[HOG_DEPTH_BYTES];
    /* Dùng volatile cho cả biến i để ép biến i nằm trên Stack (4 bytes) */
    for (volatile size_t i = 0; i < sizeof(hog); i++) {
        hog[i] = 0xAA;
    }
#endif
    /* Gọi shared_work thay vì viết lại vòng lặp for(;;) tại đây */
    shared_work(argument);
}

static void stat_work(void *argument)
{
    /* Cho stat_work cũng gọi qua 1 lớp giống hệt dyn_work để khi ENABLE_HOG = 0,
     * cả 2 task có độ sâu lời gọi hàm (Call Depth) giống nhau 100% (TODO 2) */
    shared_work(argument);
}

void app_main(void)
{
    /* TODO 3: dyn_task  via xTaskCreate,       stack DYN_TASK_STACK_WORDS
     *         stat_task via xTaskCreateStatic, stack STATIC_TASK_STACK_WORDS
     *         ram_report_task via xTaskCreate
     *
     * Check every return value. xTaskCreateStatic returns a handle, not a
     * BaseType_t — explain the difference in REPORT.md. */

    /* TODO 4 (step 6 of the README, enable only for that run):
     *         declare a HOG_DEPTH_BYTES local array inside dyn_task's work
     *         function and write to every element, so the overflow is real and
     *         not optimised away. Think about what -O2 does to a dead array. */

    /* 1. Tạo ram_report_task trước để đệm đầu heap, bảo vệ các biến kernel */
    BaseType_t check_ram_report = xTaskCreate(
        ram_report_task, "report", 256, NULL, 3, &report_task);

    /* 2. Tạo stat_task (trỏ vào stat_work) */
    stat_task = xTaskCreateStatic(
        stat_work, "stat", STATIC_TASK_STACK_WORDS, NULL, 2, stat_stack, &stat_tcb);

    /* 3. Tạo dyn_task (trỏ vào dyn_work) với stack kiểm thử của Mục 6 */
    BaseType_t check_dyn = xTaskCreate(
        dyn_work, "dyn", DYN_TEST_STACK_WORDS, NULL, 2, &dyn_task);

#if ENABLE_EXTRA_TASK
    BaseType_t check_extra = xTaskCreate(
        extra_work, "extra", 128, NULL, 2, &extra_task);
    configASSERT(check_extra == pdPASS);
#endif

    configASSERT(check_ram_report == pdPASS);
    configASSERT(stat_task != NULL);
    configASSERT(check_dyn == pdPASS);

    vTaskStartScheduler();
    /* Scheduler returned => not enough heap for the idle/timer task. */
    for (;;) {
    }
}