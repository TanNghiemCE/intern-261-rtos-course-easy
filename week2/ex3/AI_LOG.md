# AI_LOG.md — Week 2 / Ex3

> Bắt buộc: ghi lại MỌI prompt đã dùng với AI khi làm bài này. Thiếu file
> này hoặc không tự giải thích được code mình nộp khi vấn đáp = phần vấn
> đáp của bài = 0 (xem SYLLABUS.md "Điểm liệt").

| # | Prompt (tóm tắt) | AI dùng | Mình giữ lại gì / sửa gì |
|---|---|---|---|
| 1 | Cú pháp và cách khởi tạo `s_resource` bằng Binary Semaphore vs Mutex trong `resource_init()` | Gemini 3.8 Flash | Nắm bản chất: Binary Semaphore khởi tạo rỗng (count = 0) nên phải `xSemaphoreGive` 1 lần trước để available; Mutex khởi tạo có sẵn count = 1 và hỗ trợ Priority Inheritance. Phát hiện và sửa lỗi nghiêm trọng `configASSERT(s_resource == pdTRUE)` thành `configASSERT(s_resource != NULL)`. |
| 2 | Hướng dẫn cấu trúc `task_low` (TODO 3) và `task_mid` (TODO 4) | Gemini 3.8 Flash | Phát hiện lỗi lệch đơn vị 1000 lần: `LOW_HOLD_MS` và `MID_WORK_MS` tính bằng ms, hàm `dwt_busy_wait_us` nhận us nên phải nhân `1000UL`. Hiểu lý do `task_mid` tuyệt đối không được đụng vào `s_resource` để đóng vai trò kẻ chen ngang cướp CPU từ `task_low`. Gán chân riêng: `task_low` (PA0), `task_mid` (PA4). |
| 3 | Sửa lỗi logic đo đạc ở `task_high` và hoàn thiện `app_main.c` | Gemini 3.8 Flash | Kéo chân PA6 lên HIGH trước khi gọi `xSemaphoreTake` để đánh dấu thời điểm $t_0$; sửa lỗi chạy bận 50 ms thành công việc ngắn (1000 us). Trong `app_main.c`: dùng `xTaskCreate` tạo 3 task với ưu tiên tách biệt (1 < 2 < 3), assert kiểm tra `pdTRUE`, gọi `dwt_init()` trước khi start scheduler. |
| 4 | Giải thích tại sao thực tế đo thời gian block ở Binary Semaphore ra 17 ms / 27 ms thay vì 42 ms lý thuyết | Gemini 4 Argon | Hiểu bản chất vật lý: con số 42 ms là worst-case. Thực tế có độ lệch pha giữa các task: `task_low` đã chạy trước 5 ms trong lúc `task_mid` ngủ; thanh ghi phần cứng `DWT->CYCCNT` đếm liên tục theo thời gian thực (wall-clock) ngay cả khi bị preempt. Khi `task_high` thức dậy ở $t_0$, thời gian chờ là tổng phần còn lại của `task_mid` và `task_low`. |
| 5 | Chẩn đoán lỗi nạp Mode 1 (Mutex) nhưng PulseView không hiện tín hiệu | Gemini 4 Argon | Rà soát cấu hình `#define configUSE_MUTEXES 1` trong `FreeRTOSConfig.h`, kiểm tra tắt Trigger (None) trên các kênh PulseView và bấm nút RESET vật lý trên kit STM32 để chip thoát trạng thái Halt sau khi nạp. |
