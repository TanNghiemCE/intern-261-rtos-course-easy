# Dự đoán — Week 2 / Ex4

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510343    `EVENT_BIT_COUNT` = 3  `SYNC_TIMEOUT_MS` = 50

1. RAM event group chiếm (bytes) dự đoán: 24 bytes . Dựa vào đâu (cấu trúc dữ
   liệu nào trong FreeRTOS)? Dựa vào cấu trúc `EvenGroup_t` gồm `EventBits_t uxEventBits` (4 bytes trên vi điều khiển 32-bit) và danh sách liên kết chờ `List_t xTasksWaitingForBits` (20 bytes)
2. RAM thêm khi dùng task notification cho 1 điểm đồng bộ: dự đoán = 0
   bytes. Vì sao? Các trường lưu giá trị và trạng thái notification (`ulNotifiedValue`, `ucNotifyState`) đã nằm sẵn bên trong TCB (TaskControlBlock) được cấp phát khi tạo task.
3. Dự đoán thời gian đánh thức (producer gọi → sync_task chạy) của notification
   so với event-group-bit: nhanh hơn / chậm hơn / xấp xỉ? Vì sao (nghĩ đến số
   lớp hàm bên dưới mỗi API)?
   - Notification nhanh hơn là `xTaskNotifyGive` tác động trực tiếp lên con trỏ TCB của task nhận đã biết trước; trong khi `xEventGroupSetBits` phải thực hiện critical section dài hơn để duyệt qua danh sách liên kết `xTasksWaitingForBits` và kiểm tra điều kiện bitmask.
4. Nếu `sync_task` bị timeout (`SYNC_TIMEOUT_MS` hết hạn) vì một producer
   không set bit kịp, `xEventGroupWaitBits` trả về gì? Làm sao phân biệt được
   "đủ bit" với "timeout"? Trả về giá trị các bit hiện có tại thời điểm hết hạn timeout.Dùng phép kiểm tra bitmask: `(uxBits & ALL_BITS_MASK) == ALL_BITS_MASK`. Nếu bằng là đủ bit; nếu khác (kể cả khi uxBits > 0) là timeout.
5. Nêu trước một tình huống bạn nghĩ **không thể** thay toàn bộ event group
   bằng notification — sẽ kiểm chứng lại ở REPORT.md.
   Phát sự kiện quảng bá (Broadcasting / 1-to-N) — một nguồn sự kiện kích hoạt nhiều task cùng lúc;
