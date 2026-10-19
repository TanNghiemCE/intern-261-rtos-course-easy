# Report — Week 1 / Ex3

`student_id`: 2510343    Commit của PREDICTION.md: 16/10/2026
## 1. Bảng thống kê (tính từ CSV, không đọc tay)
| # | Delay | Tick | min (ms) | max (ms) | tb (ms) | jitter = max−min (us) | drift tổng (ms) |
|---|---|---|---|---|---|---|---|
| 1 | `vTaskDelay` | 1000 |12.998 |13.999 |13.461 |1001.0 |223.373 |
| 2 | `xTaskDelayUntil` | 1000 |12.998 |12.999 |12.999 |1.0 | -0.603 |
| 3 | `vTaskDelay` | 100 |9.999 |10.000 |9.999 |1.0 |-1500.466|
| 4 | `xTaskDelayUntil` | 100 |9.999 |10.000 |9.999 |1.0 |-1500.466 |

- Script dùng để tính: `app/calc_stats.py`
- CSV gốc: `csv_file/` (`cfg1_vtaskdelay_1000hz.csv`, `cfg2_xtaskdelayuntil_1000hz.csv`, `cfg3_vtaskdelay_100hz.csv`, `cfg4_xtaskdelayuntil_100hz.csv`)
- Ảnh: `![](w1_ex3_image/ex3_cfg1.png)` `![](w1_ex3_image/ex3_cfg2.png)` `![](w1_ex3_image/ex3_cfg3.png)` `![](w1_ex3_image/ex3_cfg4.png)`

## 2. Tại sao `vTaskDelay` bị drift
Công thức drift theo `PERIOD_MS` và `WORK_JITTER_US` của **bạn**, rồi so với số đo:
- Xác suất tải vượt qua 1000us: 1800-1000/1800 = 44.44%
- Drift tổng : 500*44.44% * 1 = 222.22ms
|---|---|---|---|
| Drift tổng (ms) |222.22 |223.373 |1.153 |

## 3. Tại sao tick 100 Hz sai nhiều hơn
`PERIOD_MS` = 13ms ; một tick = 10  ms. Chứng minh bằng số đo, không bằng lý thuyết: 
- Chu kỳ thực tế ở cấu hình 3 và 4 đều là 9.999ms ( thay vì 13.000ms), sai số : 23.08%
- Sau 500 chu kỳ, tổng thời gian đo được chỉ đạt 4999.5ms ( thiếu hụt so với 6500ms lý thuyết), dẫn tới drift đo được là -1500.466ms

## 4. `xTaskDelayUntil` trả về gì
Giá trị trả về nghĩa là gì, và trong 4 lần chạy của bạn nó có bao giờ báo
"đã trễ deadline" không? Bao nhiêu lần / `SAMPLE_COUNT`?
- Giá trị trả về kiêu BaseType_t : pdTRUE ( delay thành công), task được đưa vào trạng thái BLOCKED để chờ đúng thời điểm hoạt động; pdFALSE(trể deadline), thời gian đánh thức tiếp theo đã trôi qua trong quá khứ.
- Không báo trễ dealine

## 5. Lệch so với dự đoán
Mỗi dòng lệch trong bảng → lệch bao nhiêu + nguyên nhân vật lý.

### Bảng so sánh Dự đoán vs Đo thực tế:
| # | Cấu hình | Chu kỳ tb (ms)<br>Dự đoán / Đo / Lệch | Jitter (us)<br>Dự đoán / Đo / Lệch | Drift tổng (ms)<br>Dự đoán / Đo / Lệc | Tóm tắt nguyên nhân vật lý |
|:---:|---|:---:|:---:|:---:|---|
| 1 | `vTaskDelay` (1000 Hz) | 13.3 / 13.447 / +0.147 | 1800 / 1001.0 / -799.0 | 450 / 223.373 / -226.627 | FreeRTOS lượng tử hóa tick 1 ms: chỉ khi tải W >= 1000 us (44.44%) mới trượt 1 tick (1 ms). Jitter bị chặn ở 1 ms, drift = 500 * 44.44% * 1 ms ≈ 222.2 ms. |
| 2 | `xTaskDelayUntil` (1000 Hz) | 13 / 12.999 / -0.001 | 0 / 1.0 / 1.0 | 0 / -0.603 / -0.603 | `xTaskDelayUntil` khóa tuyệt đối chu kỳ. Jitter 1.0 us là sai sô lấy mẫu Logic Analyzer; drift âm -0.6 ms do sai số thạch anh . |
| 3 | `vTaskDelay` (100 Hz) | 10.3 / 9.999 / -0.301 | 1800 / 1.0 / -1799.0 | 1350 / -1500.466 / Lệch dấu | 1 tick ở 100 Hz dài 10 ms; tải tối đa 1.8 ms < 10 ms nên task luôn gọi delay trước khi hết tick hiện tại, luôn thức đúng tick kế tiêếeeeeps |
| 4 | `xTaskDelayUntil` (100 Hz) | 10.0 / 9.999 / -0.001 | 0 / 1.0 / +1.0 | 1500 / -1500.466 / -0.466 | `pdMS_TO_TICKS(13)` bị chia nguyên làm tròn xuống 1 tick = 10 ms. Mất 3 ms mỗi chu kỳ → sau 500 chu kỳ trôi -1500 ms. Lẻ -0.466 ms do thạch anh. |
## 6. Câu hỏi đánh đổi
Tick 1000 Hz cho độ chính xác tốt hơn nhưng **tốn** gì?
-> Tốn chu kỳ CPU ( ngắt Systick chạy 1000 lần mỗi giây), tốn năng lượng tiêu thụ ( mỗi ms hệ thống bị đánh thức 1 lần)
Dựa vào dữ liệu của bạn:
đo CPU overhead của tick ISR ở 100 Hz và 1000 Hz (gợi ý: cho một task ưu tiên thấp
nhất đếm vòng lặp trong 1 giây, so sánh 2 cấu hình). Điền số:

| Tick rate | Số vòng lặp idle đếm được trong 1 s | Overhead suy ra (%) |
|---|---|---|
| 100 Hz | 2654210 | 0|
| 1000 Hz | 2632510 | 0.818  |
- Công thức tính: `(N100 - N1000)/N100 = 0.818`

Với ứng dụng `PERIOD_MS` = 13ms của bạn, bạn chọn tick rate nào? Bảo vệ bằng số trên.
- Chọn ticks rate = 1000Hz
- Bảo vệ: độ chính xác cao, nếu chọn 1000Hz thì 1 ticks tương đương 1 ms, không bị cắt phần thập phân. Nếu chọn 100Hz, pdMS_TO_TICKS(13) bị chia nguyên thành 1 tick= 10ms, chu kỳ lệch 23.07%, và gây trôi thời gian tích lũy (-1500.466ms - mất 1.5 giây sau 500 chu kỳ). Nếu chọn 1000Hz, 13ms tương ứng 13 ticks, khi dùng `xTaskDelayUntil(), chu kỳ đo được là 12.999ms, sai số nhỏ, drift gần như triệt tiêu
