#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

struct DiscoveredSession {
    std::wstring applicationId;    // e.g. "ephe"
    std::wstring applicationName;  // e.g. "Ephe"
    std::wstring sessionId;        // e.g. "abc123"
    std::wstring sessionName;      // e.g. "College Notes"
    std::wstring version;          // e.g. "0.8.0"
    uint16_t     port = 0;         // e.g. 5500
    std::wstring localUrl;         // e.g. "http://localhost:5500"
    std::wstring hostUrl;          // e.g. "http://localhost:5500/?role=desktop"
    std::wstring status;           // e.g. "active"
    int          connectedDevices = 0;

    // Helper to get the preferred launch URL for opening desktop host experience
    std::wstring GetLaunchUrl() const {
        return !hostUrl.empty() ? hostUrl : localUrl;
    }
};

class IAppDiscoveryProvider {
public:
    virtual ~IAppDiscoveryProvider() = default;
    virtual std::wstring GetProviderId() const = 0;
    virtual std::vector<DiscoveredSession> DiscoverSessions() = 0;
};

class EpheDiscoveryProvider : public IAppDiscoveryProvider {
public:
    std::wstring GetProviderId() const override { return L"ephe"; }
    std::vector<DiscoveredSession> DiscoverSessions() override;

    // Public helpers for verification, testing, and custom directory probing
    static std::vector<DiscoveredSession> DiscoverFromDirectory(const std::wstring& directoryPath, bool verifyReachability = true);
    static bool ParseAndValidateManifest(const std::string& jsonContent, DiscoveredSession& outSession);
    static bool VerifyLocalPortReachable(uint16_t port, int timeoutMs = 25);
};

class EpheLauncher {
public:
    // Finds the ephe.exe binary on the system (App Paths, standard install locations, PATH)
    static std::wstring FindEpheExecutable();

    // Returns true if Ephe is installed or available on this system
    static bool IsEpheAvailable();

    // Launches ephe.exe with --new-session parameter
    static bool LaunchNewSession(const std::wstring& customExePath = L"");
};

class AppDiscoveryManager {
public:
    static AppDiscoveryManager& Instance();

    void RegisterProvider(std::unique_ptr<IAppDiscoveryProvider> provider);
    std::vector<DiscoveredSession> DiscoverAllVerifiedSessions();
    const std::vector<DiscoveredSession>& GetCachedSessions() const { return m_cachedSessions; }
    void Refresh();

private:
    AppDiscoveryManager();
    std::vector<std::unique_ptr<IAppDiscoveryProvider>> m_providers;
    std::vector<DiscoveredSession> m_cachedSessions;
};
