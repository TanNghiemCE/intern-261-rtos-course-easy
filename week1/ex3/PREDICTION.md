# Dự đoán — Week 1 / Ex3

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510343    `PERIOD_MS` = 13ms    `WORK_JITTER_US` = 600us    `SAMPLE_COUNT` = 500

1. Bảng dự đoán (điền trước khi đo):

| # | Delay | Tick | Chu kỳ tb (ms) | Jitter (us) | Drift tổng sau SAMPLE_COUNT (ms) |
|---|---|---|---|---|---|
| 1 | `vTaskDelay` | 1000 | 13.3 |600 | 150|
| 2 | `xTaskDelayUntil` | 1000 | 13 | 0  | 0|
| 3 | `vTaskDelay` | 100 | 10.3 |600  | 1350(1.3tick bị căt xuống 1tick) |
| 4 | `xTaskDelayUntil` | 100  | 10 |0 | 1500 |

2. Cấu hình nào có drift lớn nhất? Vì sao, viết bằng công thức có `WORK_JITTER_US`. 
- Nếu chỉ xét riêng drift do tải gây ra thì cấu hình 1 có drift lớn nhất. `DRIFT = SAMPLE_COUNT * WORK_JITTER_US/2 = 150
3. `PERIOD_MS` = 13ms . Với tick 100 Hz, một tick = 10 ms, nên chu kỳ thực tế sẽ
   bị làm tròn thành 10 ms. Sai số hệ thống: 23.07 %.
4. Jitter ở tick 1000 Hz so với 100 Hz: lớn hơn / nhỏ hơn / bằng? Vì sao? -> Bằng nhau, vì tải biến thiên W = 600us được điều khiển bởi DWT không đổi giữa 2 cấu hình
5. Nếu task này **không** phải ưu tiên cao nhất, đại lượng nào trong bảng bị ảnh hưởng
   nhiều nhất? Jitter bị ảnh hưởng nhiều nhất. Vì khi task có ưu tiên cao hơn chen ngang, thời gian thực thi sẽ bị kéo dài ngẫu nhiên tùy thuộc vào thời gian chạy của task ưu tiên cao hơn đó.
