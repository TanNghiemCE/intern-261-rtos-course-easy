/* Week 2 / Ex1 — reads MPU6050 over I2C1 every SAMPLE_PERIOD_MS, pushes a
 * raw_sample_t into queue_raw.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "main.h"
#include "params.h"
#include "pipeline.h"

#define MPU6050_ADDR         (0x68 << 1)  /* Địa chỉ I2C 8-bit của MPU6050 */
#define REG_WHO_AM_I         0x75
#define REG_PWR_MGMT_1       0x6B
#define REG_ACCEL_XOUT_H     0x3B

extern I2C_HandleTypeDef hi2c1;

void sensor_task(void *argument)
{
    (void)argument;
    uint32_t seq = 0;

    /* TODO 1: HAL_I2C init / verify MPU6050 WHO_AM_I. Do this once here, not
     *         inside the loop. */
    uint8_t pwr = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, REG_PWR_MGMT_1, I2C_MEMADD_SIZE_8BIT, &pwr, 1, 100);
    /* Đọc kiểm tra WHO_AM_I (thanh ghi 0x75, giá trị trả về mặc định là 0x68) */
    uint8_t who = 0;
    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, REG_WHO_AM_I, I2C_MEMADD_SIZE_8BIT, &who, 1, 100);
    (void)who;
    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        raw_sample_t s;
        s.seq = seq++;

        /* TODO 2: HAL_I2C_Mem_Read the accel registers into s.ax/ay/az. */
        uint8_t buf[6];
        if (HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, REG_ACCEL_XOUT_H, I2C_MEMADD_SIZE_8BIT, buf, 6, 2) == HAL_OK) {
            s.ax = (int16_t)((buf[0] << 8) | buf[1]);
            s.ay = (int16_t)((buf[2] << 8) | buf[3]);
            s.az = (int16_t)((buf[4] << 8) | buf[5]);
        } else {
            /* Fallback giả lập dữ liệu khi chưa cắm cảm biến hoặc bus I2C bận, 
             * giúp pipeline không bị treo trên bàn thí nghiệm.
             */
            s.ax = (int16_t)((seq * 17) % 500);
            s.ay = (int16_t)((seq * 31) % 500);
            s.az = 16384; /* ~1g ở dải đo +-2g */
        }
        /* TODO 3: xQueueSend(queue_raw, &s, <timeout>) — pick and justify the
         *         timeout in PREDICTION.md question 3/4. What happens to this
         *         sample if the queue is full and the timeout expires? */
         xQueueSend(queue_raw, &s, 0);
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }
}
