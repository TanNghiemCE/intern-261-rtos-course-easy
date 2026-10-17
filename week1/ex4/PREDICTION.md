# Dự đoán — Week 1 / Ex4

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2510343    `HEAP_SIZE_BYTES` = 8192    `DYN_TASK_STACK_WORDS` = 128
`STATIC_TASK_STACK_WORDS` = 96    `HOG_DEPTH_BYTES` = 384    `TARGET_FREE_HE AP_BYTES` = 2048

1. Ngân sách RAM dự đoán (tổng phải = 20480 bytes):

| Mục | Bytes dự đoán |
|---|---|
| `.data` | 16 |
| `.bss` (không kể heap FreeRTOS) |2400 |
| Heap FreeRTOS (`ucHeap`) | 8192 |
| Stack của main (MSP) | 1024  |
| Còn lại / chưa dùng | 8848 |
| **Tổng** | **20480** |

2. `xPortGetFreeHeapSize()` ngay sau khi scheduler chạy: 6670 bytes.
   `HEAP_SIZE_BYTES` − số này = 1522 . Số đó là của những gì? gồm: `dyn-task`,`ram_report_task`, block header 
3. High water mark dự đoán (words): dyn 80 , stat 48 , idle 100 , timer 85 .
4. `dyn_task` có stack 128 words = 512 bytes. Mảng `HOG_DEPTH_BYTES` = 384 bytes.
   Hook tràn stack **có** chạy không? Vì sao? 
   - Có chạy vì đã thiết lập cơ chế `configCHECK_FOR_STACK_OVERFLOW = 2`
5. Heap trong `.map` và `xPortGetFreeHeapSize()` sẽ lệch nhau. Lệch vì: `.map` là dung lượng heap do linker đặt ra, còn `xPortGetFreeHeapSize()` là heap còn trống, do dung lượng heap sẽ tiêu tốn do khởi tạo task nên 2 cái không thể bằng nhau
6. Stack nhỏ nhất an toàn của `dyn_task` dự đoán: 128 words. Dựa vào đâu?
- Stack nhỏ nhất = Mức tiêu thụ đỉnh + Biến an toàn  = 384/4 + 32 = 128
