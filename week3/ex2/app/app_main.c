/* Week 3 / Ex2 -- software timer debounce.
 * Native FreeRTOS API only.
 */
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "params.h"
#include "debounce.h"

#define PORTB               GPIOB
#define PB11                GPIO_PIN_11
#define BTN_GPIO_PIN        PB11
#define BTN_EXTI_IRQn       EXTI15_10_IRQn

#define PORTA               GPIOA
#define PA7                 GPIO_PIN_7
#define ACK_GPIO_PIN        PA7

#define UART_BAUDRATE       (115200UL)

static void uart1_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    /* PA9: USART1_TX (Alternate Function Push-Pull) */
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA10: USART1_RX (Input Floating / Pull-Up) */
    GPIO_InitStruct.Pin  = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Cấu hình thanh ghi USART1: 115200 8N1 dựa trên tần số PCLK2 thực tế (72 MHz) */
    USART1->CR1 = 0U;
    USART1->CR2 = 0U;
    USART1->CR3 = 0U;

    uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();
    USART1->BRR = (pclk2 + (UART_BAUDRATE / 2U)) / UART_BAUDRATE;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;

    /* Tắt buffer của stdout để printf đẩy ra UART ngay lập tức */
    setvbuf(stdout, NULL, _IONBF, 0);
}

/* Retarget printf() trong debounce.c sang USART1 */
int _write(int file, char *ptr, int len)
{
    (void)file;
    for (int i = 0; i < len; i++) {
        while ((USART1->SR & USART_SR_TXE) == 0U) {
        }
        USART1->DR = (uint8_t)ptr[i];
    }
    while ((USART1->SR & USART_SR_TC) == 0U) {
    }
    return len;
}

void exti_button_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    /* Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(PORTA, ACK_GPIO_PIN, GPIO_PIN_RESET);

    /* Configure GPIO pin : PA7 */
    GPIO_InitStruct.Pin = ACK_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PORTA, &GPIO_InitStruct);

    /* Configure GPIO pin : PB11 */
    GPIO_InitStruct.Pin = BTN_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(PORTB, &GPIO_InitStruct);

    /* Xóa cờ ngắt tồn đọng trước khi bật NVIC */
    __HAL_GPIO_EXTI_CLEAR_IT(BTN_GPIO_PIN);
    HAL_NVIC_ClearPendingIRQ(BTN_EXTI_IRQn);

    HAL_NVIC_SetPriority(BTN_EXTI_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(BTN_EXTI_IRQn);
}

void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(PB11);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BTN_GPIO_PIN) {
        uint8_t is_button_down =
            (HAL_GPIO_ReadPin(PORTB, PB11) == GPIO_PIN_RESET) ? 1U : 0U;

        debounce_on_edge(is_button_down);
    }
}

void app_main(void)
{
    /* TODO: GPIO init for the button pin as EXTI, both edges
     *       (GPIO_MODE_IT_RISING_FALLING) -- you need every bounce edge, not
     *       just the first one. */

    uart1_init();
    debounce_init();
    exti_button_init();

    printf("W3.2 Ready: DEBOUNCE_MS=%d, LONGPRESS_MS=%d\r\n", DEBOUNCE_MS, LONGPRESS_MS);

    vTaskStartScheduler();

    for (;;) {
    }
}
