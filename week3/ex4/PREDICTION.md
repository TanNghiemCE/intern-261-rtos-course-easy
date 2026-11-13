# Dự đoán — Week 3 / Ex4

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510343

1. Chạy `app/deadlock_demo.c` y nguyên như được cấp. Dự đoán: hệ thống sẽ
   treo sau khoảng bao lâu (ngay lập tức, hay sau vài chu kỳ)? Vì sao không
   phải ngay lập tức (gợi ý: cần cả 2 task "đua" vào đúng thời điểm)?
   - Deadlock có thể hình thành ngay vòng đầu, khoảng thời gian delay 5 ms cộng lệch khởi chạy,scheduling jitter
2. Khi treo, `uxTaskGetSystemState` sẽ báo trạng thái gì cho `task_alpha` và
   `task_beta`? Dự đoán tên mutex mỗi task đang chờ (dựa trên đọc code, chưa
   chạy công cụ).
   - Dự đoán state: `eBlocked`
   - Tên mutex của alpha đang chờ là `s_mutex_b` , beta đang chờ là `s_mutex_a`
3. Tính reload value cho IWDG với `IWDG_TIMEOUT_MS` của bạn (LSI ~40 kHz).
- Công thức: T_iwdg(ms) ≈ (RLR + 1) × P × 1000 / f_LSI
- Nên : RLR ≈ IWDG_TIMEOUT_MS × f_LSI / (1000 × P) - 1
    = 250 × 40 000 / (1000 × 4) - 1  = 2499
4. Sau khi sửa lỗi deadlock và thêm supervisor + IWDG, nếu bạn cố tình tái
   tạo lại tình huống deadlock cũ (tạm thời), dự đoán board sẽ phản ứng thế
   nào và sau bao lâu.
- Sau khi alpha/beta kẹt, các task đó không hoàn thành vòng và không phát heartbeat mới. Sau khi hết các bit còn lưu, supervisor không gom đủ mask 0b111, nên ngừng refresh. IWDG tiếp tục đếm và reset board.
- Dự đoán: 300–500 ms kể từ lúc treo