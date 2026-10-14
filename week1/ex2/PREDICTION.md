# Dự đoán — Week 1 / Ex2

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510343     Ưu tiên của tôi: A=3 B=1 C=4

1. Bước 2 (`configUSE_PREEMPTION`=1, ưu tiên khác nhau): task nào chạy? task nào
   **không bao giờ** chạy? 
- Task C có độ ưu tiên cao nhất, sẽ chạy trước. Các task A và B không bao giờ chạy

- CPU share dự đoán:
   | Task | CPU share dự đoán (%) |
   |---|---|
   | A | 0 |
   | B | 0 |
   | C | 100 |

2. Bước 3 (`configUSE_PREEMPTION`=0): dự đoán thay đổi gì so với bước 2? Vì sao? 
- `configUSE_PREEMPTION`= 0 vẫn không có gì thay đổi, mặc dù các task ưu tiên cao không thể cướp quyền, tuy nhiên do taskC ( có ưu tiên cao nhất) đang trong vòng lặp vô tận nên không chịu nhường cho các task khác.
3. Bước 4 (3 task cùng ưu tiên): time slice dự đoán = 1 ms. Dựa vào đâu?
- Dựa vào `configTICK_RATE_HZ = 1000` tương ứng `time_slice = 1/1000 = 1ms`.  
4. Bước 5: tôi sẽ sửa starvation bằng cách: thêm lệnh `vTaskDelay()` để đưa task vào `BLOCKED` . Cách này đánh đổi gì?
- Đánh đổi: Task C không còn chạy liên tục tối đa công suất mà bắt buộc phải dừng định kì nhường CPU cho task khác, tăng độ trễ phản hồi khi có công việc phát sinh trong thời gian bị `BLOCKED`. Tốn thêm chi phí chuyển ngữ cảnh, CPU tốn chu kỳ lưu/khôi phục ngữ cảnh khi chuyển qua lại giữa các task
5. Chân nào sẽ không bao giờ đổi trạng thái trên logic analyzer ở bước 2? `PA0,PA4`
