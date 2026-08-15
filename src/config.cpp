#include "config.h"
#include <vector>

namespace ConfigManager {

namespace {

const wchar_t RUN_KEY_PATH[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const wchar_t APP_NAME[]     = L"PortPeek";
const wchar_t CONFIG_KEY[]   = L"Software\\PortPeek";
const wchar_t HOTKEY_VALUE[] = L"EnableGlobalHotkey";

} // namespace

std::wstring GetCurrentExecutablePath() {
    wchar_t buffer[MAX_PATH * 2] = { 0 };
    DWORD len = GetModuleFileNameW(nullptr, buffer, static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0])));
    if (len > 0) {
        return std::wstring(buffer, len);
    }
    return L"";
}

bool IsAutoStartEnabled() {
    HKEY hKey = nullptr;
    LSTATUS status = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        RUN_KEY_PATH,
        0,
        KEY_READ,
        &hKey
    );

    if (status != ERROR_SUCCESS) return false;

    wchar_t buffer[MAX_PATH * 2] = { 0 };
    DWORD dataSize = sizeof(buffer);
    DWORD type = 0;

    status = RegQueryValueExW(
        hKey,
        APP_NAME,
        nullptr,
        &type,
        reinterpret_cast<LPBYTE>(buffer),
        &dataSize
    );

    RegCloseKey(hKey);
    return (status == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ));
}

bool SetAutoStartEnabled(bool enable) {
    HKEY hKey = nullptr;
    LSTATUS status = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        RUN_KEY_PATH,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        nullptr,
        &hKey,
        nullptr
    );

    if (status != ERROR_SUCCESS) return false;

    if (enable) {
        std::wstring exePath = GetCurrentExecutablePath();
        if (exePath.empty()) {
            RegCloseKey(hKey);
            return false;
        }

        std::wstring quotedPath = L"\"" + exePath + L"\"";
        DWORD byteSize = static_cast<DWORD>((quotedPath.length() + 1) * sizeof(wchar_t));

        status = RegSetValueExW(
            hKey,
            APP_NAME,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(quotedPath.c_str()),
            byteSize
        );
    } else {
        status = RegDeleteValueW(hKey, APP_NAME);
        if (status == ERROR_FILE_NOT_FOUND) {
            status = ERROR_SUCCESS;
        }
    }

    RegCloseKey(hKey);
    return (status == ERROR_SUCCESS);
}

bool IsGlobalHotkeyEnabled() {
    HKEY hKey = nullptr;
    LSTATUS status = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        CONFIG_KEY,
        0,
        KEY_READ,
        &hKey
    );

    if (status != ERROR_SUCCESS) {
        // Default to enabled
        return true;
    }

    DWORD val = 1;
    DWORD dataSize = sizeof(val);
    DWORD type = 0;

    status = RegQueryValueExW(
        hKey,
        HOTKEY_VALUE,
        nullptr,
        &type,
        reinterpret_cast<LPBYTE>(&val),
        &dataSize
    );

    RegCloseKey(hKey);
    if (status == ERROR_SUCCESS && type == REG_DWORD) {
        return (val != 0);
    }
    return true; // Default
}

bool SetGlobalHotkeyEnabled(bool enable) {
    HKEY hKey = nullptr;
    LSTATUS status = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        CONFIG_KEY,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        nullptr,
        &hKey,
        nullptr
    );

    if (status != ERROR_SUCCESS) return false;

    DWORD val = enable ? 1 : 0;
    status = RegSetValueExW(
        hKey,
        HOTKEY_VALUE,
        0,
        REG_DWORD,
        reinterpret_cast<const BYTE*>(&val),
        sizeof(val)
    );

    RegCloseKey(hKey);
    return (status == ERROR_SUCCESS);
}

} // namespace ConfigManager
