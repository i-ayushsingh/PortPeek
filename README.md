<div align="center">

  <img src="appicon.png" width="130" height="130" alt="PortPeek Icon" />

  # PortPeek

  ### *Stop typing `localhost:3000`. Just click and go.*

  [![Platform](https://img.shields.io/badge/Platform-Windows%2011%20%7C%2010%20(x64)-0078D6?style=for-the-badge&logo=windows)](https://github.com/i-ayushsingh/PortPeek)
  [![Binary Size](https://img.shields.io/badge/Binary-~340%20KB-22c55e?style=for-the-badge&logo=speedtest)](https://github.com/i-ayushsingh/PortPeek)
  [![CPU Usage](https://img.shields.io/badge/Idle%20CPU-0.0%25-brightgreen?style=for-the-badge)](https://github.com/i-ayushsingh/PortPeek)
  [![RAM Footprint](https://img.shields.io/badge/Memory-%3C%208%20MB-blue?style=for-the-badge)](https://github.com/i-ayushsingh/PortPeek)
  [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](https://opensource.org/licenses/MIT)

  <br/>

  <p align="center">
    <strong>PortPeek</strong> is an ultra-fast, invisible Windows system-tray utility that watches your local dev ports.<br/>
    Click your tray icon or press <code>Win + Alt + P</code> to see every active local server and jump straight to your app in one click.
  </p>

  <p align="center">
    <a href="#-quick-download--install"><b>Download Now</b></a> •
    <a href="#-why-portpeek"><b>Why PortPeek?</b></a> •
    <a href="#-how-it-works"><b>How It Works</b></a> •
    <a href="#-power-features"><b>Power Features</b></a> •
    <a href="#-shortcuts--tips"><b>Shortcuts & Tips</b></a>
  </p>

</div>

---

## ⚡ The Pain vs The Cure

| 😫 Without PortPeek | 🚀 With PortPeek |
|---|---|
| Open new browser tab $\rightarrow$ try to remember if it was `:3000`, `:5173`, or `:8080`. | **Click tray icon $\rightarrow$ Click port.** You're instantly in your app. |
| Open terminal $\rightarrow$ run `netstat -ano \| findstr :3000` $\rightarrow$ copy PID $\rightarrow$ `taskkill /F /PID 14200`. | **Hover `Stop Process` $\rightarrow$ Click port.** Done in 0.5 seconds. |
| Run 3 microservices $\rightarrow$ open 3 browser tabs one by one. | **Click `Open All Active (3)`** $\rightarrow$ all apps open together. |
| Heavy Electron / Webview background tools that eat 300 MB of RAM. | **Pure Win32 C++ native:** **~340 KB binary**, **0.0% CPU**, **< 8 MB RAM**. |

---

## 📥 Quick Download & Install

### Option A: Standalone Portable EXE *(Easiest)*
1. Go to **[Releases](https://github.com/i-ayushsingh/PortPeek/releases)**.
2. Download **`PortPeek.exe`**.
3. Place it anywhere and double-click to run. *Zero install, zero setup, zero dependencies.*

### Option B: Windows Package Managers *(Coming Soon)*
```powershell
# Via Winget
winget install PortPeek

# Via Scoop
scoop install portpeek
```

> 💡 **Pro-Tip:** Once running, click `Preferences ▶ Start with Windows` to have PortPeek silently available every time your PC boots!

---

## 🎮 How It Works

```text
┌─────────────────────────────────────────────────────────────┐
│  Press [Win + Alt + P]  OR  Click PortPeek System Tray Icon │
└──────────────────────────────┬──────────────────────────────┘
                               │
                               ▼
  ┌─────────────────────────────────────────────────────────┐
  │ PortPeek (3 active)                                     │
  ├─────────────────────────────────────────────────────────┤
  │ 3000    PortPeak [Next.js] (48 MB)                      │ ──▶ [1-Click] Opens http://localhost:3000
  │ 5173    dashboard [Vite] (34 MB)                        │ ──▶ [1-Click] Opens http://localhost:5173
  │ 8000    api-service [FastAPI] (28 MB)                   │ ──▶ [1-Click] Opens http://localhost:8000
  │ Open All Active (3)                                     │ ──▶ [1-Click] Opens all 3 in browser tabs
  ├─────────────────────────────────────────────────────────┤
  │ System & Background (24)                              ▶ │ ──▶ Windows OS & background listeners
  │ Copy URL                                              ▶ │ ──▶ Copy http://localhost:<port>
  │ Developer Tools                                       ▶ │ ──▶ Open in VS Code, Terminal, Markdown
  │ Stop Process                                          ▶ │ ──▶ Cleanly terminate stuck servers
  │ Preferences                                           ▶ │ ──▶ Start with Windows & Hotkey toggle
  ├─────────────────────────────────────────────────────────┤
  │ Refresh                                                 │
  │ Exit                                                    │
  └─────────────────────────────────────────────────────────┘
```

---

## 🎯 Features at a Glance

- 🚀 **1-Click Localhost Access:** Click any port in your tray menu to immediately launch `http://localhost:<port>` or `https://localhost:<port>` in your default browser.
- 🔍 **Framework & Stack Intelligence:** Automatically identifies **Next.js, Vite, React, Vue, FastAPI, Django, Flask, Express, NestJS, Go, Rust, Docker, Ollama**, and displays real-time RAM usage.
- 🔒 **HTTPS Auto-Detection:** Automatically checks local TLS/SSL handshakes to route to `https://` when applicable.
- ⚡ **"Open All Active":** Have a frontend, backend, and documentation server running? Open all of them at once with a single click.
- 🎨 **Native Windows 11 Light & Dark Themes:** Uses native `uxtheme` hooks to match your system theme with pixel-perfect Per-Monitor V2 DPI clarity.
- 🪟 **True Zero-Window Utility:** Invisible until you need it. No clutter on your Alt+Tab or taskbar.

---

## 🕹️ Shortcuts & Quick Tips

| Shortcut / Action | What it does |
|---|---|
| **`Win + Alt + P`** | **Summon PortPeek anywhere** on your screen right under your cursor! |
| **1-Click on Port** | Opens `localhost:<port>` in your browser immediately. |
| **`Open All Active (N)`** | Launches every active dev server in a new browser tab. |
| **`Developer Tools ▶ Open in VS Code`** | Launches VS Code directly in the detected project folder. |
| **`Developer Tools ▶ Open in Terminal`** | Launches Windows Terminal (`wt.exe`) in the project folder. |
| **`Developer Tools ▶ Export Ports as Markdown`** | Copies an organized Markdown summary of all your active ports to share in Slack/Discord or notes. |
| **`Stop Process`** | Kills an orphaned or stuck dev server holding your port hostage. |

---

<details>
<summary><b>🛠️ Power Developer Features & Architecture (Click to expand)</b></summary>

<br/>

### Deep Process & Kernel Inspection
- Uses low-level Win32 `GetExtendedTcpTable` (IPv4 and IPv6) with zero process spawning overhead.
- Reads process command lines in user-mode via `NtQueryInformationProcess(ProcessCommandLineInformation = 60)` with least-privilege security.
- Isolates project directory names without needing administrator permissions.

### Fast Loopback Probing (< 2ms)
- Sends non-blocking loopback HTTP requests with `TCP_NODELAY` and strict 25ms timeout bounds.
- Extracts HTML `<title>` tags and `Server:` / `X-Powered-By:` response headers without ever hanging your system.

### Exporting Ports Example
Clicking `Developer Tools ▶ Export Ports as Markdown` generates clean clipboard markdown:
```markdown
### Active Local Ports (PortPeek)
- [Next.js App](http://localhost:3000) — `node.exe` (Next.js) • 48 MB • PID 14200
- [FastAPI Docs](http://localhost:8000) — `python.exe` (FastAPI) • 28 MB • PID 18900
```

</details>

---

## 🔨 Building from Source

If you want to compile PortPeek locally or contribute:

```cmd
# 1. Clone the repository
git clone https://github.com/i-ayushsingh/PortPeek.git
cd PortPeek

# 2. Build Release x64 binary (MSVC)
build.bat

# 3. Run automated verification test suite
run_tests.bat
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for full architecture notes and guidelines.

---

## 📄 License

PortPeek is free and open-source software licensed under the **[MIT License](LICENSE)**.
