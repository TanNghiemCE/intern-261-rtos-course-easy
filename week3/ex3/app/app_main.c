/* Week 3 / Ex3 -- UART RX -> stream buffer -> parser.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "uart_rx.h"
#include "parser_task.h"

void app_main(void)
{
    uart_rx_init();

    /* TODO: xTaskCreate parser_task and demo_task. parser_task must run at a
     *       priority high enough that it drains rx_stream faster than bytes
     *       arrive at RX_BAUD -- justify the number in REPORT.md with the
     *       byte time you computed. */
     BaseType_t check = xTaskCreate(parser_task, "parser", 256, NULL, 4, NULL);
    configASSERT(check == pdPASS);
    check= xTaskCreate(demo_task, "demo", 128, NULL, 1, NULL);
    configASSERT(check == pdPASS);
    vTaskStartScheduler();

    for (;;) {
    }
}
