# Report — Week 2 / Ex2

`student_id`: 2510343    Commit của PREDICTION.md: 19a3650

## 1. Lỗi tái hiện (UART_GUARD_MODE = 0)
Log UART dán nguyên văn, đánh dấu vị trí ký tự lẫn lộn:

```text
[W0:0000] AA[W1:0000] BB[W2:0000] C[W3:0000] DDAAAAAAAAAA
BBBBBBBBBB
DDDDDDDDDD
CCCCCCCCCCC
[W1:0001] 
BBBBBBBBBB[W0:0001] ABB
[W3:0001AAAAAAAAAAA[W2:0001] CC] DDDDDDDDD
[W1:0002] CCCCCCCCCC
DDD
BBBBBBB
[W0:0002] BBBBB
AAAAA[W3:0002] DAAAAAAA
[W2DDDDDDDDDDD
[W1:0003] B:0002] CCCCC
[W0:0003] BBBBBBBBBBB
CCCCCCC
AA
[W3:0003] DAAAAAAAAAA
DDDDDDDDDDD
[W2:0003] C
[W1:0004] B
CCCCCCCCCCBBBBBBBBBBB
C
[W0:0004
[W3:0004] D] AAAAAAAAADDDDDDDDDDD
[W2:0004] CAAA
[W1:000
CCCCCCCCCC5] BBBBBBBBBC
[W0:0005BBB
[W3:000] AAAAAAAAA5] DDDDDDDDD[W2:0005] CCAAA
[W1:00DDD
CCCCCCC06] BBBBBBBCCC
[W0:000BBBBB
[W3:6] AAAAAAAAA0006] DDDDD[W2:0006] CCAAA
[W1:00DDDDDDD
CCC07] BBBBBBBCCCCCCC
[W0BBBBB
[W3::0007] AAAAA0007] DDDDD[W2:0007] CCAAAAAAA
[WDDDDDDD
CCC1:0008] BBBCCCCCCC
[W0BBBBBBBBB
:0008] AAAAA[W3:0008] D[W2:0008] CCAAAAAAA
[W1DDDDDDDDDDDCCCCCCCCCC
:0009] BBBB[W0:0009] AA
BBBBBBBB
[W2:0009] CCAAAAAAAAAA
CCCCCCCCCC
[W3:0009] D

DDDDDDDDDD[W1:0010] BD
BBBBBBBBB[W0:0010] A[W2:0010] CCBB
AAAAAAA[W3:0010] DDCCCCCCCCCC
AAAA
DDDDDD[W1:0011] B
DDDD
BBBBB[W0:0011] ABBBBBB
AAAA[W2:0011] C[W3:0011] DDAAAAAAA
CCC[W1:0012] BDDDDDDDDDD
CCCCCCCC
[BBBBBBBBBBB
W0:0012] AA[W3:0012] DD
[W2:0012] AAAAAAAAAA
CCCCCCCCCCCDDDDDDDDDD
C
```


- **Định dạng chuẩn của một bản tin (`MSG_LEN = 24` bytes):**
  - Cấu trúc: `[W<id>:<seq>]
  - W0 in `A`, W1 in `B`, W2 in `C`, W3 in `D`.

---

#### 1. Vị trí lẫn lộn ngay chu kỳ đầu tiên (`seq = 0000`, dòng 1 - 3 của log):
```text
[W0:0000] AA[W1:0000] BB[W2:0000] C[W3:0000] DDAAAAAAAAAABBBBBBBBBB
DDDDDDDDDD
CCCCCCCCCCC
```
- **Phân tích từng đoạn chen ngang:**
  1. `[W0:0000] AA` (12 bytes của W0): W0 in được 12 bytes thì hết tick $1\text{ ms}$. Ngắt SysTick xảy ra, kernel preempt W0 và nhường CPU cho W1 (Round-Robin).
  2. `[W1:0000] BB` (12 bytes của W1): W1 nhảy vào ghi tiếp 12 bytes thì hết tick $2\text{ ms}$, bị preempt chuyển sang W2.
  3. `[W2:0000] C` (11 bytes của W2): W2 in được 11 bytes thì hết tick $3\text{ ms}$, bị preempt chuyển sang W3.
  4. `[W3:0000] DD` (12 bytes của W3): W3 in được 12 bytes thì hết tick $4\text{ ms}$, scheduler quay vòng lại W0.
  5. `AAAAAAAAAA\r\n` (12 bytes còn lại của W0): W0 chạy tiếp vòng lặp in nốt 10 ký tự `A` và `\r\n` (khiến terminal nhảy dòng).
  6. `BBBBBBBBBB\r\n` (12 bytes còn lại của W1): W1 in nốt 10 ký tự `B` và `\r\n`.
  7. `DDDDDDDDDD\r\n` (12 bytes còn lại của W3): W3 in nốt 10 ký tự `D` và `\r\n`.
  8. `CCCCCCCCCCC\r\n` (13 bytes còn lại của W2): W2 in nốt 11 ký tự `C` và `\r\n`.

---


Tái hiện lại bao nhiêu lần / bao nhiêu lần thấy lỗi: **10 / 10** lần trên mạch thật (100% các lần nạp/chạy đều thấy lỗi; trên log thực tế thu được, toàn bộ **37 / 37** chu kỳ gửi từ `seq: 0000` đến `seq: 0036` đều bị lẫn lộn ký tự hoàn toàn).

## 2. Bảng so sánh 3 cách
| Cách | Độ trễ in trung bình (us) | Độ trễ in max (us) | Ảnh hưởng task khác | RAM thêm (bytes) |
|---|---|---|---|---|
| Mutex | 5744 | 7300 | Task không dùng UART vẫn chạy bình thường; task cạnh tranh UART bị block chờ nhả khóa (có Priority Inheritance) | 84 |
| Critical section | 5329 | 8530 | Tắt ngắt toàn hệ thống > 2 ms, làm mất tick SysTick, trễ ngắt ngoại vi | 0 |
| Gatekeeper | 5436 | 8697 | Không tắt ngắt; writer chỉ gửi queue. Khi UART nghẽn (queue đầy), writer bị block chờ chỗ trống | 1416 |

Cách đo độ trễ in:
- Sử dụng DWT Cycle Counter (`DWT->CYCCNT`) đo trực tiếp trên chip STM32F103 (72 MHz: 72 cycle = 1 µs). Bấm giờ ngay trước khi gọi hàm bảo vệ (`t_start = DWT->CYCCNT`) và kết thúc ngay sau khi hàm gửi hoàn tất (`t_end = DWT->CYCCNT`). Độ trễ được tính bằng `(t_end - t_start) / 72UL`

Dữ liệu chi tiết từng task đo được từ thực tế phần cứng (`latency_report.log`)

Chi tiết tính toán RAM thêm:
- **Mutex**: Cấp phát động 1 cấu trúc `Queue_t` đặc biệt (length = 1, item size = 0) tốn 80 bytes heap + 4 bytes con trỏ `s_uart_mutex` = **84 bytes**.
- **Critical section**: Chỉ thao tác trực tiếp trên thanh ghi phần cứng `BASEPRI` của Cortex-M3 và biến `uxCriticalNesting` có sẵn trong TCB = **0 bytes**.
- **Gatekeeper**:
  - Hàng đợi `s_gatekeeper_queue` (8 phần tử x 24 bytes = 192 bytes buffer + 80 bytes struct `Queue_t` + 4 bytes con trỏ) = 276 bytes.
  - Gatekeeper task: Stack 256 words (1024 bytes) + TCB task (~112 bytes) + 4 bytes con trỏ = 1140 bytes.
  - Tổng RAM thêm: $276 + 1140 = \mathbf{1416\text{ bytes}}$.

## 3. Vì sao critical section nguy hiểm nhất
- Đo được: một lần in trong critical section dài 2140 µs (min 2140 / max 8530), bằng 2.14 tick. Công thức: MSG_LEN×10/UART_BAUD = 2083.3 ; MSG_LEN×10/UART_BAUD=2083.3 µs.
- Trong 2140 µs đó, BASEPRI che mọi ngắt có priority ≥ configMAX_SYSCALL_INTERRUPT_PRIORITY (=5)priority≥configMAX_SYSCALL_INTERRUPT_PRIORITY (=5): SysTick, TIM4, USART1_RX. Ngắt priority cao hơn ngưỡng vẫn chạy.
Đo được: trong 1.0 s, xTaskGetTickCount() đếm 1000000/2140 = 467 ticks trong khi thời gian thật là 1000 ms (100ticks) => mất 533 tick (53.3 %). Mọi vTaskDelay / xTaskDelayUntil bị kéo dài tương ứng.
Khác mutex: mutex chỉ làm các task muốn in phải chờ, còn SysTick, ngắt và task khác vẫn chạy bình thường. Critical section tắt cả scheduler, nên một đoạn in dài ảnh hưởng toàn hệ thống. Bên cạnh đó HAL_UART_Transmit đếm timeout bằng HAL_GetTick() (TIM4). Khi TIM4 bị che, timeout không bao giờ hết; nếu UART kẹt thì hệ thống treo với ngắt bị tắt.
## 4. Lệch so với dự đoán
Đối chiếu 3 chiều giữa `PREDICTION.md` và số đo phần cứng thực tế:

| Mục so sánh | Dự đoán (PREDICTION.md) | Đo thực tế | Độ lệch | Nguyên nhân vật lý / cơ chế kernel |
|---|---|---|---|---|
| **Độ trễ trung bình** | Critical < Mutex < Gatekeeper | Critical (5329 µs) < Gatekeeper (5436 µs) < Mutex (5744 µs) | Đúng thứ tự Critical và Mutex; Gatekeeper thấp hơn Mutex 308 µs | Mutex tốn thêm chi phí context switch và đưa task vào danh sách block `xTasksWaitingToReceive`. Critical section không tốn chi phí IPC. Gatekeeper nhanh hơn Mutex vì các writer chỉ thực hiện ghi nhanh vào RAM queue rồi nhường cho task gatekeeper xử lý. |
| **Độ trễ Max** | Dự đoán Mutex max cao nhất | Gatekeeper (8697 µs) $\approx$ Critical (8530 µs) > Mutex (7300 µs) | Lệch thứ tự Max (+1397 µs ở Gatekeeper so với Mutex) | Do `WRITER_PERIOD_MS = 3 ms < 4 \times 2.14 ms = 8.56 ms`, hệ thống bị nghẽn 100%. Ở Mode 2 và 3, các task xếp hàng nối đuôi nhau tuyệt đối: Task 3 (2140 µs), Task 2 (4266 µs), Task 1 (6392 µs), Task 0 (8530 µs). Ở Mutex, cơ chế round-robin và unblock động làm phân tán tải khiến max của một lần chạy chỉ đạt 7300 µs. |
| **Phân bố độ trễ từng task** | Dự đoán các task có độ trễ phân tán ngẫu nhiên | Mode 2 & 3: Bậc thang tăng dần theo cấp số cộng của $T_{\text{tx}}$ (2140, 4266, 6392, 8519 µs) | Lệch dự đoán phân tán | Do các task cùng độ ưu tiên và chu kỳ 3 ms ngắn hơn thời gian truyền 4 bản tin (8.56 ms), scheduler xếp 4 task thành một hàng đợi xoay vòng cố định theo thứ tự khởi tạo task. |

## 5. Câu hỏi đánh đổi
**Câu hỏi**: Với `WRITER_COUNT = 4`, nếu tăng gấp đôi lên `8` thì cách nào trong 3 cách bị ảnh hưởng độ trễ nhiều nhất?

### 1. Phân tích lý thuyết (Dựa trên số liệu đo lúc WRITER_COUNT = 4):
- Thời gian truyền UART cơ sở đo được ở $N = 4$: $T_{\text{tx}} \approx 2100\ \mu\text{s}$ (Min Mode 1: $2100\ \mu\text{s}$, Mode 2: $2140\ \mu\text{s}$, Mode 3: $2183\ \mu\text{s}$).
- Khi tăng từ $N = 4$ lên $N = 8$:
  - Tổng thời gian bận của UART tăng từ $4 \times 2.10\text{ ms} = 8.4\text{ ms}$ lên $8 \times 2.10\text{ ms} = \mathbf{16.8\text{ ms}}$, trong khi chu kỳ task vẫn giữ nguyên `WRITER_PERIOD_MS = 3 ms`. Nút cổ chai UART quá tải gấp $16.8 / 3 = 5.6\text{ lần}$.
  - **Với Critical Section và Gatekeeper**: Các task xếp hàng chờ . Khi $N$ tăng gấp đôi, hàng đợi tăng gấp đôi $\implies$ Độ trễ cực đại (Max) tăng tuyến tính xấp xỉ 2 lần ($8 \times 2.14\text{ ms} \approx 17.1\text{ ms}$).
  - **Với Mutex**: Khi 8 task cùng mức ưu tiên tranh chấp 1 Mutex duy nhất mà UART bị nghẽn 100%, một task nhả Mutex sau khi truyền xong sẽ delay 3 ms rồi lại cạnh tranh. Do danh sách chờ (`xTasksWaitingToReceive`) có tới 7 task và cơ chế unblock động, task có thể bị trượt lượt qua nhiều chu kỳ phục vụ ($> 12$ lượt truyền).
- **Dự đoán lý thuyết**: **Cách 1 (Mutex) bị ảnh hưởng độ trễ nặng nhất**, cả về độ trễ trung bình lẫn độ trễ cực đại (Max).

---

### 2. Đo kiểm chứng thực tế trên phần cứng (So sánh giữa N = 4 và N = 8):
Dữ liệu đối chiếu từ hai file log thực tế `latency_report(4).log` ($N = 4$) và `latency_report(8).log` ($N = 8$):

| Phương pháp | Chỉ số | N = 4 (`latency_report(4).log`) | N = 8 (`latency_report(8).log`) | Độ lệch tuyệt đối | Tỉ lệ thay đổi (%) |
|---|---|:---:|:---:|:---:|:---:|
| **Cách 1: Mutex** | **Avg**<br>**Max**<br>Min | **5,744 µs**<br>**7,300 µs**<br>2,100 µs | **13,704 µs**<br>**26,898 µs**<br>2,096 µs | **+7,960 µs**<br>**+19,598 µs**<br>-4 µs | **+138.6%** (gấp 2.39 lần)<br>**+268.5%** (gấp 3.68 lần)<br>~0% |
| **Cách 2: Critical Section** | **Avg**<br>**Max**<br>Min | **5,329 µs**<br>**8,530 µs**<br>2,140 µs | **9,478 µs**<br>**16,842 µs**<br>2,194 µs | **+4,149 µs**<br>**+8,312 µs**<br>+54 µs | **+77.9%** (gấp 1.78 lần)<br>**+97.4%** (gấp 1.97 lần)<br>+2.5% |
| **Cách 3: Gatekeeper** | **Avg**<br>**Max**<br>Min | **5,436 µs**<br>**8,697 µs**<br>2,183 µs | **9,688 µs**<br>**17,172 µs**<br>2,238 µs | **+4,252 µs**<br>**+8,475 µs**<br>+55 µs | **+78.2%** (gấp 1.78 lần)<br>**+97.4%** (gấp 1.97 lần)<br>+2.5% |

---
   -> Về **độ trễ cá nhân của task in**: Mutex chịu ảnh hưởng tệ nhất.
  