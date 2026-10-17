import sys
import os

# Dam bao terminal Windows khong bi loi font Unicode
if hasattr(sys.stdout, 'reconfigure'):
    try:
        sys.stdout.reconfigure(encoding='utf-8')
    except Exception:
        pass

def analyze_csv(file_path, nominal_period_ms=13.0, sample_count=500, sample_rate_hz=1000000.0):
    print(f"\n--- Analyzing: {file_path} ---")
    
    if not os.path.exists(file_path):
        print(f"Error: File not found '{file_path}'")
        return

    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
        first_line = f.readline().strip()
        parts = [p.strip() for p in first_line.split(',')]
        
        # Kiem tra co cot timestamp khong
        has_time_col = False
        if parts and parts[0].lower() in ['t', 'time', 'timestamp', 'time[s]']:
            has_time_col = True
            
        f.seek(0)
        
        # Doc 2000 dong dau de xem cau truc
        preview_lines = []
        for _ in range(2000):
            line = f.readline()
            if not line:
                break
            line = line.strip()
            if not line or line.startswith(';') or line.startswith('#'):
                continue
            cols = [p.strip() for p in line.split(',')]
            preview_lines.append(cols)
            
        active_col = -1
        if preview_lines:
            num_cols = len(preview_lines[0])
            start_col = 1 if has_time_col else 0
            
            # Quet qua 50000 dong dau de tim kenh co dao trang thai (toggles)
            f.seek(0)
            transitions_count = [0] * num_cols
            prev_row = None
            
            for row_idx, line in enumerate(f):
                if row_idx > 50000:
                    break
                line = line.strip()
                if not line or line.startswith(';') or line.startswith('#'):
                    continue
                cols = [p.strip() for p in line.split(',')]
                try:
                    vals = [int(float(c)) for c in cols]
                except ValueError:
                    continue
                    
                if prev_row is not None:
                    for c in range(start_col, min(len(vals), len(prev_row))):
                        if vals[c] != prev_row[c]:
                            transitions_count[c] += 1
                prev_row = vals
                
            best_col = -1
            best_cnt = 0
            for c in range(start_col, num_cols):
                if transitions_count[c] > best_cnt:
                    best_cnt = transitions_count[c]
                    best_col = c
            active_col = best_col

        if active_col == -1:
            print("Warning: Cannot auto-detect active channel, defaulting to first channel.")
            active_col = 1 if has_time_col else 0
        else:
            print(f"-> Auto-detected PA6 signal on channel column: {active_col}")

        # Quet toan bo file de lay danh sach cac lan chuyen muc logic
        f.seek(0)
        edge_indices = []
        edge_times = []
        prev_val = None
        sample_idx = 0
        
        for line in f:
            line = line.strip()
            if not line or line.startswith(';') or line.startswith('#'):
                continue
            cols = [p.strip() for p in line.split(',')]
            try:
                if has_time_col:
                    t = float(cols[0])
                    val = int(float(cols[active_col]))
                    if prev_val is not None and val != prev_val:
                        edge_times.append(t)
                    prev_val = val
                else:
                    val = int(float(cols[active_col]))
                    if prev_val is not None and val != prev_val:
                        edge_indices.append(sample_idx)
                    prev_val = val
                    sample_idx += 1
            except (ValueError, IndexError):
                continue

    # Tinh chu ky giua 2 lan toggle (ms)
    if has_time_col:
        print(f"Total toggles found: {len(edge_times)}")
        if len(edge_times) < 2:
            print("Error: Not enough edges to calculate period.")
            return
        periods = [(edge_times[i] - edge_times[i - 1]) * 1000.0 for i in range(1, len(edge_times))]
    else:
        print(f"Total samples: {sample_idx}, Total toggles: {len(edge_indices)}")
        if len(edge_indices) < 2:
            print("Error: Not enough edges to calculate period.")
            return
        ms_per_sample = 1000.0 / sample_rate_hz
        periods = [(edge_indices[i] - edge_indices[i - 1]) * ms_per_sample for i in range(1, len(edge_indices))]

    if len(periods) < sample_count:
        print(f"Notice: Only {len(periods)} cycles measured (requested {sample_count}).")
        sample_count = len(periods)
    else:
        print(f"-> Analyzing first {sample_count} cycles.")

    subset = periods[:sample_count]

    min_ms = min(subset)
    max_ms = max(subset)
    mean_ms = sum(subset) / len(subset)
    jitter_us = (max_ms - min_ms) * 1000.0
    
    total_time_ms = sum(subset)
    expected_total_ms = sample_count * nominal_period_ms
    drift_ms = total_time_ms - expected_total_ms

    print("\n" + "="*70)
    print("STATISTICS TABLE (Copy to REPORT.md):")
    print("="*70)
    print("| min (ms) | max (ms) | tb (ms) | jitter = max−min (us) | drift tổng (ms) |")
    print("|---|---|---|---|---|")
    print(f"| {min_ms:.3f} | {max_ms:.3f} | {mean_ms:.3f} | {jitter_us:.1f} | {drift_ms:.3f} |")
    print("="*70 + "\n")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python week1/ex3/app/calc_stats.py <path_to_csv>")
        sys.exit(1)
        
    csv_path = sys.argv[1]
    analyze_csv(csv_path, nominal_period_ms=13.0, sample_count=500, sample_rate_hz=1000000.0)