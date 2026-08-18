#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include "probe.h"

struct ListeningPort {
    uint16_t port = 0;
    DWORD pid = 0;
    std::wstring processName;        // Executable name (e.g. "node.exe")
    std::wstring projectName;        // Extracted project/script name (e.g. "PortPeak", "server.js")
    std::wstring projectFolder;      // Full directory path if available
    std::wstring pageTitle;          // Probed HTML <title>
    std::wstring serverHeader;       // HTTP Server header
    std::wstring framework;          // Detected framework (e.g. "Next.js", "FastAPI", "Vite", "Docker")
    std::wstring commandLine;        // Full command line
    std::wstring protocol = L"http"; // "http" or "https"
    std::wstring customAlias;        // User or workspace alias (e.g. "Main Postgres DB")
    std::wstring lanUrl;             // Local Wi-Fi URL (e.g. "http://192.168.1.45:3000")
    size_t memoryMb = 0;             // Process RAM working set in MB
    int latencyMs = 0;               // Measured socket RTT in ms
    NetProbe::PortHealth health = NetProbe::PortHealth::Healthy;
    bool isDevServer = false;        // True if classified as active developer/web server
};

// Determines whether a process / port combination represents a development/web server
bool IsDevelopmentServer(uint16_t port, const std::wstring& processName, DWORD pid);

// Enumerates all local TCP ports, extracts process metadata, probes HTTP titles, and sorts ascending
std::vector<ListeningPort> EnumerateListeningTcpPorts();

// Opens http(s)://localhost:<port> in the user's default browser
bool OpenPortInBrowser(uint16_t port, const std::wstring& protocol = L"http");

// Opens all active dev ports in separate browser tabs
void OpenAllPortsInBrowser(const std::vector<ListeningPort>& ports);

// Copies text to the Windows Clipboard
bool CopyTextToClipboard(HWND hWnd, const std::wstring& text);

// Generates a clean markdown summary of active ports
std::wstring GeneratePortsMarkdown(const std::vector<ListeningPort>& ports);
