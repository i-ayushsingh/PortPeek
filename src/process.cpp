#include "process.h"
#include <winternl.h>
#include <psapi.h>
#include <shellapi.h>
#include <vector>
#include <algorithm>
#include <cwctype>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shell32.lib")

namespace ProcessUtil {

namespace {

constexpr PROCESSINFOCLASS ProcessCommandLineInformation = static_cast<PROCESSINFOCLASS>(60);

typedef NTSTATUS(NTAPI* PFN_NtQueryInformationProcess)(
    HANDLE ProcessHandle,
    PROCESSINFOCLASS ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength
);

PFN_NtQueryInformationProcess GetNtQueryInformationProcess() {
    static PFN_NtQueryInformationProcess pfn = []() -> PFN_NtQueryInformationProcess {
        HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
        if (!hNtdll) return nullptr;
        return reinterpret_cast<PFN_NtQueryInformationProcess>(
            GetProcAddress(hNtdll, "NtQueryInformationProcess")
        );
    }();
    return pfn;
}

std::wstring ToLower(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) {
        return static_cast<wchar_t>(std::towlower(c));
    });
    return s;
}

std::wstring NormalizeSeparators(std::wstring p) {
    std::replace(p.begin(), p.end(), L'/', L'\\');
    return p;
}

std::wstring GetFileName(const std::wstring& path) {
    auto norm = NormalizeSeparators(path);
    size_t lastSlash = norm.find_last_of(L'\\');
    return (lastSlash != std::wstring::npos) ? norm.substr(lastSlash + 1) : norm;
}

bool EndsWith(const std::wstring& str, const wchar_t* suffix) {
    size_t sLen = wcslen(suffix);
    if (str.length() < sLen) return false;
    return str.compare(str.length() - sLen, sLen, suffix) == 0;
}

bool Contains(const std::wstring& haystack, const wchar_t* needle) {
    return haystack.find(needle) != std::wstring::npos;
}

std::wstring DetectFramework(const std::wstring& cmdLower, const std::wstring& procLower) {
    if (Contains(cmdLower, L"next") || Contains(cmdLower, L"next-server")) return L"Next.js";
    if (Contains(cmdLower, L"vite")) return L"Vite";
    if (Contains(cmdLower, L"astro")) return L"Astro";
    if (Contains(cmdLower, L"remix")) return L"Remix";
    if (Contains(cmdLower, L"nuxt")) return L"Nuxt";
    if (Contains(cmdLower, L"react-scripts") || Contains(cmdLower, L"webpack-dev-server")) return L"React";
    if (Contains(cmdLower, L"express")) return L"Express";
    if (Contains(cmdLower, L"nest")) return L"NestJS";
    if (Contains(cmdLower, L"uvicorn")) return L"FastAPI";
    if (Contains(cmdLower, L"gunicorn")) return L"Gunicorn";
    if (Contains(cmdLower, L"flask")) return L"Flask";
    if (Contains(cmdLower, L"manage.py") || Contains(cmdLower, L"django")) return L"Django";
    if (Contains(cmdLower, L"cargo") || procLower == L"cargo.exe") return L"Rust";
    if (procLower == L"go.exe" || procLower == L"air.exe") return L"Go";
    if (procLower == L"docker-proxy.exe" || procLower == L"docker.exe") return L"Docker";
    if (procLower == L"ollama.exe" || procLower == L"ollama_llama_server.exe") return L"Ollama AI";
    if (procLower == L"postgres.exe") return L"PostgreSQL";
    if (procLower == L"mysqld.exe" || procLower == L"mariadbd.exe") return L"MySQL";
    if (procLower == L"redis-server.exe") return L"Redis";
    if (procLower == L"code.exe") return L"VS Code Live Server";

    return L"";
}

std::wstring ExtractProjectFolderFromPath(const std::wstring& rawPath, std::wstring& outFullFolder) {
    if (rawPath.empty()) return L"";
    std::wstring path = NormalizeSeparators(rawPath);

    while (!path.empty() && path.back() == L'\\') path.pop_back();

    std::wstring lower = ToLower(path);

    static const wchar_t* const FOLDERS[] = {
        L"\\node_modules", L"\\.venv", L"\\venv", L"\\site-packages",
        L"\\vendor", L"\\target", L"\\bin\\debug", L"\\bin\\release"
    };

    for (const wchar_t* folder : FOLDERS) {
        size_t idx = lower.find(folder);
        if (idx != std::wstring::npos) {
            std::wstring rootPath = path.substr(0, idx);
            outFullFolder = rootPath;
            size_t slash = rootPath.find_last_of(L'\\');
            return (slash != std::wstring::npos) ? rootPath.substr(slash + 1) : rootPath;
        }
    }

    size_t lastSlash = path.find_last_of(L'\\');
    if (lastSlash != std::wstring::npos) {
        std::wstring parent = path.substr(0, lastSlash);
        std::wstring lowerParent = ToLower(parent);
        if (EndsWith(lowerParent, L"\\src") || EndsWith(lowerParent, L"\\app") ||
            EndsWith(lowerParent, L"\\cmd") || EndsWith(lowerParent, L"\\dist") ||
            EndsWith(lowerParent, L"\\lib") || EndsWith(lowerParent, L"\\bin")) {
            size_t prevSlash = parent.find_last_of(L'\\');
            if (prevSlash != std::wstring::npos) {
                parent = parent.substr(0, prevSlash);
            }
        }
        outFullFolder = parent;
        size_t pSlash = parent.find_last_of(L'\\');
        return (pSlash != std::wstring::npos) ? parent.substr(pSlash + 1) : parent;
    }

    return L"";
}

bool IsFlagOrOption(const std::wstring& token) {
    return token.rfind(L"-", 0) == 0 || token.rfind(L"/", 0) == 0;
}

bool IsGenericRuntimeCommand(const std::wstring& lower) {
    return lower == L"run" || lower == L"start" || lower == L"dev" || lower == L"serve" ||
           lower == L"exec" || lower == L"watch" || lower == L"node" || lower == L"python" ||
           lower == L"uvicorn" || lower == L"gunicorn" || lower == L"dotnet" || lower == L"bun" ||
           lower == L"deno" || lower == L"npx" || lower == L"pnpm" || lower == L"yarn" || lower == L"npm";
}

} // namespace

struct ScopedHandle {
    HANDLE handle = nullptr;
    explicit ScopedHandle(HANDLE h) : handle(h) {}
    ~ScopedHandle() {
        if (handle && handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
        }
    }
    operator HANDLE() const { return handle; }
    bool IsValid() const { return handle && handle != INVALID_HANDLE_VALUE; }

    ScopedHandle(const ScopedHandle&) = delete;
    ScopedHandle& operator=(const ScopedHandle&) = delete;
    ScopedHandle(ScopedHandle&& other) noexcept : handle(other.handle) { other.handle = nullptr; }
    ScopedHandle& operator=(ScopedHandle&& other) noexcept {
        if (this != &other) {
            if (handle && handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }
};

std::wstring ExtractFileName(const std::wstring& fullPath) {
    if (fullPath.empty()) return L"";
    size_t lastSlash = fullPath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos && lastSlash + 1 < fullPath.length()) {
        return fullPath.substr(lastSlash + 1);
    }
    return fullPath;
}

std::wstring GetProcessNameByPid(DWORD pid) {
    if (pid == 0) return L"System Idle Process";
    if (pid == 4) return L"System";

    ScopedHandle hProcess(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
    if (!hProcess.IsValid()) {
        hProcess = ScopedHandle(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid));
    }

    if (!hProcess.IsValid()) {
        return L"Unknown process";
    }

    wchar_t pathBuffer[MAX_PATH * 4] = { 0 };
    DWORD bufferSize = static_cast<DWORD>(sizeof(pathBuffer) / sizeof(pathBuffer[0]));

    if (QueryFullProcessImageNameW(hProcess, 0, pathBuffer, &bufferSize) && bufferSize > 0) {
        return ExtractFileName(pathBuffer);
    }

    bufferSize = static_cast<DWORD>(sizeof(pathBuffer) / sizeof(pathBuffer[0]));
    if (QueryFullProcessImageNameW(hProcess, PROCESS_NAME_NATIVE, pathBuffer, &bufferSize) && bufferSize > 0) {
        return ExtractFileName(pathBuffer);
    }

    if (GetProcessImageFileNameW(hProcess, pathBuffer, static_cast<DWORD>(sizeof(pathBuffer) / sizeof(pathBuffer[0]))) > 0) {
        return ExtractFileName(pathBuffer);
    }

    if (GetModuleBaseNameW(hProcess, NULL, pathBuffer, static_cast<DWORD>(sizeof(pathBuffer) / sizeof(pathBuffer[0]))) > 0) {
        return pathBuffer;
    }

    return L"Unknown process";
}

std::wstring GetProcessCommandLine(DWORD pid) {
    if (pid == 0 || pid == 4) return L"";

    auto pfnNtQuery = GetNtQueryInformationProcess();
    if (!pfnNtQuery) return L"";

    ScopedHandle hProcess(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
    if (!hProcess.IsValid()) {
        hProcess = ScopedHandle(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid));
    }
    if (!hProcess.IsValid()) return L"";

    ULONG returnLength = 0;
    NTSTATUS status = pfnNtQuery(
        hProcess,
        ProcessCommandLineInformation,
        nullptr,
        0,
        &returnLength
    );

    if (returnLength == 0) return L"";

    std::vector<BYTE> buffer(returnLength + sizeof(wchar_t), 0);
    status = pfnNtQuery(
        hProcess,
        ProcessCommandLineInformation,
        buffer.data(),
        static_cast<ULONG>(buffer.size()),
        &returnLength
    );

    if (status < 0 || returnLength < sizeof(UNICODE_STRING)) {
        return L"";
    }

    auto* pUnicodeStr = reinterpret_cast<PUNICODE_STRING>(buffer.data());
    if (!pUnicodeStr->Buffer || pUnicodeStr->Length == 0) {
        return L"";
    }

    size_t charCount = pUnicodeStr->Length / sizeof(wchar_t);
    return std::wstring(pUnicodeStr->Buffer, charCount);
}

size_t GetProcessMemoryUsageMb(DWORD pid) {
    if (pid == 0 || pid == 4) return 0;

    ScopedHandle hProcess(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
    if (!hProcess.IsValid()) return 0;

    PROCESS_MEMORY_COUNTERS pmc = { sizeof(PROCESS_MEMORY_COUNTERS) };
    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / (1024 * 1024);
    }
    return 0;
}

ProjectInfo ParseCommandLine(const std::wstring& cmdLine, const std::wstring& fallbackProcessName) {
    ProjectInfo res;
    std::wstring lowerCmd = ToLower(cmdLine);
    std::wstring lowerProc = ToLower(fallbackProcessName);

    res.framework = DetectFramework(lowerCmd, lowerProc);

    if (cmdLine.empty()) {
        res.projectName = fallbackProcessName;
        res.displayLabel = fallbackProcessName;
        return res;
    }

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(cmdLine.c_str(), &argc);
    if (!argv || argc == 0) {
        res.projectName = fallbackProcessName;
        res.displayLabel = fallbackProcessName;
        return res;
    }

    struct ArgvGuard {
        LPWSTR* ptr;
        ~ArgvGuard() { if (ptr) LocalFree(ptr); }
    } guard{ argv };

    std::vector<std::wstring> tokens;
    tokens.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        tokens.emplace_back(argv[i]);
    }

    // Check for Python module syntax: python -m <module>
    for (size_t i = 1; i < tokens.size(); ++i) {
        if (tokens[i] == L"-m" && i + 1 < tokens.size()) {
            res.scriptName = tokens[i + 1];
            if ((tokens[i + 1] == L"uvicorn" || tokens[i + 1] == L"gunicorn") && i + 2 < tokens.size() && !IsFlagOrOption(tokens[i + 2])) {
                res.scriptName = tokens[i + 2];
            }
            res.projectName = res.scriptName;
            res.displayLabel = res.scriptName + L" (" + fallbackProcessName + L")";
            return res;
        }
    }

    // Inspect positional arguments for folders & files
    for (size_t i = 1; i < tokens.size(); ++i) {
        const auto& tok = tokens[i];
        if (IsFlagOrOption(tok)) continue;

        std::wstring lowerTok = ToLower(tok);
        if (IsGenericRuntimeCommand(lowerTok)) continue;

        if (tok.find(L'\\') != std::wstring::npos || tok.find(L'/') != std::wstring::npos || tok.find(L'.') != std::wstring::npos) {
            std::wstring folderFull;
            std::wstring folder = ExtractProjectFolderFromPath(tok, folderFull);
            std::wstring file = GetFileName(tok);

            res.projectFolder = folderFull;
            if (!folder.empty() && !file.empty()) {
                res.projectName = folder;
                res.scriptName = file;
                res.displayLabel = folder + L" (" + fallbackProcessName + L")";
                return res;
            } else if (!file.empty()) {
                res.scriptName = file;
                res.projectName = file;
                res.displayLabel = file + L" (" + fallbackProcessName + L")";
                return res;
            }
        }

        if (tok.find(L':') != std::wstring::npos) {
            res.projectName = tok;
            res.scriptName = tok;
            res.displayLabel = tok + L" (" + fallbackProcessName + L")";
            return res;
        }
    }

    for (size_t i = 1; i < tokens.size(); ++i) {
        if (!IsFlagOrOption(tokens[i])) {
            res.projectName = GetFileName(tokens[i]);
            res.displayLabel = res.projectName + L" (" + fallbackProcessName + L")";
            return res;
        }
    }

    res.projectName = !fallbackProcessName.empty() ? fallbackProcessName : GetFileName(tokens[0]);
    res.displayLabel = res.projectName;
    return res;
}

bool TerminateProcessByPid(DWORD pid) {
    if (pid == 0 || pid == 4) return false;

    ScopedHandle hProcess(OpenProcess(PROCESS_TERMINATE, FALSE, pid));
    if (!hProcess.IsValid()) return false;

    return TerminateProcess(hProcess, 1) != FALSE;
}

bool OpenProjectInEditor(const std::wstring& folderPath, const std::wstring& editorCmd) {
    if (folderPath.empty()) return false;

    HINSTANCE hInst = ShellExecuteW(
        nullptr,
        L"open",
        editorCmd.c_str(),
        (L"\"" + folderPath + L"\"").c_str(),
        nullptr,
        SW_SHOWNORMAL
    );

    return (reinterpret_cast<INT_PTR>(hInst) > 32);
}

bool OpenProjectInTerminal(const std::wstring& folderPath) {
    if (folderPath.empty()) return false;

    // Try Windows Terminal first (wt.exe -d <path>)
    HINSTANCE hInst = ShellExecuteW(
        nullptr,
        L"open",
        L"wt.exe",
        (L"-d \"" + folderPath + L"\"").c_str(),
        nullptr,
        SW_SHOWNORMAL
    );

    if (reinterpret_cast<INT_PTR>(hInst) > 32) {
        return true;
    }

    // Fallback to powershell.exe with WorkingDirectory
    hInst = ShellExecuteW(
        nullptr,
        L"open",
        L"powershell.exe",
        L"-NoExit",
        folderPath.c_str(),
        SW_SHOWNORMAL
    );

    return (reinterpret_cast<INT_PTR>(hInst) > 32);
}

} // namespace ProcessUtil
