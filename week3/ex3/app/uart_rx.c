/* Week 3 / Ex3 -- UART RX interrupt -> stream buffer.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"

#include "main.h"
#include "params.h"
#include "uart_rx.h"

StreamBufferHandle_t rx_stream;
extern UART_HandleTypeDef huart1;
static uint8_t s_rx_byte;
static volatile uint32_t s_rx_dropped;      /* số byte bị bỏ */
static volatile uint32_t s_rx_rearm_fail; /* check HAL_OK*/

void uart_rx_init(void)
{
    /* TODO 1: rx_stream = xStreamBufferCreate(STREAM_BUFFER_BYTES,
     *         TRIGGER_LEVEL). Check non-NULL.
     *
     * TODO 2: HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1) to arm the first
     *         byte. huart1 must already be MX_USART1_UART_Init()'d at
     *         RX_BAUD -- that part lives in template/, not here. */
    rx_stream = xStreamBufferCreate(
        STREAM_BUFFER_BYTES,TRIGGER_LEVEL
    );
    configASSERT(rx_stream!=NULL);
    HAL_StatusTypeDef st = HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1);
    configASSERT(st == HAL_OK);
    (void)st; 
}

void uart_rx_isr_handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* TODO 3: xStreamBufferSendFromISR(rx_stream, &s_rx_byte, 1,
     *         &xHigherPriorityTaskWoken). Check the return value: what do you
     *         do with the byte if the stream buffer is full and it returns 0?
     *         Answer this in PREDICTION.md, not just here. */

    /* TODO 4: HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1) again -- re-arm for
     *         the next byte. Forgetting this is the single most common way
     *         to silently stop receiving after the first byte. */

    /* TODO 5: portYIELD_FROM_ISR(xHigherPriorityTaskWoken). */
    (void)xHigherPriorityTaskWoken;
    if (xStreamBufferSendFromISR(rx_stream, &s_rx_byte, 1, &xHigherPriorityTaskWoken) != 1) {s_rx_dropped++;}
    if (s_rx_byte == '\r') {
        xStreamBufferSendCompletedFromISR(rx_stream, &xHigherPriorityTaskWoken);
    }
    if (HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1) != HAL_OK) {
        s_rx_rearm_fail++;
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        uart_rx_isr_handler();
    }
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        (void)HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1);
    }
}
