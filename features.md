# PortPeek — Feature Specifications & Status (`features.md`)

This document specifies the architecture, user experience, and technical implementation for all developer productivity features in PortPeek v0.2.1.

---

## 1. 📱 Mobile LAN Tester & QR Code ("Test on Phone")
**Status:** ✅ **Shipped & Verified in v0.2.1**

### Overview
When building responsive web applications, frontend developers constantly need to test UI, touch gestures, safe areas, and viewport rendering on physical iOS/Android devices connected to the same Wi-Fi network.

### User Flow
1. Open the PortPeek tray popover (click tray icon or press `Win + Alt + P`).
2. Click **"📱 Test on Phone (QR Code)"**.
3. **Multi-Port Chooser:** If multiple dev servers are running (e.g. `3000`, `5173`, `8000`), PortPeek prompts you to pick which server you want to test on mobile.
4. The native subview displays:
   - **Local Wi-Fi Address:** `http://192.168.1.X:<port>` with a 1-click **Copy** button.
   - **Standard ISO/IEC 18004 QR Code:** High-contrast, scannable QR code matrix rendered directly via Win32 GDI with a 28px (4-module) quiet zone.
   - **Zero-Server Safe State:** If no dev servers are active, shows a clean guidance state instead of faking port 3000.
5. The developer points their phone camera (iOS Camera, Google Lens, Samsung Camera, Brave Scanner) to immediately open the local dev site.

### Technical Implementation
- **LAN IP Discovery (`src/lan.cpp`):** Uses `GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, ...)` to find the active Wi-Fi / Ethernet adapter's non-loopback IPv4 address (`192.168.x.x` or `10.x.x.x`).
- **QR Engine (`src/qrcodegen.c` & `src/qrcode.cpp`):** Integrated the official Nayuki Model 2 QR Code generator with Reed-Solomon polynomial division, Error Correction Level M, and automatic mask penalty optimization.
- **GDI Double-Buffered Rendering:** 7px solid `#000000` modules centered on pure `#FFFFFF` with 28px quiet zone.

---

## 2. 🌐 One-Click Public Tunnel (Cloudflare Bridge)
**Status:** ✅ **Shipped & Verified in v0.2.1**

### Overview
Developers frequently need to share a work-in-progress localhost site with clients, remote teammates, or webhook endpoints (e.g., Stripe, GitHub, Twilio) without deploying to staging or configuring router port forwarding.

### User Flow
1. Click **"Public Tunnel (Cloudflare)"** in the tray menu.
2. If `cloudflared` is installed, click **"Start Cloudflare Quick Tunnel (:port)"**.
3. Spawns `cloudflared` in the background with `CREATE_NO_WINDOW`.
4. Automatically parses the temporary public URL (`https://*.trycloudflare.com`) and copies it to the Windows clipboard.
5. Displays **"Stop Active Tunnel"** with 1-click termination.
6. Also provides 1-click copyable CLI commands for **Cloudflare** and **Localtunnel** (`npx localtunnel --port <port>`).

### Technical Implementation
- Spawns child process via Win32 `CreateProcessW` with redirected anonymous pipes.
- Scans output streams asynchronously with regex for `https://[a-zA-Z0-9-]+\.trycloudflare\.com`.
- Handles process lifecycle and cleans up child processes upon exit.

---

## 3. ⏱️ Real-Time Health & Latency Meter
**Status:** ✅ **Shipped & Verified in v0.2.1**

### Overview
Shows live responsiveness metrics next to running servers so developers know immediately if their backend is fast, slowing down, or completely hung/deadlocked.

### Latency Categorization & Visual Indicators
- 🟢 **Fast / Responsive (< 50ms):** e.g., `3000 Next.js (2ms)`
- 🟡 **Slow / Busy (50ms – 1000ms):** e.g., `8000 Python Backend (450ms)`
- 🔴 **Hung / Deadlocked (> 1000ms or Connection Timeout):** e.g., `5000 Flask Server (Hung)`

### Technical Implementation
- Non-blocking Winsock loopback probe (`src/probe.cpp`):
  1. Creates socket `socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)`.
  2. Sets non-blocking mode: `u_long mode = 1; ioctlsocket(s, FIONBIO, &mode)`.
  3. Records high-resolution timestamp via `QueryPerformanceCounter`.
  4. Calls `connect()` to `127.0.0.1:<port>`.
  5. Uses `select()` with `timeval` of 25ms timeout.
  6. Calculates elapsed round-trip latency in milliseconds.
  7. Updates cached port struct with `latencyMs` and health status.

---

## 4. 📌 Custom Port Aliases & Workspace `.portpeek`
**Status:** ✅ **Shipped & Verified in v0.2.1**

### Overview
Developers working on complex microservices or backend stacks can assign human-readable custom names pinned to their database and service ports instead of generic executable names.

### Example Configuration (`.portpeek` / `portpeek.json`)
```json
{
  "aliases": {
    "3000": "Storefront Web App",
    "5432": "Main PostgreSQL Database",
    "6379": "Redis Cache & Session Store",
    "8000": "FastAPI Authentication Service",
    "11434": "Ollama LLM Inference (Llama 3)"
  },
  "pinned": [3000, 5432, 6379, 11434]
}
```

### Discovery & Priority Hierarchy (`src/alias.cpp`)
1. **Workspace File:** `.portpeek` or `portpeek.json` in the active project directory.
2. **User Global Config:** `%USERPROFILE%\.portpeek\config.json`.
3. **Registry Overrides:** `HKCU\Software\PortPeek\Aliases`.
4. **Built-in Heuristic Defaults:** Automatic framework and project name detection.

---

## 5. 🛠️ Categorized System Background Processes
**Status:** ✅ **Shipped & Verified in v0.2.1**

### Overview
Windows typically runs 20–40 background processes listening on internal RPC, SMB, and ephemeral ports. To prevent visual clutter, PortPeek organizes background processes into 3 clean sub-categories:
1. 🛠️ **IDE & Language Servers** (`Code.exe`, `OpenCode.exe`, `Antigravity.exe`, TypeScript/Rust language servers).
2. ⚙️ **Windows System Services** (`135 RPC`, `139 NetBIOS`, `445 SMB`, `5357 WSDAPI`).
3. 📦 **Background Internal Ports** (ephemeral and loopback sockets).

---

## 6. 📦 NSIS Windows Installer (`installer.nsi`)
**Status:** ✅ **Shipped & Verified in v0.2.1**

- Modern UI 2 per-user installer (`PortPeek-Setup-v0.2.1.exe`, ~365 KB).
- Installs to `%LOCALAPPDATA%\Programs\PortPeek` without requiring Administrator privileges.
- Includes Start Menu shortcuts, optional Desktop shortcut, and clean Windows uninstaller.
- Single-command build via `build_installer.bat`.
