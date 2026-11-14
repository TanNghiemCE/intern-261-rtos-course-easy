# Report — Week 3 / Ex4

`student_id`: 2510343    Commit của PREDICTION.md:  a9bbc5c

## 1. Bảng CPU %
| Task | CPU % (lần đo 1) | CPU % (lần đo 2) |
|---|---|---|
| `stats` | 3% (12,580,048 cycles) | 3% (15,118,379 cycles) |
| `IDLE` | 96% (359,940,733 cycles) | 96% (431,922,402 cycles) |
| `Tmr Svc` | <1% (2,244 cycles) | <1% (2,244 cycles) |
| `alpha` | <1% (3,794 cycles) | <1% (3,794 cycles) |
| `beta` | <1% (3,739 cycles) | <1% (3,739 cycles) |

Giải thích Idle task chiếm bao nhiêu % và vì sao:
- `IDLE` task chiếm **96%** (359,940,733 / 372,530,558 cycles = 96.62% ở lần 1; 431,922,402 / 447,050,558 cycles = 96.62% ở lần 2), `stats` chiếm **3%** (3.38%), còn `Tmr Svc`, `alpha`, `beta` mỗi task chiếm **<1%** (tổng phần nguyên hiển thị là $96\% + 3\% = 99\%$ do `vTaskGetRunTimeStats` thực hiện phép chia nguyên làm tròn xuống).
- `IDLE` chiếm 96% vì:
  - `alpha` (`tskIDLE_PRIORITY + 2`) và `beta` (`tskIDLE_PRIORITY + 2`) rơi vào trạng thái `eBlocked` (`state=2`) vĩnh viễn ngay vòng đầu do deadlock (`Abs Time` đứng yên ở `3,794` và `3,739` cycles giữa 2 lần đo).
  - `Tmr Svc` không có software timer hoạt động nên giữ nguyên trạng thái `eBlocked` (`2,244` cycles).
  - `stats` (`tskIDLE_PRIORITY + 1`) gọi `vTaskDelay(pdMS_TO_TICKS(1000))` và mỗi chu kỳ `STATS_WINDOW_MS = 1000 ms` chỉ tiêu tốn `2,538,331` cycles (35.25 ms ở HCLK 72 MHz, tương đương 3.41% cửa sổ) để gọi `vTaskGetRunTimeStats()`, `uxTaskGetSystemState()` và truyền `HAL_UART_Transmit`. Toàn bộ 96.59% thời gian còn lại không có task ứng dụng nào ở trạng thái `eReady`, nên Scheduler chuyển CPU cho `IDLE` task.

## 2. Bằng chứng tìm deadlock
Log/ảnh `uxTaskGetSystemState` **trước khi sửa**: trạng thái từng task, chỉ
rõ `task_alpha`/`task_beta` đang `eBlocked` và đang chờ mutex nào. Giải thích
chính xác thứ tự lấy mutex nào gây ra deadlock (AB-BA).

- **Snapshot `uxTaskGetSystemState()` qua UART trước khi sửa ([log_error.log](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex4/log_error.log)):**
```text
stats: state=0     (eRunning)
IDLE: state=1      (eReady)
Tmr Svc: state=2   (eBlocked)
alpha: state=2     (eBlocked)  | Abs Time = 3794 cycles (lần 1) -> 3794 cycles (lần 2)
beta: state=2      (eBlocked)  | Abs Time = 3739 cycles (lần 1) -> 3739 cycles (lần 2)
```
- **Bằng chứng đối tượng mutex đang giữ (`xMutexHolder`) và đang chờ (`xTasksWaitingToReceive`) từ Debugger:**
```text
(Queue_t *)s_mutex_a = 0x20000270 <ucHeap+8>
(Queue_t *)s_mutex_b = 0x200002c8 <ucHeap+96>
((Queue_t *)s_mutex_a)->u.xSemaphore.xMutexHolder                       = 0x20000728 <ucHeap+1216> (TCB alpha)
((Queue_t *)s_mutex_a)->xTasksWaitingToReceive.xListEnd.pxNext->pvOwner = 0x20000b98 <ucHeap+2352> (TCB beta)
((Queue_t *)s_mutex_b)->u.xSemaphore.xMutexHolder                       = 0x20000b98 <ucHeap+2352> (TCB beta)
((Queue_t *)s_mutex_b)->xTasksWaitingToReceive.xListEnd.pxNext->pvOwner = 0x20000728 <ucHeap+1216> (TCB alpha)
```

| Task | TCB Handle | Trạng thái (`eCurrentState`) | Mutex đang giữ (`xMutexHolder`) | Mutex đang chờ (`xTasksWaitingToReceive`) |
|---|---|---|---|---|
| `alpha` | `0x20000728` | `2` (`eBlocked`) | `s_mutex_a` (`0x20000270`) | `s_mutex_b` (`0x200002c8`) |
| `beta` | `0x20000b98` | `2` (`eBlocked`) | `s_mutex_b` (`0x200002c8`) | `s_mutex_a` (`0x20000270`) |

- **Giải thích cơ chế gây deadlock (AB–BA):**
  - `task_alpha` lấy khóa theo thứ tự `s_mutex_a` $\rightarrow$ `vTaskDelay(pdMS_TO_TICKS(5))` $\rightarrow$ `s_mutex_b`, còn `task_beta` lấy khóa theo thứ tự ngược lại `s_mutex_b` $\rightarrow$ `vTaskDelay(pdMS_TO_TICKS(5))` $\rightarrow$ `s_mutex_a`.
  - Tại $t = 0\text{ ms}$, `alpha` lấy `s_mutex_a` (`0x20000270`) rồi vào `vTaskDelay(5)` nhường CPU; `beta` chạy và lấy `s_mutex_b` (`0x200002c8`) rồi cũng vào `vTaskDelay(5)`.
  - Tại $t = 5\text{ ms}$, `alpha` thức dậy gọi `xSemaphoreTake(s_mutex_b, portMAX_DELAY)` và bị `eBlocked` vì `beta` đang giữ `s_mutex_b`; tiếp theo `beta` thức dậy gọi `xSemaphoreTake(s_mutex_a, portMAX_DELAY)` và bị `eBlocked` vì `alpha` đang giữ `s_mutex_a`. Hai task tạo vòng chờ khép kín với timeout `portMAX_DELAY` nên khóa cứng vĩnh viễn.

## 3. Sửa lỗi
Mô tả thay đổi bạn làm để hết deadlock (thứ tự lấy mutex thống nhất). Xác
nhận chạy ổn định sau khi sửa (thời gian chạy thử).

- **Thay đổi trong [deadlock_demo.c](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex4/app/deadlock_demo.c):**
  - Đổi thứ tự lấy mutex trong `task_beta` thành `xSemaphoreTake(s_mutex_a, portMAX_DELAY)` $\rightarrow$ `vTaskDelay(pdMS_TO_TICKS(5))` $\rightarrow$ `xSemaphoreTake(s_mutex_b, portMAX_DELAY)` và nhả theo thứ tự `xSemaphoreGive(s_mutex_b)` $\rightarrow$ `xSemaphoreGive(s_mutex_a)`, đồng nhất hoàn toàn với `task_alpha`.
  - Khi cả hai task cùng xin `s_mutex_a` trước, nếu `alpha` đang giữ `s_mutex_a` và ngủ 5 ms thì `beta` sẽ bị block ngay tại `s_mutex_a` mà chưa giữ `s_mutex_b`; nhờ đó `alpha` thức dậy luôn lấy được `s_mutex_b`, hoàn thành chu kỳ và trả cả hai mutex cho `beta`.
- **Xác nhận chạy ổn định sau khi sửa ([log_fix.log](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex4/log_fix.log)):**
  - Chạy liên tục qua 11 cửa sổ thống kê (`1,778,258,583` cycles = **24.70 s** ở 72 MHz) không xảy ra deadlock.
  - Giữa mỗi cửa sổ `STATS_WINDOW_MS = 1000 ms`, `Abs Time` của `alpha` tăng đều **+159,994 cycles/s** (từ `2,251,683` lên `3,855,535` cycles) và `beta` tăng đều **+243,639 cycles/s** (từ `3,426,440` lên `5,871,067` cycles), tương ứng mỗi worker hoàn thành đều đặn 40 chu kỳ/giây (chu kỳ `5 ms + 20 ms = 25 ms`).

## 4. Watchdog
Phép tính reload IWDG. Log/video board tự reset khi deadlock được tái tạo lại
có chủ đích, kèm thời gian từ lúc treo đến lúc reset — so với
`IWDG_TIMEOUT_MS`.

- **Phép tính Reload Value cho IWDG (`IWDG_TIMEOUT_MS = 250 ms`, LSI = 40,000 Hz):**
  - Chọn Prescaler $P = 4$ (`IWDG_PRESCALER_4`), tần số đếm lùi của IWDG là $f_{\text{cnt}} = 40,000 / 4 = 10,000\text{ Hz}$ ($0.1\text{ ms/tick}$).
  - Giá trị thanh ghi nạp lại 12-bit (`RLR` $\in [0, 4095]$):
    $$\text{RLR} = \frac{\text{IWDG\_TIMEOUT\_MS} \times f_{\text{LSI}}}{1000 \times P} - 1 = \frac{250 \times 40,000}{1000 \times 4} - 1 = 2500 - 1 = \mathbf{2499}$$
  - Thời gian timeout danh định: $T_{\text{IWDG}} = (2499 + 1) \times 4 / 40,000 = 0.250\text{ s} = \mathbf{250\text{ ms}}$.
- **Log chứng minh board tự reset qua IWDG khi tái tạo deadlock ([log_reset](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex4/log_reset)):**
```text
[DEMO] Injecting AB-BA deadlock at tick=2205 ms (last_refresh=2185 ms)
[SUPERVISOR] Timeout! bits=0x5 (expected 0x7) at tick=2285 ms (last_refresh=2185 ms) -> NO IWDG REFRESH
[SUPERVISOR] Timeout! bits=0x5 (expected 0x7) at tick=2394 ms (last_refresh=2185 ms) -> NO IWDG REFRESH
=== [BOOT] RCC_CSR=0x24000000 | IWDGRSTF=1 (IWDG WATCHDOG RESET) ===
```
- **Phân tích thời gian từ lúc treo đến lúc reset so với `IWDG_TIMEOUT_MS = 250 ms`:**
  - Trong `2185 ms` đầu tiên, cả 3 task (`alpha` bit 0, `beta` bit 1, `hbeat` bit 2) đều check-in đủ mask `0x07` mỗi `25 ms`, `supervisor_task` gọi `HAL_IWDG_Refresh(&hiwdg)` lần cuối tại mốc **`last_refresh = 2185 ms`**.
  - Tại **`tick = 2205 ms`**, `task_beta` kích hoạt lại thứ tự khóa lỗi `s_mutex_b -> s_mutex_a` (giữ `s_mutex_b`), còn `task_alpha` giữ `s_mutex_a` và cả hai khóa cứng trong vòng chờ AB–BA tại **`t_fault = 2230 ms`**.
  - Tại **`tick = 2285 ms`** và **`tick = 2394 ms`**, `xEventGroupWaitBits(..., pdMS_TO_TICKS(100))` của `supervisor_task` hết hạn với `bits = 0x05` (chỉ có bit 0 của `alpha` từ vòng cuối lúc `2205 ms` và bit 2 của `hbeat`, thiếu bit 1 của `beta`) $\rightarrow$ `supervisor_task` **không** gọi `HAL_IWDG_Refresh()`.
  - Ngay sau mốc `2394 ms` (tại $t_{\text{reset}} = t_{\text{last\_refresh}} + T_{\text{IWDG}} = 2185 + 250 = \mathbf{2435\text{ ms}}$), bộ đếm IWDG chạm 0 và reset cứng MCU (thanh ghi sau boot báo `RCC_CSR = 0x24000000`, cờ `IWDGRSTF = 1`).
  - **Thời gian thực tế từ lúc tiêm lỗi (`2205 ms`) đến lúc reset (`2435 ms`) là `230 ms` (và từ lúc khóa cứng cả 2 mutex tại `2230 ms` đến lúc reset là `205 ms`), nhỏ hơn `IWDG_TIMEOUT_MS = 250 ms` đúng bằng khoảng thời gian trôi qua từ lần refresh cuối (`2185 ms`) đến lúc treo (`20 ms` / `45 ms`)**:
    $$\Delta t_{\text{fault}\to\text{reset}} = T_{\text{IWDG}} - (t_{\text{fault}} - t_{\text{last\_refresh}}) = 250\text{ ms} - (2205 - 2185)\text{ ms} = \mathbf{230\text{ ms}}$$

## 5. Lệch so với dự đoán
Mỗi mục lệch + nguyên nhân.

- **Câu 1 (Thời điểm xảy ra deadlock):**
  - **Con số lệch:** Dự đoán trong `PREDICTION.md` là deadlock hình thành ngay vòng đầu sau khoảng delay `5 ms`; thực đo trong [log_error.log](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex4/log_error.log) cho thấy `alpha` dừng ở `3,794` cycles ($52.69\ \mu\text{s}$) và `beta` dừng ở `3,739` cycles ($51.93\ \mu\text{s}$), tức khóa cứng ngay ở vòng đầu tiên tại mốc `5 ms` $\rightarrow$ **Lệch 0 chu kỳ vòng lặp**.
  - **Nguyên nhân vật lý / cơ chế kernel:** `task_alpha` và `task_beta` có cùng mức ưu tiên `tskIDLE_PRIORITY + 2`. Khi `alpha` lấy `s_mutex_a` và gọi `vTaskDelay(pdMS_TO_TICKS(5))`, kernel đưa `alpha` vào Delayed List và chuyển ngữ cảnh ngay sang `beta` trong cùng tick `0 ms` để `beta` lấy `s_mutex_b`; khi tick `5 ms` tới, cả hai cùng thức dậy và tạo vòng chờ AB–BA ngay lập tức.
- **Câu 2 (Trạng thái task và mutex chờ):**
  - **Con số lệch:** Dự đoán trạng thái cả hai task là `eBlocked`, `alpha` chờ `s_mutex_b` và `beta` chờ `s_mutex_a`; thực đo từ `uxTaskGetSystemState()` (`state = 2 = eBlocked`) và cấu trúc `Queue_t` trên Heap (`0x20000270`, `0x200002c8`) khớp hoàn toàn $\rightarrow$ **Lệch 0**.
  - **Nguyên nhân vật lý / cơ chế kernel:** Khi gọi `xSemaphoreTake(..., portMAX_DELAY)` trên mutex đang bị giữ (`uxMessagesWaiting == 0`), kernel móc `xEventListItem` của task gọi vào `xTasksWaitingToReceive` của mutex đó và chuyển trạng thái TCB sang `eBlocked`.
- **Câu 3 (Giá trị Reload IWDG):**
  - **Con số lệch:** Dự đoán `RLR = 2499` với Prescaler `P = 4` cho `IWDG_TIMEOUT_MS = 250 ms`; giá trị nạp thực tế `hiwdg.Init.Reload = 2499` $\rightarrow$ **Lệch 0 so với công thức RM0008 (lệch -1 đơn vị so với công thức thiếu `- 1` trong `FreeRTOSConfig.deltas` là `2500`)**.
  - **Nguyên nhân vật lý / cơ chế kernel:** Bộ đếm lùi 12-bit của IWDG đếm từ `RLR` về `0x000` nên tổng số nhịp xung LSI/4 cần đếm là `RLR + 1 = 2500` nhịp ($2500 \times 0.1\text{ ms} = 250.0\text{ ms}$).
- **Câu 4 (Thời gian từ lúc treo đến lúc IWDG reset board):**
  - **Con số lệch:** Dự đoán trong `PREDICTION.md` là `300–500 ms` kể từ lúc treo; thực đo trong [log_reset](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex4/log_reset) từ lúc tiêm lỗi (`tick = 2205 ms`, sau lần timeout cuối `tick = 2394 ms`) đến khi IWDG reset ở `2435 ms` là **`230 ms`** (hoặc `205 ms` tính từ mốc khóa cứng cả 2 mutex `2230 ms`) $\rightarrow$ **Lệch từ $-70\text{ ms}$ đến $-270\text{ ms}$ (thực tế reset sớm hơn dự đoán)**.
  - **Nguyên nhân vật lý / cơ chế kernel:** Khi dự đoán đã cộng nhầm thời gian chờ `xEventGroupWaitBits` của `supervisor_task` (`100 ms`) vào `IWDG_TIMEOUT_MS` (`250 ms`). Thực tế bộ đếm phần cứng IWDG chạy độc lập trên xung LSI và đã bắt đầu đếm lùi ngay từ mốc `last_refresh = 2185 ms` (trước khi xảy ra lỗi `20 ms`), nên tại thời điểm lỗi xảy ra (`2205 ms`), quỹ thời gian còn lại của IWDG chỉ còn $250 - (2205 - 2185) = 230\text{ ms}$.

## 6. Câu hỏi đánh đổi
Với `SUPERVISOR_BITS` và chu kỳ check-in của bạn, nếu một task thật sự chạy
chậm (không deadlock, chỉ chậm) trong đúng 1 chu kỳ, supervisor có false-trip
(reset oan) không? Trả lời bằng số đo thời gian thực tế của chu kỳ check-in.

- **(1) Dự đoán tính từ dữ liệu chu kỳ check-in gốc:**
  - Chu kỳ check-in thực tế khi chạy bình thường: `task_alpha` (bit 0) và `task_beta` (bit 1) mỗi chu kỳ gồm `vTaskDelay(5)` + vùng găng + `vTaskDelay(20)` = **`25 ms`** (40 chu kỳ/s); `heartbeat_task` (bit 2) gọi `vTaskDelay(pdMS_TO_TICKS(20))` = **`20 ms`** (50 chu kỳ/s).
  - Do `supervisor_task` chờ đủ `ALL_BITS = 0x07` (`SUPERVISOR_BITS = 3`), khoảng cách giữa 2 lần gọi `HAL_IWDG_Refresh(&hiwdg)` bình thường là $\Delta t_{\text{feed}} = \max(25\text{ ms}, 25\text{ ms}, 20\text{ ms}) = \mathbf{25\text{ ms}}$.
  - Khi một task (`task_beta`) chạy chậm trong đúng 1 chu kỳ (tăng thêm `+25 ms`), khoảng thời gian giữa 2 lần check-in liên tiếp của `beta` tăng lên $25\text{ ms} + 25\text{ ms} = \mathbf{50\text{ ms}}$. Giá trị `50 ms` này nhỏ hơn cả cửa sổ chờ `xEventGroupWaitBits(..., pdMS_TO_TICKS(100))` (`100 ms`) lẫn thời gian timeout phần cứng `IWDG_TIMEOUT_MS` (`250 ms`), nên dự đoán `supervisor_task` vẫn gom đủ `0x07` ở mốc `50 ms` và refresh IWDG bình thường, không gây reset oan.
- **(2) Số đo kiểm chứng thực tế ([log_cau6.log](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex4/log_cau6.log)):**
  - Cố ý làm `task_beta` trễ thêm 1 chu kỳ (`vTaskDelay(pdMS_TO_TICKS(25))`, tổng chu kỳ trễ là `50 ms`) tại mốc `tick >= 2200 ms` trong khi `supervisor_task` và `IWDG` (`250 ms`) đang hoạt động.
  - Kết quả trích từ [log_cau6.log](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex4/log_cau6.log): hệ thống **không bị reset** qua mốc `2200 ms` mà tiếp tục chạy ổn định suốt 7 cửa sổ đo liên tiếp sau đó (tổng chu kỳ `IDLE` tăng liên tục từ `426,669,511` cycles lên `854,943,765` cycles, tương ứng thời gian hoạt động liên tục từ **6.22 s** đến **12.40 s** mà không có bất kỳ lần khởi động lại nào):

| Mẫu đo trong `log_cau6.log` | `alpha` (`Abs Time`) | `beta` (`Abs Time`) | `hbeat` (`Abs Time`) | `super` (`Abs Time`) | `IDLE` (`Abs Time`, `%`) | Trạng thái IWDG |
|---|---:|---:|---:|---:|---:|---|
| Mẫu 1 ($t = 6.22\text{ s}$) | 1,445,964 | 2,246,909 | 463,929 | 1,117,224 | 426,669,511 (95%) | Không reset (vượt qua mốc trễ 2.2 s) |
| Mẫu 2 ($t = 7.22\text{ s}$) | 1,715,717 (+269,753) | 2,458,736 (+211,827) | 541,738 (+77,809) | 1,177,316 (+60,092) | 498,050,793 (95%) | Refresh bình thường (~40 lần/s) |
| Mẫu 7 ($t = 12.40\text{ s}$) | 3,084,234 | 3,525,186 | 930,783 | 1,482,168 | 854,943,765 (95%) | Chạy liên tục, không false-trip |

- **(3) Kết luận:**
  - **Không xảy ra false-trip (không reset oan)**: chu kỳ trễ 1 vòng thực tế là **`50 ms < 100 ms`** (cửa sổ chờ `xEventGroupWaitBits`) và **`50 ms < 250 ms`** (`IWDG_TIMEOUT_MS`), còn dư biên độ an toàn $250\text{ ms} - 50\text{ ms} = \mathbf{200\text{ ms}}$.
