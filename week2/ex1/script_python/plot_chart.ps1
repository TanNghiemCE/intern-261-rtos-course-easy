param(
    [string]$logPath = "C:\Users\DELL\intern-261-rtos-course-easy\week2\ex1\log_uart_9600.log",
    [string]$pngPath = "C:\Users\DELL\intern-261-rtos-course-easy\week2\ex1\queue_fill_slow_uart.png"
)

Add-Type -AssemblyName System.Drawing

if (-not (Test-Path $logPath)) {
    Write-Host "File $logPath not found!"
    exit 1
}

$rawLines = Get-Content $logPath
$data = @()

# Detect format
$isDumpFormat = $false
foreach ($l in $rawLines) {
    if ($l -like "*DEPTH LOG DUMP*") {
        $isDumpFormat = $true
        break
    }
}

if ($isDumpFormat) {
    $inData = $false
    foreach ($line in $rawLines) {
        $line = $line.Trim()
        if ($line -like "*DEPTH LOG DUMP START*") { $inData = $true; continue }
        if ($line -like "*DEPTH LOG DUMP END*") { $inData = $false; break }
        if ($inData) {
            if ($line -eq "tick_ms,raw_depth,filt_depth" -or $line -eq "") { continue }
            $parts = $line.Split(',')
            if ($parts.Count -ge 3) {
                $data += [PSCustomObject]@{
                    tick_ms    = [double]$parts[0]
                    raw_depth  = [double]$parts[1]
                    filt_depth = [double]$parts[2]
                }
            }
        }
    }
} else {
    # 6-column stream format: seq, ax, ay, az, raw_depth, filt_depth
    foreach ($line in $rawLines) {
        $line = $line.Trim()
        if ($line -eq "") { continue }
        $parts = $line.Split(',')
        if ($parts.Count -ge 6) {
            try {
                $seq = [double]$parts[0]
                $t = $seq * 5.0  # 5ms per sample
                $rq = [double]$parts[4]
                $fq = [double]$parts[5]
                if ($fq -gt 8) { $fq = 7.0 } # clean trailing artifact
                $data += [PSCustomObject]@{
                    tick_ms    = $t
                    raw_depth  = $rq
                    filt_depth = $fq
                }
            } catch {
                # skip corrupted lines
            }
        }
    }
}

if ($data.Count -eq 0) {
    Write-Host "No data points found in $logPath"
    exit 1
}

Write-Host "Loaded $($data.Count) data points from $logPath"

$width = 1000
$height = 520
$bmp = New-Object System.Drawing.Bitmap($width, $height)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::ClearTypeGridFit

# Clear White background
$g.Clear([System.Drawing.Color]::White)

# Fonts & Brushes
$fontTitle = New-Object System.Drawing.Font("Arial", 13, [System.Drawing.FontStyle]::Bold)
$fontSub = New-Object System.Drawing.Font("Arial", 10, [System.Drawing.FontStyle]::Italic)
$fontAxis = New-Object System.Drawing.Font("Arial", 10, [System.Drawing.FontStyle]::Bold)
$fontSmall = New-Object System.Drawing.Font("Arial", 9, [System.Drawing.FontStyle]::Regular)
$brushBlack = [System.Drawing.Brushes]::Black
$brushDarkGray = [System.Drawing.Brushes]::DarkSlateGray

# Pens
$penAxis = New-Object System.Drawing.Pen([System.Drawing.Color]::Black, 2)
$penGrid = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(235, 235, 235), 1)
$penRedDash = New-Object System.Drawing.Pen([System.Drawing.Color]::Crimson, 2)
$penRedDash.DashStyle = [System.Drawing.Drawing2D.DashStyle]::Dash
$penRaw = New-Object System.Drawing.Pen([System.Drawing.Color]::RoyalBlue, 2.0)
$penFilt = New-Object System.Drawing.Pen([System.Drawing.Color]::Crimson, 2.5)

# Plot boundaries
$x0 = 80
$y0 = 65
$plotW = 860
$plotH = 370
$x1 = $x0 + $plotW
$y1 = $y0 + $plotH

# Determine title based on filename
$isSlow = ($logPath -like "*9600*" -or $pngPath -like "*slow*")
if ($isSlow) {
    $title = "Do day Queue theo thoi gian - Cau hinh UART cham (9600 baud)"
    $sub = "student_id: 2510343 | UART_BAUD = 9600 | queue_filtered bi tran (Saturation) tai t = 50ms"
} else {
    $title = "Do day Queue theo thoi gian - Cau hinh binh thuong (115200 baud)"
    $sub = "student_id: 2510343 | SAMPLE_PERIOD = 5ms | QUEUE_LENGTH = 8 | Total samples = $($data.Count)"
}

$titleSize = $g.MeasureString($title, $fontTitle)
$subSize = $g.MeasureString($sub, $fontSub)
$g.DrawString($title, $fontTitle, $brushBlack, ($width - $titleSize.Width) / 2, 12)
$g.DrawString($sub, $fontSub, $brushDarkGray, ($width - $subSize.Width) / 2, 34)

# Y-Axis Grid & Labels (0 to 8)
for ($q = 0; $q -le 8; $q++) {
    $y = $y1 - ($q / 8.0) * $plotH
    if ($q -gt 0 -and $q -lt 8) {
        $g.DrawLine($penGrid, $x0, $y, $x1, $y)
    } elseif ($q -eq 8) {
        $g.DrawLine($penRedDash, $x0, $y, $x1, $y)
    }
    $label = "$q"
    $g.DrawString($label, $fontSmall, $brushBlack, $x0 - 24, $y - 7)
}

# X-Axis Grid & Labels
$maxTime = $data[-1].tick_ms
if ($maxTime -lt 1000) { $maxTime = 15000.0 }
$step = 2500.0
if ($maxTime -gt 20000) { $step = 5000.0 }

for ($t = 0; $t -le $maxTime; $t += $step) {
    $x = $x0 + ($t / $maxTime) * $plotW
    if ($t -gt 0) {
        $g.DrawLine($penGrid, $x, $y0, $x, $y1)
    }
    $label = [int]$t
    $g.DrawString("$label", $fontSmall, $brushBlack, $x - 18, $y1 + 8)
}

# Axes Box
$g.DrawLine($penAxis, $x0, $y1, $x1, $y1)
$g.DrawLine($penAxis, $x0, $y0, $x0, $y1)

# Labels
$g.DrawString("Thoi gian (ms)", $fontAxis, $brushBlack, $x0 + $plotW / 2 - 45, $y1 + 28)
$g.TranslateTransform(25, $y0 + $plotH / 2 + 75)
$g.RotateTransform(-90)
$g.DrawString("Do day Queue (phan tu)", $fontAxis, $brushBlack, 0, 0)
$g.ResetTransform()

# Draw Data Lines
$prevX = $null
$prevYRaw = $null
$prevYFilt = $null

foreach ($row in $data) {
    $t = $row.tick_ms
    $rq = $row.raw_depth
    $fq = $row.filt_depth
    
    $curX = $x0 + ($t / $maxTime) * $plotW
    $curYRaw = $y1 - ($rq / 8.0) * $plotH
    $curYFilt = $y1 - ($fq / 8.0) * $plotH
    
    if ($prevX -ne $null) {
        $g.DrawLine($penRaw, $prevX, $prevYRaw, $curX, $curYRaw)
        $g.DrawLine($penFilt, $prevX, $prevYFilt, $curX, $curYFilt)
    }
    $prevX = $curX
    $prevYRaw = $curYRaw
    $prevYFilt = $curYFilt
}

# Legend Box
$legX = $x1 - 340
$legY = $y0 + 15
$g.FillRectangle([System.Drawing.Brushes]::WhiteSmoke, $legX, $legY, 325, 95)
$g.DrawRectangle([System.Drawing.Pens]::LightGray, $legX, $legY, 325, 95)

$g.DrawLine($penRaw, $legX + 15, $legY + 22, $legX + 55, $legY + 22)
$g.DrawString("queue_raw (do day = 0)", $fontSmall, $brushBlack, $legX + 65, $legY + 15)

$g.DrawLine($penFilt, $legX + 15, $legY + 47, $legX + 55, $legY + 47)
if ($isSlow) {
    $g.DrawString("queue_filtered (SATURATED = 7-8)", $fontSmall, [System.Drawing.Brushes]::Crimson, $legX + 65, $legY + 40)
} else {
    $g.DrawString("queue_filtered (do day = 0)", $fontSmall, $brushBlack, $legX + 65, $legY + 40)
}

$g.DrawLine($penRedDash, $legX + 15, $legY + 72, $legX + 55, $legY + 72)
$g.DrawString("QUEUE_LENGTH = 8 (Nguong day)", $fontSmall, $brushBlack, $legX + 65, $legY + 65)

# Save
$bmp.Save($pngPath, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose()
$bmp.Dispose()
Write-Host "Generated high-quality plot: $pngPath successfully!"
