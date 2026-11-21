# Report — Week 4 / Ex1 (Design doc)

`student_id`: 2510343    Commit của PREDICTION.md: f0779cb

> File này ĐÓNG VAI TRÒ design doc 2–3 trang nộp cho mentor duyệt trước khi
> viết code. Điền đủ số liệu — không mô tả chung chung.

## 1. Task diagram
Mô tả (sơ đồ ASCII hoặc ảnh chèn vào) luồng: ISR lấy mẫu MPU6050 → hàng đợi
→ task xử lý → task ghi UART. Ghi rõ priority từng task.

```text
Timer ISR (200 Hz, moi 5 ms)
  t0: Vao ISR, ghi timestamp DWT va doc MPU6050 qua I2C
  |
  | xQueueSendFromISR() + portYIELD_FROM_ISR()
  v
raw_q (4 mau x 16 B)
  |
  v
process_task (priority 2)
  t1: Nhan va bat dau xu ly mot mau
      Xu ly, dong goi va thu gui vao log_q voi timeout 0 tick
  t2: Xu ly xong. Deadline tu t0 den t2 <= 500 us
  |
  v
log_q (16 mau x 16 B)
  |
  v
logger_task (priority 1)
  Gom 16 mau thanh mot khoi 256 B
  |
  v
USART1 (115200 baud, 8N1)
```

## 2. Timing budget

| Đại lượng | Công thức / nguồn số liệu | Giá trị |
|---|---|---|
| Chu kỳ lấy mẫu | `1 / SAMPLE_RATE_HZ = 1 / 200 s` | `5000 µs (5 ms)` |
| Latency ISR→task (đo thật, week3/ex1) | Từ cạnh EXTI đến lúc `ack_task` chạy, có `portYIELD_FROM_ISR()`. [Week 3 / Ex1, mục 2](../../week3/ex1/REPORT.md#2-latency-isr--task) | `14,67 µs` |
| Thời gian xử lý 1 mẫu (ước lượng) | `T_proc = t2 − t1`, gồm xử lý, đóng gói và thử gửi vào `log_q`. [Ngân sách trong PREDICTION.md](PREDICTION.md) | `50 µs` |
| Tổng so với `ISR_TO_PROC_DEADLINE_US` | `T_read + L_isr→task + T_proc ≈ 382,5 + 14,67 + 50 = 447,17 µs` | **Dự đoán đạt** (`447,17 µs < 500 µs`), dư `52,83 µs` |
## 3. RAM budget
| Thành phần | Kích thước | Ghi chú |
|---|---|---|
| Queue (độ dài × cỡ phần tử) | `4 × 16 + 16 × 16 = 320 B` | `raw_q` giữ 4 mẫu, `log_q` giữ 16 mẫu. Một mẫu chiếm `4 + 6 × 2 = 16 B`. |
| Stack task lấy mẫu | `0 B` heap | Timer ISR dùng stack MSP. |
| Stack task xử lý | `128 words × 4 = 512 B` | Tham chiếu `dyn_task` ở tuần 1. Stack dùng tối đa quan sát được là `128 − 103 = 25 words`. |
| Stack task ghi UART | `256 words × 4 = 1024 B` | Tham chiếu `ram_report_task` ở tuần 1. Stack dùng tối đa quan sát được là `256 − 104 = 152 words`. |
| Stack Idle task | `128 words × 4 = 512 B` | Giả định Idle cấp phát động với `configMINIMAL_STACK_SIZE = 128`. |
| Overhead cấp phát | `288 + 256 + 16 = 560 B` | `3 × (80 + 8 + 8) B` cho TCB và header cấp phát của 3 task. Dự phòng `2 × 128 B` cho quản lý hai queue và `16 B` cho sentinel và căn lề `heap_4`. |
| **Tổng heap dự kiến** | **`320 + 512 + 1024 + 512 + 560 = 2928 B`** | `MAX_HEAP_BYTES = 11264 B`, còn dư **`11264 − 2928 = 8336 B`**. |

## 4. Lựa chọn API đồng bộ
- **Chọn queue** cho `raw_q` và `log_q`. Mỗi lần gửi, queue sao chép
  đúng một `sensor_sample_t` (16 B) và giữ thứ tự mẫu. Timer ISR đưa mẫu vào
  `raw_q` bằng `xQueueSendFromISR()`. Hai task dùng `xQueueReceive()` để chờ
  khi queue của mình rỗng. `process_task` chuyển kết quả vào `log_q` bằng
  `xQueueSend(log_q, &sample, 0)` để trả về ngay khi queue đầy.
- **Event group** phù hợp cho cờ trạng thái hoặc báo lỗi. Để chuyển mẫu, cần
  thêm bộ đệm riêng và xử lý trường hợp nhiều lần set cùng một bit bị gộp.
- **Stream buffer** phù hợp cho luồng byte. Dùng cho mẫu 16 B thì phải kiểm tra
  số byte ghi/đọc và tự ghép đủ một mẫu. Queue giữ sẵn ranh giới từng mẫu.
## 5. Xử lý khi hàng đợi đầy
- **Bỏ mẫu mới** Nếu `raw_q` (4 slot) đầy, Timer ISR nhận
  `errQUEUE_FULL` từ `xQueueSendFromISR()`, bỏ mẫu vừa đọc và tăng `drop_raw`.
  Ví dụ, từ queue rỗng và task chưa lấy mẫu nào, mẫu thứ 4 lấp slot cuối, mẫu
  thứ 5 bị bỏ nếu queue vẫn đầy.
- Nếu `log_q` (16 slot) đầy, `process_task` bỏ kết quả vừa xử lý và tăng
  `drop_log`. Timeout `0 tick` giúp task tiếp tục để xử lý mẫu sau ngay cả
  khi UART đang bận.
- **Báo cáo ex3.** Ghi mức tăng của `drop_raw` và `drop_log` trong từng lần
  chạy, kèm điều kiện tải. Hai bộ đếm cho biết mẫu rớt ở chặng ISR → xử lý
  hay ở chặng xử lý → UART.
## 6. Mentor duyệt
Chữ ký / ghi chú của mentor sau buổi duyệt thiết kế: ______
