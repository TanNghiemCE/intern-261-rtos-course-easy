# Dự đoán — Week 2 / Ex3

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510343    `LOW_HOLD_MS` = 12  `MID_WORK_MS` = 30
`HIGH_PERIOD_MS` = 50  `DEADLINE_MS` = 15

1. Cấu hình binary semaphore: thời gian `task_high` bị block dự đoán = 42 ms.
   Phép tính dựa trên đâu (liệt kê các thành phần cộng lại)?  task_high  = `LOW_HOLD_MS`+`MID_WORK_MS` = 42ms
2. `task_high` có đạt `DEADLINE_MS` ở cấu hình binary semaphore không? Vì sao? không đạt vì thời gian chờ 42ms>DEADLINE_MS(15ms)
3. Cấu hình mutex: thời gian `task_high` bị block dự đoán = 12 ms. Vì sao
   ngắn hơn (hoặc không) so với câu 1? ngắn hơn là do mutex dùng cơ chế Priority Inheritance, FreeRTOS kernel nâng mức ưu tiên của task_low lên ngang hàng với `task_high` (mức cao nhất - priority 3) để tránh bị `task_mid` chiếm quyền cpu, khiến `task_high` chờ 42ms
4. Trên ảnh logic analyzer ở cấu hình binary semaphore, kênh `task_mid` sẽ ở
   trạng thái nào trong lúc `task_high` bị chặn — chạy liên tục, chạy ngắt
   quãng, hay không chạy? Vì sao? Chạy liên tục, vì `task_mid` ở trạng thái ready, ưu tiên cao hơn `task_low` nên scheduler ưu tiên chạy trước `task_low`
5. Nếu đổi `task_mid` sang ưu tiên **thấp hơn** `task_low`, inversion còn xảy ra
   không? Vì sao (đây là cách phân biệt priority inversion với việc chỉ đơn
   giản là "task_low chạy lâu").
   Inversion không còn xảy ra, trễ 42 ms của `task_high` không phải do task_low chạy lâu, mà do sự xuất hiện và chen ngang của `task_mid`.

