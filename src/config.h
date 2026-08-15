#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

namespace ConfigManager {

// Checks whether PortPeek is registered to start with Windows
bool IsAutoStartEnabled();

// Enables or disables startup with Windows in HKCU\Software\Microsoft\Windows\CurrentVersion\Run
bool SetAutoStartEnabled(bool enable);

// Checks whether the global hotkey (Win+Alt+P) is enabled
bool IsGlobalHotkeyEnabled();

// Enables or disables the global hotkey setting in HKCU\Software\PortPeek
bool SetGlobalHotkeyEnabled(bool enable);

// Gets the current executable path
std::wstring GetCurrentExecutablePath();

} // namespace ConfigManager
