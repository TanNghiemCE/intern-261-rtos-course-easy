/* Week 2 / Ex2 — race condition & mutex.
 * Native FreeRTOS API only.
 */
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "uart_guard.h"

extern UART_HandleTypeDef huart1;
void writer_task(void *argument);

void app_main(void)
{
    /* Kích hoạt DWT Cycle Counter ở xung nhịp 72 MHz (72 cycles = 1 us) */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    uart_guard_init();

    // /* Chờ PC sẵn sàng: Mở terminal, clear log xong -> Bấm ENTER để bắt đầu */
    // cmt lại khi chạy latency_report
    // char prompt[] = "\r\n=== Press ENTER to start logging ===\r\n";
    // HAL_UART_Transmit(&huart1, (uint8_t *)prompt, sizeof(prompt) - 1, 100);

    /* Thông báo trạng thái: Bấm phím ENTER trên bàn phím (hoặc tự động chạy sau 2 giây) */
    char prompt[128];
#if ENABLE_LATENCY_REPORT
    int n = snprintf(prompt, sizeof(prompt), 
        "\r\n=== MODE %d: LATENCY MEASUREMENT (Press ENTER or wait 2s) ===\r\n"
        "[DEBUG] Heap free truoc khi tao task: %u bytes\r\n", 
        (int)UART_GUARD_MODE, (unsigned int)xPortGetFreeHeapSize());
#else
    int n = snprintf(prompt, sizeof(prompt), 
        "\r\n=== MODE %d: CONTINUOUS LOG (Press ENTER or wait 2s) ===\r\n"
        "[DEBUG] Heap free truoc khi tao task: %u bytes\r\n", 
        (int)UART_GUARD_MODE, (unsigned int)xPortGetFreeHeapSize());
#endif
    HAL_UART_Transmit(&huart1, (uint8_t *)prompt, (uint16_t)n, 200);

    /* Chờ phím Enter với timeout 2000 ms: Nếu bấm Enter sẽ chạy ngay,
     * nếu không nối chân RX thì tự động bắt đầu sau 2 giây, không bao giờ bị kẹt. */
    uint8_t rx_char = 0;
    HAL_UART_Receive(&huart1, &rx_char, 1, 2000);

    /* xTaskCreate WRITER_COUNT instances of writer_task */
    for (uint32_t i = 0; i < WRITER_COUNT; i++) {
        char task_name[16];
        snprintf(task_name, sizeof(task_name), "w_%lu", (unsigned long)i);
        BaseType_t check = xTaskCreate(writer_task, task_name, 128, (void *)(uintptr_t)i, tskIDLE_PRIORITY + 1, NULL);
        if (check != pdPASS) {
            char err_msg[96];
            int en = snprintf(err_msg, sizeof(err_msg), 
                "\r\n[FATAL] Khong du Heap de tao task %lu! Heap con lai: %u bytes\r\n", 
                (unsigned long)i, (unsigned int)xPortGetFreeHeapSize());
            HAL_UART_Transmit(&huart1, (uint8_t *)err_msg, (uint16_t)en, 500);
            configASSERT(check == pdPASS);
        }
    }

    char ok_msg[96];
    int ok_n = snprintf(ok_msg, sizeof(ok_msg), 
        "[DEBUG] Da tao thanh cong %lu tasks! Heap con lai: %u bytes. Bat dau scheduler...\r\n", 
        (unsigned long)WRITER_COUNT, (unsigned int)xPortGetFreeHeapSize());
    HAL_UART_Transmit(&huart1, (uint8_t *)ok_msg, (uint16_t)ok_n, 200);

    vTaskStartScheduler();

    for (;;) {
    }
}
