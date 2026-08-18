#include "tunnel.h"
#include <sstream>
#include <thread>
#include <atomic>
#include <vector>

namespace TunnelManager {

namespace {

TunnelState g_state;

} // namespace

TunnelState GetStatus() {
    if (g_state.isRunning && g_state.hProcess) {
        DWORD exitCode = 0;
        if (GetExitCodeProcess(g_state.hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
            CloseHandle(g_state.hProcess);
            g_state.hProcess = nullptr;
            g_state.isRunning = false;
            g_state.publicUrl.clear();
        }
    }
    return g_state;
}

std::wstring GetCloudflareCliCommand(uint16_t port) {
    return L"cloudflared tunnel --url http://localhost:" + std::to_wstring(port);
}

std::wstring GetLocaltunnelCliCommand(uint16_t port) {
    return L"npx --yes localtunnel --port " + std::to_wstring(port);
}

std::wstring GetNgrokCliCommand(uint16_t port) {
    return L"ngrok http " + std::to_wstring(port);
}

void StopActiveTunnel() {
    if (g_state.hProcess) {
        TerminateProcess(g_state.hProcess, 0);
        CloseHandle(g_state.hProcess);
        g_state.hProcess = nullptr;
    }
    g_state.isRunning = false;
    g_state.publicUrl.clear();
    g_state.port = 0;
}

bool StartQuickTunnel(HWND hWnd, uint16_t port) {
    (void)hWnd;
    StopActiveTunnel();

    // Spawn cloudflared or localtunnel in background
    std::wstring cmd = L"cmd.exe /c start /b cloudflared tunnel --url http://localhost:" + std::to_wstring(port);

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};
    std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
    cmdBuf.push_back(L'\0');

    BOOL ok = CreateProcessW(
        nullptr,
        cmdBuf.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (ok) {
        CloseHandle(pi.hThread);
        g_state.isRunning = true;
        g_state.port = port;
        g_state.provider = L"Cloudflare Quick Tunnel";
        g_state.hProcess = pi.hProcess;
        g_state.publicUrl = L"https://trycloudflare.com (Connecting...)";
        return true;
    }

    return false;
}

} // namespace TunnelManager
