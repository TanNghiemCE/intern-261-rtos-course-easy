/* Week 3 / Ex2 -- debounce via one-shot timer + long-press via auto-reload
 * timer. Native FreeRTOS API only.
 */
#include <stdio.h>

#include "FreeRTOS.h"
#include "timers.h"

#include "main.h"
#include "params.h"
#include "debounce.h"

static TimerHandle_t s_debounce_timer;
static TimerHandle_t s_longpress_timer;
static volatile uint8_t s_btn_is_down = 0U;
#define PORTA  GPIOA
#define PA7           (1U<<7)
static void debounce_callback(TimerHandle_t xTimer)
{
    (void)xTimer;

    /* TODO 1: printf/HAL_UART "PRESS" + xTaskGetTickCount(). This is the
     *         stable-press event, DEBOUNCE_MS after the last edge.
     *
     * TODO 2 (temporary, for the README step-4 experiment ONLY): add
     *         vTaskDelay(pdMS_TO_TICKS(50)) here, capture what happens to
     *         longpress_timer's callback timing in your REPORT.md, then
     *         DELETE this line again before you submit. A callback that
     *         blocks here is blocking the ONE daemon task every other
     *         software timer in the system also depends on. */
     if (s_btn_is_down != 0U) {
        // vTaskDelay(pdMS_TO_TICKS(50));
        /* Tín hiệu nút nhấn đã ổn định ở trạng thái NHẤN suốt DEBOUNCE_MS (15 ms) */
        HAL_GPIO_WritePin(PORTA, PA7, GPIO_PIN_SET);
        printf("PRESS %lu\r\n", (unsigned long)xTaskGetTickCount());
    }
    else {
        HAL_GPIO_WritePin(PORTA, PA7, GPIO_PIN_RESET);
    }
    
}

static void longpress_callback(TimerHandle_t xTimer)
{
    (void)xTimer;

    /* TODO 3: printf/HAL_UART "HELD" + xTaskGetTickCount(). Fires once per
     *         LONGPRESS_MS while the button stays down. */
     printf("HELD %lu\r\n", (unsigned long)xTaskGetTickCount());
}

void debounce_init(void)
{
    /* TODO 4: s_debounce_timer = xTimerCreate(..., pdMS_TO_TICKS(DEBOUNCE_MS),
     *         pdFALSE, NULL, debounce_callback); -- pdFALSE = one-shot.
     *         s_longpress_timer = xTimerCreate(..., pdMS_TO_TICKS(LONGPRESS_MS),
     *         pdTRUE, NULL, longpress_callback); -- pdTRUE = auto-reload.
     *         Check both handles non-NULL. Do NOT xTimerStart() either here --
     *         they are driven entirely from debounce_on_edge(). */
     s_debounce_timer = xTimerCreate(
        "deb",                        
        pdMS_TO_TICKS(DEBOUNCE_MS),    
        pdFALSE,                        
        NULL,                   
        debounce_callback           
    );
     s_longpress_timer = xTimerCreate(
        "lpress",                      
        pdMS_TO_TICKS(LONGPRESS_MS),   
        pdTRUE,                         
        NULL,                
        longpress_callback              
    );
    configASSERT(s_longpress_timer != NULL);
    configASSERT(s_debounce_timer != NULL);
}

void debounce_on_edge(uint8_t pin_is_high)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // lưu trang thái
    s_btn_is_down = pin_is_high;
    /* TODO 5: xTimerResetFromISR(s_debounce_timer, &xHigherPriorityTaskWoken)
     *         on every edge -- this is what swallows bounce: each new edge
     *         restarts the DEBOUNCE_MS countdown, so the callback only fires
     *         once the line has been quiet for DEBOUNCE_MS. */

    /* TODO 6: if (pin_is_high) xTimerStartFromISR(s_longpress_timer, ...);
     *         else xTimerStopFromISR(s_longpress_timer, ...); */

    /* TODO 7: portYIELD_FROM_ISR(xHigherPriorityTaskWoken). */
    (void)pin_is_high;
    (void)xHigherPriorityTaskWoken;
    BaseType_t xResDeb = xTimerResetFromISR(s_debounce_timer, &xHigherPriorityTaskWoken);
    configASSERT(xResDeb == pdPASS);
      if (pin_is_high != 0U) {
        BaseType_t xResLp = xTimerStartFromISR(s_longpress_timer, &xHigherPriorityTaskWoken);
        configASSERT(xResLp == pdPASS);
    } else {
        BaseType_t xResLp = xTimerStopFromISR(s_longpress_timer, &xHigherPriorityTaskWoken);
        configASSERT(xResLp == pdPASS);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
