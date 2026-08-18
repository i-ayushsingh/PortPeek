@echo off
setlocal

echo ============================================================
echo  Building PortPeek NSIS Installer
echo ============================================================

set "MAKENSIS="

if exist "C:\Program Files (x86)\NSIS\makensis.exe" (
    set "MAKENSIS=C:\Program Files (x86)\NSIS\makensis.exe"
) else if exist "C:\Program Files\NSIS\makensis.exe" (
    set "MAKENSIS=C:\Program Files\NSIS\makensis.exe"
) else if exist "C:\ProgramData\chocolatey\bin\makensis.exe" (
    set "MAKENSIS=C:\ProgramData\chocolatey\bin\makensis.exe"
) else (
    where makensis.exe >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        for /f "delims=" %%I in ('where makensis.exe') do (
            if not defined MAKENSIS set "MAKENSIS=%%I"
        )
    )
)

if not defined MAKENSIS (
    echo [!] NSIS compiler makensis.exe was not found on this system.
    echo [!] Please install NSIS from https://nsis.sourceforge.io
    exit /b 1
)

echo [*] Using NSIS compiler: "%MAKENSIS%"
echo [*] Compiling installer with NSIS...
"%MAKENSIS%" /V3 "installer.nsi"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================================
    echo  [OK] Installer Build Succeeded: PortPeek-Setup-v0.2.0.exe
    echo ============================================================
) else (
    echo [!] NSIS compilation failed with error code %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)

endlocal
