import numpy as np
import os
import sys

# Configure UTF-8 output
if hasattr(sys.stdout, 'reconfigure'):
    sys.stdout.reconfigure(encoding='utf-8')

def analyze_single_file(file_path, W, max_lag=20):
    print("=" * 65)
    print(f"Phan tich file: {os.path.basename(file_path)} (FILTER_WINDOW = {W})")
    print("=" * 65)
    
    # Doc du lieu: seq, raw_ay, filt_ay, raw_q, filt_q
    data = []
    with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            parts = line.strip().split(",")
            if len(parts) >= 3:
                try:
                    data.append((int(parts[0]), float(parts[1]), float(parts[2])))
                except ValueError:
                    continue

    data = np.array(data)
    seq = data[:, 0]
    raw = data[:, 1]
    filt = data[:, 2]

    # 1. Cat bo W mau khoi dong (warm-up)
    raw = raw[W:]
    filt = filt[W:]

    # 2. Tru gia tri trung binh (Zero-mean)
    x = raw - np.mean(raw)     # Tin hieu goc
    y = filt - np.mean(filt)   # Tin hieu sau loc

    # 3. Tinh Cross-Correlation chuan hoa
    # y tre so voi x nen correlate(y, x) se co dinh tai lag k > 0
    full_corr = np.correlate(y, x, mode='full')
    norm = np.sqrt(np.sum(x**2) * np.sum(y**2))
    full_corr /= norm

    center = len(x) - 1
    lags = np.arange(-max_lag, max_lag + 1)
    corr_window = full_corr[center - max_lag : center + max_lag + 1]

    # 4. Tim dinh roi rac
    peak_idx = np.argmax(corr_window)
    k_star = lags[peak_idx]
    r_max = corr_window[peak_idx]

    # 5. Noi suy Parabol quanh dinh de ra phan le
    a = corr_window[peak_idx - 1]
    b = corr_window[peak_idx]
    c = corr_window[peak_idx + 1]
    delta = 0.5 * (a - c) / (a - 2 * b + c)
    refined_lag = k_star + delta
    refined_ms = refined_lag * 5.0  # SAMPLE_PERIOD_MS = 5 ms

    # Ly thuyet
    theory_lag = (W - 1) / 2.0
    theory_ms = theory_lag * 5.0

    print(f"- So mau phan tich:                 {len(x)}")
    print(f"- He so tuong quan dinh r_peak:     {r_max:.6f} {'[XAC THUC > 0.8: RAT TIN CAY]' if r_max >= 0.8 else '[THAP < 0.8]'}")
    print(f"- Do tre roi rac tai dinh (k*):     {k_star:+d} mau")
    print(f"- Do tre noi suy parabol (tau):     {refined_lag:+.4f} mau ({refined_ms:+.3f} ms)")
    print(f"- Do tre ly thuyet (W-1)/2:         {theory_lag:+.1f} mau ({theory_ms:+.1f} ms)")
    print(f"- Sai lech thuc te vs ly thuyet:    {abs(refined_lag - theory_lag):.4f} mau ({abs(refined_ms - theory_ms):.3f} ms)")
    print()
    print("  Bang tuong quan quanh dinh:")
    for i in range(max(0, peak_idx - 3), min(len(lags), peak_idx + 4)):
        marker = " <-- PEAK" if lags[i] == k_star else ""
        print(f"    lag = {lags[i]:+3d}: r = {corr_window[i]:.6f}{marker}")
    print()
    return refined_lag, refined_ms, r_max

base_dir = os.path.dirname(os.path.abspath(__file__))
w4_file = os.path.join(base_dir, "run_w4.log")
w8_file = os.path.join(base_dir, "run_w8.log")

res4 = None
res8 = None

if os.path.exists(w4_file):
    res4 = analyze_single_file(w4_file, W=4)
if os.path.exists(w8_file):
    res8 = analyze_single_file(w8_file, W=8)

if res4 and res8:
    diff_lag = res8[0] - res4[0]
    diff_ms = res8[1] - res4[1]
    theory_diff_lag = 2.0
    theory_diff_ms = 10.0
    print("=" * 65)
    print("TONG HOP CHENH LECH DO TRE GIUA W=8 VA W=4 (Delta = W8 - W4)")
    print("=" * 65)
    print(f"- Chenh lech thuc te uoc luong:     {diff_lag:+.4f} mau ({diff_ms:+.3f} ms)")
    print(f"- Chenh lech ly thuyet:             {theory_diff_lag:+.1f} mau ({theory_diff_ms:+.1f} ms)")
    print(f"- Sai so chenh lech:                {abs(diff_ms - theory_diff_ms):.3f} ms ({abs(diff_lag - theory_diff_lag):.4f} mau)")
    print("=" * 65)
