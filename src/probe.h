#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <string>

namespace NetProbe {

enum class PortHealth {
    Healthy, // < 50ms
    Slow,    // 50ms - 1000ms
    Hung,    // > 1000ms or unresponsive
    Unknown
};

struct HttpProbeResult {
    bool isHttp = false;
    bool isHttps = false;
    int statusCode = 0;
    std::wstring serverHeader;
    std::wstring htmlTitle;
    std::wstring frameworkHeader;
    double elapsedMs = 0.0;
    int latencyMs = 0;
    PortHealth health = PortHealth::Healthy;
};

// Performs a fast non-blocking loopback probe on 127.0.0.1:<port>
// Identifies HTTP vs HTTPS, parses HTML <title>, Server header, latency, and health
HttpProbeResult ProbeLoopbackPort(uint16_t port, uint32_t timeoutMs = 40);

} // namespace NetProbe
