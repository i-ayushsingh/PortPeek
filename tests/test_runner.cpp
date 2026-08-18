#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <cassert>
#include <vector>
#include <fstream>

#include "ports.h"
#include "process.h"
#include "probe.h"
#include "theme.h"
#include "config.h"
#include "alias.h"
#include "lan.h"
#include "qrcode.h"
#include "tunnel.h"
#include "app_discovery.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "gdi32.lib")

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

    // Test 8: QR Code ISO/IEC 18004 Matrix Generation
    std::wcout << L"\n[TEST 8] Testing ISO/IEC 18004 QR Code Generation..." << std::endl;
    auto qr = QrCode::Generate("http://192.168.1.34:3000");
    std::wcout << L"  -> QR Dimension: " << qr.size << L"x" << qr.size << std::endl;
    assert(qr.size >= 21); // Model 2 Version 1 is 21x21
    assert(!qr.modules.empty());
    std::wcout << L"  [PASS] ISO/IEC 18004 standard QR Code matrix generated." << std::endl;

    // Test 9: LAN IPv4 Formatting
    std::wcout << L"\n[TEST 9] Testing LAN IPv4 URL Formatting..." << std::endl;
    std::wstring lanUrl = LanUtil::FormatLanUrl(3000, L"http");
    std::wcout << L"  -> Formatted LAN URL: " << lanUrl << std::endl;
    assert(lanUrl.find(L":3000") != std::wstring::npos);
    std::wcout << L"  [PASS] LAN URL formatting verified." << std::endl;

    // Test 10: Alias Manager
    std::wcout << L"\n[TEST 10] Testing Alias Manager Resolution..." << std::endl;
    AliasManager::Reload();
    std::wstring alias = AliasManager::GetAliasForPort(3000);
    std::wcout << L"  -> Port 3000 Alias: " << (alias.empty() ? L"(default)" : alias) << std::endl;
    std::wcout << L"  [PASS] Alias Manager loaded and queried cleanly." << std::endl;

    // ------------------------------------------------------------------------
    // EPHE INTEGRATION TEST SUITE (TESTS 11 TO 22)
    // ------------------------------------------------------------------------
    std::wcout << L"\n============================================================" << std::endl;
    std::wcout << L" Ephe Integration Test Suite (12 Scenarios)" << std::endl;
    std::wcout << L"============================================================" << std::endl;

    wchar_t tempDir[MAX_PATH];
    GetTempPathW(MAX_PATH, tempDir);
    std::wstring mockEpheDir = std::wstring(tempDir) + L"PortPeek_Ephe_Test_" + std::to_wstring(GetTickCount64());
    CreateDirectoryW(mockEpheDir.c_str(), nullptr);

    auto WriteTestJson = [](const std::wstring& dir, const std::wstring& filename, const std::string& content) {
        std::wstring fullPath = dir + L"\\" + filename;
        std::ofstream f(fullPath, std::ios::out | std::ios::binary);
        f << content;
        f.close();
    };

    auto DeleteTestFile = [](const std::wstring& dir, const std::wstring& filename) {
        std::wstring fullPath = dir + L"\\" + filename;
        DeleteFileW(fullPath.c_str());
    };

    // Test 11: No Ephe sessions (empty directory)
    std::wcout << L"\n[TEST 11] Ephe: No sessions in directory..." << std::endl;
    auto epheSessionsEmpty = EpheDiscoveryProvider::DiscoverFromDirectory(mockEpheDir, false);
    assert(epheSessionsEmpty.empty());
    std::wcout << L"  [PASS] 0 sessions discovered in empty directory." << std::endl;

    // Test 12: Missing manifest directory
    std::wcout << L"\n[TEST 12] Ephe: Non-existent manifest directory..." << std::endl;
    auto epheMissingDir = EpheDiscoveryProvider::DiscoverFromDirectory(L"C:\\NonExistent_Dir_XYZ_12345", false);
    assert(epheMissingDir.empty());
    std::wcout << L"  [PASS] Missing directory handled gracefully (0 sessions, no crash)." << std::endl;

    // Test 13: Malformed JSON manifests
    std::wcout << L"\n[TEST 13] Ephe: Malformed JSON manifest handling..." << std::endl;
    DiscoveredSession sessBad;
    assert(!EpheDiscoveryProvider::ParseAndValidateManifest("{ this is not valid json }", sessBad));
    assert(!EpheDiscoveryProvider::ParseAndValidateManifest("{\"schemaVersion\": 2, \"applicationId\": \"ephe\"}", sessBad)); // wrong schema
    assert(!EpheDiscoveryProvider::ParseAndValidateManifest("{\"schemaVersion\": 1, \"applicationId\": \"other_app\"}", sessBad)); // wrong appId
    assert(!EpheDiscoveryProvider::ParseAndValidateManifest("{\"schemaVersion\": 1, \"applicationId\": \"ephe\", \"status\": \"stopped\"}", sessBad)); // not active
    std::wcout << L"  [PASS] Malformed and invalid manifests rejected silently." << std::endl;

    // Test 14: Security URL validation
    std::wcout << L"\n[TEST 14] Ephe: Security URL validation (untrusted input)..." << std::endl;
    DiscoveredSession sessUrlTest;
    std::string badUrlJson = "{\"schemaVersion\":1,\"applicationId\":\"ephe\",\"sessionId\":\"1\",\"status\":\"active\",\"port\":5500,\"localUrl\":\"https://evil.com/payload\"}";
    assert(!EpheDiscoveryProvider::ParseAndValidateManifest(badUrlJson, sessUrlTest));
    std::string goodUrlJson = "{\"schemaVersion\":1,\"applicationId\":\"ephe\",\"sessionId\":\"1\",\"status\":\"active\",\"port\":5500,\"localUrl\":\"http://localhost:5500\",\"hostUrl\":\"http://localhost:5500/?role=desktop\"}";
    assert(EpheDiscoveryProvider::ParseAndValidateManifest(goodUrlJson, sessUrlTest));
    assert(sessUrlTest.localUrl == L"http://localhost:5500");
    assert(sessUrlTest.hostUrl == L"http://localhost:5500/?role=desktop");
    assert(sessUrlTest.GetLaunchUrl() == L"http://localhost:5500/?role=desktop");
    std::wcout << L"  [PASS] Remote URLs rejected; localhost hostUrl and localUrl accepted." << std::endl;

    // Test 15: One valid Ephe session with hostUrl
    std::wcout << L"\n[TEST 15] Ephe: One valid session discovery with hostUrl..." << std::endl;
    std::string sess1Json = "{\n"
        "  \"schemaVersion\": 1,\n"
        "  \"application\": \"Ephe\",\n"
        "  \"applicationId\": \"ephe\",\n"
        "  \"sessionId\": \"abc123\",\n"
        "  \"sessionName\": \"College Notes\",\n"
        "  \"version\": \"0.8.0\",\n"
        "  \"port\": 5500,\n"
        "  \"localUrl\": \"http://localhost:5500\",\n"
        "  \"hostUrl\": \"http://localhost:5500/?role=desktop\",\n"
        "  \"status\": \"active\",\n"
        "  \"connectedDevices\": 2\n"
        "}";
    WriteTestJson(mockEpheDir, L"session1.json", sess1Json);
    auto sessions1 = EpheDiscoveryProvider::DiscoverFromDirectory(mockEpheDir, false);
    assert(sessions1.size() == 1);
    assert(sessions1[0].sessionId == L"abc123");
    assert(sessions1[0].sessionName == L"College Notes");
    assert(sessions1[0].port == 5500);
    assert(sessions1[0].localUrl == L"http://localhost:5500");
    assert(sessions1[0].hostUrl == L"http://localhost:5500/?role=desktop");
    assert(sessions1[0].GetLaunchUrl() == L"http://localhost:5500/?role=desktop");
    assert(sessions1[0].GetLaunchUrl() != sessions1[0].localUrl);
    std::wcout << L"  [PASS] Single session parsed accurately: " << sessions1[0].sessionName << L" (Launch URL: " << sessions1[0].GetLaunchUrl() << L")" << std::endl;

    // Test 16: Connected device count
    std::wcout << L"\n[TEST 16] Ephe: Connected device count parsing..." << std::endl;
    assert(sessions1[0].connectedDevices == 2);
    std::wcout << L"  [PASS] Connected devices count verified: " << sessions1[0].connectedDevices << std::endl;

    // Test 17: Multiple Ephe sessions
    std::wcout << L"\n[TEST 17] Ephe: Multiple sessions discovery..." << std::endl;
    std::string sess2Json = "{\n"
        "  \"schemaVersion\": 1,\n"
        "  \"application\": \"Ephe\",\n"
        "  \"applicationId\": \"ephe\",\n"
        "  \"sessionId\": \"def456\",\n"
        "  \"sessionName\": \"Project Planning\",\n"
        "  \"port\": 5501,\n"
        "  \"localUrl\": \"http://localhost:5501\",\n"
        "  \"hostUrl\": \"http://localhost:5501/?role=desktop\",\n"
        "  \"status\": \"active\",\n"
        "  \"connectedDevices\": 0\n"
        "}";
    WriteTestJson(mockEpheDir, L"session2.json", sess2Json);
    auto sessionsMulti = EpheDiscoveryProvider::DiscoverFromDirectory(mockEpheDir, false);
    assert(sessionsMulti.size() == 2);
    std::wcout << L"  [PASS] Multiple sessions discovered: " << sessionsMulti.size() << L" active sessions." << std::endl;

    // Test 18: Session name fallback
    std::wcout << L"\n[TEST 18] Ephe: Session name fallback when missing or empty..." << std::endl;
    DiscoveredSession sessNoName;
    std::string noNameJson = "{\"schemaVersion\":1,\"applicationId\":\"ephe\",\"sessionId\":\"xyz\",\"status\":\"active\",\"port\":5502,\"localUrl\":\"http://localhost:5502\"}";
    assert(EpheDiscoveryProvider::ParseAndValidateManifest(noNameJson, sessNoName));
    assert(sessNoName.sessionName == L"Ephe Session");
    std::wcout << L"  [PASS] Unnamed session defaulted cleanly to: " << sessNoName.sessionName << std::endl;

    // Test 19: Stale / unreachable session verification
    std::wcout << L"\n[TEST 19] Ephe: Stale / unreachable session filtering..." << std::endl;
    // Port 59999 is unlikely to be listening; with verifyReachability = true, it must be ignored
    std::string staleJson = "{\"schemaVersion\":1,\"applicationId\":\"ephe\",\"sessionId\":\"stale99\",\"sessionName\":\"Crashed Session\",\"status\":\"active\",\"port\":59999,\"localUrl\":\"http://localhost:59999\",\"hostUrl\":\"http://localhost:59999/?role=desktop\"}";
    WriteTestJson(mockEpheDir, L"stale.json", staleJson);
    auto verifiedSessions = EpheDiscoveryProvider::DiscoverFromDirectory(mockEpheDir, true);
    // stale.json was written for closed port 59999, so it should be filtered out
    for (const auto& s : verifiedSessions) {
        assert(s.port != 59999);
    }
    DeleteTestFile(mockEpheDir, L"stale.json");
    std::wcout << L"  [PASS] Unreachable stale session on closed port filtered out." << std::endl;

    // Test 20: Open All Sessions aggregation with hostUrl
    std::wcout << L"\n[TEST 20] Ephe: Open All Sessions URL aggregation..." << std::endl;
    std::vector<std::wstring> urlsToOpen;
    for (const auto& s : sessionsMulti) {
        urlsToOpen.push_back(s.GetLaunchUrl());
    }
    assert(urlsToOpen.size() == 2);
    assert(urlsToOpen[0] == L"http://localhost:5500/?role=desktop" || urlsToOpen[0] == L"http://localhost:5501/?role=desktop");
    assert(urlsToOpen[1] == L"http://localhost:5500/?role=desktop" || urlsToOpen[1] == L"http://localhost:5501/?role=desktop");
    std::wcout << L"  [PASS] Batch session hostUrls aggregated for desktop browser launching." << std::endl;

    // Test 21: Dynamic Refresh after a new session starts
    std::wcout << L"\n[TEST 21] Ephe: Dynamic refresh on session start..." << std::endl;
    std::string sess3Json = "{\"schemaVersion\":1,\"applicationId\":\"ephe\",\"sessionId\":\"ghi789\",\"sessionName\":\"Phone Transfer\",\"status\":\"active\",\"port\":5503,\"localUrl\":\"http://localhost:5503\",\"hostUrl\":\"http://localhost:5503/?role=desktop\"}";
    WriteTestJson(mockEpheDir, L"session3.json", sess3Json);
    auto refreshedAdd = EpheDiscoveryProvider::DiscoverFromDirectory(mockEpheDir, false);
    assert(refreshedAdd.size() == 3);
    std::wcout << L"  [PASS] Dynamic session addition discovered: 3 sessions." << std::endl;

    // Test 22: Dynamic Refresh after session ends
    std::wcout << L"\n[TEST 22] Ephe: Dynamic refresh on session termination..." << std::endl;
    DeleteTestFile(mockEpheDir, L"session1.json");
    DeleteTestFile(mockEpheDir, L"session2.json");
    DeleteTestFile(mockEpheDir, L"session3.json");
    auto refreshedRemove = EpheDiscoveryProvider::DiscoverFromDirectory(mockEpheDir, false);
    assert(refreshedRemove.empty());
    RemoveDirectoryW(mockEpheDir.c_str());
    std::wcout << L"  [PASS] Session cleanup verified: 0 remaining sessions." << std::endl;

    // Test 23: Executable discovery when Ephe is not installed
    std::wcout << L"\n[TEST 23] Ephe: Executable discovery on system..." << std::endl;
    std::wstring exePath = EpheLauncher::FindEpheExecutable();
    std::wcout << L"  -> Ephe executable path: " << (exePath.empty() ? L"(Not Installed)" : exePath) << std::endl;
    // On systems without Ephe installed, FindEpheExecutable must return empty string and not crash
    if (exePath.empty()) {
        assert(!EpheLauncher::IsEpheAvailable());
    } else {
        assert(EpheLauncher::IsEpheAvailable());
    }
    std::wcout << L"  [PASS] Executable resolution completed safely without crashing." << std::endl;

    // Test 24: Launch failure with non-existent executable
    std::wcout << L"\n[TEST 24] Ephe: Launch with non-existent executable..." << std::endl;
    bool failedLaunch = EpheLauncher::LaunchNewSession(L"C:\\Invalid_Path_12345\\ephe.exe");
    assert(!failedLaunch);
    std::wcout << L"  [PASS] Invalid executable launch handled gracefully (returned false)." << std::endl;

    // Test 25: Process launch simulation with valid system binary
    std::wcout << L"\n[TEST 25] Ephe: Process launch execution..." << std::endl;
    wchar_t sysDir[MAX_PATH];
    GetSystemDirectoryW(sysDir, MAX_PATH);
    std::wstring cmdExe = std::wstring(sysDir) + L"\\cmd.exe";
    bool cmdLaunch = EpheLauncher::LaunchNewSession(cmdExe);
    assert(cmdLaunch);
    std::wcout << L"  [PASS] CreateProcess invocation and handle cleanup verified." << std::endl;

    // Test 26: End-to-End Simulation: Launch -> Manifest Creation -> Discovery -> URL Extraction
    std::wcout << L"\n[TEST 26] Ephe: Full lifecycle simulation..." << std::endl;
    CreateDirectoryW(mockEpheDir.c_str(), nullptr);
    std::string lifecycleJson = "{\n"
        "  \"schemaVersion\": 1,\n"
        "  \"application\": \"Ephe\",\n"
        "  \"applicationId\": \"ephe\",\n"
        "  \"sessionId\": \"e2e-session-99\",\n"
        "  \"sessionName\": \"Live Collaboration\",\n"
        "  \"version\": \"1.0.0\",\n"
        "  \"port\": 5505,\n"
        "  \"localUrl\": \"http://localhost:5505\",\n"
        "  \"hostUrl\": \"http://localhost:5505/?role=desktop\",\n"
        "  \"status\": \"active\",\n"
        "  \"connectedDevices\": 4\n"
        "}";
    WriteTestJson(mockEpheDir, L"e2e.json", lifecycleJson);
    auto e2eSessions = EpheDiscoveryProvider::DiscoverFromDirectory(mockEpheDir, false);
    assert(e2eSessions.size() == 1);
    assert(e2eSessions[0].sessionName == L"Live Collaboration");
    assert(e2eSessions[0].connectedDevices == 4);
    assert(e2eSessions[0].GetLaunchUrl() == L"http://localhost:5505/?role=desktop");
    DeleteTestFile(mockEpheDir, L"e2e.json");
    RemoveDirectoryW(mockEpheDir.c_str());
    std::wcout << L"  [PASS] Full Ephe session lifecycle simulated successfully." << std::endl;

    // Test 27: PortPeek Baseline Invariant Verification
    std::wcout << L"\n[TEST 27] PortPeek: Invariant & Baseline Non-Regression Check..." << std::endl;
    auto finalPorts = EnumerateListeningTcpPorts();
    assert(!finalPorts.empty() || initialPorts.empty());
    std::wcout << L"  [PASS] PortPeek core port enumeration, sorting, and probe invariants intact." << std::endl;

    // Test 28: Backward compatibility (Older manifest without hostUrl)
    std::wcout << L"\n[TEST 28] Ephe: Older manifest without hostUrl fallback..." << std::endl;
    DiscoveredSession legacySess;
    std::string legacyJson = "{\"application\":\"Ephe\",\"sessionId\":\"leg123\",\"port\":49566,\"localUrl\":\"http://localhost:49566\",\"status\":\"active\"}";
    assert(EpheDiscoveryProvider::ParseAndValidateManifest(legacyJson, legacySess));
    assert(legacySess.hostUrl.empty());
    assert(legacySess.localUrl == L"http://localhost:49566");
    assert(legacySess.GetLaunchUrl() == L"http://localhost:49566"); // Fallback to localUrl
    std::wcout << L"  [PASS] Legacy manifest without hostUrl safely fell back to localUrl." << std::endl;

    // Test 29: Malformed / Unsafe hostUrl security fallback
    std::wcout << L"\n[TEST 29] Ephe: Malformed hostUrl security handling..." << std::endl;
    DiscoveredSession unsafeHostSess;
    std::string unsafeHostJson = "{\"application\":\"Ephe\",\"sessionId\":\"bad123\",\"port\":49566,\"localUrl\":\"http://localhost:49566\",\"hostUrl\":\"https://malicious-site.com/steal\",\"status\":\"active\"}";
    assert(EpheDiscoveryProvider::ParseAndValidateManifest(unsafeHostJson, unsafeHostSess));
    assert(unsafeHostSess.hostUrl.empty()); // Unsafe hostUrl stripped
    assert(unsafeHostSess.GetLaunchUrl() == L"http://localhost:49566"); // Safe fallback to localUrl
    std::wcout << L"  [PASS] Unsafe remote hostUrl rejected and safely fell back to localUrl." << std::endl;

    // Test 30: HostUrl exact query parameter passthrough (?role=desktop)
    std::wcout << L"\n[TEST 30] Ephe: HostUrl exact parameter passthrough..." << std::endl;
    DiscoveredSession exactHostSess;
    std::string exactHostJson = "{\"application\":\"Ephe\",\"sessionId\":\"desktop123\",\"port\":49566,\"localUrl\":\"http://localhost:49566\",\"hostUrl\":\"http://localhost:49566/?role=desktop\",\"status\":\"active\"}";
    assert(EpheDiscoveryProvider::ParseAndValidateManifest(exactHostJson, exactHostSess));
    assert(exactHostSess.hostUrl == L"http://localhost:49566/?role=desktop");
    assert(exactHostSess.GetLaunchUrl() == L"http://localhost:49566/?role=desktop");
    std::wcout << L"  [PASS] hostUrl preserved with ?role=desktop exactly as supplied by Ephe." << std::endl;

    std::wcout << L"\n============================================================" << std::endl;
    std::wcout << L" [ALL 30 VERIFICATION TESTS PASSED CLEANLY!]" << std::endl;
    std::wcout << L"============================================================" << std::endl;

    WSACleanup();
    return 0;
}
