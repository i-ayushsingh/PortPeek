#include "tray.h"
#include "resource.h"
#include "ports.h"
#include "process.h"
#include "theme.h"
#include "config.h"
#include "lan.h"
#include "tunnel.h"
#include "alias.h"
#include "qrcode.h"

#include <windowsx.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>

#pragma comment(lib, "dwmapi.lib")

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace {
const wchar_t MSG_WINDOW_CLASS[]    = L"PortPeek_Hidden_Msg_Class";
const wchar_t FLYOUT_WINDOW_CLASS[] = L"PortPeek_Native_Flyout_Class";
const wchar_t WINDOW_TITLE[]        = L"PortPeek";

const UINT_PTR TRAY_RETRY_TIMER_ID = 101;
const UINT TRAY_RETRY_INTERVAL_MS = 1500;

const UINT_PTR TRAY_SYNC_TIMER_ID = 102;
const UINT TRAY_SYNC_INTERVAL_MS = 3000;

const UINT_PTR REFRESH_FEEDBACK_TIMER_ID = 103;

const int GLOBAL_HOTKEY_ID = 9001;

std::wstring ToLowerStr(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) {
        return static_cast<wchar_t>(std::towlower(c));
    });
    return s;
}

HFONT CreateSegoeFont(int pointSize, int weight, bool italic = false) {
    HDC screen = GetDC(nullptr);
    int logPixelsY = GetDeviceCaps(screen, LOGPIXELSY);
    ReleaseDC(nullptr, screen);

    int height = -MulDiv(pointSize, logPixelsY, 72);

    return CreateFontW(
        height, 0, 0, 0,
        weight,
        italic ? TRUE : FALSE,
        FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI Variable Display"
    );
}

HFONT CreateSegoeIconFont(int pointSize) {
    HDC screen = GetDC(nullptr);
    int logPixelsY = GetDeviceCaps(screen, LOGPIXELSY);
    ReleaseDC(nullptr, screen);

    int height = -MulDiv(pointSize, logPixelsY, 72);

    return CreateFontW(
        height, 0, 0, 0,
        FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe Fluent Icons"
    );
}

std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), &str[0], size, nullptr, nullptr);
    return str;
}

} // namespace

TrayManager& TrayManager::Instance() {
    static TrayManager instance;
    return instance;
}

TrayManager::~TrayManager() {
    Shutdown();
}

bool TrayManager::Initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;
    AliasManager::Initialize();

    m_wmTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");

    int iconSize = GetSystemMetrics(SM_CXSMICON);
    if (iconSize <= 0) iconSize = 16;

    m_hIcon = static_cast<HICON>(LoadImageW(
        hInstance,
        MAKEINTRESOURCEW(IDI_APPICON),
        IMAGE_ICON,
        iconSize,
        iconSize,
        LR_DEFAULTCOLOR | LR_SHARED
    ));

    if (!m_hIcon) {
        m_hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APPICON));
    }

    if (!m_hIcon) {
        m_hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    }

    // 1. Register Hidden Message Window Class
    WNDCLASSEXW wcMsg = { sizeof(wcMsg) };
    wcMsg.lpfnWndProc   = WndProc;
    wcMsg.hInstance     = hInstance;
    wcMsg.lpszClassName = MSG_WINDOW_CLASS;
    wcMsg.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcMsg.hIcon         = m_hIcon;
    RegisterClassExW(&wcMsg);

    // 2. Register Native Windows 11 Flyout Window Class
    WNDCLASSEXW wcFlyout = { sizeof(wcFlyout) };
    wcFlyout.lpfnWndProc   = FlyoutWndProc;
    wcFlyout.hInstance     = hInstance;
    wcFlyout.lpszClassName = FLYOUT_WINDOW_CLASS;
    wcFlyout.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcFlyout.hbrBackground = nullptr;
    RegisterClassExW(&wcFlyout);

    m_hWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        MSG_WINDOW_CLASS,
        WINDOW_TITLE,
        WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
        nullptr,
        nullptr,
        hInstance,
        this
    );

    if (!m_hWnd) {
        return false;
    }

    ThemeManager::Instance().ApplyTheme(m_hWnd);

    if (!AddTrayIcon()) {
        SetTimer(m_hWnd, TRAY_RETRY_TIMER_ID, TRAY_RETRY_INTERVAL_MS, nullptr);
    }

    // Live tooltip sync timer (every 3 seconds)
    SetTimer(m_hWnd, TRAY_SYNC_TIMER_ID, TRAY_SYNC_INTERVAL_MS, nullptr);

    // Global hotkey
    if (ConfigManager::IsGlobalHotkeyEnabled()) {
        RegisterGlobalHotkey();
    }

    return true;
}

void TrayManager::Shutdown() {
    UnregisterGlobalHotkey();
    RemoveTrayIcon();
    TunnelManager::StopActiveTunnel();

    if (m_hFlyoutWnd) {
        DestroyWindow(m_hFlyoutWnd);
        m_hFlyoutWnd = nullptr;
    }

    if (m_hWnd) {
        KillTimer(m_hWnd, TRAY_RETRY_TIMER_ID);
        KillTimer(m_hWnd, TRAY_SYNC_TIMER_ID);
        KillTimer(m_hWnd, REFRESH_FEEDBACK_TIMER_ID);
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }

    UnregisterClassW(MSG_WINDOW_CLASS, m_hInstance);
    UnregisterClassW(FLYOUT_WINDOW_CLASS, m_hInstance);
}

void TrayManager::RegisterGlobalHotkey() {
    if (!m_hWnd || m_hotkeyRegistered) return;
    if (RegisterHotKey(m_hWnd, GLOBAL_HOTKEY_ID, MOD_WIN | MOD_ALT | MOD_NOREPEAT, 'P')) {
        m_hotkeyRegistered = true;
    }
}

void TrayManager::UnregisterGlobalHotkey() {
    if (!m_hWnd || !m_hotkeyRegistered) return;
    UnregisterHotKey(m_hWnd, GLOBAL_HOTKEY_ID);
    m_hotkeyRegistered = false;
}

bool TrayManager::AddTrayIcon() {
    if (!m_hWnd) return false;

    NOTIFYICONDATAW nid = {};
    nid.cbSize           = sizeof(NOTIFYICONDATAW);
    nid.hWnd             = m_hWnd;
    nid.uID              = TRAY_ICON_ID;
    nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon            = m_hIcon;
    wcsncpy_s(nid.szTip, L"PortPeek", _TRUNCATE);

    if (!m_iconAdded) {
        if (!Shell_NotifyIconW(NIM_ADD, &nid)) {
            m_iconAdded = false;
            return false;
        }
    }

    NOTIFYICONDATAW nidVer = {};
    nidVer.cbSize   = sizeof(NOTIFYICONDATAW);
    nidVer.hWnd     = m_hWnd;
    nidVer.uID      = TRAY_ICON_ID;
    nidVer.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &nidVer);

    m_iconAdded = true;
    if (m_hWnd) {
        KillTimer(m_hWnd, TRAY_RETRY_TIMER_ID);
    }

    SyncTrayState();
    return true;
}

void TrayManager::RemoveTrayIcon() {
    if (!m_hWnd || !m_iconAdded) return;

    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd   = m_hWnd;
    nid.uID    = TRAY_ICON_ID;
    Shell_NotifyIconW(NIM_DELETE, &nid);
    m_iconAdded = false;
}

void TrayManager::UpdateTooltip(const wchar_t* tip) {
    if (!m_hWnd || !m_iconAdded) return;

    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd   = m_hWnd;
    nid.uID    = TRAY_ICON_ID;
    nid.uFlags = NIF_TIP | NIF_SHOWTIP;
    if (tip) {
        wcsncpy_s(nid.szTip, tip, _TRUNCATE);
    } else {
        wcsncpy_s(nid.szTip, L"PortPeek", _TRUNCATE);
    }
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void TrayManager::SyncTrayState() {
    auto ports = EnumerateListeningTcpPorts();
    size_t devCount = 0;
    std::wstring devPortsList;

    for (const auto& p : ports) {
        if (p.isDevServer) {
            if (!devPortsList.empty()) devPortsList += L", ";
            devPortsList += std::to_wstring(p.port);
            ++devCount;
        }
    }

    std::wstring tip = L"PortPeek";
    if (devCount > 0) {
        tip += L": " + std::to_wstring(devCount) + L" active (" + devPortsList + L")";
    } else {
        tip += L" (0 active dev servers)";
    }
    UpdateTooltip(tip.c_str());
}

// ----------------------------------------------------------------------------
// NATIVE FLUENT FLYOUT POPOVER & ADVANCED CATEGORIZED SUBVIEWS
// ----------------------------------------------------------------------------

void TrayManager::RebuildFlyoutLayout() {
    m_flyoutItems.clear();
    m_hoveredIndex = -1;
    m_qrRect = { 0, 0, 0, 0 };

    std::vector<size_t> devIndices;
    std::vector<size_t> sysIdeIndices;
    std::vector<size_t> sysWinIndices;
    std::vector<size_t> sysInternalIndices;

    for (size_t i = 0; i < m_activeMenuPorts.size(); ++i) {
        const auto& p = m_activeMenuPorts[i];
        if (p.isDevServer) {
            devIndices.push_back(i);
        } else {
            std::wstring proc = ToLowerStr(p.processName);
            if (proc.find(L"code") != std::wstring::npos ||
                proc.find(L"language server") != std::wstring::npos ||
                proc.find(L"antigravity") != std::wstring::npos ||
                proc.find(L"node") != std::wstring::npos ||
                proc.find(L"python") != std::wstring::npos) {
                sysIdeIndices.push_back(i);
            } else if (p.port == 135 || p.port == 139 || p.port == 445 || p.port == 5357 ||
                       p.pid == 0 || p.pid == 4 || proc.find(L"system") != std::wstring::npos ||
                       proc.find(L"svchost") != std::wstring::npos) {
                sysWinIndices.push_back(i);
            } else {
                sysInternalIndices.push_back(i);
            }
        }
    }

    size_t totalSysCount = sysIdeIndices.size() + sysWinIndices.size() + sysInternalIndices.size();

    const int width = 360;
    int currentY = 6;

    switch (m_currentView) {
    case FlyoutView::Main: {
        // 1. Header: PortPeek (N active) or Refreshed
        FlyoutItem hdr;
        hdr.action = FlyoutItemAction::Header;
        if (m_justRefreshed) {
            hdr.title = L"PortPeek (Refreshed ✓)";
            hdr.isHeaderActive = true;
        } else {
            hdr.title = L"PortPeek";
            if (!devIndices.empty()) {
                hdr.title += L" (" + std::to_wstring(devIndices.size()) + L" active)";
                hdr.isHeaderActive = true;
            } else {
                hdr.title += L" (0 active)";
                hdr.isHeaderActive = false;
            }
        }
        hdr.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(hdr);

        // 2. Dev Ports (2-Line layout with Real-Time Latency Meter)
        if (!devIndices.empty()) {
            for (size_t idx : devIndices) {
                const auto& p = m_activeMenuPorts[idx];
                FlyoutItem item;
                item.action = FlyoutItemAction::OpenPort;
                item.icon = FlyoutIconType::Dot;
                item.portIndex = idx;
                item.portText = std::to_wstring(p.port);
                item.health = p.health;

                std::wstring name = !p.projectName.empty() ? p.projectName : (!p.framework.empty() ? p.framework : p.processName);
                if (p.latencyMs > 0) {
                    name += L" (" + std::to_wstring(p.latencyMs) + L"ms)";
                }
                item.title = name;

                // Subtitle Line 2: Process + Memory
                item.subtitle = p.processName;
                if (p.memoryMb > 0) {
                    item.subtitle += L" • " + std::to_wstring(p.memoryMb) + L" MB";
                }
                if (!p.pageTitle.empty()) {
                    item.subtitle += L" • \"" + p.pageTitle + L"\"";
                }

                item.hasChevron = true;
                item.rect = { 6, currentY, width - 6, currentY + 48 };
                currentY += 50;
                m_flyoutItems.push_back(item);
            }
        } else {
            FlyoutItem item;
            item.action = FlyoutItemAction::None;
            item.icon = FlyoutIconType::Dot;
            item.title = m_activeMenuPorts.empty() ? L"No listening ports detected" : L"No active dev servers";
            item.rect = { 6, currentY, width - 6, currentY + 38 };
            currentY += 40;
            m_flyoutItems.push_back(item);
        }

        // 2b. Optional Discovered App Sessions (e.g. Ephe)
        if (!m_activeEpheSessions.empty() || EpheLauncher::IsEpheAvailable()) {
            FlyoutItem epheItem;
            epheItem.action = FlyoutItemAction::NavToEpheSessions;
            epheItem.icon = FlyoutIconType::EpheApp;
            if (!m_activeEpheSessions.empty()) {
                epheItem.title = L"Ephe Sessions (" + std::to_wstring(m_activeEpheSessions.size()) + L")";
            } else {
                epheItem.title = L"Ephe Sessions";
            }
            epheItem.hasChevron = true;
            epheItem.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(epheItem);
        }

        // 3. System Background Categorized Entry
        {
            FlyoutItem sysItem;
            sysItem.action = FlyoutItemAction::NavToSystemBackground;
            sysItem.icon = FlyoutIconType::Dot;
            sysItem.title = L"System Background (" + std::to_wstring(totalSysCount) + L")";
            sysItem.hasChevron = true;
            sysItem.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(sysItem);
        }

        // Separator
        {
            FlyoutItem sep;
            sep.action = FlyoutItemAction::Separator;
            sep.rect = { 6, currentY, width - 6, currentY + 8 };
            currentY += 9;
            m_flyoutItems.push_back(sep);
        }

        // 4. Action Items
        {
            // Copy URL (No chevron)
            FlyoutItem copyItem;
            copyItem.action = FlyoutItemAction::CopyUrlDirect;
            copyItem.icon = FlyoutIconType::CopyLink;
            copyItem.title = L"Copy URL";
            copyItem.hasChevron = false;
            copyItem.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(copyItem);

            // 📱 Mobile LAN Tester & QR Code
            FlyoutItem phoneItem;
            phoneItem.action = FlyoutItemAction::NavToTestOnPhone;
            phoneItem.icon = FlyoutIconType::Phone;
            phoneItem.title = devIndices.empty() ? L"Test on Phone (0 active)" : L"Test on Phone (QR Code)";
            phoneItem.hasChevron = true;
            phoneItem.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(phoneItem);

            // 🌐 Public Tunnel
            FlyoutItem tunnelItem;
            tunnelItem.action = FlyoutItemAction::NavToPublicTunnel;
            tunnelItem.icon = FlyoutIconType::Globe;
            tunnelItem.title = L"Public Tunnel (Cloudflare)";
            tunnelItem.hasChevron = true;
            tunnelItem.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(tunnelItem);

            // Developer Tools
            FlyoutItem devItem;
            devItem.action = FlyoutItemAction::NavToDeveloperTools;
            devItem.icon = FlyoutIconType::DevToolsCode;
            devItem.title = L"Developer Tools";
            devItem.hasChevron = true;
            devItem.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(devItem);

            // Stop Process / Free Port Helper
            FlyoutItem stopItem;
            stopItem.action = FlyoutItemAction::NavToStopProcess;
            stopItem.icon = FlyoutIconType::StopProcessSquare;
            stopItem.title = L"Stop Process / Free Port";
            stopItem.hasChevron = true;
            stopItem.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(stopItem);

            // 📌 Custom Port Aliases
            FlyoutItem aliasItem;
            aliasItem.action = FlyoutItemAction::NavToPortAliases;
            aliasItem.icon = FlyoutIconType::Pin;
            aliasItem.title = L"Port Aliases (.portpeek)";
            aliasItem.hasChevron = true;
            aliasItem.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(aliasItem);

            // Preferences
            FlyoutItem prefItem;
            prefItem.action = FlyoutItemAction::NavToPreferences;
            prefItem.icon = FlyoutIconType::PreferencesGear;
            prefItem.title = L"Preferences";
            prefItem.hasChevron = true;
            prefItem.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(prefItem);
        }

        // Separator
        {
            FlyoutItem sep;
            sep.action = FlyoutItemAction::Separator;
            sep.rect = { 6, currentY, width - 6, currentY + 8 };
            currentY += 9;
            m_flyoutItems.push_back(sep);
        }

        // 5. Utility Items: Refresh and Exit
        {
            FlyoutItem refItem;
            refItem.action = FlyoutItemAction::Refresh;
            refItem.icon = FlyoutIconType::RefreshLoop;
            refItem.title = L"Refresh";
            refItem.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(refItem);

            FlyoutItem exitItem;
            exitItem.action = FlyoutItemAction::Exit;
            exitItem.icon = FlyoutIconType::ExitPower;
            exitItem.title = L"Exit";
            exitItem.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(exitItem);
        }
        break;
    }

    case FlyoutView::SelectPhonePortView: {
        // Port selector when multiple ports are running
        FlyoutItem backItem;
        backItem.action = FlyoutItemAction::BackToMain;
        backItem.icon = FlyoutIconType::BackArrow;
        backItem.title = L"Select Port for Mobile QR";
        backItem.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(backItem);

        FlyoutItem sep;
        sep.action = FlyoutItemAction::Separator;
        sep.rect = { 6, currentY, width - 6, currentY + 8 };
        currentY += 9;
        m_flyoutItems.push_back(sep);

        for (size_t idx : devIndices) {
            const auto& p = m_activeMenuPorts[idx];
            FlyoutItem item;
            item.action = FlyoutItemAction::SelectPhonePort;
            item.portIndex = idx;
            item.icon = FlyoutIconType::Phone;
            item.portText = std::to_wstring(p.port);
            
            std::wstring name = !p.projectName.empty() ? p.projectName : (!p.framework.empty() ? p.framework : p.processName);
            item.title = name + L" (:" + std::to_wstring(p.port) + L")";
            item.subtitle = p.lanUrl;
            item.hasChevron = true;
            item.extraData = p.lanUrl;
            item.rect = { 6, currentY, width - 6, currentY + 48 };
            currentY += 50;
            m_flyoutItems.push_back(item);
        }
        break;
    }

    case FlyoutView::TestOnPhone: {
        FlyoutItem backItem;
        backItem.action = FlyoutItemAction::BackToMain;
        backItem.icon = FlyoutIconType::BackArrow;
        backItem.title = L"Test on Phone";
        backItem.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(backItem);

        FlyoutItem sep;
        sep.action = FlyoutItemAction::Separator;
        sep.rect = { 6, currentY, width - 6, currentY + 8 };
        currentY += 9;
        m_flyoutItems.push_back(sep);

        if (devIndices.empty()) {
            m_currentQr.size = 0;
            m_currentQr.modules.clear();
            m_qrRect = { 0, 0, 0, 0 };

            FlyoutItem emptyTitle;
            emptyTitle.action = FlyoutItemAction::None;
            emptyTitle.icon = FlyoutIconType::Dot;
            emptyTitle.title = L"No Active Dev Servers";
            emptyTitle.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(emptyTitle);

            FlyoutItem emptyHelp;
            emptyHelp.action = FlyoutItemAction::None;
            emptyHelp.title = L"Start a dev server (e.g. npm run dev)";
            emptyHelp.rect = { 6, currentY, width - 6, currentY + 28 };
            currentY += 30;
            m_flyoutItems.push_back(emptyHelp);
            break;
        }

        if (devIndices.size() > 1) {
            backItem.action = FlyoutItemAction::BackToSelectPhonePort;
            backItem.title = L"Port " + std::to_wstring(m_phoneTargetPort) + L" QR (Change)";
            m_flyoutItems[0] = backItem;
        }

        if (m_phoneTargetLanUrl.empty() || m_phoneTargetPort == 0) {
            m_phoneTargetPort = m_activeMenuPorts[devIndices[0]].port;
            m_phoneTargetLanUrl = LanUtil::FormatLanUrl(m_phoneTargetPort);
        }

        FlyoutItem copyLan;
        copyLan.action = FlyoutItemAction::CopyLanUrl;
        copyLan.icon = FlyoutIconType::CopyLink;
        copyLan.title = L"Copy: " + m_phoneTargetLanUrl;
        copyLan.extraData = m_phoneTargetLanUrl;
        copyLan.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 40;
        m_flyoutItems.push_back(copyLan);

        // Generate standard ISO/IEC 18004 QR code
        m_currentQr = QrCode::Generate(WideToUtf8(m_phoneTargetLanUrl));
        int modSize = 7;
        int qrPixelSize = m_currentQr.size * modSize;
        int qrX = (width - qrPixelSize) / 2;
        m_qrRect = { qrX, currentY + 20, qrX + qrPixelSize, currentY + 20 + qrPixelSize };
        currentY += qrPixelSize + 48;

        FlyoutItem info;
        info.action = FlyoutItemAction::None;
        info.title = L"Scan with camera on same Wi-Fi (Port " + std::to_wstring(m_phoneTargetPort) + L")";
        info.rect = { 6, currentY, width - 6, currentY + 28 };
        currentY += 30;
        m_flyoutItems.push_back(info);
        break;
    }

    case FlyoutView::SystemBackground: {
        FlyoutItem backItem;
        backItem.action = FlyoutItemAction::BackToMain;
        backItem.icon = FlyoutIconType::BackArrow;
        backItem.title = L"System Background (" + std::to_wstring(totalSysCount) + L")";
        backItem.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(backItem);

        FlyoutItem sep;
        sep.action = FlyoutItemAction::Separator;
        sep.rect = { 6, currentY, width - 6, currentY + 8 };
        currentY += 9;
        m_flyoutItems.push_back(sep);

        // 1. IDE & Language Servers Category
        FlyoutItem ideCat;
        ideCat.action = FlyoutItemAction::NavToSysIdeTools;
        ideCat.icon = FlyoutIconType::IdeTools;
        ideCat.title = L"IDE & Language Servers (" + std::to_wstring(sysIdeIndices.size()) + L")";
        ideCat.hasChevron = true;
        ideCat.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(ideCat);

        // 2. Windows System Services Category
        FlyoutItem winCat;
        winCat.action = FlyoutItemAction::NavToSysWinServices;
        winCat.icon = FlyoutIconType::WinServices;
        winCat.title = L"Windows System Services (" + std::to_wstring(sysWinIndices.size()) + L")";
        winCat.hasChevron = true;
        winCat.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(winCat);

        // 3. Background Internal Ports Category
        FlyoutItem internalCat;
        internalCat.action = FlyoutItemAction::NavToSysInternal;
        internalCat.icon = FlyoutIconType::InternalBox;
        internalCat.title = L"Background Internal Ports (" + std::to_wstring(sysInternalIndices.size()) + L")";
        internalCat.hasChevron = true;
        internalCat.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(internalCat);
        break;
    }

    case FlyoutView::SysIdeTools: {
        FlyoutItem backItem;
        backItem.action = FlyoutItemAction::BackToSystemBackground;
        backItem.icon = FlyoutIconType::BackArrow;
        backItem.title = L"IDE & Language Servers (" + std::to_wstring(sysIdeIndices.size()) + L")";
        backItem.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(backItem);

        FlyoutItem sep;
        sep.action = FlyoutItemAction::Separator;
        sep.rect = { 6, currentY, width - 6, currentY + 8 };
        currentY += 9;
        m_flyoutItems.push_back(sep);

        for (size_t idx : sysIdeIndices) {
            const auto& p = m_activeMenuPorts[idx];
            FlyoutItem item;
            item.action = FlyoutItemAction::OpenPort;
            item.portIndex = idx;
            item.icon = FlyoutIconType::Dot;
            item.portText = std::to_wstring(p.port);
            item.title = p.processName;
            if (p.memoryMb > 0) {
                item.rightText = std::to_wstring(p.memoryMb) + L" MB";
            }
            item.rect = { 6, currentY, width - 6, currentY + 34 };
            currentY += 36;
            m_flyoutItems.push_back(item);
        }
        break;
    }

    case FlyoutView::SysWinServices: {
        FlyoutItem backItem;
        backItem.action = FlyoutItemAction::BackToSystemBackground;
        backItem.icon = FlyoutIconType::BackArrow;
        backItem.title = L"Windows System Services (" + std::to_wstring(sysWinIndices.size()) + L")";
        backItem.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(backItem);

        FlyoutItem sep;
        sep.action = FlyoutItemAction::Separator;
        sep.rect = { 6, currentY, width - 6, currentY + 8 };
        currentY += 9;
        m_flyoutItems.push_back(sep);

        for (size_t idx : sysWinIndices) {
            const auto& p = m_activeMenuPorts[idx];
            FlyoutItem item;
            item.action = FlyoutItemAction::OpenPort;
            item.portIndex = idx;
            item.icon = FlyoutIconType::Dot;
            item.portText = std::to_wstring(p.port);
            
            std::wstring desc = p.processName;
            if (p.port == 135) desc = L"RPC Endpoint Mapper";
            else if (p.port == 139) desc = L"NetBIOS Session";
            else if (p.port == 445) desc = L"SMB File Sharing";
            else if (p.port == 5357) desc = L"WSDAPI Web Services";

            item.title = desc;
            item.rect = { 6, currentY, width - 6, currentY + 34 };
            currentY += 36;
            m_flyoutItems.push_back(item);
        }
        break;
    }

    case FlyoutView::SysInternal: {
        FlyoutItem backItem;
        backItem.action = FlyoutItemAction::BackToSystemBackground;
        backItem.icon = FlyoutIconType::BackArrow;
        backItem.title = L"Background Internal (" + std::to_wstring(sysInternalIndices.size()) + L")";
        backItem.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(backItem);

        FlyoutItem sep;
        sep.action = FlyoutItemAction::Separator;
        sep.rect = { 6, currentY, width - 6, currentY + 8 };
        currentY += 9;
        m_flyoutItems.push_back(sep);

        for (size_t idx : sysInternalIndices) {
            const auto& p = m_activeMenuPorts[idx];
            FlyoutItem item;
            item.action = FlyoutItemAction::OpenPort;
            item.portIndex = idx;
            item.icon = FlyoutIconType::Dot;
            item.portText = std::to_wstring(p.port);
            item.title = p.processName;
            if (p.memoryMb > 0) {
                item.rightText = std::to_wstring(p.memoryMb) + L" MB";
            }
            item.rect = { 6, currentY, width - 6, currentY + 34 };
            currentY += 36;
            m_flyoutItems.push_back(item);
        }
        break;
    }

    case FlyoutView::PublicTunnel: {
        FlyoutItem backItem;
        backItem.action = FlyoutItemAction::BackToMain;
        backItem.icon = FlyoutIconType::BackArrow;
        backItem.title = L"Public Tunnel Bridge";
        backItem.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(backItem);

        FlyoutItem sep;
        sep.action = FlyoutItemAction::Separator;
        sep.rect = { 6, currentY, width - 6, currentY + 8 };
        currentY += 9;
        m_flyoutItems.push_back(sep);

        uint16_t targetPort = 3000;
        for (const auto& p : m_activeMenuPorts) {
            if (p.isDevServer) {
                targetPort = p.port;
                break;
            }
        }

        auto status = TunnelManager::GetStatus();
        if (status.isRunning) {
            FlyoutItem tunStatus;
            tunStatus.action = FlyoutItemAction::None;
            tunStatus.icon = FlyoutIconType::Dot;
            tunStatus.title = L"Tunnel Active: " + status.publicUrl;
            tunStatus.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(tunStatus);

            FlyoutItem stopTun;
            stopTun.action = FlyoutItemAction::StopActiveTunnel;
            stopTun.icon = FlyoutIconType::StopProcessSquare;
            stopTun.title = L"Stop Active Tunnel";
            stopTun.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(stopTun);
        } else {
            FlyoutItem startCf;
            startCf.action = FlyoutItemAction::StartCloudflareTunnel;
            startCf.icon = FlyoutIconType::Globe;
            startCf.title = L"Start Cloudflare Quick Tunnel (:" + std::to_wstring(targetPort) + L")";
            startCf.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(startCf);

            FlyoutItem copyCf;
            copyCf.action = FlyoutItemAction::CopyTunnelCliCommand;
            copyCf.icon = FlyoutIconType::CopyLink;
            copyCf.title = L"Copy Cloudflare CLI Command";
            copyCf.extraData = TunnelManager::GetCloudflareCliCommand(targetPort);
            copyCf.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(copyCf);

            FlyoutItem copyLt;
            copyLt.action = FlyoutItemAction::CopyTunnelCliCommand;
            copyLt.icon = FlyoutIconType::CopyLink;
            copyLt.title = L"Copy Localtunnel npx Command";
            copyLt.extraData = TunnelManager::GetLocaltunnelCliCommand(targetPort);
            copyLt.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(copyLt);
        }
        break;
    }

    case FlyoutView::PortAliases: {
        FlyoutItem backItem;
        backItem.action = FlyoutItemAction::BackToMain;
        backItem.icon = FlyoutIconType::BackArrow;
        backItem.title = L"Custom Port Aliases (.portpeek)";
        backItem.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(backItem);

        FlyoutItem sep;
        sep.action = FlyoutItemAction::Separator;
        sep.rect = { 6, currentY, width - 6, currentY + 8 };
        currentY += 9;
        m_flyoutItems.push_back(sep);

        auto aliases = AliasManager::GetAllAliases();
        for (const auto& [pNum, name] : aliases) {
            FlyoutItem item;
            item.action = FlyoutItemAction::None;
            item.icon = FlyoutIconType::Pin;
            item.portText = std::to_wstring(pNum);
            item.title = std::to_wstring(pNum) + L" ➔ " + name;
            item.rect = { 6, currentY, width - 6, currentY + 34 };
            currentY += 36;
            m_flyoutItems.push_back(item);
        }

        FlyoutItem tipItem;
        tipItem.action = FlyoutItemAction::None;
        tipItem.title = L"Add .portpeek in workspace to customize";
        tipItem.rect = { 6, currentY, width - 6, currentY + 28 };
        currentY += 30;
        m_flyoutItems.push_back(tipItem);
        break;
    }

    case FlyoutView::DeveloperTools: {
        FlyoutItem backItem;
        backItem.action = FlyoutItemAction::BackToMain;
        backItem.icon = FlyoutIconType::BackArrow;
        backItem.title = L"Developer Tools";
        backItem.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(backItem);

        FlyoutItem sep;
        sep.action = FlyoutItemAction::Separator;
        sep.rect = { 6, currentY, width - 6, currentY + 8 };
        currentY += 9;
        m_flyoutItems.push_back(sep);

        for (size_t idx : devIndices) {
            const auto& p = m_activeMenuPorts[idx];
            if (!p.projectFolder.empty()) {
                FlyoutItem vsItem;
                vsItem.action = FlyoutItemAction::OpenInVsCode;
                vsItem.icon = FlyoutIconType::VsCode;
                vsItem.title = L"Open " + p.projectName + L" in VS Code";
                vsItem.targetFolder = p.projectFolder;
                vsItem.rect = { 6, currentY, width - 6, currentY + 36 };
                currentY += 38;
                m_flyoutItems.push_back(vsItem);

                FlyoutItem termItem;
                termItem.action = FlyoutItemAction::OpenInTerminal;
                termItem.icon = FlyoutIconType::Terminal;
                termItem.title = L"Open " + p.projectName + L" in Terminal";
                termItem.targetFolder = p.projectFolder;
                termItem.rect = { 6, currentY, width - 6, currentY + 36 };
                currentY += 38;
                m_flyoutItems.push_back(termItem);
            }
        }

        FlyoutItem mdItem;
        mdItem.action = FlyoutItemAction::CopyAllMarkdown;
        mdItem.icon = FlyoutIconType::CopyLink;
        mdItem.title = L"Copy All Ports as Markdown";
        mdItem.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(mdItem);

        FlyoutItem killAll;
        killAll.action = FlyoutItemAction::KillAllDev;
        killAll.icon = FlyoutIconType::StopProcessSquare;
        killAll.title = L"Stop All Dev Servers";
        killAll.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(killAll);
        break;
    }

    case FlyoutView::StopProcess: {
        FlyoutItem backItem;
        backItem.action = FlyoutItemAction::BackToMain;
        backItem.icon = FlyoutIconType::BackArrow;
        backItem.title = L"Free Up Port (EADDRINUSE)";
        backItem.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(backItem);

        FlyoutItem sep;
        sep.action = FlyoutItemAction::Separator;
        sep.rect = { 6, currentY, width - 6, currentY + 8 };
        currentY += 9;
        m_flyoutItems.push_back(sep);

        if (!devIndices.empty()) {
            for (size_t idx : devIndices) {
                const auto& p = m_activeMenuPorts[idx];
                FlyoutItem item;
                item.action = FlyoutItemAction::KillSinglePort;
                item.icon = FlyoutIconType::StopProcessSquare;
                item.targetPid = p.pid;
                item.title = L"Kill :" + std::to_wstring(p.port) + L" — " + p.processName;
                item.rightText = L"PID " + std::to_wstring(p.pid);
                item.rect = { 6, currentY, width - 6, currentY + 36 };
                currentY += 38;
                m_flyoutItems.push_back(item);
            }

            FlyoutItem killAll;
            killAll.action = FlyoutItemAction::KillAllDev;
            killAll.icon = FlyoutIconType::StopProcessSquare;
            killAll.title = L"Kill All Node / Dev Processes";
            killAll.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(killAll);
        } else {
            FlyoutItem item;
            item.action = FlyoutItemAction::None;
            item.title = L"No conflicting dev processes";
            item.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(item);
        }
        break;
    }

    case FlyoutView::Preferences: {
        FlyoutItem backItem;
        backItem.action = FlyoutItemAction::BackToMain;
        backItem.icon = FlyoutIconType::BackArrow;
        backItem.title = L"Preferences";
        backItem.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(backItem);

        FlyoutItem sep;
        sep.action = FlyoutItemAction::Separator;
        sep.rect = { 6, currentY, width - 6, currentY + 8 };
        currentY += 9;
        m_flyoutItems.push_back(sep);

        bool isAuto = ConfigManager::IsAutoStartEnabled();
        FlyoutItem autoItem;
        autoItem.action = FlyoutItemAction::ToggleAutoStart;
        autoItem.icon = FlyoutIconType::Checkmark;
        autoItem.isChecked = isAuto;
        autoItem.title = L"Start with Windows";
        autoItem.rightText = isAuto ? L"On" : L"Off";
        autoItem.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(autoItem);

        bool isHotkey = ConfigManager::IsGlobalHotkeyEnabled();
        FlyoutItem hotkeyItem;
        hotkeyItem.action = FlyoutItemAction::ToggleHotkey;
        hotkeyItem.icon = FlyoutIconType::Checkmark;
        hotkeyItem.isChecked = isHotkey;
        hotkeyItem.title = L"Global Hotkey (Win+Alt+P)";
        hotkeyItem.rightText = isHotkey ? L"On" : L"Off";
        hotkeyItem.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(hotkeyItem);
        break;
    }

    case FlyoutView::EpheSessionsView: {
        // 1. Back Header
        FlyoutItem backItem;
        backItem.action = FlyoutItemAction::BackToMain;
        backItem.icon = FlyoutIconType::BackArrow;
        if (!m_activeEpheSessions.empty()) {
            backItem.title = L"Ephe Sessions (" + std::to_wstring(m_activeEpheSessions.size()) + L")";
        } else {
            backItem.title = L"Ephe Sessions";
        }
        backItem.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(backItem);

        FlyoutItem sep;
        sep.action = FlyoutItemAction::Separator;
        sep.rect = { 6, currentY, width - 6, currentY + 8 };
        currentY += 9;
        m_flyoutItems.push_back(sep);

        // 2. Active Ephe Sessions
        if (!m_activeEpheSessions.empty()) {
            for (const auto& sess : m_activeEpheSessions) {
                FlyoutItem item;
                item.action = FlyoutItemAction::OpenEpheSession;
                item.icon = FlyoutIconType::Dot;
                item.portText = std::to_wstring(sess.port);
                item.title = sess.sessionName;
                item.subtitle = sess.applicationName;
                if (sess.connectedDevices > 0) {
                    item.subtitle += L" • " + std::to_wstring(sess.connectedDevices) + L" device" + (sess.connectedDevices > 1 ? L"s" : L"");
                }
                item.extraData = sess.GetLaunchUrl();
                item.hasChevron = true;
                item.rect = { 6, currentY, width - 6, currentY + 48 };
                currentY += 50;
                m_flyoutItems.push_back(item);
            }
        } else {
            FlyoutItem emptyItem;
            emptyItem.action = FlyoutItemAction::None;
            emptyItem.icon = FlyoutIconType::Dot;
            emptyItem.title = L"No active sessions";
            emptyItem.rect = { 6, currentY, width - 6, currentY + 36 };
            currentY += 38;
            m_flyoutItems.push_back(emptyItem);
        }

        // 3. Actions Separator
        FlyoutItem botSep;
        botSep.action = FlyoutItemAction::Separator;
        botSep.rect = { 6, currentY, width - 6, currentY + 8 };
        currentY += 9;
        m_flyoutItems.push_back(botSep);

        // 4. New Session
        FlyoutItem newItem;
        newItem.action = FlyoutItemAction::LaunchNewEpheSession;
        newItem.icon = FlyoutIconType::Plus;
        newItem.title = L"New Session";
        newItem.hasChevron = false;
        newItem.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(newItem);

        // 5. Open All Sessions
        FlyoutItem openAll;
        openAll.action = FlyoutItemAction::OpenAllEpheSessions;
        openAll.icon = FlyoutIconType::Globe;
        if (!m_activeEpheSessions.empty()) {
            openAll.title = L"⚡ Open All Sessions (" + std::to_wstring(m_activeEpheSessions.size()) + L")";
        } else {
            openAll.title = L"Open All Sessions";
        }
        openAll.hasChevron = false;
        openAll.rect = { 6, currentY, width - 6, currentY + 36 };
        currentY += 38;
        m_flyoutItems.push_back(openAll);
        break;
    }
    }

    // Resize window if open while keeping locked position above system tray
    if (m_hFlyoutWnd) {
        int totalHeight = currentY + 6;
        
        HWND hTrayWnd = FindWindowW(L"Shell_TrayWnd", nullptr);
        HMONITOR hMonitor = hTrayWnd ? MonitorFromWindow(hTrayWnd, MONITOR_DEFAULTTOPRIMARY) : MonitorFromWindow(m_hFlyoutWnd, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi = { sizeof(MONITORINFO) };
        GetMonitorInfoW(hMonitor, &mi);

        int posX = mi.rcWork.right - width - 12;
        int posY = mi.rcWork.bottom - totalHeight - 12;

        SetWindowPos(m_hFlyoutWnd, HWND_TOPMOST, posX, posY, width, totalHeight, SWP_NOACTIVATE);
    }
}

void TrayManager::ShowFlyout(int x, int y) {
    (void)x; (void)y;
    HideFlyout();

    m_activeMenuPorts = EnumerateListeningTcpPorts();
    m_activeEpheSessions = AppDiscoveryManager::Instance().DiscoverAllVerifiedSessions();
    m_currentView = FlyoutView::Main;
    RebuildFlyoutLayout();

    const int width = 360;
    int totalHeight = 0;
    if (!m_flyoutItems.empty()) {
        totalHeight = m_flyoutItems.back().rect.bottom + 6;
    } else {
        totalHeight = 200;
    }

    HWND hTrayWnd = FindWindowW(L"Shell_TrayWnd", nullptr);
    HMONITOR hMonitor = hTrayWnd ? MonitorFromWindow(hTrayWnd, MONITOR_DEFAULTTOPRIMARY) : MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi = { sizeof(MONITORINFO) };
    GetMonitorInfoW(hMonitor, &mi);

    int posX = mi.rcWork.right - width - 12;
    int posY = mi.rcWork.bottom - totalHeight - 12;

    m_hFlyoutWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        FLYOUT_WINDOW_CLASS,
        L"PortPeek Popover",
        WS_POPUP,
        posX, posY, width, totalHeight,
        m_hWnd,
        nullptr,
        m_hInstance,
        nullptr
    );

    if (!m_hFlyoutWnd) return;

    DWORD corner = 2;
    DwmSetWindowAttribute(m_hFlyoutWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

    BOOL isDark = TRUE;
    DwmSetWindowAttribute(m_hFlyoutWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &isDark, sizeof(isDark));

    ShowWindow(m_hFlyoutWnd, SW_SHOW);
    SetForegroundWindow(m_hFlyoutWnd);
    SetFocus(m_hFlyoutWnd);
}

void TrayManager::HideFlyout() {
    if (m_hFlyoutWnd) {
        DestroyWindow(m_hFlyoutWnd);
        m_hFlyoutWnd = nullptr;
    }
    m_hoveredIndex = -1;
    m_isMouseTracking = false;
    m_currentView = FlyoutView::Main;
}

void TrayManager::HandleItemClick(const FlyoutItem& item) {
    switch (item.action) {
    case FlyoutItemAction::OpenPort: {
        if (item.portIndex < m_activeMenuPorts.size()) {
            OpenPortInBrowser(m_activeMenuPorts[item.portIndex].port, m_activeMenuPorts[item.portIndex].protocol);
        }
        HideFlyout();
        break;
    }

    case FlyoutItemAction::BackToMain: {
        m_currentView = FlyoutView::Main;
        RebuildFlyoutLayout();
        InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        break;
    }

    case FlyoutItemAction::BackToSystemBackground: {
        m_currentView = FlyoutView::SystemBackground;
        RebuildFlyoutLayout();
        InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        break;
    }

    case FlyoutItemAction::NavToSystemBackground: {
        m_currentView = FlyoutView::SystemBackground;
        RebuildFlyoutLayout();
        InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        break;
    }

    case FlyoutItemAction::NavToSysIdeTools: {
        m_currentView = FlyoutView::SysIdeTools;
        RebuildFlyoutLayout();
        InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        break;
    }

    case FlyoutItemAction::NavToSysWinServices: {
        m_currentView = FlyoutView::SysWinServices;
        RebuildFlyoutLayout();
        InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        break;
    }

    case FlyoutItemAction::NavToSysInternal: {
        m_currentView = FlyoutView::SysInternal;
        RebuildFlyoutLayout();
        InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        break;
    }

    case FlyoutItemAction::BackToSelectPhonePort: {
        m_currentView = FlyoutView::SelectPhonePortView;
        RebuildFlyoutLayout();
        InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        break;
    }

    case FlyoutItemAction::NavToTestOnPhone: {
        std::vector<size_t> devIdx;
        for (size_t i = 0; i < m_activeMenuPorts.size(); ++i) {
            if (m_activeMenuPorts[i].isDevServer) devIdx.push_back(i);
        }
        if (devIdx.size() > 1) {
            m_currentView = FlyoutView::SelectPhonePortView;
        } else if (devIdx.size() == 1) {
            m_phoneTargetPort = m_activeMenuPorts[devIdx[0]].port;
            m_phoneTargetLanUrl = m_activeMenuPorts[devIdx[0]].lanUrl;
            m_currentView = FlyoutView::TestOnPhone;
        } else {
            m_phoneTargetPort = 0;
            m_phoneTargetLanUrl = L"";
            m_currentView = FlyoutView::TestOnPhone;
        }
        RebuildFlyoutLayout();
        InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        break;
    }

    case FlyoutItemAction::SelectPhonePort: {
        if (item.portIndex < m_activeMenuPorts.size()) {
            m_phoneTargetPort = m_activeMenuPorts[item.portIndex].port;
            m_phoneTargetLanUrl = m_activeMenuPorts[item.portIndex].lanUrl;
        }
        m_currentView = FlyoutView::TestOnPhone;
        RebuildFlyoutLayout();
        InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        break;
    }

    case FlyoutItemAction::NavToPublicTunnel: {
        m_currentView = FlyoutView::PublicTunnel;
        RebuildFlyoutLayout();
        InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        break;
    }

    case FlyoutItemAction::NavToPortAliases: {
        m_currentView = FlyoutView::PortAliases;
        RebuildFlyoutLayout();
        InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        break;
    }

    case FlyoutItemAction::NavToEpheSessions: {
        m_currentView = FlyoutView::EpheSessionsView;
        RebuildFlyoutLayout();
        InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        break;
    }

    case FlyoutItemAction::OpenEpheSession: {
        if (!item.extraData.empty()) {
            ShellExecuteW(nullptr, L"open", item.extraData.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
        HideFlyout();
        break;
    }

    case FlyoutItemAction::OpenAllEpheSessions: {
        for (const auto& sess : m_activeEpheSessions) {
            std::wstring launchUrl = sess.GetLaunchUrl();
            if (!launchUrl.empty()) {
                ShellExecuteW(nullptr, L"open", launchUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                Sleep(100);
            }
        }
        HideFlyout();
        break;
    }

    case FlyoutItemAction::LaunchNewEpheSession: {
        EpheLauncher::LaunchNewSession();
        HideFlyout();
        SetTimer(m_hWnd, REFRESH_FEEDBACK_TIMER_ID, 800, nullptr);
        break;
    }

    case FlyoutItemAction::NavToDeveloperTools: {
        m_currentView = FlyoutView::DeveloperTools;
        RebuildFlyoutLayout();
        InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        break;
    }

    case FlyoutItemAction::NavToStopProcess: {
        m_currentView = FlyoutView::StopProcess;
        RebuildFlyoutLayout();
        InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        break;
    }

    case FlyoutItemAction::NavToPreferences: {
        m_currentView = FlyoutView::Preferences;
        RebuildFlyoutLayout();
        InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        break;
    }

    case FlyoutItemAction::CopyUrlDirect: {
        if (!m_activeMenuPorts.empty()) {
            for (const auto& p : m_activeMenuPorts) {
                if (p.isDevServer) {
                    std::wstring url = p.protocol + L"://localhost:" + std::to_wstring(p.port);
                    CopyTextToClipboard(m_hWnd, url);
                    break;
                }
            }
        }
        HideFlyout();
        break;
    }

    case FlyoutItemAction::CopyLanUrl: {
        if (!item.extraData.empty()) {
            CopyTextToClipboard(m_hWnd, item.extraData);
        }
        HideFlyout();
        break;
    }

    case FlyoutItemAction::CopyTunnelCliCommand: {
        if (!item.extraData.empty()) {
            CopyTextToClipboard(m_hWnd, item.extraData);
        }
        HideFlyout();
        break;
    }

    case FlyoutItemAction::StartCloudflareTunnel: {
        uint16_t targetPort = 3000;
        for (const auto& p : m_activeMenuPorts) {
            if (p.isDevServer) { targetPort = p.port; break; }
        }
        TunnelManager::StartQuickTunnel(m_hWnd, targetPort);
        RebuildFlyoutLayout();
        InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        break;
    }

    case FlyoutItemAction::StopActiveTunnel: {
        TunnelManager::StopActiveTunnel();
        RebuildFlyoutLayout();
        InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        break;
    }

    case FlyoutItemAction::CopyAllMarkdown: {
        std::wstring md = GeneratePortsMarkdown(m_activeMenuPorts);
        CopyTextToClipboard(m_hWnd, md);
        HideFlyout();
        break;
    }

    case FlyoutItemAction::OpenInVsCode: {
        if (!item.targetFolder.empty()) {
            ProcessUtil::OpenProjectInEditor(item.targetFolder, L"code");
        }
        HideFlyout();
        break;
    }

    case FlyoutItemAction::OpenInTerminal: {
        if (!item.targetFolder.empty()) {
            ProcessUtil::OpenProjectInTerminal(item.targetFolder);
        }
        HideFlyout();
        break;
    }

    case FlyoutItemAction::KillSinglePort: {
        if (item.targetPid > 4) {
            ProcessUtil::TerminateProcessByPid(item.targetPid);
            Sleep(150);
            m_activeMenuPorts = EnumerateListeningTcpPorts();
            SyncTrayState();
            RebuildFlyoutLayout();
            InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        }
        break;
    }

    case FlyoutItemAction::KillAllDev: {
        for (const auto& p : m_activeMenuPorts) {
            if (p.isDevServer && p.pid > 4) {
                ProcessUtil::TerminateProcessByPid(p.pid);
            }
        }
        Sleep(150);
        m_activeMenuPorts = EnumerateListeningTcpPorts();
        SyncTrayState();
        HideFlyout();
        break;
    }

    case FlyoutItemAction::ToggleAutoStart: {
        bool current = ConfigManager::IsAutoStartEnabled();
        ConfigManager::SetAutoStartEnabled(!current);
        RebuildFlyoutLayout();
        InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        break;
    }

    case FlyoutItemAction::ToggleHotkey: {
        bool current = ConfigManager::IsGlobalHotkeyEnabled();
        ConfigManager::SetGlobalHotkeyEnabled(!current);
        if (!current) {
            RegisterGlobalHotkey();
        } else {
            UnregisterGlobalHotkey();
        }
        RebuildFlyoutLayout();
        InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
        break;
    }

    case FlyoutItemAction::Refresh: {
        m_activeMenuPorts = EnumerateListeningTcpPorts();
        m_activeEpheSessions = AppDiscoveryManager::Instance().DiscoverAllVerifiedSessions();
        SyncTrayState();
        m_justRefreshed = true;
        RebuildFlyoutLayout();
        if (m_hFlyoutWnd) {
            InvalidateRect(m_hFlyoutWnd, nullptr, TRUE);
            UpdateWindow(m_hFlyoutWnd);
        }
        SetTimer(m_hWnd, REFRESH_FEEDBACK_TIMER_ID, 1200, nullptr);
        break;
    }

    case FlyoutItemAction::Exit: {
        HideFlyout();
        PostQuitMessage(0);
        break;
    }

    default:
        break;
    }
}

// ----------------------------------------------------------------------------
// FLYOUT WINDOW PROCEDURE (Custom Paint & Interaction)
// ----------------------------------------------------------------------------

LRESULT CALLBACK TrayManager::FlyoutWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    TrayManager& mgr = TrayManager::Instance();

    switch (uMsg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT clientRc;
        GetClientRect(hWnd, &clientRc);
        int width = clientRc.right - clientRc.left;
        int height = clientRc.bottom - clientRc.top;

        // Double buffer
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBM = CreateCompatibleBitmap(hdc, width, height);
        HGDIOBJ oldBM = SelectObject(memDC, memBM);

        SetBkMode(memDC, TRANSPARENT);

        // 1. Background Fluent Dark Surface (#1a1a1e)
        HBRUSH hBgBrush = CreateSolidBrush(RGB(26, 26, 30));
        FillRect(memDC, &clientRc, hBgBrush);
        DeleteObject(hBgBrush);

        // Subtle 1px Outer Border (#2d2d34)
        HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(45, 45, 52));
        HBRUSH hNullBrush = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
        HGDIOBJ oldPen = SelectObject(memDC, hBorderPen);
        HGDIOBJ oldBrush = SelectObject(memDC, hNullBrush);
        RoundRect(memDC, 0, 0, width, height, 12, 12);
        SelectObject(memDC, oldPen);
        SelectObject(memDC, oldBrush);
        DeleteObject(hBorderPen);

        // Colors
        COLORREF colWhite = RGB(245, 245, 245);
        COLORREF colMuted = RGB(148, 163, 184); // #94a3b8
        COLORREF colGrey  = RGB(113, 113, 122); // #71717a
        COLORREF colSep   = RGB(45, 45, 52);

        // 2. Draw QR Code if in TestOnPhone view
        if (mgr.m_currentView == FlyoutView::TestOnPhone && mgr.m_currentQr.size > 0 && mgr.m_qrRect.right > 0) {
            // White card background with 28px quiet zone (4 modules minimum)
            int quietPad = 28;
            RECT qrBg = { mgr.m_qrRect.left - quietPad, mgr.m_qrRect.top - quietPad, mgr.m_qrRect.right + quietPad, mgr.m_qrRect.bottom + quietPad };
            HBRUSH hQrBg = CreateSolidBrush(RGB(255, 255, 255));
            FillRect(memDC, &qrBg, hQrBg);
            DeleteObject(hQrBg);

            HBRUSH hBlackBrush = CreateSolidBrush(RGB(0, 0, 0));
            int modSize = (mgr.m_qrRect.right - mgr.m_qrRect.left) / mgr.m_currentQr.size;
            for (int qy = 0; qy < mgr.m_currentQr.size; ++qy) {
                for (int qx = 0; qx < mgr.m_currentQr.size; ++qx) {
                    if (mgr.m_currentQr.Get(qx, qy)) {
                        RECT modRc = {
                            mgr.m_qrRect.left + (qx * modSize),
                            mgr.m_qrRect.top + (qy * modSize),
                            mgr.m_qrRect.left + ((qx + 1) * modSize),
                            mgr.m_qrRect.top + ((qy + 1) * modSize)
                        };
                        FillRect(memDC, &modRc, hBlackBrush);
                    }
                }
            }
            DeleteObject(hBlackBrush);
        }

        // Draw each item
        for (int i = 0; i < static_cast<int>(mgr.m_flyoutItems.size()); ++i) {
            const auto& item = mgr.m_flyoutItems[i];
            bool isHovered = (i == mgr.m_hoveredIndex);

            // Hover Background
            if (isHovered && item.action != FlyoutItemAction::Header && item.action != FlyoutItemAction::Separator && item.action != FlyoutItemAction::None) {
                HBRUSH hSelBrush = CreateSolidBrush(RGB(45, 45, 52));
                HPEN hSelPen = CreatePen(PS_SOLID, 1, RGB(55, 55, 62));
                HGDIOBJ oB = SelectObject(memDC, hSelBrush);
                HGDIOBJ oP = SelectObject(memDC, hSelPen);

                RoundRect(memDC, item.rect.left, item.rect.top, item.rect.right, item.rect.bottom, 8, 8);

                SelectObject(memDC, oB);
                SelectObject(memDC, oP);
                DeleteObject(hSelBrush);
                DeleteObject(hSelPen);
            }

            int itemH = item.rect.bottom - item.rect.top;
            int itemCenterY = item.rect.top + (itemH / 2);

            switch (item.action) {
            case FlyoutItemAction::Header: {
                HFONT hHdrFont = CreateSegoeFont(10, FW_SEMIBOLD);
                HGDIOBJ oF = SelectObject(memDC, hHdrFont);
                SetTextColor(memDC, item.isHeaderActive ? colWhite : colMuted);

                RECT txtRc = { item.rect.left + 10, item.rect.top, item.rect.right - 30, item.rect.bottom };
                DrawTextW(memDC, item.title.c_str(), -1, &txtRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                // Right Live Dot (Functional Green)
                if (item.isHeaderActive) {
                    HBRUSH hDotBrush = CreateSolidBrush(RGB(34, 197, 94));
                    HPEN hNullPen = static_cast<HPEN>(GetStockObject(NULL_PEN));
                    HGDIOBJ oDotB = SelectObject(memDC, hDotBrush);
                    HGDIOBJ oDotP = SelectObject(memDC, hNullPen);

                    Ellipse(memDC, item.rect.right - 20, itemCenterY - 4, item.rect.right - 12, itemCenterY + 4);

                    SelectObject(memDC, oDotB);
                    SelectObject(memDC, oDotP);
                    DeleteObject(hDotBrush);
                }

                SelectObject(memDC, oF);
                DeleteObject(hHdrFont);
                break;
            }

            case FlyoutItemAction::BackToMain:
            case FlyoutItemAction::BackToSystemBackground:
            case FlyoutItemAction::BackToSelectPhonePort: {
                // Standard Back arrow glyph ‹ (Segoe Icon E72B)
                HFONT hIconFont = CreateSegoeIconFont(11);
                HGDIOBJ oIF = SelectObject(memDC, hIconFont);
                SetTextColor(memDC, colWhite);

                RECT iconRc = { item.rect.left + 10, item.rect.top, item.rect.left + 26, item.rect.bottom };
                DrawTextW(memDC, L"\uE72B", -1, &iconRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                SelectObject(memDC, oIF);
                DeleteObject(hIconFont);

                HFONT hTitleFont = CreateSegoeFont(10, FW_SEMIBOLD);
                HGDIOBJ oF = SelectObject(memDC, hTitleFont);
                SetTextColor(memDC, colWhite);

                RECT titleRc = { item.rect.left + 30, item.rect.top, item.rect.right - 10, item.rect.bottom };
                DrawTextW(memDC, item.title.c_str(), -1, &titleRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                SelectObject(memDC, oF);
                DeleteObject(hTitleFont);
                break;
            }

            case FlyoutItemAction::OpenEpheSession:
            case FlyoutItemAction::SelectPhonePort:
            case FlyoutItemAction::OpenPort: {
                // Status Dot (Dynamic Latency/Health Color)
                {
                    COLORREF dotCol = RGB(34, 197, 94); // Healthy Green
                    if (item.health == NetProbe::PortHealth::Slow) {
                        dotCol = RGB(234, 179, 8); // Slow Yellow
                    } else if (item.health == NetProbe::PortHealth::Hung) {
                        dotCol = RGB(239, 68, 68); // Hung Red
                    }

                    HBRUSH hDotBrush = CreateSolidBrush(dotCol);
                    HPEN hNullPen = static_cast<HPEN>(GetStockObject(NULL_PEN));
                    HGDIOBJ oDotB = SelectObject(memDC, hDotBrush);
                    HGDIOBJ oDotP = SelectObject(memDC, hNullPen);

                    Ellipse(memDC, item.rect.left + 12, itemCenterY - 4, item.rect.left + 21, itemCenterY + 5);

                    SelectObject(memDC, oDotB);
                    SelectObject(memDC, oDotP);
                    DeleteObject(hDotBrush);
                }

                // Bold Port Number
                {
                    HFONT hPortFont = CreateSegoeFont(11, FW_BOLD);
                    HGDIOBJ oF = SelectObject(memDC, hPortFont);
                    SetTextColor(memDC, colWhite);

                    RECT portRc = { item.rect.left + 30, item.rect.top, item.rect.left + 85, item.rect.bottom };
                    DrawTextW(memDC, item.portText.c_str(), -1, &portRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                    SelectObject(memDC, oF);
                    DeleteObject(hPortFont);
                }

                // Title (Line 1)
                {
                    HFONT hTitleFont = CreateSegoeFont(10, FW_SEMIBOLD);
                    HGDIOBJ oF = SelectObject(memDC, hTitleFont);
                    SetTextColor(memDC, colWhite);

                    RECT titleRc = { item.rect.left + 90, item.rect.top + 6, item.rect.right - 30, item.rect.top + 24 };
                    DrawTextW(memDC, item.title.c_str(), -1, &titleRc, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);

                    SelectObject(memDC, oF);
                    DeleteObject(hTitleFont);
                }

                // Subtitle (Line 2)
                {
                    HFONT hSubFont = CreateSegoeFont(9, FW_NORMAL);
                    HGDIOBJ oF = SelectObject(memDC, hSubFont);
                    SetTextColor(memDC, colGrey);

                    RECT subRc = { item.rect.left + 90, item.rect.top + 26, item.rect.right - 30, item.rect.bottom - 4 };
                    DrawTextW(memDC, item.subtitle.c_str(), -1, &subRc, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);

                    SelectObject(memDC, oF);
                    DeleteObject(hSubFont);
                }

                // Right Chevron › (Segoe Icon E76C)
                if (item.hasChevron) {
                    HFONT hIconFont = CreateSegoeIconFont(9);
                    HGDIOBJ oIF = SelectObject(memDC, hIconFont);
                    SetTextColor(memDC, colGrey);

                    RECT chevRc = { item.rect.right - 26, item.rect.top, item.rect.right - 10, item.rect.bottom };
                    DrawTextW(memDC, L"\uE76C", -1, &chevRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                    SelectObject(memDC, oIF);
                    DeleteObject(hIconFont);
                }
                break;
            }

            case FlyoutItemAction::NavToEpheSessions:
            case FlyoutItemAction::NavToSystemBackground: {
                // Left Grey Dot / App Glyph
                if (item.icon == FlyoutIconType::EpheApp) {
                    HFONT hIconFont = CreateSegoeIconFont(11);
                    HGDIOBJ oIF = SelectObject(memDC, hIconFont);
                    SetTextColor(memDC, colMuted);

                    RECT iconRc = { item.rect.left + 10, item.rect.top, item.rect.left + 28, item.rect.bottom };
                    DrawTextW(memDC, L"\uE8A5", -1, &iconRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                    SelectObject(memDC, oIF);
                    DeleteObject(hIconFont);
                } else {
                    HBRUSH hDotBrush = CreateSolidBrush(colGrey);
                    HPEN hNullPen = static_cast<HPEN>(GetStockObject(NULL_PEN));
                    HGDIOBJ oDotB = SelectObject(memDC, hDotBrush);
                    HGDIOBJ oDotP = SelectObject(memDC, hNullPen);

                    Ellipse(memDC, item.rect.left + 12, itemCenterY - 4, item.rect.left + 21, itemCenterY + 5);

                    SelectObject(memDC, oDotB);
                    SelectObject(memDC, oDotP);
                    DeleteObject(hDotBrush);
                }

                // Title
                {
                    HFONT hTitleFont = CreateSegoeFont(10, FW_SEMIBOLD);
                    HGDIOBJ oF = SelectObject(memDC, hTitleFont);
                    SetTextColor(memDC, colWhite);

                    RECT titleRc = { item.rect.left + 30, item.rect.top, item.rect.right - 30, item.rect.bottom };
                    DrawTextW(memDC, item.title.c_str(), -1, &titleRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                    SelectObject(memDC, oF);
                    DeleteObject(hTitleFont);
                }

                // Right Chevron ›
                if (item.hasChevron) {
                    HFONT hIconFont = CreateSegoeIconFont(9);
                    HGDIOBJ oIF = SelectObject(memDC, hIconFont);
                    SetTextColor(memDC, colGrey);

                    RECT chevRc = { item.rect.right - 26, item.rect.top, item.rect.right - 10, item.rect.bottom };
                    DrawTextW(memDC, L"\uE76C", -1, &chevRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                    SelectObject(memDC, oIF);
                    DeleteObject(hIconFont);
                }
                break;
            }

            case FlyoutItemAction::Separator: {
                HPEN hPen = CreatePen(PS_SOLID, 1, colSep);
                HGDIOBJ oP = SelectObject(memDC, hPen);

                MoveToEx(memDC, item.rect.left + 8, itemCenterY, nullptr);
                LineTo(memDC, item.rect.right - 8, itemCenterY);

                SelectObject(memDC, oP);
                DeleteObject(hPen);
                break;
            }

            default: {
                // Action Glyph via Segoe Fluent Icons / Segoe MDL2 Assets
                const wchar_t* glyphStr = nullptr;
                switch (item.icon) {
                case FlyoutIconType::EpheApp:           glyphStr = L"\uE8A5"; break; // Ephe Document
                case FlyoutIconType::Plus:              glyphStr = L"\uE710"; break; // Plus / Add / New
                case FlyoutIconType::CopyLink:          glyphStr = L"\uE71B"; break; // Link icon
                case FlyoutIconType::Phone:             glyphStr = L"\uE8EA"; break; // Mobile Phone
                case FlyoutIconType::Globe:             glyphStr = L"\uE774"; break; // Globe / Public Network
                case FlyoutIconType::Pin:               glyphStr = L"\uE718"; break; // Pin / Alias
                case FlyoutIconType::IdeTools:          glyphStr = L"\uE943"; break; // IDE / Code
                case FlyoutIconType::WinServices:       glyphStr = L"\uE770"; break; // Windows Services / Devices
                case FlyoutIconType::InternalBox:       glyphStr = L"\uE8B7"; break; // Internal box
                case FlyoutIconType::DevToolsCode:        glyphStr = L"\uE943"; break; // Code </>
                case FlyoutIconType::StopProcessSquare:   glyphStr = L"\uF78A"; break; // Stop box
                case FlyoutIconType::PreferencesGear:     glyphStr = L"\uE713"; break; // Settings gear
                case FlyoutIconType::RefreshLoop:         glyphStr = L"\uE72C"; break; // Refresh loop
                case FlyoutIconType::ExitPower:           glyphStr = L"\uE7E8"; break; // Power button
                case FlyoutIconType::Checkmark:           glyphStr = item.isChecked ? L"\uE73E" : L""; break; // Checkmark
                case FlyoutIconType::Terminal:            glyphStr = L"\uE756"; break; // Terminal
                case FlyoutIconType::VsCode:              glyphStr = L"\uE943"; break; // VS Code
                case FlyoutIconType::Dot:                 glyphStr = L"\uE915"; break; // Dot
                default: break;
                }

                if (glyphStr && glyphStr[0] != L'\0') {
                    HFONT hIconFont = CreateSegoeIconFont(11);
                    HGDIOBJ oIF = SelectObject(memDC, hIconFont);
                    SetTextColor(memDC, colMuted);

                    RECT iconRc = { item.rect.left + 10, item.rect.top, item.rect.left + 28, item.rect.bottom };
                    DrawTextW(memDC, glyphStr, -1, &iconRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                    SelectObject(memDC, oIF);
                    DeleteObject(hIconFont);
                }

                // Title Text
                {
                    HFONT hTitleFont = CreateSegoeFont(10, (item.action == FlyoutItemAction::None) ? FW_NORMAL : FW_SEMIBOLD);
                    HGDIOBJ oF = SelectObject(memDC, hTitleFont);
                    SetTextColor(memDC, (item.action == FlyoutItemAction::None) ? colMuted : colWhite);

                    RECT titleRc = { item.rect.left + 34, item.rect.top, item.rect.right - 40, item.rect.bottom };
                    DrawTextW(memDC, item.title.c_str(), -1, &titleRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

                    SelectObject(memDC, oF);
                    DeleteObject(hTitleFont);
                }

                // Right Chevron (if applicable)
                if (item.hasChevron) {
                    HFONT hIconFont = CreateSegoeIconFont(9);
                    HGDIOBJ oIF = SelectObject(memDC, hIconFont);
                    SetTextColor(memDC, colGrey);

                    RECT chevRc = { item.rect.right - 26, item.rect.top, item.rect.right - 10, item.rect.bottom };
                    DrawTextW(memDC, L"\uE76C", -1, &chevRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                    SelectObject(memDC, oIF);
                    DeleteObject(hIconFont);
                } else if (!item.rightText.empty()) {
                    HFONT hRightFont = CreateSegoeFont(9, FW_NORMAL);
                    HGDIOBJ oF = SelectObject(memDC, hRightFont);
                    SetTextColor(memDC, colGrey);

                    RECT rightRc = { item.rect.right - 80, item.rect.top, item.rect.right - 14, item.rect.bottom };
                    DrawTextW(memDC, item.rightText.c_str(), -1, &rightRc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

                    SelectObject(memDC, oF);
                    DeleteObject(hRightFont);
                }
                break;
            }
            }
        }

        BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBM);
        DeleteObject(memBM);
        DeleteDC(memDC);

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE: {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

        if (!mgr.m_isMouseTracking) {
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hWnd;
            TrackMouseEvent(&tme);
            mgr.m_isMouseTracking = true;
        }

        int newHover = -1;
        for (int i = 0; i < static_cast<int>(mgr.m_flyoutItems.size()); ++i) {
            if (PtInRect(&mgr.m_flyoutItems[i].rect, pt)) {
                if (mgr.m_flyoutItems[i].action != FlyoutItemAction::Header &&
                    mgr.m_flyoutItems[i].action != FlyoutItemAction::Separator &&
                    mgr.m_flyoutItems[i].action != FlyoutItemAction::None) {
                    newHover = i;
                }
                break;
            }
        }

        if (newHover != mgr.m_hoveredIndex) {
            mgr.m_hoveredIndex = newHover;
            InvalidateRect(hWnd, nullptr, FALSE);
        }

        if (newHover >= 0) {
            SetCursor(LoadCursor(nullptr, IDC_HAND));
        } else {
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
        }
        return 0;
    }

    case WM_MOUSELEAVE: {
        mgr.m_isMouseTracking = false;
        if (mgr.m_hoveredIndex != -1) {
            mgr.m_hoveredIndex = -1;
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        SetCapture(hWnd);
        return 0;
    }

    case WM_LBUTTONUP: {
        ReleaseCapture();
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

        RECT clientRc;
        GetClientRect(hWnd, &clientRc);
        if (!PtInRect(&clientRc, pt)) {
            mgr.HideFlyout();
            return 0;
        }

        for (const auto& item : mgr.m_flyoutItems) {
            if (PtInRect(&item.rect, pt)) {
                mgr.HandleItemClick(item);
                break;
            }
        }
        return 0;
    }

    case WM_ACTIVATE: {
        if (LOWORD(wParam) == WA_INACTIVE) {
            mgr.HideFlyout();
        }
        return 0;
    }

    case WM_KEYDOWN: {
        if (wParam == VK_ESCAPE) {
            if (mgr.m_currentView == FlyoutView::SysIdeTools ||
                mgr.m_currentView == FlyoutView::SysWinServices ||
                mgr.m_currentView == FlyoutView::SysInternal) {
                mgr.m_currentView = FlyoutView::SystemBackground;
                mgr.RebuildFlyoutLayout();
                InvalidateRect(hWnd, nullptr, TRUE);
            } else if (mgr.m_currentView == FlyoutView::TestOnPhone && mgr.m_activeMenuPorts.size() > 1) {
                mgr.m_currentView = FlyoutView::SelectPhonePortView;
                mgr.RebuildFlyoutLayout();
                InvalidateRect(hWnd, nullptr, TRUE);
            } else if (mgr.m_currentView != FlyoutView::Main) {
                mgr.m_currentView = FlyoutView::Main;
                mgr.RebuildFlyoutLayout();
                InvalidateRect(hWnd, nullptr, TRUE);
            } else {
                mgr.HideFlyout();
            }
            return 0;
        } else if (wParam == VK_F5) {
            mgr.m_activeMenuPorts = EnumerateListeningTcpPorts();
            mgr.SyncTrayState();
            mgr.m_justRefreshed = true;
            mgr.RebuildFlyoutLayout();
            InvalidateRect(hWnd, nullptr, TRUE);
            SetTimer(mgr.m_hWnd, REFRESH_FEEDBACK_TIMER_ID, 1200, nullptr);
            return 0;
        }
        break;
    }

    default:
        break;
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

// ----------------------------------------------------------------------------
// MESSAGE WINDOW PROCEDURE
// ----------------------------------------------------------------------------

LRESULT CALLBACK TrayManager::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    TrayManager& mgr = TrayManager::Instance();

    if (uMsg != 0 && uMsg == mgr.m_wmTaskbarCreated) {
        mgr.AddTrayIcon();
        return 0;
    }

    switch (uMsg) {
    case WM_TIMER: {
        if (wParam == TRAY_RETRY_TIMER_ID) {
            mgr.AddTrayIcon();
        } else if (wParam == TRAY_SYNC_TIMER_ID) {
            mgr.SyncTrayState();
        } else if (wParam == REFRESH_FEEDBACK_TIMER_ID) {
            KillTimer(hWnd, REFRESH_FEEDBACK_TIMER_ID);
            mgr.m_justRefreshed = false;
            if (mgr.m_hFlyoutWnd) {
                mgr.RebuildFlyoutLayout();
                InvalidateRect(mgr.m_hFlyoutWnd, nullptr, TRUE);
            }
        }
        return 0;
    }

    case WM_HOTKEY: {
        if (wParam == GLOBAL_HOTKEY_ID) {
            POINT pt;
            GetCursorPos(&pt);
            mgr.ShowFlyout(pt.x, pt.y);
        }
        return 0;
    }

    case WM_TRAYICON: {
        UINT eventMsg = LOWORD(lParam);
        switch (eventMsg) {
        case WM_CONTEXTMENU:
        case NIN_SELECT:
        case NIN_KEYSELECT:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP: {
            int x = GET_X_LPARAM(wParam);
            int y = GET_Y_LPARAM(wParam);
            mgr.ShowFlyout(x, y);
            break;
        }
        }
        return 0;
    }

    case WM_SETTINGCHANGE: {
        if (lParam && wcscmp(reinterpret_cast<LPCWSTR>(lParam), L"ImmersiveColorSet") == 0) {
            ThemeManager::Instance().OnThemeChanged(hWnd);
        }
        return 0;
    }

    case WM_THEMECHANGED: {
        ThemeManager::Instance().OnThemeChanged(hWnd);
        return 0;
    }

    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }

    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}
