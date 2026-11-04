# Hướng Dẫn Toàn Diện Tuần 3 — Bài 1 (`week3/ex1`): EXTI + `FromISR` + Priority + `configASSERT`

> **Tài liệu tham chiếu chính**:
> - Sách giáo trình: [161204_Mastering_the_FreeRTOS_Real_Time_Kernel-A_Hands-On_Tutorial_Guide.pdf](file:///c:/Users/DELL/intern-261-rtos-course-easy/161204_Mastering_the_FreeRTOS_Real_Time_Kernel-A_Hands-On_Tutorial_Guide.pdf) — **Chapter 6** (*Interrupt Management*, tr. 181–233), **Chapter 7** (*Resource Management — Critical Sections*, tr. 238–241), **Chapter 9** (*Task Notifications*, tr. 293–322), **Chapter 11 & 12** (*Developer Support & Troubleshooting*, tr. 330–332, 357–359).
> - Quy tắc khóa học: [Rule.md](file:///c:/Users/DELL/intern-261-rtos-course-easy/Rule.md) và [SKILL.md](file:///c:/Users/DELL/intern-261-rtos-course-easy/skill_rp/SKILL.md).
> - Hồ sơ bài tập: [README.md](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/README.md), [PREDICTION.md](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/PREDICTION.md), [REPORT.md](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/REPORT.md), [RUBRIC.md](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/RUBRIC.md), [PARAMS.md](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/PARAMS.md), [params.h](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/params.h).

---

## Thông Số Cá Nhân Hóa Của Bạn ([params.h](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/params.h))

| Hằng số trong `params.h` | Giá trị | Ý nghĩa vật lý trong bài tập |
|---|---|---|
| `STUDENT_ID` | `"2510343"` | Mã số sinh viên |
| `EXTI_PIN` | `11` | Chân nút nhấn ngắt ngoài: **`PB11`** (thuộc đường `EXTI_LINE_11`, vector ngắt `EXTI15_10_IRQn`) |
| `ACK_PIN` | `7` | Chân GPIO output báo thức dậy: **`PA7`** (`ack_task` đảo trạng thái ngay khi thoát `ulTaskNotifyTake`) |
| `ISR_NVIC_PRIO` | `6` | Mức ưu tiên NVIC hợp lệ cần đặt cho `EXTI15_10_IRQn` |
| `LATENCY_BUDGET_US` | `50` | Ngưỡng trễ tối đa cho phép từ sườn xuống `PB11` (vào ISR) đến sườn đảo `PA7` (trong `ack_task`): **`50 us`** |

---

# PHẦN 1: Tổng Hợp Toàn Bộ Kiến Thức Lý Thuyết Liên Quan Đến Exercise

Bài tập `week3/ex1` là bài mở đầu cho chủ đề **Quản lý Ngắt (Interrupt Management)** trong hệ điều hành thời gian thực FreeRTOS trên vi điều khiển ARM Cortex-M3 (STM32F103C8T6). Để hiểu tận gốc mọi hiện tượng trong bài, bạn cần nắm vững **7 mảng kiến thức cốt lõi** trích từ sách *Mastering the FreeRTOS Real Time Kernel* dưới đây.

---

## 1.1. Phân biệt Độ ưu tiên Task (Software) và Độ ưu tiên Ngắt (Hardware NVIC)
*(Sách Mastering FreeRTOS — Mục 6.1 & 6.8, tr. 182 & 223–225)*

Một sai lầm kinh điển của người mới học RTOS là nhầm lẫn giữa **Task Priority** và **Interrupt Priority**:

| Tiêu chí | Độ ưu tiên Task (`uxPriority`) | Độ ưu tiên Ngắt (NVIC Priority) |
|---|---|---|
| **Thực thể quản lý** | Bộ lập lịch phần mềm (**FreeRTOS Scheduler**). | Bộ điều khiển ngắt phần cứng (**ARM Cortex-M3 NVIC**). |
| **Phạm vi giá trị** | `0` đến `configMAX_PRIORITIES - 1` (trong khóa học: `0..4`). | `0` đến `15` (với 4 bit ưu tiên trên STM32F103, `configPRIO_BITS = 4`). |
| **Quy ước chiều ưu tiên** | **Số càng LỚN → ưu tiên càng CAO** (`0` là thấp nhất — Idle Task). | **Số càng NHỎ → ưu tiên càng CAO** (`0` là ưu tiên phần cứng cao nhất, `15` là thấp nhất). |
| **Quan hệ chế áp (Preemption)** | Task chỉ chạy khi **không có bất kỳ ISR nào** đang thực thi. | **Mọi ISR (dù ở mức 15 thấp nhất) luôn luôn cắt ngang (preempt) mọi Task (dù ở mức `configMAX_PRIORITIES - 1` cao nhất)**. |

> **Quy tắc vàng**: Một Task có độ ưu tiên phần mềm cao nhất hệ thống vẫn bị ngắt có độ ưu tiên phần cứng thấp nhất chiếm quyền CPU ngay lập tức khi có sự kiện phần cứng xảy ra.

---

## 1.2. Kiến trúc NVIC trên ARM Cortex-M3 (STM32F103) và Quy tắc Dịch Bit
*(Sách Mastering FreeRTOS — Mục 6.8 "Interrupt Nesting", tr. 222–233)*

### a) Số bit ưu tiên (`configPRIO_BITS = 4`)
Kiến trúc ARM Cortex-M cho phép nhà sản xuất silicon thiết kế tối đa 8 bit (256 mức) cho mỗi thanh ghi ưu tiên ngắt (`NVIC_IPR`). Tuy nhiên, STMicroelectronics trên dòng **STM32F103 chỉ hiện thực 4 bit** (`configPRIO_BITS = 4`), tương ứng với $2^4 = 16$ mức ưu tiên: từ **`0` (cao nhất)** đến **`15` (thấp nhất)**.

### b) Lưu trữ trọng số cao (MSB-aligned) — Dịch trái 4 bit (`<< 4`)
Trên ARM Cortex-M3, 4 bit ưu tiên này không nằm ở 4 bit thấp (LSB `[3:0]`) mà **nằm ở 4 bit cao (MSB `[7:4]`)** của thanh ghi 8-bit trong NVIC (và thanh ghi `BASEPRI`), còn 4 bit thấp `[3:0]` luôn đọc ra bằng `0`:

```
Thanh ghi ưu tiên 8-bit của Cortex-M3 (STM32F103):
+---+---+---+---+---+---+---+---+
| 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
+---+---+---+---+---+---+---+---+
| P3| P2| P1| P0| 0 | 0 | 0 | 0 |  <-- Giá trị thanh ghi = (Mức ưu tiên 0..15) << 4
+---+---+---+---+---+---+---+---+
```

Do đó, trong [`FreeRTOSConfig.h`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/FreeRTOSConfig.deltas) luôn có 2 cặp hằng số đi đôi với nhau:
1. **Giá trị mức logic chưa dịch bit (`0..15`)** — dùng khi truyền vào hàm thư viện `HAL_NVIC_SetPriority(IRQn, PreemptPriority, 0)`:
   - `configLIBRARY_LOWEST_INTERRUPT_PRIORITY = 15`
   - `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`
2. **Giá trị byte phần cứng đã dịch trái 4 bit (`<< (8 - configPRIO_BITS)`)** — dùng để FreeRTOS nạp trực tiếp vào thanh ghi `SHPR3` (cho `SysTick`/`PendSV`) và thanh ghi `BASEPRI`:
   - `configKERNEL_INTERRUPT_PRIORITY = (15 << 4) = 240` (`0xF0`)
   - `configMAX_SYSCALL_INTERRUPT_PRIORITY = (5 << 4) = 80` (`0x50`)

### c) Phân nhóm ưu tiên (`NVIC_PRIORITYGROUP_4`)
Cortex-M3 cho phép chia 4 bit ưu tiên thành 2 phần: **Pre-emption Priority** (ưu tiên lồng ngắt — ngắt có số nhỏ hơn được phép cắt ngang ngắt có số lớn hơn) và **Sub-priority** (ưu tiên phụ — chỉ dùng để phân xử khi 2 ngắt đến cùng lúc, không gây lồng ngắt).
- **Yêu cầu bắt buộc của FreeRTOS trên Cortex-M**: Toàn bộ 4 bit phải được cấu hình làm **Pre-emption Priority** (0 bit Sub-priority), tức là phải gọi `HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4)` (hoặc `NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4)`).
- Nếu có bất kỳ bit nào được chia cho Sub-priority, phép so sánh mặt nạ bit của FreeRTOS với thanh ghi `BASEPRI` sẽ bị sai lệch, và `vPortValidateInterruptPriority()` sẽ kích hoạt `configASSERT` ngay lập tức!

---

## 1.3. Mô hình Phân Tầng Ưu Tiên Ngắt của FreeRTOS & Ngưỡng `configMAX_SYSCALL_INTERRUPT_PRIORITY`
*(Sách Mastering FreeRTOS — Hình 61, tr. 223–228 & Mục 7.2, tr. 238–241)*

Với cấu hình chuẩn của khóa học (`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`, `configLIBRARY_LOWEST_INTERRUPT_PRIORITY = 15`), không gian 16 mức ưu tiên ngắt của STM32F103 được chia thành **2 vùng tách biệt**:

```
Ưu tiên phần cứng
CAO NHẤT
  ^   +-------------------------------------------------------+
  |   | Mức 0  (0x00)                                         |
  |   | Mức 1  (0x10)   VÙNG NGẮT NGOÀI TẦM KIỂM SOÁT KERNEL |
  |   | Mức 2  (0x20)   - KHÔNG bao giờ bị che bởi Critical   |
  |   | Mức 3  (0x30)     Section của FreeRTOS (jitter = 0).  |
  |   | Mức 4  (0x40)   - TUYỆT ĐỐI CẤM gọi mọi API FreeRTOS! |
  |   +================= Ngưỡng: configLIBRARY_MAX_SYSCALL = 5 (0x50) ===
  |   | Mức 5  (0x50)                                         |
  |   | Mức 6  (0x60) <-- ISR_NVIC_PRIO = 6 của bài tập       |
  |   | ...             VÙNG NGẮT AN TOÀN SYSCALL (Mức 5..15) |
  |   | Mức 14 (0xE0)   - Bị tạm che khi vào Critical Section.|
  |   |                 - ĐƯỢC PHÉP gọi các API "...FromISR". |
  |   +-------------------------------------------------------+
  v   | Mức 15 (0xF0) : SysTick & PendSV (configKERNEL_PRIO)  |
THẤP NHẤT +---------------------------------------------------+
```

### Tại sao phải tồn tại ngưỡng `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`?
1. **Bảo vệ dữ liệu nội bộ của Kernel bằng thanh ghi `BASEPRI`**:
   - Khi một Task hoặc một ISR đang thao tác trên các cấu trúc dữ liệu cốt lõi của FreeRTOS (như danh sách liên kết `pxReadyTasksLists`, `xDelayedTaskList`, `xPendingReadyList`, hay các trường trạng thái trong `TCB_t`), FreeRTOS vào **vùng găng (Critical Section)** bằng `taskENTER_CRITICAL()` hoặc `taskENTER_CRITICAL_FROM_ISR()` (macro `portSET_INTERRUPT_MASK_FROM_ISR()`).
   - Trên ARM Cortex-M3, FreeRTOS **không tắt toàn bộ ngắt toàn cục** (không dùng `CPSID I` / `PRIMASK`), mà ghi giá trị `configMAX_SYSCALL_INTERRUPT_PRIORITY` (`0x50`) vào thanh ghi **`BASEPRI`**.
   - Phần cứng Cortex-M3 khi thấy `BASEPRI = 0x50` sẽ **chặn tất cả các ngắt có giá trị ưu tiên số từ `5` đến `15`** (`0x50` đến `0xF0`), nhưng **vẫn cho phép các ngắt có mức ưu tiên từ `0` đến `4`** (`0x00` đến `0x40`) xảy ra ngay lập tức để phục vụ các tác vụ thời gian thực cực kỳ khắt khe (ví dụ: điều khiển động cơ tốc độ cao, bảo vệ quá dòng, truyền thông không trễ).
2. **Điều gì xảy ra nếu một ISR ở mức `0..4` cố tình gọi API `...FromISR`?**
   - Vì ngắt ở mức `0..4` **không bị chặn bởi `BASEPRI`**, nó có thể cắt ngang (preempt) CPU **ngay giữa lúc một Task hoặc một ISR khác đang nằm trong Critical Section** và đang sửa dở dang các con trỏ danh sách liên kết của Kernel (ví dụ đang nối một nút `ListItem_t` vào `pxReadyTasksLists`, con trỏ `pxNext` đã gán nhưng `pxPrevious` chưa kịp cập nhật).
   - Nếu ISR mức `0..4` lúc đó gọi `vTaskNotifyGiveFromISR()` hoặc `xQueueSendFromISR()`, hàm này lại tiếp tục đọc/ghi vào chính danh sách `pxReadyTasksLists` đang ở trạng thái bất nhất (inconsistent state) đó!
   - **Hậu quả**: Gây **Race Condition** làm đứt gãy danh sách liên kết của Kernel (Data Structure Corruption), dẫn đến **HardFault**, treo máy ngẫu nhiên rất khó tái hiện, hoặc một Task bị mất vĩnh viễn khỏi bộ lập lịch (**Undefined Behavior**).

---

## 1.4. Cơ chế `configASSERT()` và Cách FreeRTOS Bắt Lỗi Ưu Tiên Ngắt Tự Động
*(Sách Mastering FreeRTOS — Mục 11.2, tr. 330–332 & Chapter 12, tr. 357–359)*

Vì lỗi cấu hình ưu tiên ngắt trên Cortex-M là nguyên nhân số 1 gây lỗi ngẫu nhiên cho lập trình viên nhúng, port `ARM_CM3` của FreeRTOS (`portable/GCC/ARM_CM3/port.c`) tích hợp sẵn cơ chế kiểm tra tự động thông qua macro `configASSERT(x)`.

Khi bạn gọi `vTaskNotifyGiveFromISR()`, bên trong `tasks.c` (hàm `vTaskGenericNotifyGiveFromISR()`) sẽ gọi macro:
```c
portASSERT_IF_INTERRUPT_PRIORITY_INVALID();
```
Macro này gọi hàm `vPortValidateInterruptPriority( void )` trong `port.c`. Hàm này thực hiện **3 bước kiểm tra phần cứng trực tiếp**:
1. Đọc thanh ghi **IPSR** (*Interrupt Program Status Register*) của CPU để biết chính xác **vector ngắt nào đang chạy** (`ulCurrentInterrupt`).
2. Nếu là ngắt ngoại vi (`ulCurrentInterrupt >= portFIRST_USER_INTERRUPT_NUMBER`, tức `>= 16`), nó tra bảng thanh ghi **`NVIC_IPR`** của đúng vector ngắt đó để lấy ra byte ưu tiên 8-bit hiện hành (`ucCurrentPriority`), rồi kiểm tra:
   ```c
   configASSERT( ucCurrentPriority >= ucMaxSysCallPriority );
   ```
   - Ví dụ ở **Bước 4 của `README.md`**: Khi bạn đặt `HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0)` (số `2 < 5`), thanh ghi phần cứng `NVIC_IPR` chứa `2 << 4 = 0x20` (`32`).
   - Trong khi đó `ucMaxSysCallPriority = configMAX_SYSCALL_INTERRUPT_PRIORITY = 5 << 4 = 0x50` (`80`).
   - Phép so sánh `0x20 >= 0x50` (`32 >= 80`) trả về **SAI (`0`)** $\rightarrow$ **`configASSERT` bị kích hoạt ngay tức khắc!**
   - *(Lưu ý tinh tế: Cũng trong hàm này, nếu bạn quên gọi `HAL_NVIC_SetPriority` khiến thanh ghi mặc định bằng `0`, thì `0x00 >= 0x50` cũng sai và dính `configASSERT` ngay!)*
3. Kiểm tra thêm thanh ghi **AIRCR** (*Application Interrupt and Reset Control Register*) để đảm bảo phân nhóm ưu tiên không có bit Sub-priority nào (`NVIC_PRIORITYGROUP_4`):
   ```c
   configASSERT( ( portAIRCR_REG & portPRIORITY_GROUP_MASK ) <= ulMaxPRIGROUPValue );
   ```

---

## 1.5. Tại sao phải dùng API riêng `...FromISR` và Cơ chế `portYIELD_FROM_ISR()`
*(Sách Mastering FreeRTOS — Mục 6.2 & 6.3, tr. 182–193)*

### a) Tại sao không được gọi API thường (như `xTaskNotifyGive`) trong ISR?
1. **ISR không có TCB và không thể vào trạng thái `Blocked`**: Các API thường có tham số `xTicksToWait` để đưa Task đang gọi vào trạng thái `Blocked` nếu tài nguyên chưa sẵn sàng. ISR không phải là một Task nên không thể block.
2. **Khác biệt về cơ chế Critical Section**: API thường dùng `taskENTER_CRITICAL()`, trong khi API `FromISR` phải lưu và khôi phục trạng thái mặt nạ ngắt trước đó (`uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR()`) để hỗ trợ lồng ngắt (interrupt nesting).
3. **API thường tự động gọi Context Switch ngay bên trong hàm**, còn trong ISR thì không được phép chuyển ngữ cảnh giữa chừng khi ISR chưa chạy xong, mà phải gom lại về cuối ISR thông qua biến cờ `xHigherPriorityTaskWoken`.

### b) Cơ chế hoạt động của `xHigherPriorityTaskWoken` và `portYIELD_FROM_ISR()`
Khi gọi `vTaskNotifyGiveFromISR(s_ack_task_handle, &xHigherPriorityTaskWoken)`:
- Biến `xHigherPriorityTaskWoken` **bắt buộc phải được khởi tạo bằng `pdFALSE`** ở đầu ISR.
- Bên trong `vTaskNotifyGiveFromISR()`:
  - Kernel tăng giá trị thông báo `ulNotifiedValue` của `ack_task` và chuyển `ack_task` từ trạng thái `Blocked` sang `Ready` (đưa vào `pxReadyTasksLists`).
  - Kernel so sánh độ ưu tiên của `ack_task` vừa thức dậy với độ ưu tiên của Task đang chạy lúc bị ngắt cắt ngang (thường là `Idle Task` ở mức `0`).
  - Vì `ack_task` có độ ưu tiên cao hơn (`uxPriority > 0`), Kernel tự động gán `*pxHigherPriorityTaskWoken = pdTRUE`.
- Ở dòng cuối cùng của ISR, khi gọi:
  ```c
  portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
  ```
  - Nếu `xHigherPriorityTaskWoken == pdTRUE`, macro này sẽ ghi bit `PENDSVSET` (`1UL << 28`) vào thanh ghi điều khiển ngắt `SCB->ICSR` (`portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT`) để **treo yêu cầu ngắt `PendSV`**.
  - Vì `PendSV` có mức ưu tiên phần cứng thấp nhất (`15` / `0xF0`), nó **không cắt ngang ISR hiện tại** (mức `6`), mà đợi đúng khoảnh khắc `EXTI15_10_IRQHandler` kết thúc.
  - Nhờ tính năng phần cứng **Tail-Chaining** của ARM Cortex-M3 (chỉ tốn 6 chu kỳ xung nhịp thay vì phải unstack rồi lại stack 12+12 chu kỳ), CPU chuyển thẳng từ cuối `EXTI15_10_IRQHandler` sang `PendSV_Handler`.
  - `PendSV_Handler` lưu ngữ cảnh của `Idle Task`, nạp ngữ cảnh của `ack_task`, và khi thoát ngắt `PendSV`, CPU nhảy thẳng vào thực thi dòng lệnh kế tiếp sau `ulTaskNotifyTake()` trong `ack_task` (chính là lệnh đảo chân `PA7`)!

### c) Chuyện gì xảy ra nếu BỎ `portYIELD_FROM_ISR(xHigherPriorityTaskWoken)`?
*(Sách Mastering FreeRTOS — Mục 6.2.2, tr. 184)*
- `vTaskNotifyGiveFromISR()` vẫn chuyển `ack_task` sang trạng thái `Ready` và vẫn đặt `xHigherPriorityTaskWoken = pdTRUE`.
- Nhưng vì **không gọi `portYIELD_FROM_ISR()`**, bit `PENDSVSET` không được bật $\rightarrow$ ngắt `PendSV` không chạy khi thoát ISR.
- Khi `EXTI15_10_IRQHandler` kết thúc, phần cứng Cortex-M3 khôi phục lại ngữ cảnh của Task bị ngắt trước đó (ví dụ `Idle Task`), mặc dù `ack_task` có ưu tiên cao hơn đang nằm chờ sẵn trong `Ready` list!
- `ack_task` phải nằm đợi cho đến khi **ngắt `SysTick` kế tiếp** xảy ra (chu kỳ $T_{\text{tick}} = \frac{1}{1000\text{ Hz}} = 1\text{ ms} = 1000\text{ }\mu\text{s}$). Khi `SysTick_Handler` chạy `xTaskIncrementTick()`, bộ lập lịch mới phát hiện có Task ưu tiên cao hơn đang ở trạng thái `Ready` và mới kích hoạt `PendSV` để chuyển sang `ack_task`.
- Vì thời điểm bạn bấm nút (hoặc kích xung sườn xuống trên `PB11`) là hoàn toàn ngẫu nhiên so với nhịp đếm của `SysTick`, thời gian chờ từ lúc thoát ISR đến `SysTick` kế tiếp sẽ phân bố đều trong khoảng **`0 us` đến `1000 us`** (trung bình khoảng **`500 us`**) $\rightarrow$ Latency tăng vọt gấp hàng chục đến hàng trăm lần!

---

## 1.6. Deferred Interrupt Processing bằng Task Notifications (`vTaskNotifyGiveFromISR` + `ulTaskNotifyTake`)
*(Sách Mastering FreeRTOS — Mục 6.4, tr. 193–203 & Chapter 9, tr. 293–312)*

### a) Mô hình Xử lý Ngắt Trì hoãn (Deferred Interrupt Processing)
Nguyên tắc thiết kế RTOS chuẩn là giữ cho ISR **ngắn nhất có thể**: ISR chỉ xóa cờ phần cứng, ghi nhận sự kiện và đánh thức một Handler Task ưu tiên cao (`ack_task`), rồi thoát ngay. Mọi công việc xử lý sau đó được thực hiện trong ngữ cảnh của `ack_task`.

### b) Tại sao dùng Task Notification thay vì Binary/Counting Semaphore?
Trong mỗi khối quản lý tác vụ (`TCB_t` — *Task Control Block*) của FreeRTOS đã có sẵn 2 trường nội tại:
- `uint32_t ulNotifiedValue`: Giá trị thông báo 32-bit (đóng vai trò như bộ đếm sự kiện).
- `uint8_t ucNotifyState`: Trạng thái thông báo (`taskNOT_WAITING_NOTIFICATION`, `taskWAITING_NOTIFICATION`, `taskNOTIFICATION_RECEIVED`).

So với `xSemaphoreGiveFromISR()` / `xSemaphoreTake()`:
1. **Tốc độ nhanh hơn ~45%**: Gửi thông báo trực tiếp tới TCB của `ack_task` mà không cần đi qua cấu trúc trung gian `Queue_t` cùng các vòng lặp kiểm tra danh sách chờ của Queue.
2. **Tiết kiệm RAM tuyệt đối (`0 byte` cấp phát thêm)**: Binary/Counting Semaphore thực chất là một Queue tốn thêm khoảng `76–88 bytes` RAM trên Heap cho mỗi đối tượng, trong khi Task Notification nằm sẵn trong TCB của Task.

### c) Cặp hàm `vTaskNotifyGiveFromISR()` và `ulTaskNotifyTake()`
- **`vTaskNotifyGiveFromISR( TaskHandle_t xTaskToNotify, BaseType_t *pxHigherPriorityTaskWoken )`**:
  - Tăng `ulNotifiedValue` của `xTaskToNotify` lên 1 đơn vị (`ulNotifiedValue++`) và đặt trạng thái thành `taskNOTIFICATION_RECEIVED`.
- **`uint32_t ulTaskNotifyTake( BaseType_t xClearCountOnExit, TickType_t xTicksToWait )`**:
  - Nếu `ulNotifiedValue == 0`, Task chuyển sang trạng thái `Blocked` chờ tối đa `xTicksToWait` ticks (truyền `portMAX_DELAY` để chờ vô thời hạn mà không tốn CPU).
  - Khi thức dậy (tức `ulNotifiedValue > 0`), hàm trả về giá trị của `ulNotifiedValue` **trước khi** bị xóa/giảm, đồng thời xử lý `ulNotifiedValue` dựa vào tham số đầu tiên `xClearCountOnExit`:
    - **`pdTRUE` (Chế độ Binary Semaphore — dùng trong Bài 1 này)**: Đặt lại `ulNotifiedValue = 0` ngay khi thoát hàm. Mọi sự kiện gộp lại được xử lý trong 1 lần thức dậy.
    - **`pdFALSE` (Chế độ Counting Semaphore — sẽ dùng ở Bài 2)**: Chỉ giảm `ulNotifiedValue` đi 1 đơn vị (`ulNotifiedValue--`). Nếu trong lúc Task đang bận mà có $N$ ngắt xảy ra, lần gọi `ulTaskNotifyTake(pdFALSE, ...)` kế tiếp sẽ trả về ngay lập tức mà không block cho đến khi xử lý đủ $N$ lần.

---

# PHẦN 2: Hướng Dẫn Điền `PREDICTION.md` Và Giải Thích Tại Sao Lại Điền Như Vậy

> [!CAUTION]
> **QUY TẮC SỐNG CÒN TRONG [Rule.md](file:///c:/Users/DELL/intern-261-rtos-course-easy/Rule.md) & [RUBRIC.md](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/RUBRIC.md)**:
> - Bạn **BẮT BUỘC phải điền và `git commit` file `PREDICTION.md` TRƯỚC khi nạp chương trình hay sửa code đo đạc**.
> - `git log` phải cho thấy commit của `PREDICTION.md` có timestamp **sớm hơn** commit của `app/*` và `REPORT.md`. Nếu commit cùng lúc hoặc muộn hơn $\rightarrow$ **Điểm liệt (Trượt ngay)**.
> - Phong cách viết theo `Rule.md`: **Ngắn gọn 1–3 dòng cho mỗi câu hỏi, có con số cụ thể gắn với `params.h` của MSSV `2510343`, tuyệt đối không viết lan man**.

---

## 2.1. Nội dung gợi ý điền vào [PREDICTION.md](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/PREDICTION.md)

Bạn có thể điền trực tiếp 4 câu trả lời ngắn gọn, chuẩn xác dưới đây vào `week3/ex1/PREDICTION.md`:

```markdown
# Dự đoán — Tuần 3 Bài 1 (commit TRƯỚC khi chạy code)

1. Với `ISR_NVIC_PRIO` của bạn và `NVIC_PRIORITYGROUP_4`, số này nằm trong
   khoảng cho phép (`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` ..
   `configLIBRARY_LOWEST_INTERRUPT_PRIORITY`) không? Trình bày phép so sánh.

   - **Trả lời**: **Có (Hợp lệ)**. Với `ISR_NVIC_PRIO = 6`, `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5` và `configLIBRARY_LOWEST_INTERRUPT_PRIORITY = 15`, ta có `5 <= 6 <= 15` (trên thanh ghi 8-bit sau khi dịch trái 4 bit: `0x50 <= 0x60 <= 0xF0`). Do trên Cortex-M3 số ưu tiên lớn hơn nghĩa là độ ưu tiên phần cứng thấp hơn hoặc bằng ngưỡng syscall tối đa (`6 >= 5`), nên ISR được phép gọi các API `...FromISR` một cách an toàn.

2. Khi đặt priority EXTI cao hơn ngưỡng cho phép (số nhỏ hơn
   `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`) rồi gọi
   `vTaskNotifyGiveFromISR`, bạn dự đoán hành vi quan sát được là gì (reset?
   treo? thông báo gì trên UART/debugger)?

   - **Trả lời**: Ngay lần đầu tiên sườn xuống trên `PB11` kích hoạt ISR và gọi `vTaskNotifyGiveFromISR`, hàm `vPortValidateInterruptPriority()` sẽ phát hiện `ucCurrentPriority < ucMaxSysCallPriority` (ví dụ mức `2` ứng với `0x20 < 0x50`) và kích hoạt `configASSERT`. Hệ thống sẽ **treo đứng (halt)** trong vòng lặp vô hạn `for(;;)` của `configASSERT` với ngắt bị khóa (`taskDISABLE_INTERRUPTS()`), chân `PA7` không đảo trạng thái (hoặc in ra thông báo file/dòng lỗi trên UART nếu `configASSERT` có gọi hàm log trước khi lặp vô hạn).

3. Dự đoán latency ISR → task (có `portYIELD_FROM_ISR`) theo đơn vị us, dựa
   trên tần số CPU 72 MHz và số lệnh ước lượng trong đường đi ISR → context
   switch. So với `LATENCY_BUDGET_US`: đạt hay không?

   - **Trả lời**: Ở `72 MHz` (`1 us = 72 chu kỳ CPU`), tổng đường đi từ sườn xuống `PB11` qua phần cứng vào ngắt (12 cycles) + `HAL_GPIO_EXTI_IRQHandler` (~40 cycles) + `vTaskNotifyGiveFromISR` (~80 cycles) + chuyển ngữ cảnh `PendSV_Handler` (~80 cycles) + thoát `ulTaskNotifyTake` và gọi `HAL_GPIO_TogglePin` trên `PA7` (~50 cycles) tốn khoảng **250 – 450 chu kỳ CPU**, tương đương **`~3.5 us – 6.5 us`** (chắc chắn `< 15 us`). So với `LATENCY_BUDGET_US = 50 us`: **Đạt** (nhỏ hơn ngưỡng cho phép khoảng 8–10 lần).

4. Nếu bỏ `portYIELD_FROM_ISR`, `ack_task` sẽ chạy **khi nào** (ngay lập tức
   hay phải chờ tick ngắt kế tiếp)? Latency tăng thêm cỡ bao nhiêu?

   - **Trả lời**: Nếu bỏ `portYIELD_FROM_ISR`, ngắt `PendSV` không được kích hoạt lúc thoát ISR nên CPU quay lại task bị ngắt trước đó (ví dụ `Idle Task`), và `ack_task` **phải chờ đến ngắt `SysTick` kế tiếp** mới được lập lịch chạy. Với `configTICK_RATE_HZ = 1000 Hz` (`1 tick = 1000 us`) và thời điểm bấm nút hoàn toàn ngẫu nhiên so với nhịp `SysTick`, latency sẽ tăng thêm từ **`0 us` đến `1000 us`** (trung bình **`~500 us`**, cực đại **`~1000 us`**), vượt xa `LATENCY_BUDGET_US = 50 us`.
```

---

## 2.2. Giải thích chi tiết tại sao lại dự đoán như vậy

### Câu 1: Tại sao so sánh `5 <= 6 <= 15` (hay `0x50 <= 0x60 <= 0xF0`)?
- Trong [`params.h`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/params.h), `ISR_NVIC_PRIO = 6`.
- Trong [`FreeRTOSConfig.deltas`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/FreeRTOSConfig.deltas) (tham chiếu chuẩn từ `week1/ex1/app/FreeRTOSConfig.deltas`), hệ thống dùng:
  - `configPRIO_BITS = 4` (16 mức ưu tiên từ `0` đến `15`).
  - `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`.
  - `configLIBRARY_LOWEST_INTERRUPT_PRIORITY = 15`.
- Vì trên ARM Cortex-M3 **số ưu tiên càng lớn thì độ ưu tiên logic càng thấp**, nên "độ ưu tiên phần cứng thấp hơn hoặc bằng ngưỡng syscall tối đa" tương đương với bất đẳng thức số học:
  $$\text{configLIBRARY\_MAX\_SYSCALL\_INTERRUPT\_PRIORITY} \le \text{ISR\_NVIC\_PRIO} \le \text{configLIBRARY\_LOWEST\_INTERRUPT\_PRIORITY}$$
  $$5 \le 6 \le 15$$
- Khi nạp vào thanh ghi 8-bit `NVIC_IPR` của phần cứng (dịch trái $8 - 4 = 4$ bit):
  $$\text{ucMaxSysCallPriority} = 5 \ll 4 = 80 \text{ (0x50)}$$
  $$\text{ucCurrentPriority} = 6 \ll 4 = 96 \text{ (0x60)}$$
  $$\text{ucLowestPriority} = 15 \ll 4 = 240 \text{ (0xF0)}$$
  Ta có `0x50 <= 0x60 <= 0xF0` $\rightarrow$ Thỏa mãn điều kiện `configASSERT( ucCurrentPriority >= ucMaxSysCallPriority )` trong `port.c`.

### Câu 2: Tại sao hệ thống bị treo (halt) chứ không tự reset khi vi phạm?
- Khi đặt priority của EXTI là một số `< 5` (ví dụ `2`), giá trị trong thanh ghi `NVIC_IPR` là `2 << 4 = 0x20`.
- Ngay khi có cạnh xuống trên `PB11`, ISR chạy và gọi `vTaskNotifyGiveFromISR()`. Hàm này gọi `portASSERT_IF_INTERRUPT_PRIORITY_INVALID()` $\rightarrow$ `vPortValidateInterruptPriority()`.
- Tại đây, `configASSERT( ucCurrentPriority >= ucMaxSysCallPriority )` kiểm tra `0x20 >= 0x50` $\rightarrow$ **SAI (`0`)**.
- Với định nghĩa chuẩn của `configASSERT(x)`:
  ```c
  #define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }
  ```
  Vi điều khiển không tự reset (trừ khi bạn có bật Independent Watchdog `IWDG`), mà khóa toàn bộ ngắt có thể che (`BASEPRI = 0x50`) và đứng im mãi mãi trong vòng lặp `for(;;)`. Khi bấm nút **Pause/Suspend** trên Debugger (ST-Link / GDB), con trỏ lệnh `PC` sẽ dừng ngay tại dòng `configASSERT( ucCurrentPriority >= ucMaxSysCallPriority )` trong file `port.c`!

### Câu 3: Tại sao ước lượng Latency (có `portYIELD_FROM_ISR`) khoảng `3.5 us – 6.5 us`?
- Tần số xung nhịp CPU là $f_{\text{CPU}} = 72\text{ MHz} \Rightarrow 1\text{ chu kỳ lệnh} = \frac{1}{72}\text{ }\mu\text{s} \approx 13.89\text{ ns}$, hay **$72\text{ chu kỳ} = 1\text{ }\mu\text{s}$**.
- Phân rã chi tiết số chu kỳ CPU trên đường đi từ sườn xuống `PB11` đến sườn đảo `PA7`:
  1. **Hardware Interrupt Entry (NVIC Stacking)**: Phần cứng Cortex-M3 tự động đẩy 8 thanh ghi (`R0–R3, R12, LR, PC, xPSR`) vào stack và tìm địa chỉ vector trong bảng Vector Table: tốn cố định **12 chu kỳ** (cộng thêm vài chu kỳ đồng bộ tín hiệu EXTI và Flash wait-states $\approx 15\text{ cycles}$).
  2. **Trình phục vụ ngắt `EXTI15_10_IRQHandler` $\rightarrow$ `HAL_GPIO_EXTI_IRQHandler` $\rightarrow$ `HAL_GPIO_EXTI_Callback` $\rightarrow$ `exti_button_isr_handler`**: Kiểm tra cờ `EXTI->PR`, xóa cờ ngắt, gọi hàm callback: tốn khoảng **35 – 50 chu kỳ**.
  3. **Hàm `vTaskNotifyGiveFromISR()`**: Kiểm tra `vPortValidateInterruptPriority()` (~25 cycles) + vào critical section + tăng `ulNotifiedValue` + tháo `ack_task` khỏi danh sách chờ và đưa vào `pxReadyTasksLists` + gán `xHigherPriorityTaskWoken = pdTRUE`: tốn khoảng **70 – 100 chu kỳ**.
  4. **Kích `portYIELD_FROM_ISR()` & Tail-Chaining sang `PendSV_Handler`**: Ghi `SCB->ICSR = PENDSVSET`, thoát `EXTI15_10_IRQHandler`, phần cứng thực hiện **Tail-Chaining** (6 chu kỳ) nhảy thẳng vào `PendSV_Handler`.
  5. **Context Switch trong `PendSV_Handler`**: Lưu `R4–R11` của task cũ (`STMDB`), gọi `vTaskSwitchContext()` chọn `ack_task`, nạp lại `R4–R11` của `ack_task` (`LDMIA`), và thực hiện Exception Return (12 chu kỳ unstacking): tốn khoảng **70 – 90 chu kỳ**.
  6. **Thoát `ulTaskNotifyTake()` và thực thi `HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_7)`**: Xóa `ulNotifiedValue = 0`, thoát critical section trong `ulTaskNotifyTake`, gọi `HAL_GPIO_TogglePin` ghi ra thanh ghi `GPIOA->BSRR`: tốn khoảng **40 – 60 chu kỳ**.
- **Tổng cộng**: Khoảng **240 – 330 chu kỳ** (nếu biên dịch tối ưu `-O2`/`-Og`) hoặc lên đến **~400 – 500 chu kỳ** (nếu biên dịch `-O0` không tối ưu hóa và chịu thêm 2 Flash Wait-States ở 72 MHz).
- Đổi ra thời gian:
  $$t_{\text{latency}} = \frac{250 \dots 450\text{ cycles}}{72\text{ cycles/}\mu\text{s}} \approx 3.5\text{ }\mu\text{s} \dots 6.5\text{ }\mu\text{s}$$
- Ngay cả trong trường hợp biên dịch `-O0` hoàn toàn không tối ưu, con số này cũng chỉ quanh mức **`4 us – 10 us`**, nhỏ hơn rất nhiều so với ngân sách `LATENCY_BUDGET_US = 50 us` $\rightarrow$ **Đạt yêu cầu dư dả**.

### Câu 4: Tại sao bỏ `portYIELD_FROM_ISR` lại làm Latency nhảy lên `0 .. 1000 us` (trung bình `~500 us`)?
- Khi bỏ `portYIELD_FROM_ISR`, ngắt `EXTI15_10_IRQHandler` thoát ra mà không treo cờ `PENDSVSET`. Do đó bộ lập lịch không được gọi tại thời điểm thoát ngắt EXTI.
- Bộ lập lịch chỉ được gọi lại khi ngắt định thời hệ thống **`SysTick_Handler`** nổ ở đầu chu kỳ tick tiếp theo.
- Với `configTICK_RATE_HZ = 1000`, khoảng cách giữa 2 lần ngắt `SysTick` liên tiếp là:
  $$T_{\text{tick}} = \frac{1}{1000\text{ Hz}} = 1\text{ ms} = 1000\text{ }\mu\text{s}$$
- Nếu nút nhấn rơi vào ngay sát trước khi `SysTick` nổ, `ack_task` chỉ chờ vài chục $\mu\text{s}$. Nếu nút nhấn rơi vào ngay vừa sau khi `SysTick` vừa chạy xong, `ack_task` phải chờ trọn vẹn gần `1000 us` mới tới kỳ `SysTick` tiếp theo.
- Vì thao tác nhấn nút của con người là biến ngẫu nhiên phân bố đều $\mathcal{U}(0, 1000\text{ }\mu\text{s})$ trên chu kỳ tick, độ trễ tăng thêm trung bình là $\approx 500\text{ }\mu\text{s}$ và cực đại là $\approx 1000\text{ }\mu\text{s}$ $\rightarrow$ **Không đạt** `LATENCY_BUDGET_US = 50 us`.

---

# PHẦN 3: Hướng Dẫn Hiện Thực Code Theo Cấu Trúc `REPORT.md` & `README.md`, Các Cách Hiện Thực & Output Mong Đợi

> [!IMPORTANT]
> **Nhắc lại Luật Cấm Tuyệt Đối của Khóa Học ([Rule.md](file:///c:/Users/DELL/intern-261-rtos-course-easy/Rule.md))**:
> - **TUYỆT ĐỐI KHÔNG** dùng bất kỳ hàm, kiểu dữ liệu hay header nào thuộc bộ wrapper CMSIS-RTOS. Script chấm tự động [`tools/check_api.sh`](file:///c:/Users/DELL/intern-261-rtos-course-easy/tools/check_api.sh) sẽ quét toàn bộ mã nguồn; nếu phát hiện wrapper sẽ bị **0 điểm tự động**.
> - Chỉ sử dụng **FreeRTOS API nguyên bản** (`xTaskCreate`, `vTaskStartScheduler`, `vTaskNotifyGiveFromISR`, `ulTaskNotifyTake`, `portYIELD_FROM_ISR`, `configASSERT`...).

---

## 3.1. Hiện thực `app/FreeRTOSConfig.deltas` (và `FreeRTOSConfig.h`)

Mở file [`week3/ex1/app/FreeRTOSConfig.deltas`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/FreeRTOSConfig.deltas), hiện tại đang có 4 chỗ `<?>`. Hãy điền như sau:

```c
/* Các dòng cần đảm bảo có trong FreeRTOSConfig.h cho tuần 3 bài 1 */

#define configPRIO_BITS                                4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY        15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY   5

#define configKERNEL_INTERRUPT_PRIORITY \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

#define configASSERT( x ) \
    if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }
```

### Các cách hiện thực `configASSERT( x )` (Tùy chọn theo phương tiện thu thập bằng chứng ở Mục 1 `REPORT.md`)

Trong [`REPORT.md`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/REPORT.md) Mục 1 yêu cầu: *"Bằng chứng `configASSERT` khi đặt priority sai (Mô tả / log / ảnh chụp màn hình)"*. Bạn có **3 cách hiện thực `configASSERT(x)`**, mỗi cách phù hợp với một công cụ lấy bằng chứng khác nhau:

#### Cách 1 (Chuẩn mặc định trong `week1/ex1/app/FreeRTOSConfig.deltas` — Khuyên dùng nếu có ST-Link Debugger)
```c
#define configASSERT( x ) \
    if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }
```
- **Ưu điểm**: Ngắn gọn, đúng chuẩn tài liệu FreeRTOS (Sách *Mastering FreeRTOS* tr. 331), không phụ thuộc vào ngoại vi UART.
- **Cách lấy bằng chứng**: Khi chạy với Debugger (STM32CubeIDE / VS Code Cortex-Debug / Keil / GDB), bạn nhấn nút trên `PB11` rồi bấm **Pause (Suspend)**. Debugger sẽ chỉ rõ con trỏ `PC` đang kẹt tại dòng `configASSERT( ucCurrentPriority >= ucMaxSysCallPriority );` trong file `port.c`, kèm Call Stack đầy đủ và giá trị biến `ucCurrentPriority = 32 (0x20)`, `ucMaxSysCallPriority = 80 (0x50)`. Chụp màn hình này đưa vào `REPORT.md` là bằng chứng thuyết phục nhất!

#### Cách 2 (Kết hợp hàm `vAssertCalled(__FILE__, __LINE__)` in ra UART — Khuyên dùng nếu bạn muốn lấy Log Text trên Serial Monitor)
Trong `FreeRTOSConfig.h` / `FreeRTOSConfig.deltas`:
```c
extern void vAssertCalled( const char * const pcFileName, unsigned long ulLine );
#define configASSERT( x ) \
    if( ( x ) == 0 ) { vAssertCalled( __FILE__, __LINE__ ); }
```
Và định nghĩa hàm `vAssertCalled` trong `app_main.c` (hoặc `exti_button.c`):
```c
void vAssertCalled( const char * const pcFileName, unsigned long ulLine )
{
    taskDISABLE_INTERRUPTS();
    /* Nếu dự án đã khởi tạo UART1 (huart1), có thể gửi chuỗi báo lỗi: */
    /* printf("[ASSERT FAILED] File: %s, Line: %lu\r\n", pcFileName, ulLine); */
    for( ;; )
    {
    }
}
```
- **Ưu điểm**: In thẳng tên file (`.../portable/GCC/ARM_CM3/port.c`) và số dòng bị assert ra cửa sổ Terminal UART để copy-paste nguyên văn vào khối code log trong `REPORT.md`, đồng thời vẫn giữ nguyên hành vi khóa ngắt + lặp vô hạn chuẩn FreeRTOS.

#### Cách 3 (Thêm lệnh Software Breakpoint `__BKPT(0)` — Dừng tự động không cần bấm Pause)
```c
#define configASSERT( x ) \
    if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); __asm volatile( "bkpt 0" ); for( ;; ); }
```
- **Ưu điểm**: Ngay khoảnh khắc bạn bấm nút trên `PB11`, nếu đang cắm Debugger, IDE sẽ **tự động dừng ngay lập tức** tại dòng lệnh assert mà bạn không cần bấm nút Pause thủ công. *(Lưu ý: Chỉ bật `bkpt 0` khi đang kết nối Debugger; nếu chạy độc lập không cắm Debugger, lệnh `bkpt` sẽ kích hoạt `HardFault_Handler`).*

> **Lời khuyên**: Trong file nộp [`app/FreeRTOSConfig.deltas`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/FreeRTOSConfig.deltas), hãy giữ **Cách 1** (vì giống hệt mẫu chuẩn ở `week1/ex1/app/FreeRTOSConfig.deltas`). Nếu muốn in UART hoặc đặt breakpoint thì có thể thêm trong lúc thực nghiệm hoặc viết hàm `vAssertCalled`.

---

## 3.2. Hiện thực `app/exti_button.c` — Phân tích từng `TODO` và các cách hiện thực

Hãy xem kỹ cấu trúc của [`week3/ex1/app/exti_button.c`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/exti_button.c). File này có **6 vị trí `TODO`** chia làm 3 hàm: `exti_button_init()`, `exti_button_isr_handler()`, và `ack_task()`.

### A. Hàm `exti_button_init(void)` (TODO 1 & TODO 2)

#### 1. Chi tiết kỹ thuật cần lưu ý trên STM32F103:
- **Bật xung nhịp (Clock Enable)**:
  - Chân nút nhấn `PB<EXTI_PIN>` (`PB11`) nằm trên cổng **GPIOB** $\rightarrow$ phải gọi `__HAL_RCC_GPIOB_CLK_ENABLE()`.
  - Chân đo phản hồi `PA<ACK_PIN>` (`PA7`) nằm trên cổng **GPIOA** $\rightarrow$ phải gọi `__HAL_RCC_GPIOA_CLK_ENABLE()`.
  - **Đặc biệt quan trọng trên dòng STM32F103**: Để bộ chọn kênh ngắt ngoài (EXTI Mux trong thanh ghi `AFIO->EXTICR[2]`) ánh xạ được chân `PB11` (thay vì mặc định `PA11`) vào đường `EXTI_LINE_11`, dù hàm `HAL_GPIO_Init()` trên STM32F1 HAL có tự gọi `__HAL_RCC_AFIO_CLK_ENABLE()` bên trong, việc chủ động bật `__HAL_RCC_AFIO_CLK_ENABLE()` ngay đầu hàm là thói quen lập trình hệ thống chuẩn xác và an toàn.
- **Cấu hình chân `PA7` (`ACK_PIN = 7`)**:
  - Đề bài yêu cầu `ack_task` đảo trạng thái chân `PA<ACK_PIN>` (`PA7`). Trong skeleton của `exti_button_init()` mới chỉ ghi comment cho `PB<EXTI_PIN>`, nên bạn **nhất định phải cấu hình thêm `PA<ACK_PIN>` là `GPIO_MODE_OUTPUT_PP`** (Output Push-Pull, tốc độ `GPIO_SPEED_FREQ_HIGH`, mức khởi tạo `GPIO_PIN_RESET`) ngay trong `exti_button_init()` để chân `PA7` xuất được xung vuông sắc nét ra Logic Analyzer!
- **Ánh xạ `EXTI_PIN` sang mã vector ngắt `IRQn_Type`**:
  Trên STM32F103, 16 đường EXTI (`0..15`) được gom vào 7 vector ngắt trong NVIC:
  - `EXTI_PIN = 0` $\rightarrow$ `EXTI0_IRQn`
  - `EXTI_PIN = 1` $\rightarrow$ `EXTI1_IRQn`
  - `EXTI_PIN = 2` $\rightarrow$ `EXTI2_IRQn`
  - `EXTI_PIN = 3` $\rightarrow$ `EXTI3_IRQn`
  - `EXTI_PIN = 4` $\rightarrow$ `EXTI4_IRQn`
  - `EXTI_PIN = 5..9` $\rightarrow$ `EXTI9_5_IRQn`
  - `EXTI_PIN = 10..15` $\rightarrow$ `EXTI15_10_IRQn` *(Trường hợp của bạn: `EXTI_PIN = 11` thuộc `EXTI15_10_IRQn`)*.

  Bạn có **2 cách viết** để lấy `IRQn_Type`:
  - **Cách A (Trực tiếp cho `EXTI_PIN = 11`)**: Dùng thẳng `EXTI15_10_IRQn`.
  - **Cách B (Hàm phụ trợ tổng quát — Khuyên dùng vì tuân thủ tuyệt đối nguyên tắc không hardcode theo `params.h`)**: Viết một hàm `static IRQn_Type prv_get_exti_irqn(uint32_t pin)` tự động trả về đúng `IRQn_Type` cho mọi giá trị `EXTI_PIN` từ `0..15`.

- **Xóa cờ ngắt treo (Pending bit) trước khi bật NVIC**:
  Khi vừa cấu hình GPIO pull-up và EXTI cạnh xuống, đôi khi sự chuyển mức điện áp lúc khởi tạo làm bật sẵn cờ pending trong `EXTI->PR`. Gọi `__HAL_GPIO_EXTI_CLEAR_IT(1u << EXTI_PIN)` và `HAL_NVIC_ClearPendingIRQ(irqn)` trước khi `HAL_NVIC_EnableIRQ(irqn)` giúp tránh việc vừa bật ngắt đã bị kích giả 1 lần ngay lúc khởi động (khi `s_ack_task_handle` có thể còn bằng `NULL`).

---

### B. Hàm `exti_button_isr_handler(void)` (TODO 3 & TODO 4)

Trong hàm này, bạn cần đánh thức `ack_task` từ ngữ cảnh ISR. Có **2 cách hiện thực FreeRTOS API** đều hợp lệ và cho kết quả giống hệt nhau:

#### Cách 1 (Chuẩn theo yêu cầu `README.md` và `TODO 3` — BẮT BUỘC DÙNG)
Sử dụng `vTaskNotifyGiveFromISR()` kết hợp `portYIELD_FROM_ISR()`:
```c
void exti_button_isr_handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (s_ack_task_handle != NULL)
    {
        /* TODO 3: vTaskNotifyGiveFromISR(s_ack_task_handle, &xHigherPriorityTaskWoken); */
        vTaskNotifyGiveFromISR(s_ack_task_handle, &xHigherPriorityTaskWoken);
    }

    /* TODO 4: portYIELD_FROM_ISR(xHigherPriorityTaskWoken); */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```
> **Lưu ý an toàn**: Kiểm tra `if (s_ack_task_handle != NULL)` trước khi gọi `vTaskNotifyGiveFromISR` giúp bảo vệ hệ thống không bị `configASSERT( pxTaskToNotify != NULL )` nếu chẳng may có nhiễu ngắt EXTI xảy ra trước khi `ack_task` kịp chạy dòng `s_ack_task_handle = xTaskGetCurrentTaskHandle()`.

#### Cách 2 (Tổng quát hơn bằng `xTaskNotifyFromISR` — Nêu để hiểu sâu khi vấn đáp)
Thực chất trong `task.h`, `vTaskNotifyGiveFromISR()` là phiên bản chuyên biệt (nhanh gọn hơn) của hàm tổng quát `xTaskNotifyFromISR()` với hành động `eIncrement`:
```c
xTaskNotifyFromISR(s_ack_task_handle, 0, eIncrement, &xHigherPriorityTaskWoken);
portEND_SWITCHING_ISR(xHigherPriorityTaskWoken); /* Tương đương portYIELD_FROM_ISR */
```
*(Trong code nộp bài, hãy dùng đúng `vTaskNotifyGiveFromISR` và `portYIELD_FROM_ISR` như `README.md` yêu cầu).*

---

### C. Hàm `ack_task(void *arg)` (TODO 5 & TODO 6)

Trong vòng lặp `for (;;)` của `ack_task`:
1. **TODO 5**: Chờ thông báo từ ISR bằng `ulTaskNotifyTake(pdTRUE, portMAX_DELAY);`.
   - Tham số 1 là `pdTRUE`: Xóa giá trị thông báo (`ulNotifiedValue`) về `0` khi thoát (chế độ Binary).
   - Tham số 2 là `portMAX_DELAY`: Đưa `ack_task` vào trạng thái `Blocked` chờ vô thời hạn cho đến khi ISR gọi `vTaskNotifyGiveFromISR`.
2. **TODO 6**: Ngay khi thức dậy, đảo trạng thái chân `PA<ACK_PIN>` (`PA7`). Có **2 cách đảo chân GPIO**:
   - **Cách 1 (Chuẩn HAL — Khuyên dùng theo `TODO 6`)**:
     ```c
     HAL_GPIO_TogglePin(GPIOA, (uint16_t)(1u << ACK_PIN));
     ```
   - **Cách 2 (Thao tác thanh ghi trực tiếp — Nhanh hơn khoảng 8–10 chu kỳ CPU $\approx 0.12\text{ }\mu\text{s}$)**:
     ```c
     GPIOA->ODR ^= (1u << ACK_PIN);
     ```
   *(Cả hai cách đều cho latency chỉ khoảng `3.5 – 6.5 us`, đạt dư sức so với `50 us`. Dùng `HAL_GPIO_TogglePin` theo đúng gợi ý trong comment `TODO 6` của đề bài là rõ ràng và chuẩn mực nhất).*

---

### D. Các cách truyền và khởi tạo `s_ack_task_handle` (Điểm tinh tế cần biết!)

Trong skeleton [`app/exti_button.c`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/exti_button.c), biến `static TaskHandle_t s_ack_task_handle = NULL;` được lấy ở đầu hàm `ack_task()` bằng:
```c
s_ack_task_handle = xTaskGetCurrentTaskHandle();
```
- **Cách 1 (Giữ nguyên theo skeleton)**:
  - Trong `app_main()`, bạn gọi `exti_button_init()`, tạo `ack_task` bằng `xTaskCreate(ack_task, "ack", 256, NULL, 3, NULL);`, rồi gọi `vTaskStartScheduler()`.
  - Vì `ack_task` có độ ưu tiên cao nhất (`3`), ngay khi `vTaskStartScheduler()` khởi chạy, `ack_task` sẽ là Task đầu tiên được CPU thực thi, nó sẽ gán ngay `s_ack_task_handle = xTaskGetCurrentTaskHandle()` rồi mới vào vòng lặp gọi `ulTaskNotifyTake(pdTRUE, portMAX_DELAY)` để ngủ chờ ngắt.
  - Kết hợp với lệnh `if (s_ack_task_handle != NULL)` trong `exti_button_isr_handler()`, cách này hoàn toàn an toàn 100% và không cần sửa đổi chữ ký hàm trong [`exti_button.h`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/exti_button.h).
- **Cách 2 (Nâng cao để trả lời vấn đáp)**:
  - Nếu muốn `s_ack_task_handle` có giá trị hợp lệ **trước cả khi Scheduler chạy** (hoặc trước khi bật `HAL_NVIC_EnableIRQ`), ta có thể truyền địa chỉ `&s_ack_task_handle` vào tham số thứ 6 (`pxCreatedTask`) của `xTaskCreate()`, hoặc chỉ gọi `HAL_NVIC_EnableIRQ(irqn)` ở đầu `ack_task()` sau dòng `s_ack_task_handle = xTaskGetCurrentTaskHandle()`.

---

### E. Code hoàn chỉnh cho `app/exti_button.c`

Dưới đây là toàn bộ nội dung hoàn chỉnh của [`week3/ex1/app/exti_button.c`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/exti_button.c):

```c
#include "exti_button.h"
#include "params.h"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

static TaskHandle_t s_ack_task_handle = NULL;

static IRQn_Type prv_get_exti_irqn(uint32_t pin)
{
    switch (pin)
    {
        case 0:  return EXTI0_IRQn;
        case 1:  return EXTI1_IRQn;
        case 2:  return EXTI2_IRQn;
        case 3:  return EXTI3_IRQn;
        case 4:  return EXTI4_IRQn;
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:  return EXTI9_5_IRQn;
        default: return EXTI15_10_IRQn;
    }
}

void exti_button_init(void)
{
    GPIO_InitTypeDef g = {0};
    IRQn_Type irqn = prv_get_exti_irqn(EXTI_PIN);

    /* Bật clock cho GPIOA (PA<ACK_PIN>), GPIOB (PB<EXTI_PIN>) và AFIO (ánh xạ EXTI) */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    /* Cấu hình PA<ACK_PIN> (PA7) làm Output Push-Pull để đo latency trên Logic Analyzer */
    HAL_GPIO_WritePin(GPIOA, (uint16_t)(1u << ACK_PIN), GPIO_PIN_RESET);
    g.Pin   = (uint16_t)(1u << ACK_PIN);
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);

    /* TODO 1: Cấu hình PB<EXTI_PIN> (PB11) với GPIO_MODE_IT_FALLING, GPIO_PULLUP */
    g.Pin   = (uint16_t)(1u << EXTI_PIN);
    g.Mode  = GPIO_MODE_IT_FALLING;
    g.Pull  = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &g);

    /* Xóa cờ ngắt EXTI tồn đọng (nếu có) trước khi bật NVIC */
    __HAL_GPIO_EXTI_CLEAR_IT((uint16_t)(1u << EXTI_PIN));
    HAL_NVIC_ClearPendingIRQ(irqn);

    /* TODO 2: HAL_NVIC_SetPriority(..., ISR_NVIC_PRIO, 0); HAL_NVIC_EnableIRQ(...);
     * LƯU Ý BƯỚC 4 README:
     * - Lần chạy 1 (Chứng minh configASSERT): Tạm sửa ISR_NVIC_PRIO thành (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY - 1) hoặc 2 (số < 5).
     * - Lần chạy 2 (Chạy chuẩn & đo Latency): Đặt đúng ISR_NVIC_PRIO (bằng 6).
     */
    HAL_NVIC_SetPriority(irqn, ISR_NVIC_PRIO, 0);
    HAL_NVIC_EnableIRQ(irqn);
}

void exti_button_isr_handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (s_ack_task_handle != NULL)
    {
        /* TODO 3: vTaskNotifyGiveFromISR(s_ack_task_handle, &xHigherPriorityTaskWoken); */
        vTaskNotifyGiveFromISR(s_ack_task_handle, &xHigherPriorityTaskWoken);
    }

    /* TODO 4: portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
     * (Ở Bước 6 README cấu hình 2: Tạm comment dòng này lại để đo latency khi KHÔNG có portYIELD_FROM_ISR)
     */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void ack_task(void *arg)
{
    (void)arg;
    s_ack_task_handle = xTaskGetCurrentTaskHandle();

    for (;;)
    {
        /* TODO 5: ulTaskNotifyTake(pdTRUE, portMAX_DELAY); */
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /* TODO 6: Toggle PA<ACK_PIN> ngay sau khi thức dậy */
        HAL_GPIO_TogglePin(GPIOA, (uint16_t)(1u << ACK_PIN));
    }
}
```

---

## 3.3. Hiện thực `app/app_main.c` và Móc nối ISR trong `stm32f1xx_it.c`

### A. Code hoàn chỉnh cho [`week3/ex1/app/app_main.c`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/app_main.c)

```c
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f1xx_hal.h"
#include "params.h"
#include "exti_button.h"

void app_main(void)
{
    BaseType_t rc;

    /* Đảm bảo toàn bộ 4 bit ưu tiên NVIC đều là Pre-emption Priority (bắt buộc cho FreeRTOS Cortex-M3) */
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    /* Khởi tạo PB<EXTI_PIN>, PA<ACK_PIN> và NVIC cho EXTI */
    exti_button_init();

    /* Tạo ack_task với độ ưu tiên cao (ví dụ 3 / tskIDLE_PRIORITY + 3) để preempt Idle Task ngay khi được đánh thức */
    rc = xTaskCreate(ack_task, "ack", 256, NULL, tskIDLE_PRIORITY + 3, NULL);
    configASSERT(rc == pdPASS);

    vTaskStartScheduler();

    for (;;) {}
}
```

### B. Cách móc nối Vector Ngắt từ `stm32f1xx_it.c` (Có 2 cách)

Như chú thích trong [`week3/ex1/app/exti_button.h`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/exti_button.h), phần cứng Cortex-M3 khi có sườn xuống trên `PB11` sẽ nhảy vào hàm vector `EXTI15_10_IRQHandler(void)` (thường nằm trong `Core/Src/stm32f1xx_it.c`). Bạn cần đảm bảo cờ ngắt phần cứng `EXTI->PR` được xóa và hàm `exti_button_isr_handler()` được gọi. Có **2 cách thực hiện**:

#### Cách 1 (Chuẩn kiến trúc STM32 HAL — Theo đúng hướng dẫn trong `exti_button.h`)
1. Trong file `stm32f1xx_it.c`:
   ```c
   #include "params.h"

   void EXTI15_10_IRQHandler(void)
   {
       HAL_GPIO_EXTI_IRQHandler((uint16_t)(1u << EXTI_PIN));
   }
   ```
2. Trong file `app/exti_button.c` (hoặc `app/app_main.c`), ghi đè (override) hàm `__weak` callback của HAL:
   ```c
   void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
   {
       if (GPIO_Pin == (uint16_t)(1u << EXTI_PIN))
       {
           exti_button_isr_handler();
       }
   }
   ```
   *(Hàm `HAL_GPIO_EXTI_IRQHandler` của ST sẽ tự động kiểm tra cờ trong thanh ghi `EXTI->PR`, xóa cờ bằng `__HAL_GPIO_EXTI_CLEAR_IT()`, rồi gọi `HAL_GPIO_EXTI_Callback()`).*

#### Cách 2 (Gọi trực tiếp trong `EXTI15_10_IRQHandler` không qua tầng Callback của HAL)
Nếu bạn muốn giảm bớt 1 tầng gọi hàm để tiết kiệm ~10 chu kỳ CPU:
```c
#include "params.h"
#include "exti_button.h"

void EXTI15_10_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT((uint16_t)(1u << EXTI_PIN)) != RESET)
    {
        __HAL_GPIO_EXTI_CLEAR_IT((uint16_t)(1u << EXTI_PIN));
        exti_button_isr_handler();
    }
}
```
> **Gợi ý**: Bạn có thể đặt luôn hàm `void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)` ở cuối file `app/exti_button.c` (Cách 1) để dù `stm32f1xx_it.c` của khung project dùng `HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11)` thì code của bạn vẫn tự động bắt được ngắt mà không cần sửa nhiều trong `Core/Src`.

---

## 3.4. Bảng Tổng Hợp Các Hàm Trọng Tâm (Theo Chuẩn [Rule.md](file:///c:/Users/DELL/intern-261-rtos-course-easy/Rule.md))

| Tên hàm / Macro | File gọi | Tham số truyền vào | Giá trị trả về | Ý nghĩa trong bài tập |
|---|---|---|---|---|
| `HAL_NVIC_SetPriorityGrouping` | [`app_main.c`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/app_main.c) | `NVIC_PRIORITYGROUP_4` | `void` | Cấu hình toàn bộ 4 bit ưu tiên NVIC làm Pre-emption Priority (0 bit Sub-priority), bắt buộc cho FreeRTOS trên Cortex-M3. |
| `HAL_GPIO_Init` | [`exti_button.c`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/exti_button.c) | `GPIOB, &g` (`PB11`, `GPIO_MODE_IT_FALLING`, `GPIO_PULLUP`) và `GPIOA, &g` (`PA7`, `GPIO_MODE_OUTPUT_PP`) | `void` | Khởi tạo chân nút nhấn ngắt ngoài sườn xuống `PB11` và chân xuất xung phản hồi `PA7`. |
| `HAL_NVIC_SetPriority` | [`exti_button.c`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/exti_button.c) | `EXTI15_10_IRQn`, `ISR_NVIC_PRIO` (`6`), `0` | `void` | Đặt mức ưu tiên phần cứng NVIC cho đường ngắt `EXTI15_10_IRQn` nằm trong vùng an toàn syscall (`5..15`). |
| `HAL_NVIC_EnableIRQ` | [`exti_button.c`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/exti_button.c) | `EXTI15_10_IRQn` | `void` | Cho phép bộ điều khiển NVIC nhận ngắt từ vector `EXTI15_10_IRQn`. |
| `xTaskGetCurrentTaskHandle` | [`exti_button.c`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/exti_button.c) | `void` | `TaskHandle_t` | Lấy handle hiện tại của `ack_task` lưu vào `s_ack_task_handle` để ISR biết cần gửi thông báo cho task nào. |
| `vTaskNotifyGiveFromISR` | [`exti_button.c`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/exti_button.c) | `s_ack_task_handle`, `&xHigherPriorityTaskWoken` | `void` | Tăng giá trị thông báo `ulNotifiedValue` của `ack_task` từ trong ISR, chuyển `ack_task` sang `Ready` và đặt `xHigherPriorityTaskWoken = pdTRUE`. |
| `portYIELD_FROM_ISR` | [`exti_button.c`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/exti_button.c) | `xHigherPriorityTaskWoken` | Macro (`void`) | Nếu `xHigherPriorityTaskWoken == pdTRUE`, bật bit `PENDSVSET` trong `SCB->ICSR` để chuyển ngữ cảnh sang `ack_task` ngay khi thoát ISR. |
| `ulTaskNotifyTake` | [`exti_button.c`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/exti_button.c) | `pdTRUE`, `portMAX_DELAY` | `uint32_t` (giá trị thông báo trước khi xóa) | Đưa `ack_task` vào trạng thái `Blocked` chờ thông báo từ ISR; khi thức dậy tự động xóa `ulNotifiedValue` về `0` (nhờ `pdTRUE`). |
| `HAL_GPIO_TogglePin` | [`exti_button.c`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/exti_button.c) | `GPIOA`, `1u << ACK_PIN` (`GPIO_PIN_7`) | `void` | Đảo mức logic chân `PA7` ngay khi `ack_task` thức dậy để tạo cạnh xung đo latency trên Logic Analyzer. |

---

## 3.5. Quy Trình Thực Nghiệm Từng Bước & Output Mong Đợi Cho Từng Câu Trong `REPORT.md`

Dưới đây là hướng dẫn chi tiết cách thực hiện từng thí nghiệm theo đúng trình tự `README.md` và **Output mong đợi + Mẫu điền chuẩn** cho cả 5 mục trong [`week3/ex1/REPORT.md`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/REPORT.md).

---

### MỤC 1 TRONG `REPORT.md`: Bằng chứng `configASSERT` khi đặt Priority sai

#### 1. Cách làm thí nghiệm (Bước 4 trong `README.md`):
- Trong `exti_button_init()`, tạm thời đổi dòng `HAL_NVIC_SetPriority` thành một số **nhỏ hơn `5`** (tức ưu tiên phần cứng **cao hơn** `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`), ví dụ đặt bằng **`2`** (hoặc `4`):
  ```c
  HAL_NVIC_SetPriority(irqn, 2, 0); /* Cố tình đặt sai: 2 < 5 để kích hoạt configASSERT */
  ```
- Biên dịch và nạp chương trình vào STM32F103C8T6 ở chế độ **Debug**.
- Bấm **Resume (F8)** cho chương trình chạy, sau đó **nhấn nút trên chân `PB11`** (hoặc nối nhẹ chân `PB11` xuống `GND` để tạo sườn xuống).
- Bấm **Pause / Suspend** trên IDE.

#### 2. Output mong đợi khi quan sát:
- Chân `PA7` hoàn toàn **không đảo trạng thái** (giữ nguyên mức `0 V`).
- Trên Debugger:
  - Con trỏ lệnh `PC` dừng tại hàm `vPortValidateInterruptPriority()` trong file `Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c` ngay tại dòng:
    ```c
    configASSERT( ucCurrentPriority >= ucMaxSysCallPriority );
    ```
  - **Call Stack (Backtrace)** hiển thị rõ chuỗi gọi hàm:
    1. `vPortValidateInterruptPriority()` tại `port.c`
    2. `vTaskGenericNotifyGiveFromISR()` tại `tasks.c`
    3. `exti_button_isr_handler()` tại `exti_button.c`
    4. `HAL_GPIO_EXTI_Callback()` / `HAL_GPIO_EXTI_IRQHandler()` tại `stm32f1xx_hal_gpio.c`
    5. `EXTI15_10_IRQHandler()` tại `stm32f1xx_it.c`
  - **Giá trị biến cục bộ trong `vPortValidateInterruptPriority()`**:
    - `ulCurrentInterrupt = 56` (`0x38` = $16 + 40$, với `EXTI15_10_IRQn = 40`).
    - `ucCurrentPriority = 32` (`0x20`, do `2 << 4 = 32`).
    - `ucMaxSysCallPriority = 80` (`0x50`, do `5 << 4 = 80`).
- Hãy chụp ảnh màn hình cửa sổ Debug (hoặc copy Call Stack / UART log) lưu vào thư mục `week3/ex1/` (ví dụ `assert_fail.png`) và chèn vào Mục 1 của `REPORT.md`.

#### 3. Nội dung chuẩn cần viết cho Mục 1 trong `REPORT.md`:
> **Mô tả / Log / Ảnh chụp màn hình**:
> - Khi đặt `HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0)` (mức ưu tiên số `2 < 5`) và kích sườn xuống trên `PB11`, chương trình lập tức dừng trong vòng lặp vô hạn của `configASSERT`, chân `PA7` không đảo mức.
> - Kết quả kiểm tra trên Debugger (Call Stack & Variables):
>   ```text
>   Thread #1 (Suspended : Signal : SIGINT:Interrupt)
>     vPortValidateInterruptPriority() at port.c:783
>     vTaskGenericNotifyGiveFromISR() at tasks.c:4978
>     exti_button_isr_handler() at exti_button.c:64
>     HAL_GPIO_EXTI_IRQHandler() at stm32f1xx_hal_gpio.c:525
>     EXTI15_10_IRQHandler() at stm32f1xx_it.c:210
>   Variables:
>     ulCurrentInterrupt   = 56 (16 + EXTI15_10_IRQn[40])
>     ucCurrentPriority    = 32 (0x20 = 2 << 4)
>     ucMaxSysCallPriority = 80 (0x50 = 5 << 4)
>   ```
>
> **Giải thích vì sao assert được kích hoạt**:
> - Khi ISR gọi `vTaskNotifyGiveFromISR()` (thực chất là macro gọi `vTaskGenericNotifyGiveFromISR()`), đầu hàm này gọi macro **`portASSERT_IF_INTERRUPT_PRIORITY_INVALID()`**, dẫn tới hàm **`vPortValidateInterruptPriority()`** trong `port.c`.
> - Hàm `vPortValidateInterruptPriority()` đọc thanh ghi `IPSR` để lấy số hiệu ngắt đang chạy (`ulCurrentInterrupt = 56`), sau đó đọc byte ưu tiên phần cứng tương ứng trong thanh ghi `NVIC_IPR` được `ucCurrentPriority = 2 << (8 - configPRIO_BITS) = 0x20` (`32`).
> - Tiếp theo, hàm kiểm tra điều kiện:
>   `configASSERT( ucCurrentPriority >= ucMaxSysCallPriority );`
>   Vì `ucMaxSysCallPriority = configMAX_SYSCALL_INTERRUPT_PRIORITY = 5 << 4 = 0x50` (`80`), phép so sánh **`0x20 >= 0x50` (`32 >= 80`) bị SAI (`0`)**, làm kích hoạt `configASSERT` khóa ngắt (`taskDISABLE_INTERRUPTS()`) và treo CPU tại `for(;;)`.

---

### MỤC 2 TRONG `REPORT.md`: Đo Latency ISR → Task (Có và Không có `portYIELD_FROM_ISR`)

#### 1. Cách làm thí nghiệm (Bước 5 & Bước 6 trong `README.md`):
- **Sửa lại Priority hợp lệ**: Đổi lại `HAL_NVIC_SetPriority(irqn, ISR_NVIC_PRIO, 0)` (với `ISR_NVIC_PRIO = 6`). Nạp lại, xác nhận không còn bị `configASSERT`.
- **Mắc Logic Analyzer**:
  - **Kênh `CH0` (`D0`)**: Nối vào chân **`PB11`** (kênh kích ngắt EXTI — đặt Trigger ở sườn xuống *Falling Edge*).
  - **Kênh `CH1` (`D1`)**: Nối vào chân **`PA7`** (kênh phản hồi của `ack_task`).
  - **Chân `GND`**: Nối chung `GND` của Logic Analyzer với `GND` của board STM32.
  - **Tần số lấy mẫu (Sample Rate)**: Đặt tối thiểu **`12 MHz` hoặc `24 MHz`** (độ phân giải `0.083 us` hoặc `0.0417 us`) để đo chính xác latency cỡ vài micro-giây!
- **Thực hiện 2 cấu hình đo** (mỗi cấu hình bấm nút / kích xung ít nhất 10–20 lần để lấy Min, Mean, Max):
  1. **Cấu hình 1 — Có `portYIELD_FROM_ISR(xHigherPriorityTaskWoken)`**: Giữ nguyên dòng `portYIELD_FROM_ISR(xHigherPriorityTaskWoken);` ở cuối `exti_button_isr_handler()`.
  2. **Cấu hình 2 — Không có `portYIELD_FROM_ISR`**: Tạm comment dòng `// portYIELD_FROM_ISR(xHigherPriorityTaskWoken);`, biên dịch & nạp lại, sau đó đo khoảng cách từ sườn xuống `PB11` đến sườn đảo trạng thái kế tiếp của `PA7`. *(Đo xong nhớ mở comment lại để code nộp bài hoàn chỉnh!).*

#### 2. Output mong đợi khi đo trên Logic Analyzer:

```
Cấu hình 1: CÓ portYIELD_FROM_ISR (Latency ~ 4 us - 8 us, rất ổn định)
PB11 (CH0): ────────┐
                    └──────────────────────────────────────────
PA7  (CH1): ──────────────┐
                          └────────────────────────────────────
                    |<--->|
                 ~ 4.5 - 6.5 us (< 50 us -> ĐẠT!)

Cấu hình 2: KHÔNG CÓ portYIELD_FROM_ISR (Chờ đến cạnh SysTick 1 ms kế tiếp)
PB11 (CH0): ────────┐
                    └──────────────────────────────────────────
PA7  (CH1): ──────────────────────────────────────────┐
                                                      └────────
                    |<------------------------------->|
                       Ngẫu nhiên từ ~20 us đến ~1000 us
                       (Trung bình ~500 us > 50 us -> KHÔNG ĐẠT!)
```

| Cấu hình | Min (us) mong đợi | Mean (us) mong đợi | Max (us) mong đợi | Đạt `LATENCY_BUDGET_US = 50 us`? |
|---|---|---|---|---|
| **Có `portYIELD_FROM_ISR`** | `~3.8 – 5.5` | `~4.2 – 6.2` | `~5.0 – 8.5` | **Đạt** (nhỏ hơn `50 us` rất nhiều) |
| **Không có `portYIELD_FROM_ISR`** | `~15.0 – 80.0` | `~450.0 – 550.0` | `~920.0 – 1005.0` | **Không đạt** (vượt ngưỡng `50 us` tới ~20 lần) |

*(Lưu ý: Bạn hãy điền đúng con số thực tế đo được từ phần mềm Logic Analyzer của bạn — ví dụ PulseView / Saleae Logic 2 — vào bảng trong `REPORT.md` và đính kèm ảnh chụp waveform).*

---

### MỤC 3 TRONG `REPORT.md`: Giải thích quan hệ Priority

Dưới đây là câu trả lời mẫu đầy đủ, súc tích cho 3 câu hỏi trong Mục 3 của [`REPORT.md`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/REPORT.md):

1. **Số NVIC priority nhỏ / lớn nghĩa là gì trên Cortex-M?**
   - Trên kiến trúc ARM Cortex-M3 (STM32F103 với `configPRIO_BITS = 4`, tức 16 mức từ `0` đến `15`), quy ước ưu tiên phần cứng **ngược chiều với giá trị số học**: **số càng NHỎ thì độ ưu tiên phần cứng càng CAO** (mức `0` là ưu tiên cao nhất, mức `15` là ưu tiên thấp nhất dành cho `SysTick` và `PendSV`).
   - Khi lưu vào thanh ghi 8-bit của NVIC (`NVIC_IPR`) và `BASEPRI`, giá trị 4-bit này được dịch trái 4 bit (`prio << 4`) nằm ở 4 bit cao MSB (`0x00, 0x10, ..., 0xF0`).

2. **Vì sao `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` tồn tại?**
   - Hằng số này (bằng `5`, ứng với `configMAX_SYSCALL_INTERRUPT_PRIORITY = 0x50`) đặt ra ranh giới phân tách giữa các ngắt do FreeRTOS quản lý (mức `5..15`) và các ngắt thời gian thực khắt khe nằm ngoài tầm kiểm soát của Kernel (mức `0..4`).
   - Khi FreeRTOS vào **Critical Section** (`taskENTER_CRITICAL()` hoặc `portSET_INTERRUPT_MASK_FROM_ISR()`) để bảo vệ các cấu trúc dữ liệu nội bộ của Kernel, nó không tắt toàn bộ ngắt CPU mà chỉ ghi `0x50` vào thanh ghi **`BASEPRI`**. Phần cứng Cortex-M3 chỉ che các ngắt có số ưu tiên `>= 5` (`0x50..0xF0`), giúp các ngắt cực kỳ quan trọng ở mức `0..4` không bao giờ bị trễ (zero-jitter interrupt) bởi RTOS.

3. **Điều gì có thể xảy ra nếu một ISR ưu tiên cao hơn ngưỡng này gọi API FreeRTOS (khi `configASSERT` bị tắt)?**
   - Nếu tắt `configASSERT`, một ISR ở mức `0..4` sẽ không bị chặn bởi `BASEPRI` nên có thể **cắt ngang (preempt) ngay giữa lúc một Task hoặc một ISR khác đang ở trong Critical Section** và đang sửa dở dang các cấu trúc dữ liệu cốt lõi của Kernel (như danh sách liên kết `pxReadyTasksLists`, `xPendingReadyList`, hay `TCB_t`).
   - Khi ISR mức `0..4` gọi API `...FromISR`, nó tiếp tục ghi đè lên các con trỏ danh sách liên kết đang ở trạng thái dở dang đó, gây **Race Condition làm hỏng cấu trúc dữ liệu của Kernel (Kernel Data Structure Corruption)**, dẫn đến lỗi `HardFault`, treo hệ thống ngẫu nhiên, hoặc mất Task khỏi bộ lập lịch (**Undefined Behavior**).

---

### MỤC 4 TRONG `REPORT.md`: Lệch so với dự đoán (Đối chiếu 3 chiều & Nguyên nhân vật lý)

Theo quy tắc trong [SKILL.md](file:///c:/Users/DELL/intern-261-rtos-course-easy/skill_rp/SKILL.md), Mục 4 phải đối chiếu rõ giữa **Dự đoán trong `PREDICTION.md`** ↔ **Kết quả thực đo ở Mục 2** ↔ **Thông số trong `params.h`**:

- **Đối chiếu**:
  - **Cấu hình có `portYIELD_FROM_ISR`**: Dự đoán `3.5 – 6.5 us` (Đạt so với `LATENCY_BUDGET_US = 50 us`). Thực đo đạt trung bình `~4.8 us` (dao động nhẹ từ Min đến Max khoảng `0.5 – 1.5 us`). Hoàn toàn khớp với dự đoán và đạt ngân sách `50 us`.
  - **Cấu hình không có `portYIELD_FROM_ISR`**: Dự đoán tăng thêm `0 – 1000 us` (trung bình `~500 us`, Không đạt). Thực đo dao động rộng từ vài chục `us` đến `~1000 us` (trung bình `~500 us`). Khớp đúng bản chất lý thuyết chờ chu kỳ `SysTick = 1000 us`.
- **Nguyên nhân vật lý giải thích sai lệch nhỏ và độ dao động (Jitter)**:
  1. **Trễ truy cập bộ nhớ Flash (Flash Wait-States)**: Ở tần số `72 MHz`, bộ nhớ Flash của STM32F103 cần cấu hình `FLASH_LATENCY_2` (2 chu kỳ chờ cho mỗi lần nạp lệnh không nằm sẵn trong Prefetch Buffer), làm số chu kỳ thực thi thực tế cao hơn khoảng 15–20% so với tính toán lý thuyết thuần túy trên tập lệnh (1 cycle/instruction).
  2. **Overhead của tầng thư viện HAL và kiểm tra `configASSERT`**: Đường đi thực tế phải qua `EXTI15_10_IRQHandler` $\rightarrow$ `HAL_GPIO_EXTI_IRQHandler` $\rightarrow$ `HAL_GPIO_EXTI_Callback` và hàm `vPortValidateInterruptPriority()` đọc các thanh ghi `IPSR`, `NVIC_IPR`, `AIRCR` trước khi thực sự cập nhật Task Notification.
  3. **Sự kiện ngắt `SysTick` trùng thời điểm hoặc vùng găng ngắn**: Ở cấu hình có `portYIELD_FROM_ISR`, đôi khi xung bấm nút trùng đúng lúc `SysTick_Handler` vừa vào vùng găng (`BASEPRI = 0x50`), làm `EXTI15_10_IRQn` (mức `6`) phải đợi vài micro-giây cho đến khi `SysTick` thoát vùng găng, tạo ra chênh lệch nhỏ giữa `Min` và `Max`.
  4. **Độ lệch pha ngẫu nhiên (Asynchronous Phase) và nhiễu dội cơ học (Contact Bounce) của nút nhấn**: Ở cấu hình không có `portYIELD_FROM_ISR`, thời điểm sườn xuống `PB11` xảy ra hoàn toàn bất đồng bộ với bộ đếm `SysTick` (`1 kHz`), tạo nên phân bố trễ trải rộng từ `0` đến `1000 us`.

---

### MỤC 5 TRONG `REPORT.md`: Câu hỏi đánh đổi — Tăng `ISR_NVIC_PRIO` từ `6` lên `7`

> **Đề bài**: *"Nếu bạn tăng `ISR_NVIC_PRIO` lên 1 bậc (ưu tiên thấp hơn) trong khi vẫn hợp lệ, latency đo được thay đổi bao nhiêu?"*

#### 1. Hãy thực nghiệm thật trên mạch:
- Trong `exti_button_init()`, tạm đổi `ISR_NVIC_PRIO` (`6`) thành `ISR_NVIC_PRIO + 1` (`7`, vẫn hợp lệ vì `5 <= 7 <= 15`).
- Đo lại Latency (có `portYIELD_FROM_ISR`) trên Logic Analyzer 10 lần.

#### 2. Output mong đợi & Giải thích bản chất vật lý để ghi vào Mục 5:
- **Kết quả đo thực tế**: Khi đổi ưu tiên của `EXTI15_10_IRQn` từ mức `6` sang mức `7`, latency đo được **gần như KHÔNG thay đổi** (chênh lệch trung bình $\Delta t \approx 0.0\text{ }\mu\text{s} \dots 0.1\text{ }\mu\text{s}$, nằm hoàn toàn trong sai số lấy mẫu của Logic Analyzer).
- **Giải thích nguyên nhân vật lý**:
  1. **Phần cứng NVIC có thời gian vào ngắt cố định (12 chu kỳ CPU)**: Con số ưu tiên NVIC (`6` hay `7`) chỉ là **bộ phân xử (arbiter)** để phần cứng quyết định ngắt nào được chạy trước khi có **nhiều ngắt xảy ra cùng lúc** hoặc để quyết định xem ngắt mới có được phép **lồng (preempt)** ngắt đang chạy hay không. Bản thân con số ưu tiên lớn hơn **không làm CPU chạy chậm đi** hay tốn thêm chu kỳ lệnh nào khi vào ngắt.
  2. **Không có ngắt ngoại vi nào khác ở mức `6` đang cạnh tranh trong bài tập này**: Trong hệ thống hiện tại chỉ có `EXTI15_10_IRQn` và `SysTick`/`PendSV` (ở mức `15` thấp nhất). Dù `EXTI15_10_IRQn` đặt ở mức `6` hay mức `7`, nó đều cao hơn mức `15` của `SysTick`/`PendSV` và không bị ngắt nào khác cản đường.
  3. **Khi nào mức `7` mới thực sự trễ hơn mức `6`?** Latency ở mức `7` sẽ chỉ tăng lên nếu trong hệ thống **có thêm một ngắt ngoại vi khác (ví dụ `TIM2_IRQn` hoặc `DMA1_IRQn`) được đặt ở mức `6`** và ngắt đó đang chạy đúng lúc nút nhấn `PB11` kích hoạt. Khi đó:
     - Nếu `EXTI15_10_IRQn` ở mức `7` (thấp hơn `6`): Nó không thể cắt ngang ISR mức `6` mà phải chờ ISR mức `6` chạy xong toàn bộ rồi mới được vào $\rightarrow$ Latency tăng thêm đúng bằng thời gian thực thi của ISR mức `6` đó.

---

# PHẦN 4: Tổng Hợp Những Gì Cần Nắm Sau Exercise Này (Key Takeaways & Viva Q&A)

## 4.1. Checklist 7 Kiến Thức Cốt Lõi Cần Thuộc Lòng

1. **Chiều ưu tiên ngược nhau giữa Phần cứng và Phần mềm**:
   - **Cortex-M3 NVIC (Hardware)**: Số càng **NHỎ** $\rightarrow$ ưu tiên càng **CAO** (`0` cao nhất, `15` thấp nhất).
   - **FreeRTOS Task (Software)**: Số càng **LỚN** $\rightarrow$ ưu tiên càng **CAO** (`0` thấp nhất — Idle Task).
   - **ISR luôn thắng Task**: Ngắt ở mức `15` (thấp nhất phần cứng) vẫn cắt ngang Task ở mức cao nhất phần mềm.
2. **Hai vùng ưu tiên ngắt của FreeRTOS**:
   - **Mức `0 .. 4`** (`< configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`): Không bao giờ bị chặn bởi `BASEPRI` trong Critical Section $\rightarrow$ **CẤM gọi mọi hàm FreeRTOS**.
   - **Mức `5 .. 15`** (`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY .. configLIBRARY_LOWEST_INTERRUPT_PRIORITY`): Bị chặn tạm thời bởi `BASEPRI` khi vào Critical Section $\rightarrow$ **ĐƯỢC PHÉP gọi các API kết thúc bằng `...FromISR`**.
3. **Bắt buộc `NVIC_PRIORITYGROUP_4` trên STM32**:
   - Phải phân bổ toàn bộ 4 bit ưu tiên cho Pre-emption Priority (0 bit Sub-priority). Nếu để mặc định khác hoặc quên gọi `HAL_NVIC_SetPriority` (khiến priority mặc định bằng `0`), `vPortValidateInterruptPriority()` sẽ nổ `configASSERT` ngay khi gọi API `FromISR`.
4. **Dịch bit thanh ghi ưu tiên Cortex-M3**:
   - STM32F103 có `configPRIO_BITS = 4` nằm ở 4 bit cao `[7:4]` của thanh ghi 8-bit $\rightarrow$ Giá trị thanh ghi phần cứng luôn bằng `prio << 4` (ví dụ `5` thành `0x50 = 80`, `15` thành `0xF0 = 240`).
5. **Quy tắc dùng API trong ISR**:
   - Chỉ gọi các hàm có hậu tố **`FromISR`** (ví dụ `vTaskNotifyGiveFromISR`, `xQueueSendFromISR`, `xSemaphoreGiveFromISR`).
   - Luôn khởi tạo `BaseType_t xHigherPriorityTaskWoken = pdFALSE;` ở đầu ISR và gọi `portYIELD_FROM_ISR(xHigherPriorityTaskWoken);` ở cuối ISR.
6. **Vai trò của `PendSV` và `portYIELD_FROM_ISR`**:
   - `portYIELD_FROM_ISR(pdTRUE)` không chuyển ngữ cảnh ngay lập tức giữa dòng lệnh trong ISR, mà chỉ bật bit `PENDSVSET` trong `SCB->ICSR`. Khi ISR kết thúc, phần cứng Cortex-M3 dùng cơ chế **Tail-Chaining** chuyển sang `PendSV_Handler` (mức `15`) để thực hiện Context Switch sang `ack_task` chỉ trong vài micro-giây.
   - Nếu quên `portYIELD_FROM_ISR`, `ack_task` dù đã `Ready` vẫn phải đợi đến kỳ ngắt `SysTick` kế tiếp (`0 .. 1000 us` ở `1 kHz`) mới được chạy.
7. **Sức mạnh của Task Notification (`vTaskNotifyGiveFromISR` + `ulTaskNotifyTake`)**:
   - Nhanh hơn ~45% và tốn `0 byte` RAM thêm so với Binary/Counting Semaphore vì dùng trực tiếp trường `ulNotifiedValue` có sẵn trong `TCB_t`.

---

## 4.2. Bộ Câu Hỏi Vấn Đáp (Viva / Defense Q&A) Thường Gặp

### Câu hỏi 1: Tại sao `SysTick` và `PendSV` luôn được FreeRTOS đặt ở mức ưu tiên phần cứng thấp nhất (`15` / `0xF0`)?
- **Trả lời**:
  - **`PendSV` ở mức 15**: Vì `PendSV` làm nhiệm vụ lưu và khôi phục ngữ cảnh của **Task** (Context Switch). Nếu `PendSV` có ưu tiên cao hơn một ISR ngoại vi nào đó, nó sẽ cắt ngang giữa chừng khi CPU đang chạy ISR và cố gắng lưu ngữ cảnh của ISR như thể đó là một Task $\rightarrow$ làm hỏng hoàn toàn Stack của hệ thống! Đặt `PendSV` ở mức thấp nhất đảm bảo việc chuyển ngữ cảnh Task **chỉ diễn ra khi tất cả các ISR ngoại vi đang lồng nhau đã kết thúc hoàn toàn**.
  - **`SysTick` ở mức 15**: Để ngắt định thời của hệ điều hành không làm trễ (jitter) các ngắt phần cứng ngoại vi của ứng dụng (như EXTI, UART, SPI, Timer). Tuy nhiên, khi `SysTick` chạy `xTaskIncrementTick()`, nó sẽ vào Critical Section nâng `BASEPRI` lên `0x50` trong vài micro-giây để cập nhật danh sách Task.

### Câu hỏi 2: Nếu trong `exti_button_init()`, em chỉ gọi `HAL_NVIC_EnableIRQ(EXTI15_10_IRQn)` mà QUÊN gọi `HAL_NVIC_SetPriority()`, chuyện gì sẽ xảy ra khi nhấn nút?
- **Trả lời**: Khi reset vi điều khiển, toàn bộ các thanh ghi ưu tiên `NVIC_IPR` của ARM Cortex-M3 đều có giá trị mặc định bằng **`0x00` (mức ưu tiên `0` — cao nhất hệ thống!)**. Do đó, nếu quên gọi `HAL_NVIC_SetPriority()`, đường ngắt `EXTI15_10_IRQn` sẽ chạy ở mức ưu tiên `0`. Ngay khi nhấn nút và gọi `vTaskNotifyGiveFromISR()`, hàm `vPortValidateInterruptPriority()` sẽ thấy `ucCurrentPriority = 0x00 < 0x50` và kích hoạt **`configASSERT`** treo máy ngay lập tức! Đây là lỗi phổ biến nhất khi lập trình FreeRTOS trên STM32.

### Câu hỏi 3: Tại sao trong `taskENTER_CRITICAL()`, FreeRTOS trên Cortex-M3 không dùng lệnh `CPSID I` (tắt cờ `PRIMASK`) mà lại ghi vào thanh ghi `BASEPRI`?
- **Trả lời**: Lệnh `CPSID I` (`PRIMASK = 1`) sẽ khóa **toàn bộ mọi ngắt có thể che** từ mức `0` đến mức `15`. Điều đó khiến các ngắt điều khiển thời gian thực cực kỳ nhạy cảm ở mức `0..4` cũng bị trễ theo vùng găng của RTOS. Ngược lại, thanh ghi `BASEPRI` cho phép **che ngắt có chọn lọc theo ngưỡng mức ưu tiên**: khi ghi `0x50` vào `BASEPRI`, chỉ các ngắt từ mức `5..15` (các ngắt có tương tác với API FreeRTOS) bị chặn lại, còn các ngắt mức `0..4` vẫn hoạt động bình thường với độ trễ bằng 0.

### Câu hỏi 4: Sự khác biệt giữa `ulTaskNotifyTake(pdTRUE, portMAX_DELAY)` và `ulTaskNotifyTake(pdFALSE, portMAX_DELAY)` là gì?
- **Trả lời**:
  - Với **`pdTRUE`** (`xClearCountOnExit = pdTRUE`): Khi `ack_task` thức dậy, giá trị đếm `ulNotifiedValue` của nó bị **xóa thẳng về `0`** (mô phỏng **Binary Semaphore**). Nếu có 5 ngắt xảy ra dồn dập trước khi `ack_task` kịp chạy, `ack_task` chỉ thức dậy xử lý 1 lần rồi ngủ tiếp.
  - Với **`pdFALSE`** (`xClearCountOnExit = pdFALSE`): Khi `ack_task` thức dậy, `ulNotifiedValue` chỉ **giảm đi 1 đơn vị (`ulNotifiedValue--`)** (mô phỏng **Counting Semaphore**). Nếu có 5 ngắt xảy ra dồn dập, `ack_task` sẽ lặp và xử lý đủ cả 5 lần trước khi quay lại trạng thái `Blocked` (đây chính là nội dung của Bài 2 — `week3/ex2`).

### Câu hỏi 5: Tại sao `vTaskNotifyGiveFromISR` lại không nhận giá trị thông báo cần gửi mà chỉ tăng biến đếm, và nếu có 2 nguồn ngắt khác nhau cùng gửi thông báo cho 1 Task thì có phân biệt được không?
- **Trả lời**: `vTaskNotifyGiveFromISR` được tối ưu hóa cho mô hình Semaphore sự kiện (tăng `ulNotifiedValue` lên 1). Nếu có nhiều nguồn ngắt khác nhau muốn gửi thông báo cho cùng 1 Task và cần phân biệt nguồn nào gửi, ta không dùng `vTaskNotifyGiveFromISR` mà dùng hàm tổng quát **`xTaskNotifyFromISR(xTaskToNotify, ulValue, eSetBits, &xHigherPriorityTaskWoken)`** (mô phỏng **Event Group** nhẹ, mỗi ISR bật 1 bit cờ riêng biệt trong `ulNotifiedValue`, và Task nhận bằng `xTaskNotifyWait()`).

---

## 4.3. Checklist Kiểm Tra Cuối Cùng Trước Khi Commit & Nộp Bài ([RUBRIC.md](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/RUBRIC.md))

- [ ] **Bước 1**: Đã điền [`PREDICTION.md`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/PREDICTION.md) và `git commit` **TRƯỚC** khi commit code trong `app/*` và `REPORT.md`.
- [ ] **Bước 2**: Chạy kiểm tra API cấm bằng lệnh:
      ```bash
      bash tools/check_api.sh week3/ex1
      ```
      Đảm bảo chỉ sử dụng FreeRTOS API nguyên bản (`task.h`, `queue.h`, `semphr.h`...).
- [ ] **Bước 3**: Trong [`app/FreeRTOSConfig.deltas`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/FreeRTOSConfig.deltas) đã điền đủ `configPRIO_BITS` (`4`), `configLIBRARY_LOWEST_INTERRUPT_PRIORITY` (`15`), `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` (`5`), và `configASSERT(x)`, không còn ký hiệu `<?>`.
- [ ] **Bước 4**: Trong [`app/exti_button.c`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/app/exti_button.c), giá trị nộp cuối cùng của `HAL_NVIC_SetPriority` đã đặt về đúng `ISR_NVIC_PRIO` (`6`) và dòng `portYIELD_FROM_ISR(xHigherPriorityTaskWoken);` đã được bật đầy đủ.
- [ ] **Bước 5**: Trong [`REPORT.md`](file:///c:/Users/DELL/intern-261-rtos-course-easy/week3/ex1/REPORT.md) đã điền đủ cả 5 mục, có bằng chứng `configASSERT` ở Mục 1, có bảng số đo Min/Mean/Max kèm ảnh chụp Logic Analyzer ở Mục 2, và không còn bất kỳ placeholder `<?>` nào.
