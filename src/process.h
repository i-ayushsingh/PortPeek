#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

namespace ProcessUtil {

struct ProjectInfo {
    std::wstring projectName;    // e.g. "PortPeak", "my-app", "server.js"
    std::wstring scriptName;     // e.g. "server.js", "app.main:app"
    std::wstring projectFolder;  // e.g. "C:\Projects\PortPeak"
    std::wstring framework;      // e.g. "Next.js", "Vite", "FastAPI", "Express", "Docker"
    std::wstring displayLabel;   // Formatted for UI: "my-app (node.exe)"
};

// Resolves a process ID (PID) to its executable name (e.g. "node.exe", "vite.exe")
std::wstring GetProcessNameByPid(DWORD pid);

// Queries the full command line of a process using NtQueryInformationProcess (Class 60)
std::wstring GetProcessCommandLine(DWORD pid);

// Returns process working set memory in Megabytes (MB)
size_t GetProcessMemoryUsageMb(DWORD pid);

// Parses command line to extract active project folder, script name, framework, and display label
ProjectInfo ParseCommandLine(const std::wstring& cmdLine, const std::wstring& fallbackProcessName);

// Terminates a process by PID (for port cleanup action)
bool TerminateProcessByPid(DWORD pid);

// Extracts the file name from a full Win32 or NT path
std::wstring ExtractFileName(const std::wstring& fullPath);

// Launches project directory in VS Code / default editor
bool OpenProjectInEditor(const std::wstring& folderPath, const std::wstring& editorCmd = L"code");

// Launches project directory in Windows Terminal / PowerShell
bool OpenProjectInTerminal(const std::wstring& folderPath);

} // namespace ProcessUtil
