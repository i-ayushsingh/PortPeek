#include "tray.h"
#include "resource.h"
#include "ports.h"
#include "process.h"
#include "theme.h"
#include "config.h"

#include <windowsx.h>
#include <string>
#include <vector>

namespace {
const wchar_t WINDOW_CLASS_NAME[] = L"PortPeek_Hidden_Msg_Class";
const wchar_t WINDOW_TITLE[]      = L"PortPeek";

const UINT_PTR TRAY_RETRY_TIMER_ID = 101;
const UINT TRAY_RETRY_INTERVAL_MS = 1500;

const UINT_PTR TRAY_SYNC_TIMER_ID = 102;
const UINT TRAY_SYNC_INTERVAL_MS = 3000;

const int GLOBAL_HOTKEY_ID = 9001;

#define IDM_OPEN_ALL             20004
#define IDM_COPY_MARKDOWN        20005
#define IDM_STOP_ALL_DEV         20006
#define IDM_TOGGLE_AUTOSTART     20007
#define IDM_TOGGLE_HOTKEY        20008

#define IDM_ACTION_OPEN_BASE     30000
#define IDM_ACTION_COPY_BASE     32000
#define IDM_ACTION_VSCODE_BASE   34000
#define IDM_ACTION_TERMINAL_BASE 36000
#define IDM_ACTION_KILL_BASE     38000
#define IDM_ACTION_RANGE_MAX     40000

UINT CalculateOptimalMenuFlags(POINT pt, const MONITORINFO& mi) {
    UINT flags = TPM_RIGHTBUTTON;

    const RECT& mon = mi.rcMonitor;
    const RECT& work = mi.rcWork;

    bool taskbarAtBottom = (work.bottom < mon.bottom);
    bool taskbarAtTop    = (work.top > mon.top);
    bool taskbarAtLeft   = (work.left > mon.left);
    bool taskbarAtRight  = (work.right < mon.right);

    if (taskbarAtTop) {
        flags |= TPM_TOPALIGN;
    } else if (taskbarAtBottom) {
        flags |= TPM_BOTTOMALIGN;
    } else {
        int midY = (work.top + work.bottom) / 2;
        flags |= (pt.y > midY) ? TPM_BOTTOMALIGN : TPM_TOPALIGN;
    }

    if (taskbarAtLeft) {
        flags |= TPM_LEFTALIGN;
    } else if (taskbarAtRight) {
        flags |= TPM_RIGHTALIGN;
    } else {
        int midX = (work.left + work.right) / 2;
        flags |= (pt.x > midX) ? TPM_RIGHTALIGN : TPM_LEFTALIGN;
    }

    return flags;
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

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon         = m_hIcon;

    if (!RegisterClassExW(&wc)) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
    }

    m_hWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        WINDOW_CLASS_NAME,
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

    // Set background sync timer for live tooltip update (every 3 seconds)
    SetTimer(m_hWnd, TRAY_SYNC_TIMER_ID, TRAY_SYNC_INTERVAL_MS, nullptr);

    // Register global hotkey if enabled
    if (ConfigManager::IsGlobalHotkeyEnabled()) {
        RegisterGlobalHotkey();
    }

    return true;
}

void TrayManager::Shutdown() {
    UnregisterGlobalHotkey();

    if (m_hWnd) {
        KillTimer(m_hWnd, TRAY_RETRY_TIMER_ID);
        KillTimer(m_hWnd, TRAY_SYNC_TIMER_ID);
    }

    RemoveTrayIcon();

    if (m_hWnd) {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }

    if (m_hInstance) {
        UnregisterClassW(WINDOW_CLASS_NAME, m_hInstance);
    }
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
    nid.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon            = m_hIcon;
    wcsncpy_s(nid.szTip, L"PortPeek", _TRUNCATE);

    if (!Shell_NotifyIconW(NIM_ADD, &nid)) {
        nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
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

void TrayManager::ShowContextMenu(int x, int y) {
    if (!m_hWnd) return;

    ThemeManager::Instance().ApplyTheme(m_hWnd);

    auto listeningPorts = EnumerateListeningTcpPorts();
    m_activeMenuPorts = listeningPorts;

    std::vector<size_t> devIndices;
    std::vector<size_t> sysIndices;
    for (size_t i = 0; i < listeningPorts.size(); ++i) {
        if (listeningPorts[i].isDevServer) {
            devIndices.push_back(i);
        } else {
            sysIndices.push_back(i);
        }
    }

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    UINT pos = 0;

    // Header item
    std::wstring headerText = L"PortPeek";
    if (!devIndices.empty()) {
        headerText += L" (" + std::to_wstring(devIndices.size()) + L" active)";
    }
    InsertMenuW(hMenu, pos++, MF_BYPOSITION | MF_STRING | MF_DISABLED | MF_GRAYED, IDM_HEADER, headerText.c_str());
    InsertMenuW(hMenu, pos++, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);

    // 1. Main Direct-Click Dev Servers (1-Click launches URL in default browser)
    if (!devIndices.empty()) {
        for (size_t idx : devIndices) {
            const auto& p = listeningPorts[idx];

            std::wstring itemText = std::to_wstring(p.port) + L"\t";
            if (!p.pageTitle.empty()) {
                itemText += p.projectName + L" - \"" + p.pageTitle + L"\"";
            } else if (!p.projectName.empty() && p.projectName != p.processName) {
                itemText += p.projectName + L" (" + p.processName + L")";
            } else {
                itemText += p.processName;
            }

            if (!p.framework.empty() && p.projectName.find(p.framework) == std::wstring::npos) {
                itemText += L" [" + p.framework + L"]";
            }

            if (p.memoryMb > 0) {
                itemText += L" (" + std::to_wstring(p.memoryMb) + L" MB)";
            }

            InsertMenuW(hMenu, pos++, MF_BYPOSITION | MF_STRING, IDM_ACTION_OPEN_BASE + static_cast<WORD>(idx), itemText.c_str());
        }

        // Multi-port launch shortcut when 2+ dev servers are active
        if (devIndices.size() >= 2) {
            std::wstring openAllText = L"Open All Active (" + std::to_wstring(devIndices.size()) + L")";
            InsertMenuW(hMenu, pos++, MF_BYPOSITION | MF_STRING, IDM_OPEN_ALL, openAllText.c_str());
        }
    } else if (listeningPorts.empty()) {
        InsertMenuW(hMenu, pos++, MF_BYPOSITION | MF_STRING | MF_DISABLED | MF_GRAYED, IDM_NO_PORTS, L"No listening ports");
    } else {
        InsertMenuW(hMenu, pos++, MF_BYPOSITION | MF_STRING | MF_DISABLED | MF_GRAYED, IDM_NO_PORTS, L"No active dev servers");
    }

    // 2. Submenu for System & Background Ports
    if (!sysIndices.empty()) {
        HMENU hSubMenu = CreatePopupMenu();
        if (hSubMenu) {
            UINT subPos = 0;
            for (size_t idx : sysIndices) {
                const auto& p = listeningPorts[idx];
                std::wstring itemText = std::to_wstring(p.port) + L"\t" + p.processName;
                if (p.memoryMb > 0) {
                    itemText += L" (" + std::to_wstring(p.memoryMb) + L" MB)";
                }
                InsertMenuW(hSubMenu, subPos++, MF_BYPOSITION | MF_STRING, IDM_ACTION_OPEN_BASE + static_cast<WORD>(idx), itemText.c_str());
            }

            std::wstring subMenuTitle = L"System & Background (" + std::to_wstring(sysIndices.size()) + L")";
            InsertMenuW(hMenu, pos++, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(hSubMenu), subMenuTitle.c_str());
        }
    }

    // 3. Secondary Actions & Developer Tools
    if (!devIndices.empty()) {
        InsertMenuW(hMenu, pos++, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);

        // Copy URL Submenu
        HMENU hCopySub = CreatePopupMenu();
        if (hCopySub) {
            UINT subPos = 0;
            for (size_t idx : devIndices) {
                const auto& p = listeningPorts[idx];
                std::wstring copyText = p.protocol + L"://localhost:" + std::to_wstring(p.port);
                InsertMenuW(hCopySub, subPos++, MF_BYPOSITION | MF_STRING, IDM_ACTION_COPY_BASE + static_cast<WORD>(idx), copyText.c_str());
            }
            InsertMenuW(hCopySub, subPos++, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
            InsertMenuW(hCopySub, subPos++, MF_BYPOSITION | MF_STRING, IDM_COPY_MARKDOWN, L"Copy All as Markdown");

            InsertMenuW(hMenu, pos++, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(hCopySub), L"Copy URL");
        }

        // Developer Tools Submenu (Open VS Code / Terminal)
        HMENU hDevTools = CreatePopupMenu();
        if (hDevTools) {
            UINT toolPos = 0;
            for (size_t idx : devIndices) {
                const auto& p = listeningPorts[idx];
                if (!p.projectFolder.empty()) {
                    std::wstring vsCodeText = L"Open " + p.projectName + L" in VS Code";
                    InsertMenuW(hDevTools, toolPos++, MF_BYPOSITION | MF_STRING, IDM_ACTION_VSCODE_BASE + static_cast<WORD>(idx), vsCodeText.c_str());

                    std::wstring termText = L"Open " + p.projectName + L" in Terminal";
                    InsertMenuW(hDevTools, toolPos++, MF_BYPOSITION | MF_STRING, IDM_ACTION_TERMINAL_BASE + static_cast<WORD>(idx), termText.c_str());
                }
            }

            if (toolPos > 0) {
                InsertMenuW(hDevTools, toolPos++, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
            }

            InsertMenuW(hDevTools, toolPos++, MF_BYPOSITION | MF_STRING, IDM_COPY_MARKDOWN, L"Export Ports as Markdown");
            InsertMenuW(hDevTools, toolPos++, MF_BYPOSITION | MF_STRING, IDM_STOP_ALL_DEV, L"Stop All Dev Servers");

            InsertMenuW(hMenu, pos++, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(hDevTools), L"Developer Tools");
        }

        // Stop Process Submenu
        HMENU hKillSub = CreatePopupMenu();
        if (hKillSub) {
            UINT subPos = 0;
            for (size_t idx : devIndices) {
                const auto& p = listeningPorts[idx];
                std::wstring killText = std::to_wstring(p.port) + L" - " + p.processName + L" (PID " + std::to_wstring(p.pid) + L")";
                InsertMenuW(hKillSub, subPos++, MF_BYPOSITION | MF_STRING, IDM_ACTION_KILL_BASE + static_cast<WORD>(idx), killText.c_str());
            }
            InsertMenuW(hKillSub, subPos++, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
            InsertMenuW(hKillSub, subPos++, MF_BYPOSITION | MF_STRING, IDM_STOP_ALL_DEV, L"Stop All Dev Servers");

            InsertMenuW(hMenu, pos++, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(hKillSub), L"Stop Process");
        }
    }

    // 4. Preferences Submenu
    HMENU hPrefSub = CreatePopupMenu();
    if (hPrefSub) {
        UINT prefPos = 0;
        bool isAutoStart = ConfigManager::IsAutoStartEnabled();
        UINT autoStartFlags = MF_BYPOSITION | MF_STRING | (isAutoStart ? MF_CHECKED : MF_UNCHECKED);
        InsertMenuW(hPrefSub, prefPos++, autoStartFlags, IDM_TOGGLE_AUTOSTART, L"Start with Windows");

        bool isHotkey = ConfigManager::IsGlobalHotkeyEnabled();
        UINT hotkeyFlags = MF_BYPOSITION | MF_STRING | (isHotkey ? MF_CHECKED : MF_UNCHECKED);
        InsertMenuW(hPrefSub, prefPos++, hotkeyFlags, IDM_TOGGLE_HOTKEY, L"Global Hotkey (Win+Alt+P)");

        InsertMenuW(hMenu, pos++, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(hPrefSub), L"Preferences");
    }

    InsertMenuW(hMenu, pos++, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
    InsertMenuW(hMenu, pos++, MF_BYPOSITION | MF_STRING, IDM_REFRESH, L"Refresh");
    InsertMenuW(hMenu, pos++, MF_BYPOSITION | MF_STRING, IDM_EXIT, L"Exit");

    // 5. Pinpoint Accurate Positioning
    POINT pt;
    if (x > 0 && y > 0) {
        pt.x = x;
        pt.y = y;
    } else {
        GetCursorPos(&pt);
    }

    HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(MONITORINFO) };
    if (GetMonitorInfoW(hMonitor, &mi)) {
        if (pt.x < mi.rcWork.left + 2)  pt.x = mi.rcWork.left + 2;
        if (pt.x > mi.rcWork.right - 2) pt.x = mi.rcWork.right - 2;
        if (pt.y < mi.rcWork.top + 2)   pt.y = mi.rcWork.top + 2;
        if (pt.y > mi.rcWork.bottom - 2) pt.y = mi.rcWork.bottom - 2;
    }

    UINT flags = CalculateOptimalMenuFlags(pt, mi);

    SetForegroundWindow(m_hWnd);

    TrackPopupMenuEx(
        hMenu,
        flags,
        pt.x,
        pt.y,
        m_hWnd,
        nullptr
    );

    DestroyMenu(hMenu);

    PostMessageW(m_hWnd, WM_NULL, 0, 0);
}

void TrayManager::HandleCommand(WORD commandId) {
    if (commandId == IDM_EXIT) {
        PostQuitMessage(0);
        return;
    }

    if (commandId == IDM_REFRESH) {
        SyncTrayState();
        return;
    }

    if (commandId == IDM_OPEN_ALL) {
        OpenAllPortsInBrowser(m_activeMenuPorts);
        return;
    }

    if (commandId == IDM_COPY_MARKDOWN) {
        std::wstring md = GeneratePortsMarkdown(m_activeMenuPorts);
        CopyTextToClipboard(m_hWnd, md);
        return;
    }

    if (commandId == IDM_STOP_ALL_DEV) {
        for (const auto& p : m_activeMenuPorts) {
            if (p.isDevServer && p.pid > 4) {
                ProcessUtil::TerminateProcessByPid(p.pid);
            }
        }
        Sleep(100);
        SyncTrayState();
        return;
    }

    if (commandId == IDM_TOGGLE_AUTOSTART) {
        bool current = ConfigManager::IsAutoStartEnabled();
        ConfigManager::SetAutoStartEnabled(!current);
        return;
    }

    if (commandId == IDM_TOGGLE_HOTKEY) {
        bool current = ConfigManager::IsGlobalHotkeyEnabled();
        ConfigManager::SetGlobalHotkeyEnabled(!current);
        if (!current) {
            RegisterGlobalHotkey();
        } else {
            UnregisterGlobalHotkey();
        }
        return;
    }

    // Action: Open in Browser
    if (commandId >= IDM_ACTION_OPEN_BASE && commandId < IDM_ACTION_COPY_BASE) {
        size_t idx = commandId - IDM_ACTION_OPEN_BASE;
        if (idx < m_activeMenuPorts.size()) {
            OpenPortInBrowser(m_activeMenuPorts[idx].port, m_activeMenuPorts[idx].protocol);
        }
        return;
    }

    // Action: Copy URL
    if (commandId >= IDM_ACTION_COPY_BASE && commandId < IDM_ACTION_VSCODE_BASE) {
        size_t idx = commandId - IDM_ACTION_COPY_BASE;
        if (idx < m_activeMenuPorts.size()) {
            std::wstring url = m_activeMenuPorts[idx].protocol + L"://localhost:" + std::to_wstring(m_activeMenuPorts[idx].port);
            CopyTextToClipboard(m_hWnd, url);
        }
        return;
    }

    // Action: Open in VS Code
    if (commandId >= IDM_ACTION_VSCODE_BASE && commandId < IDM_ACTION_TERMINAL_BASE) {
        size_t idx = commandId - IDM_ACTION_VSCODE_BASE;
        if (idx < m_activeMenuPorts.size()) {
            const auto& folder = m_activeMenuPorts[idx].projectFolder;
            if (!folder.empty()) {
                ProcessUtil::OpenProjectInEditor(folder, L"code");
            }
        }
        return;
    }

    // Action: Open in Terminal
    if (commandId >= IDM_ACTION_TERMINAL_BASE && commandId < IDM_ACTION_KILL_BASE) {
        size_t idx = commandId - IDM_ACTION_TERMINAL_BASE;
        if (idx < m_activeMenuPorts.size()) {
            const auto& folder = m_activeMenuPorts[idx].projectFolder;
            if (!folder.empty()) {
                ProcessUtil::OpenProjectInTerminal(folder);
            }
        }
        return;
    }

    // Action: Stop / Kill Process
    if (commandId >= IDM_ACTION_KILL_BASE && commandId < IDM_ACTION_RANGE_MAX) {
        size_t idx = commandId - IDM_ACTION_KILL_BASE;
        if (idx < m_activeMenuPorts.size()) {
            DWORD pid = m_activeMenuPorts[idx].pid;
            ProcessUtil::TerminateProcessByPid(pid);
            Sleep(100);
            SyncTrayState();
        }
        return;
    }
}

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
        }
        return 0;
    }

    case WM_HOTKEY: {
        if (wParam == GLOBAL_HOTKEY_ID) {
            POINT pt;
            GetCursorPos(&pt);
            mgr.ShowContextMenu(pt.x, pt.y);
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
            mgr.ShowContextMenu(x, y);
            break;
        }
        }
        return 0;
    }

    case WM_COMMAND: {
        WORD cmdId = LOWORD(wParam);
        mgr.HandleCommand(cmdId);
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
