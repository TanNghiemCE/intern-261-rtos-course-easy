/* Week 3 / Ex4 -- CPU load, deadlock, watchdog.
 * Native FreeRTOS API only.
 */
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "runtime_stats.h"
#include "deadlock_demo.h"
#include "supervisor.h"

extern UART_HandleTypeDef huart1;

void app_main(void)
{
    BaseType_t ret;
    static char boot_msg[128];
    uint32_t csr = RCC->CSR;
    uint8_t iwdg_rst = (csr & RCC_CSR_IWDGRSTF) ? 1u : 0u;

    int len = snprintf(boot_msg, sizeof(boot_msg),
                       "\r\n=== [BOOT] RCC_CSR=0x%08lX | IWDGRSTF=%u (%s) ===\r\n",
                       (unsigned long)csr,
                       (unsigned int)iwdg_rst,
                       iwdg_rst ? "IWDG WATCHDOG RESET" : "POWER/PIN RESET");
    if (len > 0) {
        HAL_UART_Transmit(&huart1, (uint8_t *)boot_msg, (uint16_t)len, 100);
    }
    __HAL_RCC_CLEAR_RESET_FLAGS();

    runtime_stats_timer_init();
    deadlock_demo_init();
    supervisor_init();

    ret = xTaskCreate(task_alpha, "alpha", 128, NULL, tskIDLE_PRIORITY + 2, NULL);
    configASSERT(ret == pdPASS);

    ret = xTaskCreate(task_beta, "beta", 192, NULL, tskIDLE_PRIORITY + 2, NULL);
    configASSERT(ret == pdPASS);

    ret = xTaskCreate(heartbeat_task, "hbeat", 128, NULL, tskIDLE_PRIORITY + 2, NULL);
    configASSERT(ret == pdPASS);

    ret = xTaskCreate(runtime_stats_task, "stats", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
    configASSERT(ret == pdPASS);

    ret = xTaskCreate(supervisor_task, "super", 192, NULL, tskIDLE_PRIORITY + 3, NULL);
    configASSERT(ret == pdPASS);

    vTaskStartScheduler();

    for (;;) {
    }
}
