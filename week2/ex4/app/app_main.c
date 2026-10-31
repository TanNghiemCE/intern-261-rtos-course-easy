/* Week 2 / Ex4 — event group & task notification.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "sync_tasks.h"

void app_main(void)
{
    sync_init();
    dwt_init();

#if BCAST_TEST
    for (uintptr_t i = 0; i < BCAST_N; i++) {
        TaskHandle_t h = NULL;
        BaseType_t ret = xTaskCreate(
            listener_task,
            "listener",
            128,
            (void *)i,
            3,              /* Ưu tiên cao hơn broadcaster_task (2) */
            &h
        );
        configASSERT(ret == pdPASS);
        s_listener_h[i] = (void *)h;
    }
    BaseType_t ret_b = xTaskCreate(
        broadcaster_task,
        "broadcaster",
        256,
        NULL,
        2,
        NULL
    );
    configASSERT(ret_b == pdPASS);
#else
    TaskHandle_t h_sync = NULL;
    /* TODO 1: xTaskCreate sync_task first, store its handle into
     *         sync_task_handle (cast to void*) — producer_0 needs it when
     *         PRODUCER0_USE_NOTIFY is set. */
    BaseType_t ret1 = xTaskCreate(
        sync_task,
        "sync_task",
        256,
        NULL,
        3,
        &h_sync
    );
    configASSERT(ret1 == pdPASS);
    sync_task_handle = (void *)h_sync;

    /* TODO 2: xTaskCreate EVENT_BIT_COUNT instances of producer_task, each
     *         given its own index (0..EVENT_BIT_COUNT-1) as the task
     *         parameter. */
    for (uintptr_t i = 0; i < EVENT_BIT_COUNT; i++) {
        BaseType_t ret = xTaskCreate(
            producer_task,      
            "producer",         
            128,               
            (void *)i,          /* Truyền index i (0, 1, 2) qua pvParameters */
            2,               
            NULL             
        );
        configASSERT(ret == pdPASS);
    }
#endif

    vTaskStartScheduler();

    for (;;) {
    }
}
