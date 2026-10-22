#ifndef PIPELINE_H
#define PIPELINE_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

/* One raw sample from MPU6050, passed sensor_task -> filter_task. */
typedef struct {
    uint32_t seq;   /* monotonically increasing — the only honest way to count
                     * dropped/late samples downstream */
    int16_t  ax, ay, az;
} raw_sample_t;

/* One filtered sample, passed filter_task -> uart_task. */
typedef struct {
    uint32_t seq;
    int16_t  raw_ay;  /* <-- THÊM: Mẫu thô ay để đối chiếu độ trễ */
    int16_t  ax, ay, az;

} filtered_sample_t;

extern QueueHandle_t queue_raw;
extern QueueHandle_t queue_filtered;

void sensor_task(void *argument);
void filter_task(void *argument);
void uart_task(void *argument);

/* Periodic queue-depth sampler used to produce the fill-over-time plot.
 * TODO: decide and document where this stores/sends its samples. */
void depth_logger_task(void *argument);

#endif /* PIPELINE_H */
