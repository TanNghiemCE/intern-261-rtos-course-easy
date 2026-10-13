# Report — Week 1 / Ex1

`student_id`: 2510343    Commit của PREDICTION.md: 11/10

## 1. Số đo
| Đại lượng | Giá trị |
|---|---|
| `BLINK_PERIOD_MS` (từ params.h) | 83ms |
| Chu kỳ đo được — min | 81.992 ms |
| Chu kỳ đo được — max | 81.992ms |
| Chu kỳ đo được — trung bình | 81.9927 (11 chu kỳ)ms |
| Sai số trung bình vs `BLINK_PERIOD_MS` | -1.0073ms |
| `uxTaskGetStackHighWaterMark()` của bringup_task | 103 words (~ 10s chạy debug) |
*GHI CHÚ LẤY MẪU*: Lấy 2M mẫu ở tần số 1Mhz ( chu kì 1us)
Ảnh PulseView (>= 10 chu kỳ, đọc được giá trị): `![PulseView chân PA0, 11 chu kỳ liên tiếp](do_ex1.jpg)`

## 2. Ba handler của kernel
| Tên trong port ARM_CM3 | Tên trong vector table | Nhiệm vụ |
|---|---|---|
| `vPortSVCHandler` |`SVC_Handler` | khởi tạo context ban đầu của task  |
| `xPortPendSVHandler` | `PendSV_Handler` | chuyển đổi context giữa các task, lưu context của task hiện tại và khôi phục context của task tiếp theo |
| `xPortSysTickHandler` | `SysTick_Handler` | tạo nhịp tick hệ thống |

Log lỗi link trước khi sửa (dán nguyên văn):

```
c:/progra~2/armgnu~1/12977f~1.2mp/bin/../lib/gcc/arm-none-eabi/12.2.1/../../../../arm-none-eabi/bin/ld.exe: CMakeFiles/Week1.dir/third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c.obj: in function `SVC_Handler':
C:/Users/DELL/Intern_MLIoT/Week1/third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c:230: multiple definition of `SVC_Handler'; CMakeFiles/Week1.dir/Core/Src/stm32f1xx_it.c.obj:C:/Users/DELL/Intern_MLIoT/Week1/Core/Src/stm32f1xx_it.c:146: first defined here
c:/progra~2/armgnu~1/12977f~1.2mp/bin/../lib/gcc/arm-none-eabi/12.2.1/../../../../arm-none-eabi/bin/ld.exe: CMakeFiles/Week1.dir/third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c.obj: in function `PendSV_Handler':
C:/Users/DELL/Intern_MLIoT/Week1/third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c:459: multiple definition of `PendSV_Handler'; CMakeFiles/Week1.dir/Core/Src/stm32f1xx_it.c.obj:C:/Users/DELL/Intern_MLIoT/Week1/Core/Src/stm32f1xx_it.c:172: first defined here
c:/progra~2/armgnu~1/12977f~1.2mp/bin/../lib/gcc/arm-none-eabi/12.2.1/../../../../arm-none-eabi/bin/ld.exe: CMakeFiles/Week1.dir/third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c.obj: in function `SysTick_Handler':
C:/Users/DELL/Intern_MLIoT/Week1/third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c:492: multiple definition of `SysTick_Handler'; CMakeFiles/Week1.dir/Core/Src/stm32f1xx_it.c.obj:C:/Users/DELL/Intern_MLIoT/Week1/Core/Src/stm32f1xx_it.c:185: first defined here
```

Cách sửa: xóa hoặc comment 3 handler SVC_Handler, PendSV_Handler, Systick_Handler trong file stm32f1xx_it.c

## 3. Tại sao HAL timebase phải sang TIM4
Phải trả lời được: `HAL_IncTick()` chạy ở đâu, `configKERNEL_INTERRUPT_PRIORITY`
ảnh hưởng gì đến nó, và `HAL_Delay()` / `vTaskDelay()` khác nhau chỗ nào.


HAL_IncTick() tăng biến đếm thời gian của HAL, được sử dụng bới các hàm HAL_GetTick() hay HAL_Delay(). Khi dùng RTOS, Systick dùng để đếm tick thực hiện task, nên HAL_timebase phải chuyển sang TIM4 nhằm tách biệt đếm thời gian giữa HAL và RTOS. `configKERNEL_INTERRUPT_PRIORITY` cấu hình mức ưu tiên ngắt liên quan đến kernel. Task RTOS có ưu tiên ngắt thấp nhất, nếu dùng chung ngắt Systick với HAL thì khi có ngắt ưu tiên cao hơn,chip sẽ bị treo. HAL_Delay() chiếm dụng CPU, CPU không thể làm gì khác.  vTaskDelay() blocked task tạm thời để làm việc khác
## 4. Lệch so với dự đoán
Với mỗi mục trong `PREDICTION.md` lệch so đo: lệch bao nhiêu, và **nguyên nhân vật lý**
(không được trả lời "do sai số").
Chu kỳ led: dự đoán 83ms, thực tế: 81.9927ms ( lệch -1.0073ms); Sai số: đự đoán 3-5us, thực tế: 1007us ( lệch 1002-1004us). Nguyên nhân vật lý: dự đoán chỉ xét thạch anh nhưng bỏ quan độ phân giải của vTaskDelay: 1 tick = 1ms. Phép chia nguyên 83/2 = 41, mất 0.5 mỗi nữa chu kì, 1 chu kì mất 1ms = 1tick, gấp nhiều lần so với thạch anh. 
## 5. Câu hỏi đánh đổi
`configTICK_RATE_HZ` = 1000 trong bài này. Dựa vào số đo của **bạn**, nếu hạ xuống
100 Hz thì chu kỳ LED và sai số thay đổi thế nào? Trả lời bằng con số, không bằng lý thuyết.


100Hz ->1/100 = 10ms -> mỗi tick 10ms,

- `pdMS_TO_TICKS(83/2)` = `41*100/1000` = 4 ticks
- 4 ticks -> 40ms, 1 chu kỳ LED = 80ms
- Sai số so với 83ms: 80 - 83 = -3ms -> sai số tăng gấp 3 lần
