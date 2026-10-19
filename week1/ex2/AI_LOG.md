# AI_LOG.md — Week 1 / Ex2

> Bắt buộc: ghi lại MỌI prompt đã dùng với AI khi làm bài này. Thiếu file
> này hoặc không tự giải thích được code mình nộp khi vấn đáp = phần vấn
> đáp của bài = 0 (xem SYLLABUS.md "Điểm liệt").

| # | Prompt (tóm tắt) | AI dùng | Mình giữ lại gì / sửa gì |
|---|---|---|---|
| 1 | Ở ex2, task B và A có bao giờ chạy không và tại sao? | Gemini 3.8 Flash | Hiểu nguyên lý lập lịch ưu tiên Fixed-Priority Preemptive; nắm được hiện tượng Starvation khi task ưu tiên cao nhất chạy vòng lặp bận không block. Dùng để điền dự đoán câu 1 trong `PREDICTION.md`. |
| 2 | Task C busy-loop 240us tại sao chạy mãi? Nếu `configUSE_PREEMPTION = 0` thì có nhường không? | Gemini 3.8 Flash | Hiểu rõ task trong RTOS nằm trong vòng lặp `for (;;)` vô tận. Ở chế độ Cooperative (`preempt = 0`), task chỉ nhường khi tự gọi `taskYIELD()` hoặc bị `BLOCKED`. Điền câu 2 trong `PREDICTION.md`. |
| 3 | Bước 4 (cùng ưu tiên): time slice dự đoán bằng bao nhiêu ms và dựa vào đâu? | Gemini 3.8 Flash | Hiểu mối liên hệ giữa time slice và chu kỳ ngắt SysTick ($T = 1/\text{configTICK_RATE_HZ} = 1\text{ ms}$). Điền câu 3 trong `PREDICTION.md`. |
| 4 | Bước 5 sửa starvation thế nào, đánh đổi gì? Chân nào không đổi trạng thái ở bước 2? | Claude Opus 4.6 | Chọn giải pháp dùng `vTaskDelay()` để đưa task vào `BLOCKED`; xác định chân PA0 và PA4 không đổi mức logic. Điền câu 4 và 5 trong `PREDICTION.md`. |
| 5 | Kiểm tra lại toàn bộ nội dung file `PREDICTION.md` trước khi commit | Claude Opus 4.6 | Nhận diện và sửa 2 lỗi: sửa bảng CPU share bị ngược (A=0%, C=100%) và sửa nhầm tên chân PA6 thành PA4. Commit file trước khi nạp chip. |
| 6 | Có 2 file `app_main.c`, làm sao để cấu hình project chạy file của ex2? | Gemini 3.8 Flash | Biết cách cấu hình lại `target_sources` và `target_include_directories` trong `CMakeLists.txt` để biên dịch 3 file nguồn của ex2 (`app_main.c`, `tasks.c`, `dwt.c`). |
| 7 | Kiểm tra lại toàn bộ code ex2 xem nạp được chưa | Claude Opus 4.6 | Xóa bỏ dòng `#include <cstdint>` (header C++ không hợp lệ) trong file `dwt.c` để biên dịch C chuẩn xác. |
| 8 | Hướng dẫn cách chụp ảnh PulseView (chụp tổng quát hay zoom cụ thể) | Claude Opus 4.6 | Nắm quy chuẩn chụp ảnh: B2, B3, B5 chụp tổng quát 3 kênh để chứng minh trạng thái; B4 chụp zoom sát và dùng 2 cursor đo time slice ~1ms. |
| 9 | Đề xuất cách sửa starvation ít xâm lấn nhất; tại sao không xóa DWT thay bằng `vTaskDelay`? | Gemini 3.8 Flash | Hiểu sự khác biệt cốt lõi: DWT mô phỏng thời gian làm việc thực tế (CPU bận), còn `vTaskDelay` là thời gian nghỉ (CPU nhàn rỗi). Quyết định giữ nguyên DWT và thêm `vTaskDelay` vào task C và A. |
| 10 | Giải thích tại sao giữa Task C và A không có khoảng trống, nhưng giữa A và B lại có khoảng trống 400us | Gemini 3.8 Flash | Hiểu sâu cơ chế tick: tổng thời gian 3 task $1020\,\mu\text{s} > 1000\,\mu\text{s}$ làm tràn sang Tick 1, khiến Task B thức dậy ở Tick 6 ($6000\,\mu\text{s}$), tạo khoảng nghỉ vật lý đúng $400\,\mu\text{s}$ sau khi A kết thúc lúc $5600\,\mu\text{s}$. |
| 11 | Hướng dẫn viết các mục phân tích 2, 3, 5, 6, 7 trong `REPORT.md` và gắn link ảnh đo | Gemini 3.8 Flash | Tự đo đạc thực tế trên PulseView lấy số liệu (A=33.301%, B=33.307%, C=33.28%, time slice 0.998ms), đối chiếu giải thích nguyên nhân vật lý (ngắt phần cứng, dung sai thạch anh, overhead context switch) và điền câu hỏi đánh đổi. |

