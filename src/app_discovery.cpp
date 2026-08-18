#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "app_discovery.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "shell32.lib")

namespace {

std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), nullptr, 0);
    if (sizeNeeded <= 0) return L"";
    std::wstring wstr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), &wstr[0], sizeNeeded);
    return wstr;
}

std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

bool ExtractJsonString(const std::string& json, const std::string& key, std::string& outVal) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return false;

    size_t colonPos = json.find(':', keyPos + searchKey.length());
    if (colonPos == std::string::npos) return false;

    size_t startQuote = json.find('"', colonPos + 1);
    if (startQuote == std::string::npos) return false;

    size_t endQuote = startQuote + 1;
    while (endQuote < json.length()) {
        if (json[endQuote] == '"' && json[endQuote - 1] != '\\') {
            break;
        }
        ++endQuote;
    }
    if (endQuote >= json.length()) return false;

    outVal = json.substr(startQuote + 1, endQuote - startQuote - 1);
    return true;
}

bool ExtractJsonInt(const std::string& json, const std::string& key, int& outVal) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return false;

    size_t colonPos = json.find(':', keyPos + searchKey.length());
    if (colonPos == std::string::npos) return false;

    size_t valStart = json.find_first_of("-0123456789", colonPos + 1);
    if (valStart == std::string::npos) return false;

    size_t valEnd = json.find_first_not_of("-0123456789", valStart);
    std::string numStr = (valEnd == std::string::npos) ? json.substr(valStart) : json.substr(valStart, valEnd - valStart);

    try {
        outVal = std::stoi(numStr);
        return true;
    } catch (...) {
        return false;
    }
}

} // anonymous namespace

// ----------------------------------------------------------------------------
// EpheDiscoveryProvider Implementation
// ----------------------------------------------------------------------------

bool EpheDiscoveryProvider::VerifyLocalPortReachable(uint16_t port, int timeoutMs) {
    if (port == 0) return false;

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return false;

    u_long nonBlocking = 1;
    ioctlsocket(s, FIONBIO, &nonBlocking);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

    fd_set writeSet, errSet;
    FD_ZERO(&writeSet);
    FD_ZERO(&errSet);
    FD_SET(s, &writeSet);
    FD_SET(s, &errSet);

    timeval tv{};
    tv.tv_sec = 0;
    tv.tv_usec = timeoutMs * 1000;

    int res = select(0, nullptr, &writeSet, &errSet, &tv);
    bool reachable = false;
    if (res > 0 && FD_ISSET(s, &writeSet) && !FD_ISSET(s, &errSet)) {
        int soError = 0;
        int optLen = sizeof(soError);
        if (getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&soError), &optLen) == 0) {
            reachable = (soError == 0);
        }
    }

    closesocket(s);
    return reachable;
}

bool EpheDiscoveryProvider::ParseAndValidateManifest(const std::string& jsonContent, DiscoveredSession& outSession) {
    if (jsonContent.empty()) return false;

    int schemaVersion = 0;
    if (ExtractJsonInt(jsonContent, "schemaVersion", schemaVersion)) {
        if (schemaVersion != 1) {
            return false;
        }
    }

    std::string appId;
    ExtractJsonString(jsonContent, "applicationId", appId);

    std::string appName;
    if (!ExtractJsonString(jsonContent, "application", appName)) {
        appName = "Ephe";
    }

    if (!appId.empty() && ToLower(appId) != "ephe") {
        return false;
    }
    if (appId.empty() && ToLower(appName) != "ephe") {
        return false;
    }
    if (appId.empty()) {
        appId = "ephe";
    }

    std::string status;
    if (!ExtractJsonString(jsonContent, "status", status) || ToLower(status) != "active") {
        return false;
    }

    int portInt = 0;
    if (!ExtractJsonInt(jsonContent, "port", portInt) || portInt < 1 || portInt > 65535) {
        return false;
    }

    std::string localUrl;
    if (!ExtractJsonString(jsonContent, "localUrl", localUrl)) {
        return false;
    }

    std::string lowerUrl = ToLower(localUrl);
    // Security check: Only allow localhost / 127.0.0.1 HTTP URLs
    if (lowerUrl.rfind("http://localhost:", 0) != 0 &&
        lowerUrl.rfind("http://127.0.0.1:", 0) != 0) {
        return false;
    }

    std::string hostUrl;
    if (ExtractJsonString(jsonContent, "hostUrl", hostUrl) && !hostUrl.empty()) {
        std::string lowerHost = ToLower(hostUrl);
        // Security check: Only allow localhost / 127.0.0.1 HTTP URLs for hostUrl
        if (lowerHost.rfind("http://localhost:", 0) == 0 ||
            lowerHost.rfind("http://127.0.0.1:", 0) == 0) {
            outSession.hostUrl = Utf8ToWide(hostUrl);
        } else {
            // Unsafe or remote hostUrl ignored safely
            outSession.hostUrl.clear();
        }
    } else {
        outSession.hostUrl.clear();
    }

    std::string sessionId;
    ExtractJsonString(jsonContent, "sessionId", sessionId);
    if (sessionId.empty()) return false;

    std::string sessionName;
    if (!ExtractJsonString(jsonContent, "sessionName", sessionName) || Trim(sessionName).empty()) {
        sessionName = "Ephe Session";
    }

    std::string version;
    ExtractJsonString(jsonContent, "version", version);

    int devices = 0;
    ExtractJsonInt(jsonContent, "connectedDevices", devices);
    if (devices < 0) devices = 0;

    outSession.applicationId = Utf8ToWide(appId);
    outSession.applicationName = Utf8ToWide(appName);
    outSession.sessionId = Utf8ToWide(sessionId);
    outSession.sessionName = Utf8ToWide(sessionName);
    outSession.version = Utf8ToWide(version);
    outSession.port = static_cast<uint16_t>(portInt);
    outSession.localUrl = Utf8ToWide(localUrl);
    outSession.status = Utf8ToWide(status);
    outSession.connectedDevices = devices;

    return true;
}

std::vector<DiscoveredSession> EpheDiscoveryProvider::DiscoverFromDirectory(const std::wstring& directoryPath, bool verifyReachability) {
    std::vector<DiscoveredSession> sessions;
    if (directoryPath.empty()) return sessions;

    std::wstring searchPattern = directoryPath;
    if (searchPattern.back() != L'\\') searchPattern += L'\\';
    searchPattern += L"*.json";

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return sessions;
    }

    do {
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::wstring filePath = directoryPath;
            if (filePath.back() != L'\\') filePath += L'\\';
            filePath += findData.cFileName;

            std::ifstream file(filePath, std::ios::in | std::ios::binary);
            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string content = buffer.str();
                file.close();

                DiscoveredSession sess;
                if (ParseAndValidateManifest(content, sess)) {
                    if (!verifyReachability || VerifyLocalPortReachable(sess.port, 25)) {
                        sessions.push_back(sess);
                    }
                }
            }
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    return sessions;
}

std::vector<DiscoveredSession> EpheDiscoveryProvider::DiscoverSessions() {
    wchar_t localAppData[MAX_PATH] = { 0 };
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) {
        DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) {
            return {};
        }
    }

    std::wstring epheDir = localAppData;
    epheDir += L"\\Ephe\\PortPeek";

    return DiscoverFromDirectory(epheDir, true);
}

// ----------------------------------------------------------------------------
// AppDiscoveryManager Implementation
// ----------------------------------------------------------------------------

AppDiscoveryManager::AppDiscoveryManager() {
    RegisterProvider(std::make_unique<EpheDiscoveryProvider>());
}

AppDiscoveryManager& AppDiscoveryManager::Instance() {
    static AppDiscoveryManager s_instance;
    return s_instance;
}

void AppDiscoveryManager::RegisterProvider(std::unique_ptr<IAppDiscoveryProvider> provider) {
    if (provider) {
        m_providers.push_back(std::move(provider));
    }
}

std::vector<DiscoveredSession> AppDiscoveryManager::DiscoverAllVerifiedSessions() {
    std::vector<DiscoveredSession> allSessions;
    for (const auto& provider : m_providers) {
        if (provider) {
            auto sessions = provider->DiscoverSessions();
            allSessions.insert(allSessions.end(), sessions.begin(), sessions.end());
        }
    }
    m_cachedSessions = allSessions;
    return allSessions;
}

void AppDiscoveryManager::Refresh() {
    DiscoverAllVerifiedSessions();
}

// ----------------------------------------------------------------------------
// EpheLauncher Implementation
// ----------------------------------------------------------------------------

namespace {

bool FileExists(const std::wstring& path) {
    DWORD attrib = GetFileAttributesW(path.c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

std::wstring QueryAppPathsRegistry(HKEY rootKey, const wchar_t* subKey) {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(rootKey, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t pathBuf[MAX_PATH] = { 0 };
        DWORD bufSize = sizeof(pathBuf);
        DWORD type = 0;
        LONG res = RegQueryValueExW(hKey, nullptr, nullptr, &type, reinterpret_cast<LPBYTE>(pathBuf), &bufSize);
        RegCloseKey(hKey);
        if (res == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ)) {
            std::wstring found(pathBuf);
            if (FileExists(found)) {
                return found;
            }
        }
    }
    return L"";
}

} // anonymous namespace

std::wstring EpheLauncher::FindEpheExecutable() {
    // 1. App Paths registry (HKCU and HKLM)
    std::wstring regPath = QueryAppPathsRegistry(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\ephe.exe");
    if (!regPath.empty()) return regPath;

    regPath = QueryAppPathsRegistry(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\ephe.exe");
    if (!regPath.empty()) return regPath;

    // 2. Standard installation paths
    wchar_t localAppData[MAX_PATH] = { 0 };
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) {
        std::wstring p1 = std::wstring(localAppData) + L"\\Programs\\Ephe\\ephe.exe";
        if (FileExists(p1)) return p1;

        std::wstring p2 = std::wstring(localAppData) + L"\\Ephe\\ephe.exe";
        if (FileExists(p2)) return p2;
    }

    wchar_t progFiles[MAX_PATH] = { 0 };
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILES, nullptr, 0, progFiles))) {
        std::wstring p3 = std::wstring(progFiles) + L"\\Ephe\\ephe.exe";
        if (FileExists(p3)) return p3;
    }

    wchar_t progFilesX86[MAX_PATH] = { 0 };
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILESX86, nullptr, 0, progFilesX86))) {
        std::wstring p4 = std::wstring(progFilesX86) + L"\\Ephe\\ephe.exe";
        if (FileExists(p4)) return p4;
    }

    // 3. Search system PATH
    wchar_t foundInPath[MAX_PATH] = { 0 };
    DWORD len = SearchPathW(nullptr, L"ephe.exe", nullptr, MAX_PATH, foundInPath, nullptr);
    if (len > 0 && len < MAX_PATH && FileExists(foundInPath)) {
        return std::wstring(foundInPath);
    }

    return L"";
}

bool EpheLauncher::IsEpheAvailable() {
    return !FindEpheExecutable().empty();
}

bool EpheLauncher::LaunchNewSession(const std::wstring& customExePath) {
    std::wstring exePath = !customExePath.empty() ? customExePath : FindEpheExecutable();
    if (exePath.empty() || !FileExists(exePath)) {
        return false;
    }

    std::wstring cmdLine = L"\"" + exePath + L"\" --new-session";
    std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
    cmdBuf.push_back(L'\0');

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    BOOL success = CreateProcessW(
        exePath.c_str(),
        cmdBuf.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (success) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
    }

    return false;
}
