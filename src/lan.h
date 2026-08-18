#pragma once

#include <string>
#include <vector>

namespace LanUtil {

struct LanInterface {
    std::wstring adapterName;
    std::wstring ipv4Address;
    bool isWifi = false;
};

// Finds the best active local network IPv4 address (e.g. 192.168.1.45)
std::wstring GetPrimaryLanIpv4();

// Formats a full Mobile LAN test URL (e.g. "http://192.168.1.45:3000")
std::wstring FormatLanUrl(uint16_t port, const std::wstring& protocol = L"http");

} // namespace LanUtil
