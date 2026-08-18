# PortPeek ⟷ Ephe Integration Specification

This document defines the contract, discovery mechanism, session launcher, JSON schema, and security guarantees for the optional integration between **PortPeek** and **Ephe**.

---

## 1. Overview

Ephe is a local-first peer-to-peer productivity application. When Ephe runs local sessions (e.g., live document sync, notes, or phone transfer), it publishes lightweight JSON manifests in a well-known local directory:

```text
%LOCALAPPDATA%\Ephe\PortPeek\
```

PortPeek continuously and silently discovers active Ephe sessions during its standard port refresh cycle, verifies local reachability, and presents them within the native Windows system tray popover.

---

## 2. Architecture & Separation of Concerns

```text
PortPeek
   │
   │ launches (CreateProcessW)
   ▼
ephe.exe --new-session
   │
   │ creates
   ▼
Ephe session
   │
   │ publishes
   ▼
Manifest (%LOCALAPPDATA%\Ephe\PortPeek\<id>.json)
   │
   │ discovers & verifies
   ▼
PortPeek tray menu
```

- **Strict Boundary:** PortPeek does **NOT** create, manage, or implement session creation logic internally. PortPeek only launches `ephe.exe --new-session` and reads verified manifests.
- **No Hardcoded Ports:** Ephe dynamically chooses its port and publishes it in the manifest. PortPeek discovers and probes whatever port Ephe assigns.
- **Desktop Host URL (`hostUrl`):** Ephe exposes `hostUrl` containing `?role=desktop` (with no session token) so that launching an active session from PortPeek opens the full host desktop experience (host controls, QR code, connected device list).
- **Zero Cloud / Telemetry:** Everything runs on local loopback (`127.0.0.1`).

---

## 3. Manifest Location & Lifecycle

- **Directory Path:** `%LOCALAPPDATA%\Ephe\PortPeek\`
- **File Pattern:** `<session-id>.json` (or any `*.json` file inside the directory)
- **Lifecycle:**
  - Ephe writes `<session-id>.json` upon session creation / port binding.
  - Ephe deletes `<session-id>.json` upon session termination.
  - PortPeek treats all files as untrusted local manifests and independently verifies port reachability.

---

## 4. JSON Schema (Version 1)

```json
{
  "schemaVersion": 1,
  "application": "Ephe",
  "applicationId": "ephe",
  "sessionId": "abc123",
  "sessionName": "College Notes",
  "version": "0.8.0",
  "port": 5500,
  "localUrl": "http://localhost:5500",
  "hostUrl": "http://localhost:5500/?role=desktop",
  "status": "active",
  "connectedDevices": 2
}
```

### Field Definitions

| Field | Type | Required | Description |
|---|---|---|---|
| `schemaVersion` | `integer` | No | Schema version (`1`). If omitted, defaults to 1. |
| `application` | `string` | No | Human-readable app name (`"Ephe"`). |
| `applicationId` | `string` | No | Identifier string (`"ephe"`). |
| `sessionId` | `string` | Yes | Unique alphanumeric session identifier. |
| `sessionName` | `string` | No | Display name (e.g., `"College Notes"`). If omitted or empty, defaults to `"Ephe Session"`. |
| `version` | `string` | No | Application semantic version. |
| `port` | `integer` | Yes | Port number ($1 \le \text{port} \le 65535$). |
| `localUrl` | `string` | Yes | Local URL (must begin with `http://localhost:` or `http://127.0.0.1:`). |
| `hostUrl` | `string` | No | Desktop Host URL (e.g., `http://localhost:5500/?role=desktop`). Preferred URL for opening session in desktop browser. |
| `status` | `string` | Yes | Session status; must be `"active"`. |
| `connectedDevices` | `integer` | No | Number of currently connected peer devices ($\ge 0$). |

---

## 5. PortPeek Validation & Security Rules

1. **Untrusted Local Input:**
   - Manifests are parsed with defensive bounds checking.
   - Malformed JSON, missing required keys, or unknown schemas fail silently.
2. **Localhost URL Whitelist:**
   - Both `localUrl` and `hostUrl` must strictly start with `http://localhost:` or `http://127.0.0.1:`. Non-HTTP schemes (`file:`, `javascript:`, `powershell:`) or remote hosts are rejected immediately.
3. **Launch URL Resolution & Backward Compatibility:**
   - When the user selects an Ephe session or clicks "Open All Sessions", PortPeek resolves `GetLaunchUrl()`:
     - If `hostUrl` is present and valid, `hostUrl` is used to launch the full desktop host experience.
     - If `hostUrl` is missing (legacy manifest), PortPeek falls back to `localUrl` without crashing.
4. **Reachability Verification (Anti-Stale / Crash Protection):**
   - Even if a valid manifest file exists (e.g., after an unexpected process crash), PortPeek tests whether the reported port is actively listening and responsive before showing it in the menu.
5. **Least Privilege & Zero Telemetry:**
   - No external requests or cloud dependencies. PortPeek only probes loopback (`127.0.0.1`).

---

## 6. Ephe Executable Resolution & Launcher

When the user triggers **New Session**, PortPeek locates and launches `ephe.exe` using standard Windows resolution:

### Discovery Order:
1. **Windows App Paths Registry:**
   - `HKCU\Software\Microsoft\Windows\CurrentVersion\App Paths\ephe.exe`
   - `HKLM\Software\Microsoft\Windows\CurrentVersion\App Paths\ephe.exe`
2. **Standard Installation Directories:**
   - `%LOCALAPPDATA%\Programs\Ephe\ephe.exe`
   - `%LOCALAPPDATA%\Ephe\ephe.exe`
   - `%PROGRAMFILES%\Ephe\ephe.exe`
   - `%PROGRAMFILES(X86)%\Ephe\ephe.exe`
3. **System Environment `PATH`:**
   - Resolved dynamically via Win32 `SearchPathW(nullptr, L"ephe.exe", ...)`.

### Command Line Invocation:
```cmd
ephe.exe --new-session
```

- If Ephe is not installed, the action fails gracefully without crashing or displaying error dialogs.

---

## 7. UI Presentation in PortPeek

### Main Menu:
- When Ephe is installed OR active sessions exist, an `Ephe Sessions ›` entry appears in the main flyout.

### Ephe Sessions Submenu:
```text
‹ Ephe Sessions (3)
─────────────────────
College Notes          5500 · 2 devices
Project Planning       5501
Phone Transfer         5502
─────────────────────
➕ New Session
⚡ Open All Sessions (3)
```

- **Individual Session Click:** Opens `hostUrl` (or `localUrl` fallback) in the user's default browser via `ShellExecuteW`.
- **New Session:** Launches `ephe.exe --new-session`, closes the flyout, and triggers background discovery refresh.
- **Open All Sessions:** Discovers all verified active sessions and opens each session's desktop host URL in separate browser tabs.
