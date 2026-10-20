# Dự đoán — Week 2 / Ex2

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510343    `WRITER_COUNT` = 4   `MSG_LEN` = 24

1. Ở `UART_GUARD_MODE = 0`: dự đoán lỗi lẫn lộn sẽ xuất hiện sau khoảng bao
   nhiêu dòng log? Vì sao MSG_LEN và WRITER_PERIOD_MS của bạn ảnh hưởng đến
   con số này?  
   - Xuất hiện trong 1-3 dòng log đầu tiên
   - Một bản tin mất 24 × 10 / 115200 ≈ 2,08 ms để truyền xong.
   - Tick = 1 ms, các writer cùng priority nên round-robin: writer đang truyền
     bị cắt sau ≤ 1 ms (khoảng 11 byte), writer kế tiếp chen vào ngay.
   - `MSG_LEN` ảnh hưởng vì nó quyết định thời gian truyền (2,08 ms > 1 ms tick, `MSG_LEN` nhỏ hơn ~11 byte thì gần như không bị cắt). `WRITER_PERIOD_MS = 3 ms < 4 × 2,08 ms` nên luôn có writer khác sẵn sàng chạy ngay lúc bị cắt,lỗi gần như chắc chắn xảy ra sớm.
2. Dự đoán thứ tự **độ trễ in** (latency) từ thấp đến cao giữa mutex / critical
   section / gatekeeper. Vì sao?
   - Thấp tới cao:  critical section < mutex < gatekeeper.
    - Critical section: không có chi phí đổi ngữ cảnh/mutex, writer chạy một mạch
     đến hết bản tin.
   - Mutex: thêm chi phí take/give và context switch, writer phải chờ người
     đang giữ mutex.
   - Gatekeeper: thêm copy 24 byte vào queue và một lần đổi ngữ cảnh sang
     gatekeeper; độ trễ đầu-cuối gồm cả thời gian nằm trong queue.
   - Nếu chỉ đo thời gian writer bị chặn (đến khi xQueueSend trả về) thì
     gatekeeper lại thấp nhất, miễn là queue chưa đầy. 
3. Với critical section, task nào trong hệ thống có nguy cơ bị ảnh hưởng xấu
   nhất nếu đoạn in quá dài? (gợi ý: nghĩ đến ngắt, không chỉ task)
Task/ISR bị ảnh hưởng nhất: các ngắt (và task phụ thuộc vào chúng).
   - Critical section 2,08 ms/lần, 4 writer nối nhau gần như tắt ngắt liên tục.
   - Ngắt SysTick (tick của FreeRTOS, 1 ms) và TIM4 (HAL tick) bị chặn > 1 tick nên mất tick: vTaskDelay bị kéo dài, đồng hồ hệ thống chạy chậm.
   - Mọi ISR có ưu tiên cao nhất (ví dụ EXTI/UART RX theo ISR_NVIC_PRIO) bị trễ tới ~2 ms. Chỉ ISR ưu tiên cao hơn ngưỡng đó mới còn chạy được.
   - Task bị hại nhất: task chu kỳ ngắn/có deadline.
4. RAM dùng thêm của gatekeeper so với mutex: dự đoán tăng hay giảm, vì sao? Tăng
- Mutex: chỉ 1 đối tượng queue-based.
- Gatekeeper: queue struct  + vùng chứa length × 24 B + TCB của gatekeeper task + stack của nó (stack còn chứa msg[24]).
5. Nếu `WRITER_COUNT` tăng gấp đôi, cách nào trong 3 cách bị ảnh hưởng về độ trễ nhiều nhất? Vì sao?
WRITER_COUNT gấp đôi (8): độ trễ tăng ~tuyến tính ở cả 3 cách vì UART là
   nút cổ chai (8 × 2,08 ≈ 16,6 ms trong trường hợp xấu nhất).
   - Dự đoán mutex tăng nhiều nhất (jitter lớn nhất), vì có thêm nhiều
     task block/wake và context switch cho mỗi lần giữ mutex.
   - Gatekeeper phụ thuộc độ dài queue: nếu length < 8 thì writer cũng bị chặn
     ở xQueueSend, giống mutex.

 