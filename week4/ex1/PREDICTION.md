# Dự đoán — Week 4 / Ex1

> **Commit TRƯỚC khi gửi design doc cho mentor duyệt.**

`student_id`: 2510343

1. Với `SAMPLE_RATE_HZ` của bạn, chu kỳ lấy mẫu là bao nhiêu us? Dựa trên
   latency ISR→task thật bạn đã đo ở `week3/ex1`, dự đoán task xử lý có kịp
   `ISR_TO_PROC_DEADLINE_US` không? Trình bày phép tính, không đoán.

   - `SAMPLE_RATE_HZ = 200` → `T_sample = 1 / 200 s = 5 ms = 5000 µs`.
   - **Budget** `T_total = T_read + L_isr→task + T_proc ≤ ISR_TO_PROC_DEADLINE_US = 500 µs`.
   - `L_isr→task` đo ở `week3/ex1` là `14,67 µs` khi có
     `portYIELD_FROM_ISR()`.
   - `T_read` ở I2C `400 kHz` (fastmode) ước lượng `153 / 400000 s = 382,5 µs` trên bus.
     Burst 14 byte có `(14 + 3) × 9 = 153` xung SCL, gồm ba byte địa chỉ/thanh ghi
     và một bit ACK/NACK trên mỗi byte. `T_proc` ước lượng `50 µs` theo ngân sách thiết kế.
   - `T_total ≈ 382,5 + 14,67 + 50 = 447,17 µs` → **dự đoán đạt**, dư
     `500 − 447,17 = 52,83 µs` cho overhead và sai lệch.
   - `447,17 µs ≪ 5000 µs` nên theo ước lượng, mẫu hiện tại xử lý xong trước
     khi mẫu tiếp theo đến trong điều kiện thường.

2. Dự đoán tổng RAM (queue + stack mọi task) theo thiết kế của bạn, tính
   bằng bytes. So với `MAX_HEAP_BYTES`: còn dư bao nhiêu?
   - **Queue**. Một `sensor_sample_t` dự kiến `4 + 6 × 2 = 16 B`. Queue mẫu thô
     giữ 4 mẫu, queue gửi UART giữ 16 mẫu → `4 × 16 + 16 × 16 = 320 B`.
   - **Stack**. `process_task` dùng 128 words, `logger_task` dùng 256 words,
     Idle dự kiến dùng 128 words. Mỗi word là 4 B nên tổng stack là
     `(128 + 256 + 128) × 4 = 2048 B`.
   - **Queue + stack** là `320 + 2048 = 2368 B`. So với `MAX_HEAP_BYTES = 11264 B`,
     phần còn dư theo phép tính này là `11264 − 2368 = 8896 B`.
   - **Tính cả overhead heap**. Dự phòng `288 + 256 + 16 = 560 B` cho TCB,
     quản lý queue và `heap_4`. Tổng heap ước lượng `2368 + 560 = 2928 B`,
     còn dư **`11264 − 2928 = 8336 B`**.
3. Nếu task xử lý tạm thời chậm hơn ISR lấy mẫu (ví dụ do một task khác
   chiếm CPU), hàng đợi của bạn đầy sau bao nhiêu mẫu? Điều gì xảy ra với
   mẫu tiếp theo trong thiết kế của bạn?

   - Nếu `raw_q` ban đầu rỗng và `process_task` chưa lấy mẫu nào, queue đầy
     ngay khi ISR đưa **mẫu thứ 4** vào. Tính từ mẫu đầu tiên tại `t = 0`,
     queue đầy ở `(4 − 1) × 5 = 15 ms`.
   - Nếu queue vẫn đầy khi **mẫu thứ 5** đến ở `4 × 5 = 20 ms`,
     `xQueueSendFromISR()` trả về `errQUEUE_FULL`. ISR bỏ mẫu mới, tăng
     `drop_raw` và giữ nguyên bốn mẫu đã có theo thứ tự FIFO.

4. Bạn dự đoán priority nào (ISR task xử lý vs. task ghi UART) sẽ cho kết
   quả đạt deadline tốt hơn? Vì sao?

   - **Thời gian gửi UART**. Một khối có `16 × 16 = 256 B`. Với `115200 baud`
     và `8N1` (`10 bit/byte`), thời gian truyền lý tưởng là
     `256 × 10 / 115200 ≈ 22,22 ms`. Một khối mới được tạo mỗi `16 / 200 = 80 ms`,
     nên tốc độ UART đủ cho luồng dữ liệu dự kiến.
   - **Ảnh hưởng lên deadline**. Giả sử UART vừa bắt đầu gửi khối và mẫu kế
     tiếp đến sau `5 ms`. UART còn khoảng `22,22 − 5 = 17,22 ms` để gửi. Nếu
     `logger_task` dùng polling, giữ CPU ở priority cao hơn và mẫu phải chờ
     đến hết lượt gửi, thời gian chờ này đã vượt deadline `500 µs = 0,5 ms`.
   - **Chọn `process_task` priority `2`, `logger_task` priority `1`**. Với
     preemption bật, khi ISR đánh thức `process_task`, `portYIELD_FROM_ISR()`
     cho phép task này chạy trước logger sau khi thoát ngắt. Theo câu 1,
     đường xử lý đã dự kiến mất `447,17 µs`, chỉ còn `500 − 447,17 = 52,83 µs`
     cho độ trễ phát sinh. Khi queue mẫu thô rỗng, `process_task` block và
     `logger_task` tiếp tục gửi. Priority Timer ISR được cấu hình riêng ở NVIC.