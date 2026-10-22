# AI_LOG.md — Week 2 / Ex2

> Bắt buộc: ghi lại MỌI prompt đã dùng với AI khi làm bài này. Thiếu file
> này hoặc không tự giải thích được code mình nộp khi vấn đáp = phần vấn
> đáp của bài = 0 (xem SYLLABUS.md "Điểm liệt").

| # | Prompt (tóm tắt) | AI dùng | Mình giữ lại gì / sửa gì |
|---|---|---|---|
| 1 | Dự đoán lỗi lẫn lộn ký tự và so sánh 3 cơ chế bảo vệ UART (Mutex, Critical Section, Gatekeeper) | Gemini 3.8 Flash | Tính toán thời gian truyền 24 byte ở 115200 baud là $\approx 2.08\text{ ms} > 1\text{ ms}$ (SysTick). Hiểu cơ chế Round-Robin cắt ngang task đang in dở. Dự đoán độ trễ, ảnh hưởng tắt ngắt và hoàn thành `PREDICTION.md`. |
| 2 | Tái hiện lỗi race condition ở Mode 0 bằng code trực tiếp vào thanh ghi UART | Gemini 3.8 Flash | Bỏ qua cơ chế khóa phần mềm `gState` của HAL, ghi trực tiếp vào `USART1->DR` để tái hiện lỗi lẫn lộn byte thực tế trên phần cứng. Thu được file log `log_uart_mode0.log` với 100% các chu kỳ bị băm nát. |
| 3 | Hiện thực 3 cơ chế bảo vệ trong `uart_guard.c` (Mutex, Critical section, Gatekeeper task) | Gemini 3.8 Flash | Dùng API gốc FreeRTOS: `xSemaphoreCreateMutex`/`Take`/`Give`, `taskENTER_CRITICAL`/`EXIT_CRITICAL`, `xQueueCreate`/`Send`/`Receive`. Thu được các log sạch `log_uart_mode1.log`, `log_uart_mode2.log`, `log_uart_mode3.log`. |
| 4 | Tích hợp đo độ trễ bằng DWT Cycle Counter và xuất báo cáo độ trễ tự động | Gemini 3.8 Flash | Kích hoạt DWT ở 72 MHz (72 cycles = 1 µs). Bấm giờ trước và sau khi gọi `uart_guard_send()`. Dùng phép trừ unsigned để chống tràn thanh ghi 32-bit. |
| 5 | Tách biệt pha đo độ trễ và pha in báo cáo, khắc phục lỗi treo terminal PC do tràn buffer UART | Gemini 3.8 Flash | Khi `ENABLE_LATENCY_REPORT = 1`, không in các chuỗi của 4 task ra UART mà thực thi tải 2083 µs bằng DWT ngầm; sau khi đủ 100 mẫu/task thì dừng toàn bộ writer và chỉ xuất duy nhất bảng báo cáo ra terminal. Khắc phục dứt điểm lỗi `HAL_BUSY` và tràn buffer. |
| 6 | Tổng hợp dữ liệu thực nghiệm từ `latency_report(4).log` và hoàn thiện Mục 1 đến 4 của `REPORT.md` | Gemini 3.8 Flash | Điền số đo thật N=4: Mode 1 (Avg 5744 µs, Max 7300 µs), Mode 2 (Avg 5329 µs, Max 8530 µs), Mode 3 (Avg 5436 µs, Max 8697 µs). Phân tích chi tiết cơ chế tắt ngắt 2.14 ms làm mất 2 tick SysTick và nguy cơ treo HAL timeout. |
| 7 | Tăng WRITER_COUNT lên 8, debug tràn heap/stack, đo kiểm chứng cả 3 mode (`latency_report(8).log`) và giải quyết Mục 5 câu hỏi đánh đổi | Gemini 3.8 Flash | Tối ưu hóa stack depth (128 words), bộ đệm trực tiếp không tốn stack. Đo thực tế N=8: Mode 1 Avg 13704 µs / Max 26898 µs, Mode 2 Avg 9478 µs / Max 16842 µs, Mode 3 Avg 9688 µs / Max 17172 µs. Chứng minh Mutex bị thoái hóa nặng nhất (+138.6% Avg, +268.5% Max). |

