#ifndef TASKS_LMH_H
#define TASKS_LMH_H

/* 0 = binary semaphore (produces inversion), 1 = mutex (priority inheritance).
 * Build this file twice over the exercise, switching RESOURCE_MODE. */
#ifndef RESOURCE_MODE
#define RESOURCE_MODE 0
#endif

void resource_init(void);
void dwt_init(void);
void dwt_busy_wait_us(uint32_t microseconds);

void task_low(void *argument);
void task_mid(void *argument);
void task_high(void *argument);


#endif /* TASKS_LMH_H */
