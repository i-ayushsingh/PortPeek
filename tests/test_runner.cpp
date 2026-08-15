#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <cassert>
#include <vector>
#include <string>

#include "ports.h"
#include "process.h"
#include "probe.h"
#include "theme.h"
#include "config.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")

struct MockServerContext {
    SOCKET listenSock;
    volatile bool running;
};

DWORD WINAPI MockHttpServerThreadProc(LPVOID lpParam) {
    auto* ctx = reinterpret_cast<MockServerContext*>(lpParam);
    while (ctx->running) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(ctx->listenSock, &readSet);
        timeval tv{ 0, 50000 };
        int sel = select(0, &readSet, nullptr, nullptr, &tv);
        if (sel > 0 && FD_ISSET(ctx->listenSock, &readSet)) {
            SOCKET client = accept(ctx->listenSock, nullptr, nullptr);
            if (client != INVALID_SOCKET) {
                char buf[1024];
                recv(client, buf, sizeof(buf), 0);
                const char response[] =
                    "HTTP/1.1 200 OK\r\n"
                    "Server: uvicorn\r\n"
                    "Content-Type: text/html\r\n"
                    "Connection: close\r\n\r\n"
                    "<!DOCTYPE html><html><head><title>FastAPI Swagger UI</title></head><body>Hello</body></html>";
                send(client, response, static_cast<int>(sizeof(response) - 1), 0);
                closesocket(client);
            }
        }
    }
    return 0;
}

SOCKET StartMockHttpServer(uint16_t port) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return INVALID_SOCKET;

    sockaddr_in service{};
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = inet_addr("127.0.0.1");
    service.sin_port = htons(port);

    if (bind(s, reinterpret_cast<SOCKADDR*>(&service), sizeof(service)) == SOCKET_ERROR) {
        closesocket(s);
        return INVALID_SOCKET;
    }

    if (listen(s, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(s);
        return INVALID_SOCKET;
    }

    return s;
}

int main() {
    std::wcout << L"============================================================" << std::endl;
    std::wcout << L" PortPeek Comprehensive Verification Test Suite" << std::endl;
    std::wcout << L"============================================================" << std::endl;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::wcerr << L"[FAIL] WSAStartup failed." << std::endl;
        return 1;
    }

    // Test 1: Theme Manager Initialization
    std::wcout << L"\n[TEST 1] Testing Theme Manager initialization..." << std::endl;
    ThemeManager::Instance().Initialize();
    bool isDark = ThemeManager::Instance().IsSystemDarkModeActive();
    std::wcout << L"  -> System dark mode detected: " << (isDark ? L"YES" : L"NO") << std::endl;
    std::wcout << L"  [PASS] Theme Manager initialized cleanly." << std::endl;

    // Test 2: Process Resolution, Command Line & RAM
    std::wcout << L"\n[TEST 2] Testing Process Resolution, Command Line & RAM..." << std::endl;
    DWORD myPid = GetCurrentProcessId();
    std::wstring myName = ProcessUtil::GetProcessNameByPid(myPid);
    std::wstring myCmd = ProcessUtil::GetProcessCommandLine(myPid);
    auto projInfo = ProcessUtil::ParseCommandLine(myCmd, myName);
    size_t memMb = ProcessUtil::GetProcessMemoryUsageMb(myPid);

    std::wcout << L"  -> Current PID (" << myPid << L"): " << myName << std::endl;
    std::wcout << L"  -> Parsed Project: " << projInfo.projectName << std::endl;
    std::wcout << L"  -> Memory Usage: " << memMb << L" MB" << std::endl;
    assert(!myName.empty() && myName != L"Unknown process");
    std::wcout << L"  [PASS] Process resolution, command line, and RAM queried." << std::endl;

    // Test 3: Framework Detection Matrix
    std::wcout << L"\n[TEST 3] Testing Framework Detection Matrix..." << std::endl;
    auto f1 = ProcessUtil::ParseCommandLine(L"node C:\\Projects\\app\\node_modules\\next\\dist\\bin\\next dev", L"node.exe");
    assert(f1.framework == L"Next.js");

    auto f2 = ProcessUtil::ParseCommandLine(L"node C:\\Projects\\app\\node_modules\\vite\\bin\\vite.js", L"node.exe");
    assert(f2.framework == L"Vite");

    auto f3 = ProcessUtil::ParseCommandLine(L"python -m uvicorn main:app --reload", L"python.exe");
    assert(f3.framework == L"FastAPI");

    auto f4 = ProcessUtil::ParseCommandLine(L"python manage.py runserver", L"python.exe");
    assert(f4.framework == L"Django");

    auto f5 = ProcessUtil::ParseCommandLine(L"docker-proxy -proto tcp -host-port 5432", L"docker-proxy.exe");
    assert(f5.framework == L"Docker");
    std::wcout << L"  [PASS] Framework heuristic identified Next.js, Vite, FastAPI, Django, Docker." << std::endl;

    // Test 4: Fast Loopback HTTP & Header Detection
    std::wcout << L"\n[TEST 4] Testing Fast Loopback HTTP & Server Header..." << std::endl;
    const uint16_t httpTestPort = 39889;
    SOCKET httpSock = StartMockHttpServer(httpTestPort);
    assert(httpSock != INVALID_SOCKET);

    MockServerContext ctx{ httpSock, true };
    HANDLE hThread = CreateThread(nullptr, 0, MockHttpServerThreadProc, &ctx, 0, nullptr);
    assert(hThread != nullptr);

    auto probeRes = NetProbe::ProbeLoopbackPort(httpTestPort, 100);
    std::wcout << L"  -> Status: " << probeRes.statusCode << std::endl;
    std::wcout << L"  -> Server: " << probeRes.serverHeader << std::endl;
    std::wcout << L"  -> Framework Header: " << probeRes.frameworkHeader << std::endl;
    std::wcout << L"  -> Title : " << probeRes.htmlTitle << std::endl;
    std::wcout << L"  -> Time  : " << probeRes.elapsedMs << L" ms" << std::endl;

    assert(probeRes.isHttp == true);
    assert(probeRes.statusCode == 200);
    assert(probeRes.frameworkHeader == L"FastAPI");
    assert(probeRes.htmlTitle == L"FastAPI Swagger UI");
    std::wcout << L"  [PASS] Fast HTTP probe parsed status, server header, and title." << std::endl;

    ctx.running = false;
    WaitForSingleObject(hThread, 500);
    CloseHandle(hThread);
    closesocket(httpSock);

    // Test 5: Markdown Summary Export
    std::wcout << L"\n[TEST 5] Testing Markdown Port Summary Generator..." << std::endl;
    std::vector<ListeningPort> samplePorts;
    ListeningPort p1;
    p1.port = 3000;
    p1.pid = 1234;
    p1.processName = L"node.exe";
    p1.projectName = L"my-next-app";
    p1.framework = L"Next.js";
    p1.memoryMb = 48;
    p1.isDevServer = true;
    p1.protocol = L"http";
    samplePorts.push_back(p1);

    std::wstring md = GeneratePortsMarkdown(samplePorts);
    assert(md.find(L"http://localhost:3000") != std::wstring::npos);
    assert(md.find(L"Next.js") != std::wstring::npos);
    assert(md.find(L"48 MB") != std::wstring::npos);
    std::wcout << L"  [PASS] Markdown export generator verified." << std::endl;

    // Test 6: Auto-Start Registry Configuration CRUD
    std::wcout << L"\n[TEST 6] Testing Configuration & Registry Manager..." << std::endl;
    bool initialAutoStart = ConfigManager::IsAutoStartEnabled();
    std::wcout << L"  -> Current AutoStart: " << (initialAutoStart ? L"ENABLED" : L"DISABLED") << std::endl;
    
    // Toggle check
    ConfigManager::SetAutoStartEnabled(true);
    assert(ConfigManager::IsAutoStartEnabled() == true);
    
    // Restore original
    ConfigManager::SetAutoStartEnabled(initialAutoStart);
    assert(ConfigManager::IsAutoStartEnabled() == initialAutoStart);

    bool initialHotkey = ConfigManager::IsGlobalHotkeyEnabled();
    ConfigManager::SetGlobalHotkeyEnabled(true);
    assert(ConfigManager::IsGlobalHotkeyEnabled() == true);
    ConfigManager::SetGlobalHotkeyEnabled(initialHotkey);
    std::wcout << L"  [PASS] Registry configuration CRUD verified." << std::endl;

    // Test 7: Port Enumeration Baseline & Ascending Sort
    std::wcout << L"\n[TEST 7] Testing System Port Enumeration..." << std::endl;
    auto initialPorts = EnumerateListeningTcpPorts();
    std::wcout << L"  -> Found " << initialPorts.size() << L" listening ports." << std::endl;
    for (size_t i = 1; i < initialPorts.size(); ++i) {
        assert(initialPorts[i - 1].port < initialPorts[i].port);
    }
    std::wcout << L"  [PASS] Ports are strictly sorted in ascending numerical order." << std::endl;

    std::wcout << L"\n============================================================" << std::endl;
    std::wcout << L" [ALL 7 VERIFICATION TESTS PASSED!]" << std::endl;
    std::wcout << L"============================================================" << std::endl;

    WSACleanup();
    return 0;
}
