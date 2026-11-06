# Dự đoán — Week 3 / Ex2

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510343

1. Với nút nhấn cơ khí thông thường, nảy phím thường kéo dài bao lâu
   (tra cứu/ước lượng)? So với `DEBOUNCE_MS` của bạn: đủ lớn để lọc hết không?
   - Nảy phím thường kéo dài khoảng 1-5ms ( phụ thuộc vào nút nhấn mới hay cũ)
   - `DEBOUNCE_MS = 15ms > 5 ms` -> đủ để lọc hết.
2. Nếu `DEBOUNCE_MS` quá lớn, điều gì xảy ra khi người dùng nhấn nhả rất
   nhanh liên tiếp (double-click)?
   - Thời gian debounce quá lớn làm bỏ qua tín hiệu nhấn nút liền tiếp khi người dùng double click.
3. Khi callback của `debounce_timer` bị block 50 ms (thử nghiệm bước 4), bạn
   dự đoán `longpress_timer` sẽ bị trễ bao nhiêu? Giải thích bằng cơ chế
   daemon task + queue lệnh, không chỉ đoán số.
   - Bị trễ tối đa 50ms
   - Giải thích: Khi callback của debounce_timer bị block 50 ms, Tmr Svc chuyển sang trạng thái Blocked. Vì tất cả Software Timer dùng chung một daemon task nên longpress_timer, dù đã đến hạn, cũng không thể thực thi callback ngay lập tức. Nó phải chờ đến khi Tmr Svc được đánh thức và tiếp tục xử lý. Do đó, độ trễ của longpress_timer phụ thuộc vào thời điểm hết hạn trong khoảng block và có thể lên đến khoảng 50 ms, chưa tính overhead lập lịch và xử lý. Ngoài ra, các lệnh điều khiển timer được gửi vào Queue trong thời gian này cũng phải chờ daemon task xử lý.
4. `LONGPRESS_MS` của bạn là bao nhiêu? Giữ phím đúng ngưỡng này, bạn dự đoán
   callback `longpress_callback` chạy lần đầu ở thời điểm nào tính từ lúc
   nhấn xuống?
   - `LONGPRESS_MS = 1000ms'
   - Với LONGPRESS_MS = 1000 ms, callback longpress_callback() dự kiến chạy lần đầu khoảng 1000 ms sau cạnh nhấn xuống cuối cùng, tức khoảng 1000–1005 ms tính từ lúc nhấn vật lý nếu thời gian dội phím là 0–5 ms. Callback có thể chạy trễ hơn một chút do thời gian xử lý của Tmr Svc.
