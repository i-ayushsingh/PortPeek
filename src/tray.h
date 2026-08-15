#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <vector>
#include <cstdint>
#include "ports.h"

class TrayManager {
public:
    static TrayManager& Instance();

    bool Initialize(HINSTANCE hInstance);
    void Shutdown();

    HWND GetHwnd() const { return m_hWnd; }

private:
    TrayManager() = default;
    ~TrayManager();

    TrayManager(const TrayManager&) = delete;
    TrayManager& operator=(const TrayManager&) = delete;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    bool AddTrayIcon();
    void RemoveTrayIcon();
    void UpdateTooltip(const wchar_t* tip);
    void SyncTrayState();
    void ShowContextMenu(int x, int y);
    void HandleCommand(WORD commandId);

    void RegisterGlobalHotkey();
    void UnregisterGlobalHotkey();

    HINSTANCE                   m_hInstance = nullptr;
    HWND                        m_hWnd = nullptr;
    HICON                       m_hIcon = nullptr;
    UINT                        m_wmTaskbarCreated = 0;
    bool                        m_iconAdded = false;
    bool                        m_hotkeyRegistered = false;
    std::vector<ListeningPort>  m_activeMenuPorts;
};
