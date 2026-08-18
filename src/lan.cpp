#include "lan.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

namespace LanUtil {

std::wstring GetPrimaryLanIpv4() {
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG outBufLen = 15000;
    std::vector<BYTE> buffer(outBufLen);
    PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());

    DWORD dwRetVal = GetAdaptersAddresses(AF_INET, flags, nullptr, pAddresses, &outBufLen);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(outBufLen);
        pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
        dwRetVal = GetAdaptersAddresses(AF_INET, flags, nullptr, pAddresses, &outBufLen);
    }

    std::wstring fallbackIp;

    if (dwRetVal == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr != nullptr; pCurr = pCurr->Next) {
            // Must be Up (connected)
            if (pCurr->OperStatus != IfOperStatusUp) continue;

            // Prefer Wi-Fi (71) or Ethernet (6)
            bool isPreferred = (pCurr->IfType == IF_TYPE_IEEE80211 || pCurr->IfType == IF_TYPE_ETHERNET_CSMACD);

            for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurr->FirstUnicastAddress; pUnicast != nullptr; pUnicast = pUnicast->Next) {
                if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
                    sockaddr_in* sa_in = reinterpret_cast<sockaddr_in*>(pUnicast->Address.lpSockaddr);
                    wchar_t ipStr[INET_ADDRSTRLEN] = { 0 };
                    InetNtopW(AF_INET, &(sa_in->sin_addr), ipStr, INET_ADDRSTRLEN);

                    // Skip loopback (127.x.x.x) and link-local (169.254.x.x)
                    if (wcsncmp(ipStr, L"127.", 4) != 0 && wcsncmp(ipStr, L"169.254.", 8) != 0) {
                        if (isPreferred) {
                            return std::wstring(ipStr);
                        } else if (fallbackIp.empty()) {
                            fallbackIp = ipStr;
                        }
                    }
                }
            }
        }
    }

    if (!fallbackIp.empty()) {
        return fallbackIp;
    }

    // Fallback: gethostname
    char hostName[256] = { 0 };
    if (gethostname(hostName, sizeof(hostName)) == 0) {
        struct addrinfo hints = {}, *res = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo(hostName, nullptr, &hints, &res) == 0 && res) {
            sockaddr_in* sa = reinterpret_cast<sockaddr_in*>(res->ai_addr);
            wchar_t ipStr[INET_ADDRSTRLEN] = { 0 };
            InetNtopW(AF_INET, &(sa->sin_addr), ipStr, INET_ADDRSTRLEN);
            freeaddrinfo(res);
            if (wcsncmp(ipStr, L"127.", 4) != 0) {
                return std::wstring(ipStr);
            }
        }
    }

    return L"127.0.0.1";
}

std::wstring FormatLanUrl(uint16_t port, const std::wstring& protocol) {
    std::wstring lanIp = GetPrimaryLanIpv4();
    return protocol + L"://" + lanIp + L":" + std::to_wstring(port);
}

} // namespace LanUtil
