#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include "ports.h"
#include "qrcode.h"
#include "app_discovery.h"

enum class FlyoutView {
    Main,
    SystemBackground,
    SysIdeTools,
    SysWinServices,
    SysInternal,
    DeveloperTools,
    StopProcess,
    Preferences,
    SelectPhonePortView,
    TestOnPhone,
    PublicTunnel,
    PortAliases,
    EpheSessionsView
};

enum class FlyoutItemAction {
    None,
    Header,
    BackToMain,
    BackToSystemBackground,
    BackToSelectPhonePort,
    OpenPort,
    OpenAllActive,
    NavToSystemBackground,
    NavToSysIdeTools,
    NavToSysWinServices,
    NavToSysInternal,
    NavToDeveloperTools,
    NavToStopProcess,
    NavToPreferences,
    NavToTestOnPhone,
    NavToPublicTunnel,
    NavToPortAliases,
    NavToEpheSessions,
    OpenEpheSession,
    OpenAllEpheSessions,
    LaunchNewEpheSession,
    CopyUrlDirect,
    CopyLanUrl,
    CopyAllMarkdown,
    StartCloudflareTunnel,
    CopyTunnelCliCommand,
    StopActiveTunnel,
    OpenInVsCode,
    OpenInTerminal,
    KillSinglePort,
    KillAllDev,
    SelectPhonePort,
    ToggleAutoStart,
    ToggleHotkey,
    Refresh,
    Exit,
    Separator
};

enum class FlyoutIconType {
    None,
    Dot,
    BackArrow,
    CopyLink,
    DevToolsCode,
    StopProcessSquare,
    PreferencesGear,
    RefreshLoop,
    ExitPower,
    Checkmark,
    Terminal,
    VsCode,
    Phone,
    Globe,
    Pin,
    IdeTools,
    WinServices,
    InternalBox,
    EpheApp,
    Plus
};

struct FlyoutItem {
    FlyoutItemAction    action = FlyoutItemAction::None;
    FlyoutIconType      icon = FlyoutIconType::None;
    RECT                rect = { 0, 0, 0, 0 };
    std::wstring        portText;
    std::wstring        title;
    std::wstring        subtitle;
    std::wstring        rightText;
    bool                hasChevron = false;
    bool                isChecked = false;
    bool                isHeader = false;
    bool                isHeaderActive = false;
    size_t              portIndex = 0;
    DWORD               targetPid = 0;
    std::wstring        targetFolder;
    std::wstring        extraData;
    NetProbe::PortHealth health = NetProbe::PortHealth::Healthy;
};

class TrayManager {
public:
    static TrayManager& Instance();

    bool Initialize(HINSTANCE hInstance);
    void Shutdown();

    HWND GetHwnd() const { return m_hWnd; }
    HWND GetFlyoutHwnd() const { return m_hFlyoutWnd; }

private:
    TrayManager() = default;
    ~TrayManager();

    TrayManager(const TrayManager&) = delete;
    TrayManager& operator=(const TrayManager&) = delete;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK FlyoutWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    bool AddTrayIcon();
    void RemoveTrayIcon();
    void UpdateTooltip(const wchar_t* tip);
    void SyncTrayState();
    void ShowFlyout(int x, int y);
    void HideFlyout();
    void RebuildFlyoutLayout();
    void HandleItemClick(const FlyoutItem& item);

    void RegisterGlobalHotkey();
    void UnregisterGlobalHotkey();

    HINSTANCE                   m_hInstance = nullptr;
    HWND                        m_hWnd = nullptr;
    HWND                        m_hFlyoutWnd = nullptr;
    HICON                       m_hIcon = nullptr;
    UINT                        m_wmTaskbarCreated = 0;
    bool                        m_iconAdded = false;
    bool                        m_hotkeyRegistered = false;
    int                         m_hoveredIndex = -1;
    bool                        m_isMouseTracking = false;
    FlyoutView                  m_currentView = FlyoutView::Main;
    size_t                      m_selectedPortIndex = 0;

    std::vector<ListeningPort>    m_activeMenuPorts;
    std::vector<DiscoveredSession> m_activeEpheSessions;
    std::vector<FlyoutItem>       m_flyoutItems;
    QrCode::QrMatrix              m_currentQr;
    RECT                          m_qrRect = { 0, 0, 0, 0 };
    uint16_t                      m_phoneTargetPort = 0;
    std::wstring                  m_phoneTargetLanUrl;
    bool                          m_justRefreshed = false;
};
