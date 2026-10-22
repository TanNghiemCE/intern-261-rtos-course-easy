# Report — Week 2 / Ex1

`student_id`: 2510343    Commit của PREDICTION.md:  f387679

## 1. Kích thước queue tối thiểu
| | Dự đoán | Đo được | Khớp? |
|---|---|---|---|
| `queue_raw` | 1 | 1 | Khớp |
| `queue_filtered` | 1 | 1 | Khớp |

- **Giải thích:** Số đo thực tế hoàn toàn khớp với dự đoán trong `PREDICTION.md`. 
  - Ở cấu hình 115200 baud, thời gian truyền 1 mẫu UART (khoảng 24 bytes) mất $t_{uart} \approx \frac{24 \times 10}{115200} \approx 2.08\text{ ms} < 5\text{ ms}$ (chu kỳ sinh mẫu `SAMPLE_PERIOD_MS`).
  - `filter_task` chạy thuật toán Moving Average $O(1)$ chỉ mất khoảng $2 - 5\ \mu\text{s}$.
  - Do mức ưu tiên `sensor_task (3) > filter_task (2) > uart_task (1)`, ngay khi sensor đẩy mẫu vào `queue_raw`, `filter_task` lập tức preempt/unblock để lấy mẫu ra xử lý và chuyển tiếp sang `queue_filtered`, sau đó `uart_task` lấy ra gửi ngay. Do đó, độ đầy của cả 2 queue trong suốt 15 giây đo đạc luôn ở mức 0 (đỉnh tức thời $\le 1$), kích thước tối thiểu chỉ cần 1 phần tử là không bị mất mẫu.


## 2. Đồ thị độ đầy queue theo thời gian
Cấu hình bình thường: `![](queue_fill_115200.png)`
Cấu hình UART chậm (9600 baud): `![](queue_fill_9600_uart.png)`

Giải thích hình dạng đồ thị ở mỗi cấu hình.
- Cấu hình 115200: cả 2 đường `queue_raw` và `queue_filtered` là 2 đường thẳng nằm ngang ở mức 0 trong suốt gần 15000 mẫu đo đạc. Nguyên nhân: tốc độ thu mẫu : 200 mẫu / giây, , tốc độ Uart là 2.08ms / mẫu, xấp xỉ 480 mẫu / giây. Uart tiêu thụ nhiều hơn tốc độ thu mẫu, mẫu vừa thu được thì đã được lấy ra ngay lập tức, không bị tích trong queue.
- Cấu hình 9600: Đường `queue_raw` vẫn nằm phẳng ở mức 0. Trong khi đó, đường `queue_filtered` tăng dốc đứng từ 0 lên 4 rồi chạm mức 7-8 trong vòng 50 ms đầu tiên, sau đó duy trì đi ngang bão hòa tuyệt đối ở mức đầy cứng queue cho đến hết thời gian đo. Nguyên nhân: tốc độ Uart 40 mẫu / giây, tốc độ lấy mẫu 200 mẫu/giây, Vì vậy, `queue_filtered` có dung lượng $QUEUE\_LENGTH = 8$ nhanh chóng bị lấp đầy sau đúng 50 ms và liên tục rơi vào trạng thái bão hòa. `queue_raw` vẫn ở mức 0 vì `filter_task` (priority 2) luôn lấy mẫu ra khỏi `queue_raw` ngay lập tức để chuyển gánh nặng tích lũy sang `queue_filtered`.
## 3. Khi nào queue đầy
- **Thời điểm chính xác `queue_filtered` bắt đầu đầy và drop mẫu đầu tiên:**
  - Tại **$t = 50\text{ ms}$** (ứng với mẫu `seq = 10`).
  - *Mô phỏng timeline vật lý chi tiết quá trình làm đầy queue:*
    - **$t = 0\text{ ms}$ (`seq = 0`):** `sensor_task` sinh `seq = 0`, `filter_task` đẩy vào `queue_filtered`. `uart_task` lấy ra ngay và bắt đầu truyền `seq = 0` qua `HAL_UART_Transmit` (mất $\approx 25.0 - 26.0\text{ ms}$).
    - **Từ $t = 0 \to 25\text{ ms}$:** Trong khi `uart_task` đang bận truyền `seq = 0`, các mẫu mới liên tục được bơm vào `queue_filtered`:
      - $t = 5\text{ ms}$: `seq = 1` vào queue (queue có 1 phần tử).
      - $t = 10\text{ ms}$: `seq = 2` vào queue (queue có 2 phần tử).
      - $t = 15\text{ ms}$: `seq = 3` vào queue (queue có 3 phần tử).
      - $t = 20\text{ ms}$: `seq = 4` vào queue (queue có 4 phần tử).
      - $t = 25\text{ ms}$: `seq = 5` vào queue (queue có 5 phần tử).
    - **Tại $t \approx 25.5\text{ ms}$:** `uart_task` truyền xong `seq = 0`, gọi `xQueueReceive` lấy `seq = 1` ra. Ngay sau khi rút `seq = 1`, hàng đợi còn lại đúng 4 phần tử (`seq = 2, 3, 4, 5`). Do đó, trên dòng đầu tiên của log đo được, `seq = 1` đi kèm mức `filt_q = 4`.
    - **Từ $t = 25.5 \to 50.5\text{ ms}$:** `uart_task` bận truyền `seq = 1`. Các mẫu tiếp tục tích lũy:
      - $t = 30\text{ ms}$: `seq = 6` vào queue (queue có 5).
      - $t = 35\text{ ms}$: `seq = 7` vào queue (queue có 6).
      - $t = 40\text{ ms}$: `seq = 8` vào queue (queue có 7).
      - $t = 45\text{ ms}$: `seq = 9` vào queue (queue có 8 phần tử $\implies$ **hàng đợi đầy cứng 8/8 chỗ** gồm các mẫu `seq = 2, 3, 4, 5, 6, 7, 8, 9`).
    - **Tại $t = 50\text{ ms}$:** `sensor_task` sinh `seq = 10`. `filter_task` gọi `xQueueSend(queue_filtered, &out, 0)`. Do hàng đợi đang đầy cứng 8 phần tử và timeout = 0, hàm trả về ngay `errQUEUE_FULL` $\implies$ **Mẫu `seq = 10` bị drop lần đầu tiên tại mốc $50\text{ ms}$**.
    - **Tại $t \approx 51\text{ ms}$:** `uart_task` truyền xong `seq = 1`, rút `seq = 2` ra (giải phóng 1 ô trống, in `filt_q = 7`). Đến $t = 55\text{ ms}$, mẫu `seq = 11` được sinh ra và lọt vào ô trống vừa giải phóng này. Trên log thực tế, số `seq` nhảy cóc trực tiếp từ `9` lên `11`.

- **Số mẫu bị mất / trễ:**
  - Mã nguồn `sensor_task` đánh số `seq` bắt đầu từ `0`. Mẫu đầu tiên `seq = 0` được truyền trong giai đoạn mở cổng COM; log terminal ghi nhận ổn định từ `seq = 1` đến `seq = 2952` (tương ứng thời gian đo $2952 \times 5\text{ ms} = 14.760\text{ ms}$).
  - Tổng số mẫu cảm biến sinh ra: **2953 mẫu** (`seq = 0 \to 2952`).
  - Tổng số mẫu UART nhận và truyền thành công: **553 mẫu**.
  - Số mẫu còn lưu trong hàng đợi tại thời điểm dừng: **8 mẫu**.
  - **Tổng số mẫu bị mất (drop thực tế):** $2953 - 553 - 8 = \mathbf{2392\text{ mẫu}}$ (tỷ lệ mất $\approx 81\%$).
  - *Quy luật mất mẫu:* Cứ mỗi $\approx 26\text{ ms}$, `uart_task` mới giải phóng được 1 chỗ trống. Trong thời gian đó, cảm biến sinh ra 5 mẫu mới, dẫn đến 4 mẫu bị drop liên tiếp và chỉ 1 mẫu được nhận. Số `seq` nhảy cóc đều đặn theo bước nhảy 5 đơn vị (`11, 16, 21, 26, 31, 36, 41...`).

---

## 4. Hành vi 3 kiểu timeout
| Timeout | Kết quả quan sát khi queue đầy |
|---|---|
| `0` | Hàm `xQueueSend` trả về ngay `errQUEUE_FULL` (Non-blocking). Mẫu mới bị drop. Task gửi không bị chuyển sang trạng thái Blocked nên chu kỳ lấy mẫu 5 ms của sensor được bảo toàn nguyên vẹn. Trên terminal, số `seq` nhảy cóc đều đặn bước 5 (`11, 16, 21, 26...`). |
| Giá trị hữu hạn (ví dụ: `pdMS_TO_TICKS(5)`) | Task gửi bị chuyển sang trạng thái **Blocked** để đợi ô trống tối đa 5 ms. Do UART 9600 baud mất tới $\approx 26\text{ ms}$ mới giải phóng 1 ô trống, hầu hết các chu kỳ chờ đều bị hết hạn timeout (timeout expires) và mẫu vẫn bị drop. `filter_task` bị block trễ tối đa 5 ms mỗi chu kỳ, bắt đầu gây jitter và trễ dây chuyền sang `queue_raw`. Tuy nhiên, vì `filter_task` chờ tối đa 5 ms (đúng bằng chu kỳ sinh mẫu của `sensor_task`), `queue_raw` chỉ dao động ở mức $\approx 1$ phần tử chứ không bị bão hòa.  |
| `portMAX_DELAY` | Task gửi bị chuyển sang trạng thái **Blocked vô thời hạn** cho đến khi `uart_task` lấy bớt 1 phần tử ra. Khi `queue_filtered` đầy cứng, `filter_task` bị block; dẫn đến `queue_raw` cũng bị đầy theo và kéo theo `sensor_task` bị block. **Kết quả:** Không có mẫu nào bị mất (số `seq` liên tục $0, 1, 2, 3...$), nhưng chu kỳ lấy mẫu 5 ms của toàn hệ thống bị kéo tụt xuống bằng đúng tốc độ chậm của UART ($\approx 26\text{ ms/mẫu}$). Chu kỳ thời gian thực bị phá vỡ hoàn toàn, dữ liệu tích lũy độ trễ lớn. |

---

## 5. Lệch so với dự đoán

### Đối chiếu bảng dự đoán (`PREDICTION.md`) với thực tế đo:
1. **Kích thước queue tối thiểu:** Dự đoán 1 phần tử cho cả 2 queue $\to$ Thực tế đo được 1 phần tử (**Khớp hoàn toàn**).
2. **Queue nào đầy trước ở 9600 baud:** Dự đoán `queue_filtered` $\to$ Thực tế là `queue_filtered` (**Khớp hoàn toàn**).
3. **Mức đầy bão hòa của `queue_filtered`:** Dự đoán bão hòa ở mức **8 phần tử** (`QUEUE_LENGTH`) $\to$ Thực tế file log `log_uart_9600.log` hiển thị giá trị là **7 phần tử** (**LỆCH**).
4. **Hành vi timeout:** Các hiện tượng drop mẫu, trôi chu kỳ và nhảy cóc số `seq` đều khớp với dự đoán lý thuyết.

### Giải thích nguyên nhân điểm lệch (mức đầy 7 thay vì 8):
- *Nguyên nhân kỹ thuật:* Điểm lệch này xuất phát từ **vị trí đặt lệnh đo** trong file `app/uart_task.c`. Lệnh đo `uxQueueMessagesWaiting(queue_filtered)` được gọi **ngay sau khi** hàm `xQueueReceive` đã lấy thành công 1 phần tử ra khỏi hàng đợi.
- Do đó, tại thời điểm đọc, hàng đợi vừa được giải phóng 1 chỗ trống nên giá trị trả về luôn là $8 - 1 = \mathbf{7}$. Thực tế, ngay trước thời điểm lấy ra, hàng đợi hoàn toàn đầy 8 phần tử (thể hiện qua việc các mẫu mới sinh ra tại thời điểm đó đều bị trả về `errQUEUE_FULL` và bị drop).
- *Hạn chế của phương pháp đo trực tiếp trong task:* Việc đo độ đầy bằng cách chèn `uxQueueMessagesWaiting` vào `uart_task` chỉ phản ánh trạng thái sau khi đã rút mẫu, không thể quan sát được đỉnh tức thời chính xác. Để đo khách quan và không làm sai lệch, phương pháp chuẩn là dùng `depth_logger_task` định kỳ độc lập như đã làm ở cấu hình 115200 baud.

---

## 6. Câu hỏi đánh đổi
- **Bộ tham số riêng (`student_id = 2510343`):**
  - Chu kỳ lấy mẫu: `SAMPLE_PERIOD_MS = 5` ms ($T_s = 5\text{ ms}$).
  - Cửa sổ lọc ban đầu: `FILTER_WINDOW = 4` mẫu ($W_1 = 4$).
  - Cửa sổ lọc khi tăng gấp đôi: $W_2 = 2 \times 4 = 8$ mẫu.
     $$\tau = \frac{W - 1}{2} \text{ (chu kỳ lấy mẫu)}$$
   - Khi $W_1 = 4$: $\tau_1 = \frac{4 - 1}{2} = 1.5\text{ mẫu} \implies t_{delay1} = 1.5 \times 5\text{ ms} = \mathbf{7.5\text{ ms}}$.
   - Khi $W_2 = 8$: $\tau_2 = \frac{8 - 1}{2} = 3.5\text{ mẫu} \implies t_{delay2} = 3.5 \times 5\text{ ms} = \mathbf{17.5\text{ ms}}$.
   - $\implies$ **Độ trễ tín hiệu từ cảm biến đến UART tăng thêm:** 
     $$\Delta t = t_{delay2} - t_{delay1} = 17.5\text{ ms} - 7.5\text{ ms} = \mathbf{10\text{ ms}}$$ (tương đương chậm thêm đúng 2 chu kỳ lấy mẫu).
   - *Thời gian làm đầy cửa sổ ban đầu:* Tăng từ $4 \times 5\text{ ms} = 20\text{ ms}$ lên $8 \times 5\text{ ms} = 40\text{ ms}$ (tăng thêm $\mathbf{20\text{ ms}}$).

- Kiếm chứng bằng 2 file `run_w4.log` và `run_w8.log` thông qua script python `verify_single_run.py`:
```
=================================================================
Phan tich file: run_w4.log (FILTER_WINDOW = 4)
=================================================================
- So mau phan tich:                 2997
- He so tuong quan dinh r_peak:     0.217271
- Do tre roi rac tai dinh (k*):     +1 mau
- Do tre noi suy parabol (tau):     +1.3626 mau (+6.813 ms)
- Do tre ly thuyet (W-1)/2:         +1.5 mau (+7.5 ms)
- Sai lech thuc te vs ly thuyet:    0.1374 mau (0.687 ms)

  Bang tuong quan quanh dinh:
    lag =  -2: r = -0.016432
    lag =  -1: r = -0.030595
    lag =  +0: r = 0.205660
    lag =  +1: r = 0.217271 <-- PEAK
    lag =  +2: r = 0.215421
    lag =  +3: r = 0.055260
    lag =  +4: r = -0.028844

=================================================================
Phan tich file: run_w8.log (FILTER_WINDOW = 8)
=================================================================
- So mau phan tich:                 2993
- He so tuong quan dinh r_peak:     0.105290
- Do tre roi rac tai dinh (k*):     +4 mau
- Do tre noi suy parabol (tau):     +3.7251 mau (+18.626 ms)
- Do tre ly thuyet (W-1)/2:         +3.5 mau (+17.5 ms)
- Sai lech thuc te vs ly thuyet:    0.2251 mau (1.126 ms)

  Bang tuong quan quanh dinh:
    lag =  +1: r = 0.092624
    lag =  +2: r = 0.101247
    lag =  +3: r = 0.100942
    lag =  +4: r = 0.105290 <-- PEAK
    lag =  +5: r = 0.090323
    lag =  +6: r = 0.096140
    lag =  +7: r = 0.103797
    lag =  +8: r = 0.014521

=================================================================
TONG HOP CHENH LECH DO TRE GIUA W=8 VA W=4 (Delta = W8 - W4)
=================================================================
- Chenh lech thuc te uoc luong:     +2.3625 mau (+11.813 ms)
- Chenh lech ly thuyet:             +2.0 mau (+10.0 ms)
- Sai so chenh lech:                1.813 ms (0.3625 mau)
- Tom lai: do tre do duoc là 11.83ms, sai lech 1.813ms so voi ly thuyet
=================================================================
```
