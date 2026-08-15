@echo off
setlocal enabledelayedexpansion

echo ============================================================
echo  Building PortPeek (Release x64)
echo ============================================================

:: 1. Ensure MSVC environment is loaded
where cl >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [*] Initializing Visual Studio build tools...
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    ) else (
        echo [!] Error: Visual Studio / MSVC build tools not found!
        exit /b 1
    )
)

:: 2. Ensure resources are generated
if not exist "res\appicon.ico" (
    echo [*] Generating icon resources...
    python generate_icons.py
)

:: 3. Compile Windows Resource script
echo [*] Compiling resource script (res\resource.rc)...
rc.exe /nologo /fo "res\resource.res" "res\resource.rc"
if %ERRORLEVEL% NEQ 0 (
    echo [!] Resource compilation failed.
    exit /b 1
)

:: 4. Compile C++ sources and link
echo [*] Compiling C++ source files...
cl.exe /nologo /O1 /GL /Gy /EHsc /std:c++17 /utf-8 /W4 /WX- /I"src" ^
    src\main.cpp src\tray.cpp src\ports.cpp src\process.cpp src\probe.cpp src\theme.cpp src\config.cpp ^
    res\resource.res ^
    /Fe:PortPeek.exe ^
    /link /NOLOGO /SUBSYSTEM:WINDOWS /LTCG /OPT:REF /OPT:ICF ^
    /MANIFEST:EMBED /MANIFESTINPUT:res\app.manifest ^
    ws2_32.lib iphlpapi.lib psapi.lib dwmapi.lib uxtheme.lib comctl32.lib shell32.lib user32.lib gdi32.lib advapi32.lib

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================================
    echo  [OK] Build Succeeded: PortPeek.exe
    echo ============================================================
    del *.obj >nul 2>&1
    del res\resource.res >nul 2>&1
) else (
    echo [!] Build Failed with error code %ERRORLEVEL%.
    exit /b %ERRORLEVEL%
)

endlocal
