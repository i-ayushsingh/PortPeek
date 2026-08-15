@echo off
setlocal enabledelayedexpansion

where cl >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    )
)

cl.exe /nologo /O1 /EHsc /std:c++17 /utf-8 /Isrc tests\test_runner.cpp src\ports.cpp src\process.cpp src\probe.cpp src\theme.cpp src\config.cpp ^
    /Fe:test_runner.exe ^
    ws2_32.lib iphlpapi.lib psapi.lib dwmapi.lib uxtheme.lib shell32.lib user32.lib advapi32.lib

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [*] Running Test Suite:
    echo.
    test_runner.exe
    del *.obj test_runner.exe >nul 2>&1
) else (
    echo [!] Test compilation failed.
    exit /b %ERRORLEVEL%
)

endlocal
