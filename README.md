<div align="center">

  <img src="appicon.png" width="130" height="130" alt="PortPeek Icon" />

  # PortPeek

  ### *Stop typing `localhost:3000`. Just click and go.*

  [![Platform](https://img.shields.io/badge/Platform-Windows%2011%20%7C%2010%20(x64)-0078D6?style=for-the-badge&logo=windows)](https://github.com/i-ayushsingh/PortPeek)
  [![Binary Size](https://img.shields.io/badge/Binary-~360%20KB-22c55e?style=for-the-badge&logo=speedtest)](https://github.com/i-ayushsingh/PortPeek)
  [![CPU Usage](https://img.shields.io/badge/Idle%20CPU-0.0%25-brightgreen?style=for-the-badge)](https://github.com/i-ayushsingh/PortPeek)
  [![RAM Footprint](https://img.shields.io/badge/Memory-%3C%208%20MB-blue?style=for-the-badge)](https://github.com/i-ayushsingh/PortPeek)
  [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](https://opensource.org/licenses/MIT)

  <br/>

  <p align="center">
    <strong>PortPeek</strong> is an ultra-fast, invisible Windows system-tray utility that watches your local dev ports.<br/>
    Click your tray icon or press <code>Win + Alt + P</code> to inspect every active local server, test on mobile via QR code, share live public previews, and jump straight to your app in one click.
  </p>

  <p align="center">
    <a href="#-quick-download--install"><b>Download Now</b></a> •
    <a href="#-why-portpeek"><b>Why PortPeek?</b></a> •
    <a href="#-features-at-a-glance"><b>Features</b></a> •
    <a href="#-how-it-works"><b>How It Works</b></a> •
    <a href="#-shortcuts--tips"><b>Shortcuts & Tips</b></a> •
    <a href="https://i-ayushsingh.github.io/PortPeek/"><b>Official Website & Docs</b></a>
  </p>

</div>

---

## ⚡ The Pain vs The Cure

| 😫 Without PortPeek | 🚀 With PortPeek |
|---|---|
| Open new browser tab $\rightarrow$ try to remember if it was `:3000`, `:5173`, or `:8080`. | **Click tray icon $\rightarrow$ Click port.** You're instantly in your app. |
| Test responsive website on physical phone $\rightarrow$ find local Wi-Fi IPv4 in ipconfig $\rightarrow$ manually type URL. | **Click `Test on Phone` $\rightarrow$ Scan native QR code.** Instantly open on iOS/Android. |
| Share localhost preview with remote teammate $\rightarrow$ install & configure tunnel CLI. | **Click `Public Tunnel` $\rightarrow$ 1-click Cloudflare tunnel.** Copies HTTPS preview URL. |
| Kill stuck orphaned server: `netstat -ano \| findstr :3000` $\rightarrow$ `taskkill /F /PID 14200`. | **Click `Stop Process / Free Port` $\rightarrow$ Click port.** Done in 0.2 seconds. |
| Run 3 microservices $\rightarrow$ open 3 browser tabs one by one. | **Click `Open All Active (3)`** $\rightarrow$ all apps open together. |
| Heavy Electron / Webview background tools that eat 300 MB of RAM. | **Pure Win32 C++17 native:** **~360 KB binary**, **0.0% CPU**, **< 8 MB RAM**. |

---

## 📥 Quick Download & Install

Download the official Windows installer:

👉 **[Download PortPeek-Setup-v0.2.0.exe](https://github.com/i-ayushsingh/PortPeek/releases/latest)** *(~365 KB, Windows 11 & 10 x64)*

- 🚀 **Zero-Friction Per-User Installation:** Installs to `%LOCALAPPDATA%\Programs\PortPeek` with zero UAC Administrator prompt requirements.
- 🪟 **Seamless Shell Integration:** Adds Start Menu shortcuts, optional desktop shortcut, and a clean uninstaller in Windows Settings.
- 🍃 **Ultra-Lightweight:** Instant install in < 1 second.

> 💡 **Pro-Tip:** Once installed, click `Preferences ▶ Start with Windows` to have PortPeek silently available in your tray whenever your computer boots!

---

## 🎯 Features at a Glance

### 📱 1. Mobile LAN Tester & QR Code ("Test on Phone")
- Instant Wi-Fi LAN IP detection (`http://192.168.1.X:3000`) via native `GetAdaptersAddresses`.
- Generates standard **ISO/IEC 18004 QR Code Model 2** with 4-module quiet zone on a high-contrast white card.
- **Multi-Port Selection:** When multiple servers are active, choose which port to test on mobile.
- Scan with iOS Camera, Google Lens, Samsung Camera, or Brave Scanner for instant device testing.

### 🌐 2. One-Click Public Tunnel (Cloudflare & Localtunnel Bridge)
- Instantly share a live localhost preview with clients, remote teammates, or webhook testers (Stripe/GitHub).
- Supports 1-click **Cloudflare Quick Tunnels** (`cloudflared` free `*.trycloudflare.com`, zero login required).
- Provides copyable CLI commands for **Localtunnel** (`npx localtunnel --port <port>`).
- 1-click **Stop Tunnel** directly from the tray.

### ⏱️ 3. Real-Time Health & Latency Meter
- Subtle latency dots showing live responsiveness:
  - 🟢 **Fast / Responsive (< 50ms):** e.g. `3000 Next.js (2ms)`
  - 🟡 **Slow / Heavy (50ms – 1000ms):** e.g. `8000 Python Backend (420ms)`
  - 🔴 **Hung / Unresponsive (> 1000ms):** Auto-kill offer.

### 📌 4. Custom Port Aliases (`.portpeek`)
- Give custom, human-readable names to your database and service ports (e.g., `5432: Storefront DB`, `6379: Cache`).
- Supports workspace `.portpeek` / `portpeek.json` files, global `%USERPROFILE%\.portpeek\config.json`, and registry overrides.

### 🛠️ 5. Categorized System Background Processes
- Instead of a massive disorganized list, Windows background listeners are organized into clean submenus:
  - 🛠️ **IDE & Language Servers** (`Code.exe`, `OpenCode.exe`, `Antigravity.exe`, Language Servers)
  - ⚙️ **Windows System Services** (`135 RPC`, `139 NetBIOS`, `445 SMB`, `5357 WSDAPI`)
  - 📦 **Background Internal Ports** (ephemeral and loopback sockets)

### ⚡ 6. Port Conflict / "Free Up Port" Helper
- Click any port under `Stop Process / Free Port` to cleanly terminate whichever process holds it hostage.
- Displays PID, process name, and RAM usage before termination.

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
  │ 3000    PortPeek Docs [Next.js] (2ms • 48 MB)           │ ──▶ [1-Click] Opens http://localhost:3000
  │ 5173    dashboard [Vite] (1ms • 34 MB)                  │ ──▶ [1-Click] Opens http://localhost:5173
  │ 8000    api-service [FastAPI] (3ms • 28 MB)             │ ──▶ [1-Click] Opens http://localhost:8000
  │ Open All Active (3)                                     │ ──▶ [1-Click] Opens all 3 in browser tabs
  ├─────────────────────────────────────────────────────────┤
  │ System Background (29)                                ▶ │ ──▶ Categorized IDE, System & Internal sockets
  │ Copy URL                                                │ ──▶ Copy http://localhost:<port>
  │ Test on Phone (QR Code)                               ▶ │ ──▶ Choose port & scan mobile QR code
  │ Public Tunnel (Cloudflare)                            ▶ │ ──▶ Share temporary HTTPS preview URL
  │ Developer Tools                                       ▶ │ ──▶ Open in VS Code, Terminal, Markdown
  │ Stop Process / Free Port                              ▶ │ ──▶ Terminate stuck servers
  │ Port Aliases (.portpeek)                              ▶ │ ──▶ Custom port aliases guide & config
  │ Preferences                                           ▶ │ ──▶ Start with Windows & Hotkey toggle
  ├─────────────────────────────────────────────────────────┤
  │ Refresh                                                 │ ──▶ Instant rescan (PortPeek Refreshed ✓)
  │ Exit                                                    │
  └─────────────────────────────────────────────────────────┘
```

---

## 🕹️ Shortcuts & Quick Tips

| Shortcut / Action | What it does |
|---|---|
| **`Win + Alt + P`** | **Summon PortPeek anywhere** on your screen right near the system tray! |
| **`F5`** | **Refresh port list** with instant visual confirmation (`Refreshed ✓`). |
| **1-Click on Port** | Opens `localhost:<port>` in your browser immediately. |
| **`Open All Active (N)`** | Launches every active dev server in a new browser tab. |
| **`Test on Phone`** | Select any running dev port and scan the QR code from your phone camera. |
| **`Public Tunnel`** | Launches Cloudflare tunnel and copies public `*.trycloudflare.com` HTTPS URL. |
| **`Developer Tools ▶ Open in VS Code`** | Launches VS Code directly in the detected project folder. |
| **`Developer Tools ▶ Open in Terminal`** | Launches Windows Terminal (`wt.exe`) in the project folder. |
| **`Developer Tools ▶ Export Ports as Markdown`** | Copies an organized Markdown summary of all active ports. |
| **`Stop Process / Free Port`** | Kills an orphaned or stuck dev server holding your port hostage. |

---

<details>
<summary><b>🛠️ Power Developer Features & Architecture (Click to expand)</b></summary>

<br/>

### Deep Process & Kernel Inspection
- Direct Win32 `GetExtendedTcpTable` (IPv4 & IPv6) with zero process spawning overhead (< 1ms).
- Reads process command lines in user-mode via `NtQueryInformationProcess(ProcessCommandLineInformation = 60)` with least-privilege security (no Admin prompt required).
- Extracts active project directories automatically from script arguments (`node C:\Projects\cool-app\server.js` $\rightarrow$ `cool-app`).

### Fast Winsock Non-Blocking Probing (< 2ms)
- Non-blocking Winsock loopback probe with `TCP_NODELAY` and strict 25ms timeout bounds.
- Extracts HTML `<title>` tags and `Server:` / `X-Powered-By:` response headers without ever hanging the UI thread.
- Calculates microsecond round-trip connection latency for the real-time health meter.

### Standard ISO/IEC 18004 QR Generation
- Uses the official Nayuki Model 2 QR engine (`qrcodegen.c`) with Reed-Solomon polynomial division and 8-mask penalty scoring.
- Renders with a 4-module (28px) quiet zone for instant optical camera recognition.

### Exporting Ports Example
Clicking `Developer Tools ▶ Export Ports as Markdown` generates clean clipboard markdown:
```markdown
### Active Local Ports (PortPeek)
- [Next.js App](http://localhost:3000) — `node.exe` (Next.js) • 48 MB • PID 14200
- [Vite Dashboard](http://localhost:5173) — `node.exe` (Vite) • 34 MB • PID 16780
- [FastAPI Service](http://localhost:8000) — `python.exe` (FastAPI) • 28 MB • PID 18900
```

</details>

---

## 🔨 Building from Source

To compile PortPeek locally using Microsoft Visual C++:

```cmd
# 1. Clone the repository
git clone https://github.com/i-ayushsingh/PortPeek.git
cd PortPeek

# 2. Build Release x64 binary
build.bat

# 3. Build NSIS installer (requires NSIS 3+)
build_installer.bat
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for full architecture notes and guidelines.

---

## 📄 License

PortPeek is free and open-source software licensed under the **[MIT License](LICENSE)**.
