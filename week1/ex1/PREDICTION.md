# Dự đoán — Week 1 / Ex1

> **Commit file này TRƯỚC khi nạp chương trình lần đầu.** Report sẽ phải giải thích
> mọi chỗ dự đoán lệch so đo. Dự đoán sai không bị trừ điểm; dự đoán sau khi đo thì bị.

`student_id`: 2510343    Ngày/giờ commit: 11/10 - 20h35

1. Chu kỳ LED tôi sẽ đo được: 83 ms (`BLINK_PERIOD_MS` = 83)
2. Sai số giữa chu kỳ đo được và `BLINK_PERIOD_MS`: 3 - 5 us. Vì sao: độ lệch của thạch anh ( tối đa 50ppm), 83ms (1 tick = 1ms) -> đếm tới 83 ticks. Lấy 83ms*50ppm = 4.15us. Ngoài ra còn có thể do thời gian thực thi mã lệnh, ở tần số 72Mhz, 1 chu kỳ mất (1/fclk) = 13.88ns, vài chục lệnh thì mất vài us.
3. Nếu để HAL timebase ở lại SysTick, điều gì sẽ xảy ra khi `vTaskDelay()` được gọi?
   (chọn và giải thích: chạy bình thường / treo / tick sai nhịp / hard fault) -> Treo
   Lý do : nếu HAL timebase dùng SysTick, handler không cập nhật tick của FreeRTOS, task vào blocked, bị treo vì kernel không nhận được tick.
4. `BRINGUP_STACK_WORDS` = 128 words = 512 bytes. Task này có đủ không? Vì sao?: Task này đủ, vì ngữ cảnh lưu trong 8 thanh ghi do phần cứng tự push và 8 thanh ghi do FreeRTOS phần mềm push =  16 * 4 = 64 bytes.
5. Trước khi map handler, linker sẽ báo lỗi gì (đoạn nguyên văn thông báo):
c:/progra~2/armgnu~1/12977f~1.2mp/bin/../lib/gcc/arm-none-eabi/12.2.1/../../../../arm-none-eabi/bin/ld.exe: CMakeFiles/Week1.dir/third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c.obj: in function `SVC_Handler':
C:/Users/DELL/Intern_MLIoT/Week1/third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c:230: multiple definition of `SVC_Handler'; CMakeFiles/Week1.dir/Core/Src/stm32f1xx_it.c.obj:C:/Users/DELL/Intern_MLIoT/Week1/Core/Src/stm32f1xx_it.c:146: first defined here
c:/progra~2/armgnu~1/12977f~1.2mp/bin/../lib/gcc/arm-none-eabi/12.2.1/../../../../arm-none-eabi/bin/ld.exe: CMakeFiles/Week1.dir/third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c.obj: in function `PendSV_Handler':
C:/Users/DELL/Intern_MLIoT/Week1/third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c:459: multiple definition of `PendSV_Handler'; CMakeFiles/Week1.dir/Core/Src/stm32f1xx_it.c.obj:C:/Users/DELL/Intern_MLIoT/Week1/Core/Src/stm32f1xx_it.c:172: first defined here
c:/progra~2/armgnu~1/12977f~1.2mp/bin/../lib/gcc/arm-none-eabi/12.2.1/../../../../arm-none-eabi/bin/ld.exe: CMakeFiles/Week1.dir/third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c.obj: in function `SysTick_Handler':
C:/Users/DELL/Intern_MLIoT/Week1/third_party/FreeRTOS-Kernel/portable/GCC/ARM_CM3/port.c:492: multiple definition of `SysTick_Handler'; CMakeFiles/Week1.dir/Core/Src/stm32f1xx_it.c.obj:C:/Users/DELL/Intern_MLIoT/Week1/Core/Src/stm32f1xx_it.c:185: first defined here