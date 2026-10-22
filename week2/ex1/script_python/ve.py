import os
import subprocess
import sys

def main():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    ps1_script = os.path.join(base_dir, "plot_chart.ps1")
    
    # 1. Vẽ đồ thị cấu hình bình thường 115200 baud
    log_115200 = os.path.join(base_dir, "log_uart_115200.log")
    png_normal = os.path.join(base_dir, "queue_fill_normal.png")
    
    if os.path.exists(log_115200):
        print(f"[*] Dang ve do thi tu {log_115200}...")
        cmd = [
            "powershell", "-ExecutionPolicy", "Bypass",
            "-File", ps1_script,
            "-logPath", log_115200,
            "-pngPath", png_normal
        ]
        res = subprocess.run(cmd, capture_output=True, text=True)
        print(res.stdout)
        if res.returncode == 0:
            print(f"[+] Da tao thanh cong: {png_normal}")
        else:
            print(f"[-] Loi khi ve: {res.stderr}")
    else:
        print(f"[-] Khong tim thay file: {log_115200}")

    # 2. Vẽ đồ thị cấu hình UART chậm 9600 baud (khi có file log)
    log_9600 = os.path.join(base_dir, "log_uart_9600.log")
    png_slow = os.path.join(base_dir, "queue_fill_slow_uart.png")
    
    if os.path.exists(log_9600):
        print(f"[*] Dang ve do thi UART cham tu {log_9600}...")
        cmd = [
            "powershell", "-ExecutionPolicy", "Bypass",
            "-File", ps1_script,
            "-logPath", log_9600,
            "-pngPath", png_slow
        ]
        res = subprocess.run(cmd, capture_output=True, text=True)
        print(res.stdout)
        if res.returncode == 0:
            print(f"[+] Da tao thanh cong: {png_slow}")
        else:
            print(f"[-] Loi khi ve: {res.stderr}")
    else:
        print(f"[i] Chua co {log_9600}. Sau khi thu nghiem 9600 baud va luu file log, chay lai script de tao anh slow!")

if __name__ == "__main__":
    main()
if __name__ == "__main__":
    main()