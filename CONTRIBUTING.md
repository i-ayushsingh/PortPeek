# Contributing to PortPeek

First off, thank you for considering contributing to PortPeek! 🎉

PortPeek is built on a simple philosophy:
> **One tiny utility. One job. No bloat.**

We aim to keep the compiled executable under **500 KB**, with **0.0% idle CPU usage**, and zero third-party runtime dependencies.

---

## 🛠️ Development Setup

### Prerequisites
- **Windows 10 / 11 (x64)**
- **Visual Studio 2022** (Community or Build Tools) with the *Desktop development with C++* workload
- **Python 3** with `pillow` (for generating multi-resolution icon assets)
- **CMake 3.16+** (optional, standard MSVC `build.bat` is provided)

### Quick Build
1. Clone the repository:
   ```cmd
   git clone https://github.com/i-ayushsingh/PortPeek.git
   cd PortPeek
   ```
2. Build the release binary:
   ```cmd
   build.bat
   ```
3. Run the automated test suite:
   ```cmd
   run_tests.bat
   ```

---

## 📐 Project Architecture

- `src/main.cpp`: Entry point, single-instance mutex, message loop.
- `src/tray.h / .cpp`: Windows System Tray (`NOTIFYICONDATAW` v4), Dark Mode context menu, global hotkeys.
- `src/ports.h / .cpp`: IPv4/IPv6 port enumeration via `GetExtendedTcpTable`, port sorting, browser launchers.
- `src/process.h / .cpp`: Process name extraction, kernel command line parsing via `NtQueryInformationProcess`, RAM usage, and framework detector.
- `src/probe.h / .cpp`: Non-blocking Winsock loopback HTTP/HTTPS probe with HTML `<title>` parsing.
- `src/theme.h / .cpp`: Windows 11 Dark / Light theme detection and `uxtheme` ordinal injection.
- `src/config.h / .cpp`: Registry configuration management and Windows startup hook.

---

## 🧪 Testing Guidelines

Before submitting a Pull Request, make sure all test suites pass:
```cmd
run_tests.bat
```

If you add new heuristics or network capabilities, please add corresponding unit test assertions in `tests/test_runner.cpp`.

---

## 📜 Pull Request Process

1. Fork the repository and create your branch from `main`.
2. Ensure code is clean, formatted with UTF-8 encoding, and free of compiler warnings.
3. Verify that the binary footprint remains minimal (< 500 KB) and does not introduce external DLL dependencies.
4. Submit your Pull Request with a clear description of the problem solved or feature added.
