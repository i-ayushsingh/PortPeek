#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <cstdint>

namespace TunnelManager {

struct TunnelState {
    bool isRunning = false;
    uint16_t port = 0;
    std::wstring publicUrl;
    std::wstring provider;
    HANDLE hProcess = nullptr;
};

// Returns current tunnel status
TunnelState GetStatus();

// Spawns a Cloudflare Quick Tunnel (or npx localtunnel) for a port
bool StartQuickTunnel(HWND hWnd, uint16_t port);

// Stops the active background tunnel
void StopActiveTunnel();

// Formats the CLI command string for quick copy
std::wstring GetCloudflareCliCommand(uint16_t port);
std::wstring GetLocaltunnelCliCommand(uint16_t port);
std::wstring GetNgrokCliCommand(uint16_t port);

} // namespace TunnelManager
