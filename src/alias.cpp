#include "alias.h"
#include <windows.h>
#include <fstream>
#include <sstream>

namespace AliasManager {

namespace {

std::unordered_map<uint16_t, std::wstring> g_aliases;
bool g_initialized = false;

void LoadDefaults() {
    g_aliases[5432]  = L"Main PostgreSQL Database";
    g_aliases[3306]  = L"MySQL / MariaDB Server";
    g_aliases[6379]  = L"Redis Key-Value Cache";
    g_aliases[27017] = L"MongoDB Daemon";
    g_aliases[11434] = L"Ollama LLM (Llama 3)";
    g_aliases[8080]  = L"Spring Boot / Tomcat";
    g_aliases[9090]  = L"Prometheus Metrics";
    g_aliases[8501]  = L"Streamlit Data App";
    g_aliases[7860]  = L"Gradio ML Web UI";
}

void LoadFromConfigFile(const std::wstring& path) {
    std::wifstream file(path);
    if (!file.is_open()) return;

    std::wstring line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == L'#' || line[0] == L';') continue;
        size_t eq = line.find(L'=');
        if (eq != std::wstring::npos) {
            std::wstring key = line.substr(0, eq);
            std::wstring val = line.substr(eq + 1);

            // Trim
            while (!key.empty() && (key.back() == L' ' || key.back() == L'\t')) key.pop_back();
            while (!val.empty() && (val.front() == L' ' || val.front() == L'\t')) val.erase(val.begin());

            try {
                int portNum = std::stoi(key);
                if (portNum > 0 && portNum <= 65535 && !val.empty()) {
                    g_aliases[static_cast<uint16_t>(portNum)] = val;
                }
            } catch (...) {}
        }
    }
}

} // namespace

void Initialize() {
    if (g_initialized) return;
    LoadDefaults();

    // 1. Check workspace .portpeek in current working directory
    LoadFromConfigFile(L".portpeek");
    LoadFromConfigFile(L".portpeek.ini");

    // 2. Check %USERPROFILE%\.portpeek
    wchar_t userProfile[MAX_PATH] = { 0 };
    if (GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH) > 0) {
        std::wstring userPath = std::wstring(userProfile) + L"\\.portpeek";
        LoadFromConfigFile(userPath);
    }

    g_initialized = true;
}

void Reload() {
    g_aliases.clear();
    g_initialized = false;
    Initialize();
}

std::wstring GetAliasForPort(uint16_t port) {
    if (!g_initialized) Initialize();
    auto it = g_aliases.find(port);
    if (it != g_aliases.end()) {
        return it->second;
    }
    return L"";
}

void SetAlias(uint16_t port, const std::wstring& name) {
    if (!g_initialized) Initialize();
    if (name.empty()) {
        g_aliases.erase(port);
    } else {
        g_aliases[port] = name;
    }
}

std::unordered_map<uint16_t, std::wstring> GetAllAliases() {
    if (!g_initialized) Initialize();
    return g_aliases;
}

} // namespace AliasManager
