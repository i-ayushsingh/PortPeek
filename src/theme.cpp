#include "theme.h"
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1
#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
#endif

ThemeManager& ThemeManager::Instance() {
    static ThemeManager instance;
    return instance;
}

DWORD ThemeManager::GetWindowsBuildNumber() {
    typedef LONG(NTAPI* fnRtlGetVersion)(PRTL_OSVERSIONINFOW);
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll) {
        auto pRtlGetVersion = reinterpret_cast<fnRtlGetVersion>(GetProcAddress(hNtdll, "RtlGetVersion"));
        if (pRtlGetVersion) {
            RTL_OSVERSIONINFOW osInfo = { sizeof(osInfo) };
            if (pRtlGetVersion(&osInfo) == 0) {
                return osInfo.dwBuildNumber;
            }
        }
    }
    return 0;
}

void ThemeManager::Initialize() {
    if (m_initialized) return;
    m_initialized = true;

    m_buildNumber = GetWindowsBuildNumber();
    HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!hUxtheme) return;

    m_RefreshImmersiveColorPolicyState = reinterpret_cast<fnRefreshImmersiveColorPolicyState>(
        GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104)));
    m_ShouldAppsUseDarkMode = reinterpret_cast<fnShouldAppsUseDarkMode>(
        GetProcAddress(hUxtheme, MAKEINTRESOURCEA(132)));
    m_AllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(
        GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));

    if (m_buildNumber >= 18362) {
        // Windows 10 1903+ and Windows 11
        m_SetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(
            GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135)));
    } else if (m_buildNumber >= 17763) {
        // Windows 10 1809
        m_AllowDarkModeForApp = reinterpret_cast<fnAllowDarkModeForApp>(
            GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135)));
    }

    m_FlushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(
        GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136)));

    // Allow native dark mode on menus and windows
    if (m_SetPreferredAppMode) {
        m_SetPreferredAppMode(PreferredAppMode::AllowDark);
    } else if (m_AllowDarkModeForApp) {
        m_AllowDarkModeForApp(TRUE);
    }

    if (m_RefreshImmersiveColorPolicyState) {
        m_RefreshImmersiveColorPolicyState();
    }

    if (m_FlushMenuThemes) {
        m_FlushMenuThemes();
    }
}

bool ThemeManager::IsSystemDarkModeActive() const {
    if (m_ShouldAppsUseDarkMode) {
        return m_ShouldAppsUseDarkMode() != FALSE;
    }

    DWORD data = 1;
    DWORD size = sizeof(data);
    if (RegGetValueW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            L"AppsUseLightTheme",
            RRF_RT_REG_DWORD,
            nullptr,
            &data,
            &size) == ERROR_SUCCESS) {
        return data == 0;
    }
    return false;
}

void ThemeManager::ApplyTheme(HWND hWnd) {
    if (!hWnd) return;
    BOOL isDark = IsSystemDarkModeActive() ? TRUE : FALSE;

    if (m_AllowDarkModeForWindow) {
        m_AllowDarkModeForWindow(hWnd, isDark);
    }

    DWORD attr = (m_buildNumber >= 19041 || m_buildNumber >= 22000)
        ? DWMWA_USE_IMMERSIVE_DARK_MODE
        : DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1;

    DwmSetWindowAttribute(hWnd, attr, &isDark, sizeof(isDark));

    if (m_FlushMenuThemes) {
        m_FlushMenuThemes();
    }
}

void ThemeManager::OnThemeChanged(HWND hWnd) {
    if (m_RefreshImmersiveColorPolicyState) {
        m_RefreshImmersiveColorPolicyState();
    }
    if (m_FlushMenuThemes) {
        m_FlushMenuThemes();
    }
    if (hWnd) {
        ApplyTheme(hWnd);
    }
}
