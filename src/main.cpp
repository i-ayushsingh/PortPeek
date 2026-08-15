#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>

#include "tray.h"
#include "theme.h"

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")

namespace {
const wchar_t SINGLE_INSTANCE_MUTEX[] = L"PortPeek_SingleInstance_Mutex_7F8A";
}

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ PWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // 1. Enforce single instance so multiple launches don't clutter the system tray
    SetLastError(0);
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, SINGLE_INSTANCE_MUTEX);
    if (!hMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    // 2. Initialize Dark Mode preference before creating any windows or menus
    ThemeManager::Instance().Initialize();

    // 3. Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return 1;
    }

    // 4. Initialize System Tray Manager and hidden message window
    if (!TrayManager::Instance().Initialize(hInstance)) {
        WSACleanup();
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return 1;
    }

    // 5. Native Win32 Message Loop
    MSG msg = {};
    BOOL bRet = 0;
    while ((bRet = GetMessageW(&msg, nullptr, 0, 0)) != 0) {
        if (bRet == -1) break;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // 6. Cleanup
    TrayManager::Instance().Shutdown();
    WSACleanup();

    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    return static_cast<int>(msg.wParam);
}
