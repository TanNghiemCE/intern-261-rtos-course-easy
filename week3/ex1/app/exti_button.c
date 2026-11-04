/* Week 3 / Ex1 -- EXTI button wakes a task via task notification.
 * Native FreeRTOS API only.
 */
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include <stdint.h>
#include "exti_button.h"
#define ACK_GPIO_PIN   ((uint16_t)(1U << ACK_PIN))
#define EXTI_GPIO_PIN  ((uint16_t)(1U << EXTI_PIN))
static TaskHandle_t s_ack_task_handle;

// ánh xạ chân ngắt
static IRQn_Type prv_get_exti_irqn(uint32_t pin){
     switch (pin)
    {
        case 0:  return EXTI0_IRQn;
        case 1:  return EXTI1_IRQn;
        case 2:  return EXTI2_IRQn;
        case 3:  return EXTI3_IRQn;
        case 4:  return EXTI4_IRQn;
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:  return EXTI9_5_IRQn;
        default: return EXTI15_10_IRQn;
    }
}


void exti_button_init(void)
{
    /* TODO 1: GPIO init for PB<EXTI_PIN> as EXTI falling edge (GPIO_MODE_IT_FALLING,
     *         pull-up), and PA<ACK_PIN> as push-pull output.
     *
     * TODO 2 (do this FIRST, on purpose): HAL_NVIC_SetPriority(EXTIx_IRQn, <n>, 0)
     *         with <n> deliberately SMALLER (= higher hardware priority) than
     *         configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY. Run step 4 of the
     *         README with this wrong value first and capture the configASSERT.
     *         Only after that, set it to ISR_NVIC_PRIO and HAL_NVIC_EnableIRQ. */
    IRQn_Type irqn = prv_get_exti_irqn(EXTI_PIN);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA,ACK_GPIO_PIN, GPIO_PIN_RESET);
  /*Configure GPIO pin : PA7 */
  GPIO_InitStruct.Pin = ACK_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB11 */
  GPIO_InitStruct.Pin = EXTI_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
//xóa cờ ngắt tồn động trước khi bật NVIC
  __HAL_GPIO_EXTI_CLEAR_IT((uint16_t)(EXTI_GPIO_PIN));
    HAL_NVIC_ClearPendingIRQ(irqn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn,ISR_NVIC_PRIO,0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void exti_button_isr_handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* TODO 3: vTaskNotifyGiveFromISR(s_ack_task_handle, &xHigherPriorityTaskWoken).
     *
     * TODO 4: portYIELD_FROM_ISR(xHigherPriorityTaskWoken) -- the README asks
     *         you to temporarily remove exactly this line in step 6 to measure
     *         the latency penalty. Do NOT remove it in the version you submit. */
    (void)xHigherPriorityTaskWoken;
    if (s_ack_task_handle != NULL){
        vTaskNotifyGiveFromISR(s_ack_task_handle,&xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void ack_task(void *argument)
{
    (void)argument;
    s_ack_task_handle = xTaskGetCurrentTaskHandle();

    for (;;) {
        /* TODO 5: ulTaskNotifyTake(pdTRUE, portMAX_DELAY). */
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        /* TODO 6: toggle PA<ACK_PIN> here, nothing before it in this loop body --
         *         any extra work here inflates the latency you are measuring. */
         HAL_GPIO_TogglePin(GPIOA,ACK_GPIO_PIN);
    }
}
