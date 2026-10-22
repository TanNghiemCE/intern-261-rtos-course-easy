/* Week 2 / Ex1 — prints filtered samples to USART1 at UART_BAUD.
 * queue_filtered -> UART.
 *
 * This is also the task you deliberately starve in step 7 (temporarily set
 * UART_BAUD-equivalent hardware baud to 9600) to force queue_filtered to fill.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

#include "main.h"
#include "params.h"
#include "pipeline.h"

extern UART_HandleTypeDef huart1;

void uart_task(void *argument)
{
    (void)argument;
    char msg[64];
    for (;;) {
        filtered_sample_t in;

        /* TODO 1: xQueueReceive(queue_filtered, &in, portMAX_DELAY). */
    if (xQueueReceive(queue_filtered, &in, portMAX_DELAY) == pdTRUE) {
    UBaseType_t raw_q  = uxQueueMessagesWaiting(queue_raw);
    UBaseType_t filt_q = uxQueueMessagesWaiting(queue_filtered);

   /* Định dạng mới: seq,raw_ay,filt_ay,raw_q,filt_q */
int len = snprintf(msg, sizeof(msg), "%lu,%d,%d,%u,%u\r\n",
                   (unsigned long)in.seq, (int)in.raw_ay, (int)in.ay,
                   (unsigned int)raw_q, (unsigned int)filt_q);
    if (len > 0) {
        HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)len, 200);
    }
}
    }
}