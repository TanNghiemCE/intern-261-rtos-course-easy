# Dự đoán — Week 3 / Ex3

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510343

1. Thời gian truyền 1 byte ở `RX_BAUD` của bạn là bao nhiêu us? (8N1, tính cả
   start/stop bit). Trong thời gian đó `parser_task` có kịp xử lý 1 byte từ
   stream buffer không, nếu nó là task ưu tiên cao nhất hệ thống?
   - Thời gian truyền 1 byte = 86.8us ,  Có, dư nhiều: parser ưu tiên cao nhất được chạy ngay sau ISR (nhờ portYIELD_FROM_ISR), xử lý xong trong vài chục µs < 86.8 µs
2. `STREAM_BUFFER_BYTES` của bạn chứa tối đa bao nhiêu lệnh dài `MAX_CMD_LEN`
   byte trước khi đầy? 96/16 = 6 lệnh dài
3. Nếu `xStreamBufferSendFromISR` trả về 0 (stream buffer đầy) trong
   `uart_rx_isr_handler`, byte đó sẽ bị mất. Theo bạn, trường hợp nào thực tế
   khiến điều này xảy ra (gửi quá nhanh / `parser_task` ưu tiên quá thấp /
   cái khác)? gửi quá nhanh
4. Dự đoán: gửi lệnh bằng gõ tay bình thường có mất byte không? Gửi bằng
   script gửi liên tục không nghỉ thì sao?
- Gõ tay tấm 5 đến 10 ký tự trên giây, 1 giây uart truyền 11520B nên không mất
- Script liên tục tốc độ cao thì sẽ mất