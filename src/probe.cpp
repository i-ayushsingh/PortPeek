#include "probe.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <chrono>
#include <algorithm>
#include <cwctype>

#pragma comment(lib, "ws2_32.lib")

namespace NetProbe {

namespace detail {

std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
    if (size <= 0) {
        size = MultiByteToWideChar(CP_ACP, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
        if (size <= 0) return L"";
        std::wstring wstr(size, 0);
        MultiByteToWideChar(CP_ACP, 0, str.c_str(), static_cast<int>(str.size()), &wstr[0], size);
        return wstr;
    }
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), &wstr[0], size);
    return wstr;
}

std::wstring DecodeHtmlEntities(std::wstring str) {
    auto replaceAll = [&](const std::wstring& from, const std::wstring& to) {
        size_t start = 0;
        while ((start = str.find(from, start)) != std::wstring::npos) {
            str.replace(start, from.length(), to);
            start += to.length();
        }
    };
    replaceAll(L"&amp;", L"&");
    replaceAll(L"&lt;", L"<");
    replaceAll(L"&gt;", L">");
    replaceAll(L"&quot;", L"\"");
    replaceAll(L"&#39;", L"'");
    replaceAll(L"&apos;", L"'");
    replaceAll(L"&nbsp;", L" ");
    return str;
}

std::wstring Trim(const std::wstring& str) {
    size_t first = str.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return L"";
    size_t last = str.find_last_not_of(L" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::string ExtractHeaderValue(const std::string& lowerResp, const std::string& rawResp, const std::string& headerName) {
    std::string needle = "\n" + headerName + ":";
    size_t pos = lowerResp.find(needle);
    if (pos == std::string::npos) {
        if (lowerResp.rfind(headerName + ":", 0) == 0) {
            pos = 0;
            needle = headerName + ":";
        } else {
            return "";
        }
    }

    size_t valStart = pos + needle.size();
    while (valStart < rawResp.size() && (rawResp[valStart] == ' ' || rawResp[valStart] == '\t')) {
        valStart++;
    }
    size_t valEnd = rawResp.find("\r", valStart);
    if (valEnd == std::string::npos) valEnd = rawResp.find("\n", valStart);
    if (valEnd != std::string::npos && valEnd > valStart) {
        return rawResp.substr(valStart, valEnd - valStart);
    }
    return "";
}

} // namespace detail

HttpProbeResult ProbeLoopbackPort(uint16_t port, uint32_t timeoutMs) {
    HttpProbeResult result;
    if (port == 443 || port == 8443) {
        result.isHttps = true;
    }

    auto startTime = std::chrono::steady_clock::now();

    auto getRemainingUs = [&]() -> long {
        auto now = std::chrono::steady_clock::now();
        auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(now - startTime).count();
        long totalBudgetUs = static_cast<long>(timeoutMs) * 1000;
        return (elapsedUs < totalBudgetUs) ? static_cast<long>(totalBudgetUs - elapsedUs) : 0;
    };

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return result;

    struct SocketGuard {
        SOCKET sock;
        ~SocketGuard() { if (sock != INVALID_SOCKET) closesocket(sock); }
    } guard{ s };

    // 1. Enable Non-blocking mode
    u_long nonBlocking = 1;
    ioctlsocket(s, FIONBIO, &nonBlocking);

    // 2. Disable Nagle algorithm
    BOOL noDelay = TRUE;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));

    // 3. Connect to 127.0.0.1:<port>
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int connRes = connect(s, reinterpret_cast<SOCKADDR*>(&addr), sizeof(addr));
    if (connRes == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) return result;

        long remUs = getRemainingUs();
        if (remUs <= 0) return result;

        fd_set writeSet, errSet;
        FD_ZERO(&writeSet);
        FD_ZERO(&errSet);
        FD_SET(s, &writeSet);
        FD_SET(s, &errSet);

        timeval tv{ 0, remUs };
        int sel = select(0, nullptr, &writeSet, &errSet, &tv);
        if (sel <= 0 || FD_ISSET(s, &errSet)) return result;

        int sockErr = 0;
        int optLen = sizeof(sockErr);
        if (getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&sockErr), &optLen) != 0 || sockErr != 0) {
            return result;
        }
    }

    // 4. Send fast HTTP GET probe
    const char request[] =
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "User-Agent: PortPeek\r\n"
        "Accept: text/html,*/*\r\n"
        "Connection: close\r\n\r\n";

    long remUs = getRemainingUs();
    if (remUs <= 0) return result;

    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(s, &writeSet);
    timeval tvWrite{ 0, remUs };
    if (select(0, nullptr, &writeSet, nullptr, &tvWrite) <= 0) return result;

    send(s, request, static_cast<int>(sizeof(request) - 1), 0);

    // 5. Read response chunks
    std::string response;
    response.reserve(4096);
    char chunk[2048];

    while (true) {
        remUs = getRemainingUs();
        if (remUs <= 0) break;

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(s, &readSet);
        timeval tvRead{ 0, remUs };

        int sel = select(0, &readSet, nullptr, nullptr, &tvRead);
        if (sel <= 0) break;

        int bytes = recv(s, chunk, sizeof(chunk), 0);
        if (bytes <= 0) break;

        response.append(chunk, bytes);

        std::string lowerResp = detail::ToLower(response);
        if (lowerResp.find("</title>") != std::string::npos) break;

        if (response.size() >= 8192) break;
    }

    auto endTime = std::chrono::steady_clock::now();
    result.elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    if (response.empty()) return result;

    // Check for TLS alert / handshake (port is HTTPS)
    unsigned char firstByte = static_cast<unsigned char>(response[0]);
    if (firstByte == 0x15 || firstByte == 0x16) {
        result.isHttp = true;
        result.isHttps = true;
        return result;
    }

    // 6. Parse Status Code
    if (response.rfind("HTTP/", 0) == 0) {
        result.isHttp = true;
        size_t space = response.find(' ');
        if (space != std::string::npos && space + 3 <= response.size()) {
            try {
                result.statusCode = std::stoi(response.substr(space + 1, 3));
            } catch (...) {}
        }
    } else {
        return result;
    }

    std::string lowerResp = detail::ToLower(response);

    // Check for HTTPS error response on plaintext port (e.g. 400 The plain HTTP request was sent to HTTPS port)
    if (result.statusCode == 400 && (lowerResp.find("https") != std::string::npos || lowerResp.find("ssl") != std::string::npos)) {
        result.isHttps = true;
    }

    // 7. Parse Server Header
    std::string srv = detail::ExtractHeaderValue(lowerResp, response, "server");
    if (!srv.empty()) {
        result.serverHeader = detail::Trim(detail::Utf8ToWide(srv));
        std::string lowerSrv = detail::ToLower(srv);
        if (lowerSrv.find("uvicorn") != std::string::npos) result.frameworkHeader = L"FastAPI";
        else if (lowerSrv.find("werkzeug") != std::string::npos) result.frameworkHeader = L"Flask";
        else if (lowerSrv.find("kestrel") != std::string::npos) result.frameworkHeader = L"ASP.NET Core";
        else if (lowerSrv.find("caddy") != std::string::npos) result.frameworkHeader = L"Caddy";
    }

    // 8. Parse X-Powered-By Header
    std::string poweredBy = detail::ExtractHeaderValue(lowerResp, response, "x-powered-by");
    if (!poweredBy.empty()) {
        std::string lowerPw = detail::ToLower(poweredBy);
        if (lowerPw.find("next") != std::string::npos) result.frameworkHeader = L"Next.js";
        else if (lowerPw.find("express") != std::string::npos) result.frameworkHeader = L"Express";
    }

    // 9. Parse HTML Title
    size_t titleStart = lowerResp.find("<title");
    if (titleStart != std::string::npos) {
        size_t tagClose = response.find('>', titleStart);
        if (tagClose != std::string::npos) {
            size_t titleEnd = lowerResp.find("</title>", tagClose + 1);
            if (titleEnd != std::string::npos && titleEnd > tagClose + 1) {
                std::string rawTitle = response.substr(tagClose + 1, titleEnd - (tagClose + 1));
                result.htmlTitle = detail::DecodeHtmlEntities(detail::Trim(detail::Utf8ToWide(rawTitle)));
            }
        }
    }

    return result;
}

} // namespace NetProbe
