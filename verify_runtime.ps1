# PortPeek Runtime Verification Script
$ErrorActionPreference = "Stop"

Write-Host "============================================================"
Write-Host " PortPeek Runtime Lifecycle & Metric Verification"
Write-Host "============================================================"

# 1. Start PortPeek in background
$proc = Start-Process -FilePath ".\PortPeek.exe" -PassThru
Start-Sleep -Milliseconds 600

try {
    # 2. Inspect Process Properties
    $procInfo = Get-Process -Id $proc.Id
    $memMB = [math]::Round($procInfo.WorkingSet64 / 1MB, 2)
    $hasNoWindow = ($procInfo.MainWindowHandle -eq 0)

    Write-Host "[+] Process ID        : $($proc.Id)"
    Write-Host "[+] Process Name      : $($procInfo.ProcessName)"
    Write-Host "[+] Working Set Memory: $memMB MB"
    Write-Host "[+] Windowless (Tray) : $hasNoWindow"

    if (-not $hasNoWindow) {
        Write-Error "PortPeek should NOT have a main application window!"
    }

    # 3. Test Single Instance Guard
    Write-Host "`n[*] Launching second instance (should exit immediately)..."
    $secondProc = Start-Process -FilePath ".\PortPeek.exe" -PassThru -Wait
    Write-Host "[+] Second Instance Exit Code: $($secondProc.ExitCode)"

    if ($secondProc.ExitCode -ne 0) {
        Write-Error "Second instance did not exit with code 0"
    }

    # 4. Verify primary instance is still alive and running
    $stillRunning = Get-Process -Id $proc.Id -ErrorAction SilentlyContinue
    if ($stillRunning) {
        Write-Host "[+] Primary instance is healthy and running in system tray."
    } else {
        Write-Error "Primary instance unexpectedly terminated!"
    }
}
finally {
    # 5. Clean termination
    Write-Host "`n[*] Terminating test instance..."
    Stop-Process -Id $proc.Id -Force
    Start-Sleep -Milliseconds 300
    Write-Host "[OK] PortPeek successfully verified."
}
