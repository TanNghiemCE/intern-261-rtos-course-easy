/* Week 2 / Ex2 — four ways to serialize access to USART1 from WRITER_COUNT
 * tasks. Only one UART_GUARD_MODE is compiled at a time.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "main.h"
#include "params.h"
#include "uart_guard.h"

#define GATEKEEPER_QUEUE_LEN      (WRITER_COUNT * 2)
#define GATEKEEPER_STACK_SIZE     (128)
#define UART_TIMEOUT_MS           (100)

extern UART_HandleTypeDef huart1;

#if UART_GUARD_MODE == 1
static SemaphoreHandle_t s_uart_mutex;
#elif UART_GUARD_MODE == 3
static QueueHandle_t s_gatekeeper_queue;
#endif

void uart_guard_init(void)
{
#if UART_GUARD_MODE == 1
    /* TODO 1: xSemaphoreCreateMutex(). Check for NULL. */
    s_uart_mutex = xSemaphoreCreateMutex();
    configASSERT(s_uart_mutex != NULL);

#elif UART_GUARD_MODE == 3
    /* TODO 2: create s_gatekeeper_queue (length: pick and justify in
     *         REPORT.md), item size = MSG_LEN bytes.
     *         xTaskCreate(gatekeeper_task, ..., GATEKEEPER_PRIO). */
    s_gatekeeper_queue = xQueueCreate(GATEKEEPER_QUEUE_LEN, MSG_LEN);
    configASSERT(s_gatekeeper_queue != NULL);

    BaseType_t ret = xTaskCreate(
        gatekeeper_task,
        "gatekeeper",
        GATEKEEPER_STACK_SIZE,
        NULL,
        GATEKEEPER_PRIO,
        NULL
    );
    configASSERT(ret == pdPASS);
#endif
}

#if ENABLE_LATENCY_REPORT
/* Busy-wait đúng số microsecond bằng DWT (72 MHz: 72 cycle = 1 us).
 * Trừ unsigned an toàn tuyệt đối khi DWT wrap-around theo Rule.md. */
static inline void dwt_delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * 72UL;
    while ((uint32_t)(DWT->CYCCNT - start) < ticks) {
    }
}
#endif

void uart_guard_send(const char *buf, size_t len)
{
    if (buf == NULL || len == 0) {
        return;
    }

#if UART_GUARD_MODE == 0
    /* The bug: no serialization at all. */
#if ENABLE_LATENCY_REPORT
    /* Tái hiện chính xác tải phần cứng (24 byte @ 115200 baud = 2083 us) mà không gửi ký tự ra PC */
    dwt_delay_us((uint32_t)(len * 10UL * 1000000UL / 115200UL));
#else
    for (size_t i = 0; i < len; i++) {
        while (!(huart1.Instance->SR & UART_FLAG_TXE));
        huart1.Instance->DR = (uint8_t)buf[i];
    }
#endif

#elif UART_GUARD_MODE == 1
    /* TODO 4: xSemaphoreTake(s_uart_mutex, portMAX_DELAY), transmit, Give.
     *         Every return path must Give — no early return between Take and Give. */
    if (xSemaphoreTake(s_uart_mutex, portMAX_DELAY) == pdTRUE) {
#if ENABLE_LATENCY_REPORT
        dwt_delay_us((uint32_t)(len * 10UL * 1000000UL / 115200UL));
#else
        HAL_UART_Transmit(&huart1, (const uint8_t *)buf, (uint16_t)len, UART_TIMEOUT_MS);
#endif
        xSemaphoreGive(s_uart_mutex);
    }

#elif UART_GUARD_MODE == 2
    /* TODO 5: taskENTER_CRITICAL(), transmit, taskEXIT_CRITICAL().
     *         Measure in REPORT.md how long the critical section holds IRQs off. */
    taskENTER_CRITICAL();
#if ENABLE_LATENCY_REPORT
    dwt_delay_us((uint32_t)(len * 10UL * 1000000UL / 115200UL));
#else
    HAL_UART_Transmit(&huart1, (const uint8_t *)buf, (uint16_t)len, UART_TIMEOUT_MS);
#endif
    taskEXIT_CRITICAL();

#elif UART_GUARD_MODE == 3
    /* TODO 6: xQueueSend(s_gatekeeper_queue, buf, portMAX_DELAY).
     *         The writer never touches UART directly in this mode. */
    xQueueSend(s_gatekeeper_queue, buf, portMAX_DELAY);
#endif
}

#if UART_GUARD_MODE == 3
void gatekeeper_task(void *argument)
{
    (void)argument;
    for (;;) {
        char msg[MSG_LEN];
        /* TODO 7: xQueueReceive(s_gatekeeper_queue, msg, portMAX_DELAY), then
         *         HAL_UART_Transmit. This is the only task allowed to call
         *         HAL_UART_Transmit in this mode. */
        if (xQueueReceive(s_gatekeeper_queue, msg, portMAX_DELAY) == pdTRUE) {
#if ENABLE_LATENCY_REPORT
            dwt_delay_us((uint32_t)(MSG_LEN * 10UL * 1000000UL / 115200UL));
#else
            HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)MSG_LEN, UART_TIMEOUT_MS);
#endif
        }
    }
}
#endif
