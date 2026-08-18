@echo off
setlocal

echo ============================================================
echo  Building PortPeek NSIS Installer
echo ============================================================

set "MAKENSIS=C:\Program Files (x86)\NSIS\makensis.exe"
if not exist "%MAKENSIS%" (
    set "MAKENSIS=C:\Program Files\NSIS\makensis.exe"
)
if not exist "%MAKENSIS%" (
    where makensis.exe >nul 2>&1
    if %ERRORLEVEL% EQU 0 set "MAKENSIS=makensis.exe"
)

if not exist "%MAKENSIS%" (
    if "%MAKENSIS%" NEQ "makensis.exe" (
        echo [!] NSIS compiler not found at standard path.
        exit /b 1
    )
)

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
