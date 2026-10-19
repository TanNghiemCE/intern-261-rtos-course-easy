# Dự đoán — Week 2 / Ex1

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2519343

1. Phép tính kích thước queue tối thiểu (dùng `SAMPLE_PERIOD_MS`, thời gian xử lý
   ước lượng của `filter_task` và `uart_task` ở `UART_BAUD`):

   ```
   1 mẫu tin "seq,ax,ay,az\r\n" ≈ 24 bytes
   Khung truyền UART 8N1: 1 start bit + 8 data bits + 1 stop bit = 10 bits/byte
   t_byte = 10 / 115200 = 86.81 μs
   t_uart = N_byte × t_byte = 24 × 0.0868 ms ≈ 2.08 ms (thời gian uart_task xử lý 1 mẫu)
   thời gian xử lý `filter_task` Thuật toán Moving Average o(1) chỉ mất ∼2−5 𝜇s
   Thời gian truyền UART nhỏ hơn thời gian lấy mẫu
   ```

   Kích thước tối thiểu dự đoán: `queue_raw` = 1 , `queue_filtered` = 1 .
   So với `QUEUE_LENGTH` được cấp: đủ hay không? Vì sao?
   So với `QUEUE_LENGTH = 8`: Đủ dư, hàng đợi bình thường hầu như luôn ở mức 0 hoặc 1.

2. Khi hạ `UART_BAUD` xuống 9600 (tạm thời), queue nào sẽ đầy trước? Vì sao?
`queue_filtered` đầy trước, ở 9600 baud, 1 mẫu mất tới 24*10/9600 ≈ 25ms, sensor_task sinh mẫu mỗi 5 ms và filter_task chuyển tiếp tức thì sang `queue_filtered`. Do uart_task chỉ tiêu thụ được 40 mẫu/giây trong khi nhận vào 200 mẫu/giây (quá tải 500%), queue_filtered sẽ đầy 8 phần tử sau chỉ khoảng 50 ms.
3. Với `xQueueSend(..., 0)` (timeout = 0) khi queue đầy: điều gì xảy ra với mẫu đó? 
Trả về ngay errQUEUE_FULL, mẫu bị drop tức thì. Task gửi không bị chặn, chu kỳ lấy mẫu 5 ms được giữ nguyên vẹn. Trên terminal, số seq sẽ nhảy cóc (mất 4 mẫu mỗi 5 chu kỳ lấy mẫu).
4. Với `xQueueSend(..., portMAX_DELAY)` khi queue đầy: `sensor_task` sẽ ở trạng thái
   nào? Có nguy cơ gì cho chu kỳ lấy mẫu?
sensor_task bị chuyển sang trạng thái Blocked để đợi chỗ trống. Chu kỳ lấy mẫu định kỳ 5 ms bị phá vỡ hoàn toàn, tốc độ lấy mẫu bị kéo tụt xuống theo tốc độ chậm của UART (25 ms), gây trôi chu kỳ (jitter) nghiêm trọng
5. Độ đầy `queue_raw` theo thời gian ở cấu hình bình thường sẽ trông như thế nào
   (tăng dần / dao động quanh một mức / luôn gần 0)? Vì sao?
Luôn dao động quanh 0–1 (gần như luôn bằng 0) vì filter_task có độ ưu tiên cao hơn uart_task và xử lý O(1) tức thì ngay khi sensor_task vừa nhả mẫu.
