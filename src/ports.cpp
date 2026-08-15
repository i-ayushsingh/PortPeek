#include "ports.h"
#include "process.h"
#include "probe.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <shellapi.h>

#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cwctype>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "shell32.lib")

namespace {

std::wstring ToLower(std::wstring str) {
    std::transform(str.begin(), str.end(), str.begin(), [](wchar_t c) {
        return static_cast<wchar_t>(std::towlower(c));
    });
    return str;
}

bool ContainsSubstr(const std::wstring& haystack, const wchar_t* needle) {
    return haystack.find(needle) != std::wstring::npos;
}

struct RawBinding {
    uint16_t port;
    DWORD pid;
};

void CollectIpv4Listeners(std::vector<RawBinding>& out) {
    DWORD dwSize = 0;
    DWORD ret = GetExtendedTcpTable(nullptr, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    std::vector<BYTE> buffer;

    while (ret == ERROR_INSUFFICIENT_BUFFER) {
        buffer.resize(dwSize);
        ret = GetExtendedTcpTable(buffer.data(), &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    }

    if (ret == NO_ERROR && !buffer.empty()) {
        auto* table = reinterpret_cast<PMIB_TCPTABLE_OWNER_PID>(buffer.data());
        for (DWORD i = 0; i < table->dwNumEntries; ++i) {
            const auto& row = table->table[i];
            if (row.dwState == MIB_TCP_STATE_LISTEN) {
                uint16_t port = ntohs(static_cast<u_short>(row.dwLocalPort & 0xFFFF));
                if (port > 0) {
                    out.push_back({ port, row.dwOwningPid });
                }
            }
        }
    }
}

void CollectIpv6Listeners(std::vector<RawBinding>& out) {
    DWORD dwSize = 0;
    DWORD ret = GetExtendedTcpTable(nullptr, &dwSize, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_ALL, 0);
    std::vector<BYTE> buffer;

    while (ret == ERROR_INSUFFICIENT_BUFFER) {
        buffer.resize(dwSize);
        ret = GetExtendedTcpTable(buffer.data(), &dwSize, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_ALL, 0);
    }

    if (ret == NO_ERROR && !buffer.empty()) {
        auto* table6 = reinterpret_cast<PMIB_TCP6TABLE_OWNER_PID>(buffer.data());
        for (DWORD i = 0; i < table6->dwNumEntries; ++i) {
            const auto& row = table6->table[i];
            if (row.dwState == MIB_TCP_STATE_LISTEN) {
                uint16_t port = ntohs(static_cast<u_short>(row.dwLocalPort & 0xFFFF));
                if (port > 0) {
                    out.push_back({ port, row.dwOwningPid });
                }
            }
        }
    }
}

struct CachedPidInfo {
    std::wstring processName;
    std::wstring commandLine;
    ProcessUtil::ProjectInfo projectInfo;
    size_t memoryMb = 0;
};

} // namespace

bool IsDevelopmentServer(uint16_t port, const std::wstring& processName, DWORD pid) {
    if (pid == 0 || pid == 4) return false;
    if (port == 135 || port == 139 || port == 445 || port == 5357) return false;

    std::wstring lowerName = ToLower(processName);

    if (ContainsSubstr(lowerName, L"language_server") ||
        ContainsSubstr(lowerName, L"powertoys") ||
        ContainsSubstr(lowerName, L"antigravity") ||
        ContainsSubstr(lowerName, L"svchost") ||
        ContainsSubstr(lowerName, L"services.exe") ||
        ContainsSubstr(lowerName, L"lsass.exe") ||
        ContainsSubstr(lowerName, L"spoolsv.exe") ||
        ContainsSubstr(lowerName, L"msmpeng.exe")) {
        return false;
    }

    if (port >= 49152 && lowerName == L"unknown process") {
        return false;
    }

    static const wchar_t* const DEV_RUNTIMES[] = {
        L"node.exe", L"npm.exe", L"npx.exe", L"vite.exe", L"next.exe", L"deno.exe", L"bun.exe", L"electron.exe",
        L"python.exe", L"pythonw.exe", L"python3.exe", L"uvicorn.exe", L"gunicorn.exe", L"flask.exe", L"django.exe", L"fastapi.exe",
        L"go.exe", L"main.exe", L"air.exe",
        L"cargo.exe", L"rust.exe",
        L"java.exe", L"javaw.exe", L"kotlin.exe",
        L"dotnet.exe", L"iisexpress.exe", L"w3wp.exe",
        L"php.exe", L"php-cgi.exe",
        L"ruby.exe", L"rails.exe",
        L"nginx.exe", L"caddy.exe", L"httpd.exe", L"apache.exe",
        L"docker.exe", L"docker-proxy.exe",
        L"ollama.exe", L"ollama_llama_server.exe",
        L"postgres.exe", L"mysqld.exe", L"mariadbd.exe", L"redis-server.exe", L"mongod.exe"
    };

    for (const wchar_t* runtime : DEV_RUNTIMES) {
        if (lowerName == runtime) {
            return true;
        }
    }

    bool isStandardDevPort = (port == 80 || port == 443 || port == 5500 || port == 5501 ||
                             (port >= 3000 && port <= 3999) ||
                             (port >= 4000 && port <= 4999) ||
                             (port >= 5000 && port <= 5999) ||
                             (port >= 8000 && port <= 8999) ||
                             (port >= 9000 && port <= 9999));

    if (isStandardDevPort && lowerName != L"unknown process") {
        return true;
    }

    return false;
}

std::vector<ListeningPort> EnumerateListeningTcpPorts() {
    std::vector<RawBinding> rawBindings;
    rawBindings.reserve(64);

    CollectIpv4Listeners(rawBindings);
    CollectIpv6Listeners(rawBindings);

    // Map ensures deduplication and ascending sort by port
    std::map<uint16_t, DWORD> uniquePortToPid;
    for (const auto& binding : rawBindings) {
        auto it = uniquePortToPid.find(binding.port);
        if (it == uniquePortToPid.end()) {
            uniquePortToPid[binding.port] = binding.pid;
        } else if (it->second == 0 && binding.pid != 0) {
            it->second = binding.pid;
        }
    }

    std::unordered_map<DWORD, CachedPidInfo> pidCache;
    std::vector<ListeningPort> result;
    result.reserve(uniquePortToPid.size());

    for (const auto& [port, pid] : uniquePortToPid) {
        auto cacheIt = pidCache.find(pid);
        if (cacheIt == pidCache.end()) {
            CachedPidInfo info;
            info.processName = ProcessUtil::GetProcessNameByPid(pid);
            info.commandLine = ProcessUtil::GetProcessCommandLine(pid);
            info.projectInfo = ProcessUtil::ParseCommandLine(info.commandLine, info.processName);
            info.memoryMb    = ProcessUtil::GetProcessMemoryUsageMb(pid);
            cacheIt = pidCache.emplace(pid, std::move(info)).first;
        }

        const auto& info = cacheIt->second;
        bool isDev = IsDevelopmentServer(port, info.processName, pid);

        ListeningPort entry;
        entry.port          = port;
        entry.pid           = pid;
        entry.processName   = info.processName;
        entry.projectName   = info.projectInfo.projectName;
        entry.projectFolder = info.projectInfo.projectFolder;
        entry.framework     = info.projectInfo.framework;
        entry.commandLine   = info.commandLine;
        entry.memoryMb      = info.memoryMb;
        entry.isDevServer   = isDev;
        entry.protocol      = L"http";

        // Perform fast 25ms loopback probe for dev servers
        if (isDev) {
            auto probe = NetProbe::ProbeLoopbackPort(port, 25);
            if (probe.isHttps) {
                entry.protocol = L"https";
            }

            std::wstring lowerTitle = ToLower(probe.htmlTitle);
            std::wstring lowerProc = ToLower(info.processName);

            if (lowerProc == L"code.exe" || lowerTitle.find(L"listing directory") != std::wstring::npos || lowerTitle.find(L"index of /") != std::wstring::npos) {
                if (lowerProc == L"code.exe" || port == 5500 || port == 5501) {
                    entry.projectName = L"VS Code Live Server";
                    entry.framework   = L"Live Server";
                } else {
                    entry.projectName = L"Directory Server";
                }
                entry.pageTitle = L"";
            } else if (!probe.htmlTitle.empty()) {
                entry.pageTitle = std::move(probe.htmlTitle);
            }

            if (!probe.frameworkHeader.empty()) {
                entry.framework = std::move(probe.frameworkHeader);
            }

            if (!probe.serverHeader.empty()) {
                entry.serverHeader = std::move(probe.serverHeader);
            }
        }

        result.push_back(std::move(entry));
    }

    return result;
}

bool OpenPortInBrowser(uint16_t port, const std::wstring& protocol) {
    if (port == 0) return false;

    std::wstring url = protocol + L"://localhost:" + std::to_wstring(port);

    HINSTANCE hInst = ShellExecuteW(
        nullptr,
        L"open",
        url.c_str(),
        nullptr,
        nullptr,
        SW_SHOWNORMAL
    );

    return (reinterpret_cast<INT_PTR>(hInst) > 32);
}

void OpenAllPortsInBrowser(const std::vector<ListeningPort>& ports) {
    for (const auto& p : ports) {
        if (p.isDevServer) {
            OpenPortInBrowser(p.port, p.protocol);
        }
    }
}

bool CopyTextToClipboard(HWND hWnd, const std::wstring& text) {
    if (text.empty()) return false;

    if (!OpenClipboard(hWnd)) return false;
    EmptyClipboard();

    size_t byteCount = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, byteCount);
    if (!hGlob) {
        CloseClipboard();
        return false;
    }

    void* pBuf = GlobalLock(hGlob);
    if (pBuf) {
        memcpy(pBuf, text.c_str(), byteCount);
        GlobalUnlock(hGlob);
        SetClipboardData(CF_UNICODETEXT, hGlob);
    } else {
        GlobalFree(hGlob);
    }

    CloseClipboard();
    return true;
}

std::wstring GeneratePortsMarkdown(const std::vector<ListeningPort>& ports) {
    std::wstringstream ss;
    ss << L"### Active Local Ports (PortPeek)\n\n";

    bool hasDev = false;
    for (const auto& p : ports) {
        if (!p.isDevServer) continue;
        hasDev = true;

        std::wstring title = !p.pageTitle.empty() ? p.pageTitle : (!p.projectName.empty() ? p.projectName : p.processName);
        ss << L"- [" << title << L"](" << p.protocol << L"://localhost:" << p.port << L")";
        ss << L" — `" << p.processName << L"`";
        if (!p.framework.empty()) {
            ss << L" (" << p.framework << L")";
        }
        if (p.memoryMb > 0) {
            ss << L" • " << p.memoryMb << L" MB";
        }
        ss << L" • PID " << p.pid << L"\n";
    }

    if (!hasDev) {
        ss << L"*No active dev servers currently listening.*\n";
    }

    return ss.str();
}
