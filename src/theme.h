#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

enum class PreferredAppMode {
    Default = 0,
    AllowDark = 1,
    ForceDark = 2,
    ForceLight = 3,
    Max = 4
};

class ThemeManager {
public:
    static ThemeManager& Instance();

    void Initialize();
    bool IsSystemDarkModeActive() const;
    void ApplyTheme(HWND hWnd);
    void OnThemeChanged(HWND hWnd);

private:
    ThemeManager() = default;
    ~ThemeManager() = default;

    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    typedef PreferredAppMode (WINAPI *fnSetPreferredAppMode)(PreferredAppMode appMode);
    typedef BOOL (WINAPI *fnAllowDarkModeForApp)(BOOL allow);
    typedef BOOL (WINAPI *fnAllowDarkModeForWindow)(HWND hWnd, BOOL allow);
    typedef BOOL (WINAPI *fnShouldAppsUseDarkMode)(void);
    typedef void (WINAPI *fnFlushMenuThemes)(void);
    typedef void (WINAPI *fnRefreshImmersiveColorPolicyState)(void);

    fnSetPreferredAppMode               m_SetPreferredAppMode = nullptr;
    fnAllowDarkModeForApp               m_AllowDarkModeForApp = nullptr;
    fnAllowDarkModeForWindow            m_AllowDarkModeForWindow = nullptr;
    fnShouldAppsUseDarkMode             m_ShouldAppsUseDarkMode = nullptr;
    fnFlushMenuThemes                   m_FlushMenuThemes = nullptr;
    fnRefreshImmersiveColorPolicyState  m_RefreshImmersiveColorPolicyState = nullptr;
    DWORD                               m_buildNumber = 0;
    bool                                m_initialized = false;

    DWORD GetWindowsBuildNumber();
};
