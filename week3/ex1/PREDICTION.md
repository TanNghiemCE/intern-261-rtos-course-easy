# Dự đoán — Week 3 / Ex1

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510343

1. Với `ISR_NVIC_PRIO` của bạn và nhóm priority `NVIC_PRIORITYGROUP_4`, số này
   nằm trong khoảng cho phép (`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` ..
   `configLIBRARY_LOWEST_INTERRUPT_PRIORITY`) không? Trình bày phép so sánh.
   Có nằm trong khoảng cho phép. Vì `ISR_NVIC_PRIO = 6` , `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5` và `configLIBRARY_LOWEST_INTERRUPT_PRIORITY = 15`, ta có `5 <= 6 <= 15` (trên thanh ghi 8-bit sau khi dịch trái 4 bit: `0x50 <= 0x60 <= 0xF0`)
   nên ISR được phép gọi các API có ưu tiên thấp hơn hoặc bằng ngưỡng `...FromISR` một cách an toàn.
2. Khi đặt priority EXTI cao hơn ngưỡng cho phép rồi gọi
   `vTaskNotifyGiveFromISR`, bạn dự đoán hành vi quan sát được là gì (reset?
   treo? thông báo gì trên UART/debugger)?
   Ngay lần đầu tiên sườn xuống trên `PB11` kích hoạt ISR và gọi `vTaskNotifyGiveFromISR`, hàm `vPortValidateInterruptPriority()` sẽ phát hiện `ucCurrentPriority < ucMaxSysCallPriority` (ví dụ mức `2` ứng với `0x20 < 0x50`) và kích hoạt `configASSERT`. Hệ thống sẽ **treo đứng (halt)** trong vòng lặp vô hạn `for(;;)` của `configASSERT` với ngắt bị khóa (`taskDISABLE_INTERRUPTS()`), chân `PA7` không đảo trạng thái (hoặc in ra thông báo file/dòng lỗi trên UART nếu `configASSERT` có gọi hàm log trước khi lặp vô hạn).
3. Dự đoán latency ISR → task (có `portYIELD_FROM_ISR`) theo đơn vị us, dựa
   trên tần số CPU 72 MHz và số lệnh ước lượng trong đường đi ISR → context
   switch. So với `LATENCY_BUDGET_US`: đạt hay không?
   Ở `72 MHz` (`1 us = 72 chu kỳ CPU`), tổng đường đi từ sườn xuống `PB11` qua phần cứng vào ngắt (12 cycles) + `HAL_GPIO_EXTI_IRQHandler` (~40 cycles) + `vTaskNotifyGiveFromISR` (~80 cycles) + chuyển ngữ cảnh `PendSV_Handler` (~80 cycles) + thoát `ulTaskNotifyTake` và gọi `HAL_GPIO_TogglePin` trên `PA7` (~50 cycles) tốn khoảng **250 – 450 chu kỳ CPU**, tương đương **`~3.5 us – 6.5 us`** (chắc chắn `< 15 us`). So với `LATENCY_BUDGET_US = 50 us`: **Đạt** (nhỏ hơn ngưỡng cho phép khoảng 8–10 lần).
4. Nếu bỏ `portYIELD_FROM_ISR`, `ack_task` sẽ chạy **khi nào** (ngay lập tức
   hay phải chờ tick ngắt kế tiếp)? Latency tăng thêm cỡ bao nhiêu?
   Nếu bỏ `portYIELD_FROM_ISR`, ngắt `PendSV` không được kích hoạt lúc thoát ISR nên CPU quay lại task bị ngắt trước đó (ví dụ `Idle Task`), và `ack_task` **phải chờ đến ngắt `SysTick` kế tiếp** mới được lập lịch chạy. Với `configTICK_RATE_HZ = 1000 Hz` (`1 tick = 1000 us`) và thời điểm bấm nút hoàn toàn ngẫu nhiên so với nhịp `SysTick`, latency sẽ tăng thêm từ **`0 us` đến `1000 us`** (trung bình **`~500 us`**, cực đại **`~1000 us`**), vượt xa `LATENCY_BUDGET_US = 50 us`.
