/**
 * PortPeek Windows 11 Simulator Engine
 * Pure Vanilla JavaScript - Zero Framework Dependencies
 */

(function () {
  'use strict';

  // --- INITIAL SIMULATION STATE ---
  const DEFAULT_SERVERS = [
    {
      port: 3000,
      name: "my-app",
      framework: "Next.js",
      memory: "48 MB",
      pid: 14208,
      exe: "node.exe",
      category: "frontend",
      url: "http://localhost:3000",
      status: "listening"
    },
    {
      port: 5173,
      name: "dashboard",
      framework: "Vite",
      memory: "34 MB",
      pid: 8932,
      exe: "node.exe",
      category: "frontend",
      url: "http://localhost:5173",
      status: "listening"
    },
    {
      port: 8000,
      name: "api-service",
      framework: "FastAPI",
      memory: "28 MB",
      pid: 21044,
      exe: "python.exe",
      category: "backend",
      url: "http://localhost:8000",
      status: "listening"
    },
    {
      port: 11434,
      name: "llama3",
      framework: "Ollama",
      memory: "1.2 GB",
      pid: 5412,
      exe: "ollama.exe",
      category: "ai",
      url: "http://localhost:11434",
      status: "listening"
    }
  ];

  const SYSTEM_LISTENERS = [
    { port: 135, name: "RPC Endpoint Mapper", exe: "svchost.exe", pid: 980, mem: "14 MB" },
    { port: 445, name: "SMB Direct / File Sharing", exe: "System", pid: 4, mem: "4 MB" },
    { port: 5353, name: "Bonjour mDNS", exe: "mDNSResponder.exe", pid: 3120, mem: "8 MB" },
    { port: 5040, name: "Connected Devices Platform", exe: "svchost.exe", pid: 4508, mem: "12 MB" },
    { port: 7680, name: "Windows Delivery Optimization", exe: "svchost.exe", pid: 6112, mem: "6 MB" }
  ];

  let activeServers = JSON.parse(JSON.stringify(DEFAULT_SERVERS));
  let browserTabs = [];
  let currentActiveTabPort = null;
  let isBrowserOpen = false;
  let isBrowserMaximized = false;
  let viteCount = 0;

  // --- INITIALIZATION ---
  document.addEventListener('DOMContentLoaded', () => {
    initClock();
    renderTrayMenu();
    updateTaskbarDot();
    setupEventListeners();
    setupPlaygroundControls();
    logEvent("KERNEL", "PortPeek Win32 hook loaded. Native GetExtendedTcpTable listener active.");
    logEvent("TRAY", `PortPeek tray initialized with ${activeServers.length} active dev listeners.`);
  });

  // --- CLOCK & TASKBAR ---
  function initClock() {
    const clockEl = document.getElementById('trayClock');
    function update() {
      const now = new Date();
      let hours = now.getHours();
      const mins = String(now.getMinutes()).padStart(2, '0');
      const ampm = hours >= 12 ? 'PM' : 'AM';
      hours = hours % 12 || 12;
      const timeStr = `${hours}:${mins} ${ampm}`;
      const month = now.getMonth() + 1;
      const day = now.getDate();
      const year = now.getFullYear();
      const dateStr = `${month}/${day}/${year}`;
      if (clockEl) {
        clockEl.innerHTML = `<span>${timeStr}</span><span style="font-size:0.68rem; color:#94a3b8;">${dateStr}</span>`;
      }
    }
    update();
    setInterval(update, 1000);
  }

  function updateTaskbarDot() {
    const dotEl = document.getElementById('trayPulseBadge');
    const count = activeServers.length;
    if (dotEl) {
      if (count > 0) {
        dotEl.innerHTML = `<span class="pulse-dot"></span><span>${count}</span>`;
        dotEl.title = `PortPeek — ${count} Active Dev Server${count > 1 ? 's' : ''}`;
      } else {
        dotEl.innerHTML = `<span class="pulse-dot" style="background:#64748b; box-shadow:none;"></span><span style="color:#64748b;">0</span>`;
        dotEl.title = `PortPeek — Idle (0 Active Servers)`;
      }
    }

    const browserAppIcon = document.getElementById('taskbarChromeIcon');
    if (browserAppIcon) {
      if (isBrowserOpen) {
        browserAppIcon.classList.add('active');
      } else {
        browserAppIcon.classList.remove('active');
      }
    }
  }

  // --- RENDER TRAY MENU ---
  function renderTrayMenu() {
    const countTag = document.getElementById('trayActiveCount');
    if (countTag) {
      countTag.textContent = `${activeServers.length} active`;
    }

    const activeList = document.getElementById('trayActiveServerList');
    if (!activeList) return;

    if (activeServers.length === 0) {
      activeList.innerHTML = `
        <li style="padding: 12px 14px; font-size: 0.82rem; color: #71717a; text-align: center;">
          No active dev servers detected
        </li>
      `;
    } else {
      let html = '';
      activeServers.forEach(srv => {
        html += `
          <li class="tray-menu-item dev-port-item" data-port="${srv.port}" title="Click to open ${srv.url}">
            <div class="menu-port-left">
              <span class="menu-status-dot green"></span>
              <span class="menu-port-num">${srv.port}</span>
              <div class="menu-port-text">
                <span class="menu-port-title">${escapeHtml(srv.name)}</span>
                <span class="menu-port-sub">${escapeHtml(srv.exe)} • ${srv.memory}</span>
              </div>
            </div>
            <span class="menu-chevron">›</span>
          </li>
        `;
      });

      // 1-Click Open All Item if 2+ servers
      if (activeServers.length >= 2) {
        html += `
          <li class="tray-menu-item action-open-all" id="btnTrayOpenAll" style="margin-top: 4px;">
            <div class="menu-port-left">
              <svg class="menu-action-icon" width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"/><polyline points="15 3 21 3 21 9"/><line x1="10" y1="14" x2="21" y2="3"/></svg>
              <span style="font-weight:600;">Open All Active (${activeServers.length})</span>
            </div>
            <span class="menu-shortcut">⚡</span>
          </li>
        `;
      }

      activeList.innerHTML = html;
    }

    renderFlyouts();
  }

  // --- RENDER FLYOUT SUBMENUS ---
  function renderFlyouts() {
    // 1. System & Background Submenu
    const sysFlyout = document.getElementById('flyoutSystemList');
    if (sysFlyout) {
      sysFlyout.innerHTML = SYSTEM_LISTENERS.map(s => `
        <li class="flyout-item" onclick="window.simulator.showToast('System Port ${s.port}', '${s.name} (${s.exe} - PID ${s.pid})')">
          <span><b>${s.port}</b> ${s.name}</span>
          <span style="font-size:0.7rem; color:#64748b;">${s.mem}</span>
        </li>
      `).join('');
    }

    // 2. Copy URL Submenu
    const copyFlyout = document.getElementById('flyoutCopyList');
    if (copyFlyout) {
      let copyHtml = activeServers.map(s => `
        <li class="flyout-item" onclick="window.simulator.copyUrl('${s.url}', '${s.name}')">
          <span>${s.url}</span>
          <span style="font-size:0.7rem; color:#38bdf8;">Copy</span>
        </li>
      `).join('');

      copyHtml += `
        <li class="tray-menu-divider"></li>
        <li class="flyout-item" onclick="window.simulator.copyAllMarkdown()">
          <span>📋 Copy All as Markdown</span>
        </li>
        <li class="flyout-item" onclick="window.simulator.copyAllPlainText()">
          <span>📄 Copy All as Plain Text</span>
        </li>
      `;
      copyFlyout.innerHTML = copyHtml;
    }

    // 3. Stop Process Submenu
    const killFlyout = document.getElementById('flyoutKillList');
    if (killFlyout) {
      if (activeServers.length === 0) {
        killFlyout.innerHTML = `<li class="flyout-item" style="color:#64748b;">No dev processes to kill</li>`;
      } else {
        killFlyout.innerHTML = activeServers.map(s => `
          <li class="flyout-item kill-item" onclick="window.simulator.killPort(${s.port})">
            <span>❌ Kill :${s.port}</span>
            <span style="font-size:0.7rem;">PID ${s.pid}</span>
          </li>
        `).join('');
      }
    }
  }

  // --- BROWSER WINDOW MANAGEMENT ---
  function openBrowserWithPort(port) {
    const srv = activeServers.find(s => s.port === port);
    if (!srv) return;

    // Check if tab already exists
    const existingTab = browserTabs.find(t => t.port === port);
    if (!existingTab) {
      browserTabs.push({
        port: srv.port,
        title: `${srv.framework} :${srv.port}`,
        name: srv.name,
        framework: srv.framework,
        url: srv.url
      });
    }

    currentActiveTabPort = port;
    isBrowserOpen = true;
    const win = document.getElementById('browserWindow');
    if (win) {
      win.classList.remove('minimized');
    }

    renderBrowserTabs();
    renderBrowserViewport();
    updateTaskbarDot();

    logEvent("ACTION", `User clicked :${port} -> Launched in Virtual Browser (${srv.url})`);
    showToast(`🌐 Opened Localhost`, `Navigated to http://localhost:${port} (${srv.framework})`);
  }

  function openAllActiveServers() {
    if (activeServers.length === 0) {
      showToast("No Active Servers", "There are no active servers to open.", "warn");
      return;
    }

    browserTabs = [];
    activeServers.forEach(srv => {
      browserTabs.push({
        port: srv.port,
        title: `${srv.framework} :${srv.port}`,
        name: srv.name,
        framework: srv.framework,
        url: srv.url
      });
    });

    currentActiveTabPort = activeServers[0].port;
    isBrowserOpen = true;
    const win = document.getElementById('browserWindow');
    if (win) {
      win.classList.remove('minimized');
    }

    renderBrowserTabs();
    renderBrowserViewport();
    updateTaskbarDot();

    logEvent("ACTION", `1-Click 'Open All' triggered -> Spawned ${activeServers.length} browser tabs.`);
    showToast(`⚡ 1-Click Launch!`, `Opened all ${activeServers.length} dev servers in browser tabs simultaneously.`);
  }

  function closeBrowserTab(port, e) {
    if (e) e.stopPropagation();
    browserTabs = browserTabs.filter(t => t.port !== port);
    if (currentActiveTabPort === port) {
      if (browserTabs.length > 0) {
        currentActiveTabPort = browserTabs[browserTabs.length - 1].port;
      } else {
        currentActiveTabPort = null;
        isBrowserOpen = false;
        const win = document.getElementById('browserWindow');
        if (win) win.classList.add('minimized');
      }
    }
    renderBrowserTabs();
    renderBrowserViewport();
    updateTaskbarDot();
  }

  function renderBrowserTabs() {
    const tabsContainer = document.getElementById('browserTabsList');
    if (!tabsContainer) return;

    tabsContainer.innerHTML = browserTabs.map(t => `
      <div class="browser-tab ${t.port === currentActiveTabPort ? 'active' : ''}" onclick="window.simulator.switchTab(${t.port})">
        <span class="browser-tab-icon">${getFrameworkIcon(t.framework)}</span>
        <span class="browser-tab-title">${t.title}</span>
        <span class="browser-tab-close" onclick="window.simulator.closeTab(${t.port}, event)" title="Close tab">✕</span>
      </div>
    `).join('');
  }

  function getFrameworkIcon(framework) {
    switch (framework.toLowerCase()) {
      case 'next.js': return '▲';
      case 'vite': return '⚡';
      case 'fastapi': return '🐍';
      case 'ollama': return '🦙';
      case 'postgresql': return '🐘';
      case 'redis': return '🔴';
      case 'spring boot': return '🍃';
      case 'ruby on rails': return '💎';
      case 'astro': return '🚀';
      default: return '🌐';
    }
  }

  function renderBrowserViewport() {
    const viewport = document.getElementById('browserViewport');
    const addressText = document.getElementById('browserAddressText');
    const addressTag = document.getElementById('browserAddressTag');

    if (!currentActiveTabPort) {
      if (viewport) {
        viewport.innerHTML = `
          <div style="display:flex; flex-direction:column; align-items:center; justify-content:center; height:100%; color:#64748b;">
            <p style="font-size:1.1rem; margin-bottom:8px;">New Tab</p>
            <p style="font-size:0.85rem;">Click any port in the PortPeek Tray Menu to preview that app.</p>
          </div>
        `;
      }
      if (addressText) addressText.textContent = 'about:blank';
      if (addressTag) addressTag.textContent = 'Ready';
      return;
    }

    const currentTab = browserTabs.find(t => t.port === currentActiveTabPort);
    if (!currentTab) return;

    if (addressText) addressText.textContent = `http://localhost:${currentTab.port}`;
    if (addressTag) addressTag.textContent = currentTab.framework;

    if (!viewport) return;

    switch (currentTab.port) {
      case 3000:
        viewport.innerHTML = renderNextJsPreview();
        break;
      case 5173:
        viewport.innerHTML = renderVitePreview();
        break;
      case 8000:
        viewport.innerHTML = renderFastApiPreview();
        break;
      case 11434:
        viewport.innerHTML = renderOllamaPreview();
        break;
      default:
        viewport.innerHTML = renderGenericPreview(currentTab);
        break;
    }
  }

  // --- PREVIEW HTML GENERATORS ---
  function renderNextJsPreview() {
    return `
      <div class="preview-container nextjs-preview">
        <div class="next-badge">▲ Next.js 14.2.5 • App Router</div>
        <h2 class="next-title">Welcome to Next.js</h2>
        <p class="next-desc">Get started by editing <code>app/page.tsx</code>. Changes are hot-reloaded across localhost:3000 in ~14ms.</p>
        <div class="next-cards">
          <div class="next-card">
            <h4>Docs <span>-&gt;</span></h4>
            <p>Find in-depth information about Next.js features and API.</p>
          </div>
          <div class="next-card">
            <h4>Learn <span>-&gt;</span></h4>
            <p>Learn about Next.js in an interactive course with quizzes!</p>
          </div>
          <div class="next-card">
            <h4>Templates <span>-&gt;</span></h4>
            <p>Explore starter templates for Next.js and Tailwind.</p>
          </div>
          <div class="next-card">
            <h4>Deploy <span>-&gt;</span></h4>
            <p>Instantly deploy your Next.js site to a shareable URL.</p>
          </div>
        </div>
      </div>
    `;
  }

  function renderVitePreview() {
    return `
      <div class="preview-container vite-preview">
        <div class="vite-logos">
          <span style="font-size: 3rem;">⚡</span>
          <span style="font-size: 2rem; color:#64748b;">+</span>
          <span style="font-size: 3rem; color:#00d8ff;">⚛️</span>
        </div>
        <h2 class="vite-title">Vite + React</h2>
        <div class="vite-counter-box">
          <p style="font-size: 0.85rem; color:#94a3b8;">Interactive State Counter Demo</p>
          <div class="counter-display" id="viteCounterDisplay">count is ${viteCount}</div>
          <div class="counter-btns">
            <button class="counter-btn" onclick="window.simulator.incrementVite(-1)">- 1</button>
            <button class="counter-btn" onclick="window.simulator.incrementVite(1)">+ 1</button>
            <button class="counter-btn secondary" onclick="window.simulator.resetVite()">Reset</button>
          </div>
        </div>
        <p style="font-size:0.8rem; color:#64748b;">Edit <code>src/App.tsx</code> and save to test HMR. Server listening on port 5173.</p>
      </div>
    `;
  }

  function renderFastApiPreview() {
    return `
      <div class="preview-container fastapi-preview">
        <div class="swagger-header">
          <div class="swagger-title">
            <span>FastAPI Swagger UI</span>
            <span class="swagger-badge">v0.2.0</span>
          </div>
          <span style="font-size:0.75rem; color:#94a3b8; font-family:var(--font-mono);">/openapi.json</span>
        </div>

        <div class="swagger-endpoint">
          <div class="endpoint-header" onclick="window.simulator.toggleSwaggerBody('getHealth')">
            <div class="endpoint-left">
              <span class="method-tag">GET</span>
              <span class="endpoint-path">/api/v1/health</span>
            </div>
            <span style="font-size:0.75rem; color:#94a3b8;">Service Health Check</span>
          </div>
          <div class="endpoint-body" id="swaggerBody_getHealth">
            <button class="btn-execute" onclick="window.simulator.executeSwaggerHealth()">Execute Request ⚡</button>
            <div class="json-response-box" id="swaggerResponseBox">{
  "status": "healthy",
  "port": 8000,
  "service": "api-service",
  "uptime": "1h 14m",
  "engine": "uvicorn / fastapi"
}</div>
          </div>
        </div>

        <div class="swagger-endpoint" style="border-color: rgba(37, 99, 235, 0.3);">
          <div class="endpoint-header" style="background: rgba(37, 99, 235, 0.1);">
            <div class="endpoint-left">
              <span class="method-tag" style="background: #2563eb;">POST</span>
              <span class="endpoint-path">/api/v1/analyze</span>
            </div>
            <span style="font-size:0.75rem; color:#94a3b8;">TCP Port Diagnostic Agent</span>
          </div>
        </div>
      </div>
    `;
  }

  function renderOllamaPreview() {
    return `
      <div class="preview-container ollama-preview">
        <div class="ollama-header">
          <div>
            <strong>🦙 Ollama Local LLM</strong>
            <span style="color:#22c55e; margin-left:8px;">● Online (:11434)</span>
          </div>
          <span style="font-size:0.75rem; color:#94a3b8;">Model: <b>llama3:8b (Q4_K_M)</b></span>
        </div>

        <div class="ollama-chat-messages" id="ollamaChatMessages">
          <div class="chat-bubble ai">
            Hello! I am <b>Llama 3</b> running locally on your machine on port 11434 via Ollama. Ask me anything about your ports or web stack!
          </div>
        </div>

        <div class="ollama-input-bar">
          <input type="text" id="ollamaInput" placeholder="Ask Llama 3 (e.g. 'Why is port 3000 occupied?')" onkeydown="if(event.key==='Enter') window.simulator.sendOllamaMsg()" />
          <button onclick="window.simulator.sendOllamaMsg()">Send</button>
        </div>
      </div>
    `;
  }

  function renderGenericPreview(tab) {
    return `
      <div class="preview-container" style="display:flex; flex-direction:column; align-items:center; justify-content:center; text-align:center;">
        <span style="font-size:3.5rem; margin-bottom:12px;">${getFrameworkIcon(tab.framework)}</span>
        <h2 style="font-size:1.8rem; margin-bottom:8px;">${escapeHtml(tab.name)}</h2>
        <p style="color:#94a3b8; font-size:0.95rem; margin-bottom:16px;">
          Listening on <code>http://localhost:${tab.port}</code> • Framework: <b>${escapeHtml(tab.framework)}</b>
        </p>
        <div style="background:rgba(255,255,255,0.04); border:1px solid rgba(255,255,255,0.08); padding:16px 24px; border-radius:8px; font-family:var(--font-mono); font-size:0.8rem; color:#38bdf8;">
          ✓ Service socket active (IPv4/IPv6)<br/>
          ✓ Ready to accept incoming HTTP/TCP connections
        </div>
      </div>
    `;
  }

  // --- ACTIONS & OPERATIONS ---
  function killPort(port) {
    const srv = activeServers.find(s => s.port === port);
    if (!srv) return;

    activeServers = activeServers.filter(s => s.port !== port);
    closeBrowserTab(port);
    renderTrayMenu();
    updateTaskbarDot();

    logEvent("KILL", `Terminated PID ${srv.pid} holding port :${port} (${srv.exe})`);
    showToast(`🛑 Process Killed`, `Stopped port ${port} (PID ${srv.pid} - ${srv.exe})`, 'kill');
  }

  function spawnServer(srvObj) {
    // Check if port already exists
    const existing = activeServers.find(s => s.port === srvObj.port);
    if (existing) {
      showToast(`⚠️ Port Conflict!`, `Port ${srvObj.port} is already occupied by PID ${existing.pid} (${existing.name}).`, 'warn');
      logEvent("CONFLICT", `Failed to bind port :${srvObj.port} - EADDRINUSE (PID ${existing.pid})`);
      return false;
    }

    activeServers.unshift(srvObj);
    renderTrayMenu();
    updateTaskbarDot();

    logEvent("SPAWN", `New TCP listener detected on 0.0.0.0:${srvObj.port} -> PID ${srvObj.pid} (${srvObj.name})`);
    showToast(`✨ New Server Detected`, `PortPeek discovered ${srvObj.framework} listening on port ${srvObj.port}`);
    return true;
  }

  function copyUrl(url, name) {
    navigator.clipboard.writeText(url).then(() => {
      showToast(`📋 URL Copied`, `Copied ${url} (${name}) to clipboard.`);
      logEvent("ACTION", `Copied URL ${url} to clipboard.`);
    });
  }

  function copyAllMarkdown() {
    if (activeServers.length === 0) return;
    const text = activeServers.map(s => `- [${s.framework} (: ${s.port})](${s.url}) — \`${s.name}\``).join('\n');
    navigator.clipboard.writeText(text).then(() => {
      showToast(`📋 Markdown Copied`, `Copied all ${activeServers.length} active ports as markdown.`);
    });
  }

  function copyAllPlainText() {
    if (activeServers.length === 0) return;
    const text = activeServers.map(s => `${s.port}\t${s.url}\t${s.framework}\t${s.name}`).join('\n');
    navigator.clipboard.writeText(text).then(() => {
      showToast(`📄 Text Copied`, `Copied all ${activeServers.length} active ports to clipboard.`);
    });
  }

  // --- LOGGING & TOASTS ---
  function logEvent(type, message) {
    const logBox = document.getElementById('simEventLogBox');
    if (!logBox) return;

    const now = new Date();
    const timeStr = `${String(now.getHours()).padStart(2,'0')}:${String(now.getMinutes()).padStart(2,'0')}:${String(now.getSeconds()).padStart(2,'0')}.${String(now.getMilliseconds()).padStart(3,'0')}`;
    
    let tagClass = 'tag-tray';
    if (type === 'KERNEL') tagClass = 'tag-kernel';
    if (type === 'SPAWN') tagClass = 'tag-spawn';
    if (type === 'KILL') tagClass = 'tag-kill';
    if (type === 'CONFLICT') tagClass = 'tag-kill';

    const entry = document.createElement('div');
    entry.className = 'log-entry';
    entry.innerHTML = `<span class="time">[${timeStr}]</span> <span class="${tagClass}">[${type}]</span> ${escapeHtml(message)}`;
    
    logBox.appendChild(entry);
    logBox.scrollTop = logBox.scrollHeight;
  }

  function showToast(title, body, type = 'info') {
    const container = document.getElementById('win11ToastContainer');
    if (!container) return;

    const toast = document.createElement('div');
    toast.className = `win11-toast ${type === 'kill' || type === 'warn' ? 'kill-toast' : ''}`;
    toast.innerHTML = `
      <div class="toast-header">
        <span>PORTPEEK ACTION CENTER</span>
        <span>Just now</span>
      </div>
      <div class="toast-body">
        <strong>${escapeHtml(title)}</strong><br/>
        <span style="font-size:0.8rem; color:#cbd5e1;">${escapeHtml(body)}</span>
      </div>
    `;

    container.appendChild(toast);
    setTimeout(() => toast.classList.add('show'), 20);

    setTimeout(() => {
      toast.classList.remove('show');
      setTimeout(() => toast.remove(), 350);
    }, 3800);
  }

  // --- EVENT LISTENERS SETUP ---
  function setupEventListeners() {
    const trayBtn = document.getElementById('trayPortPeekBtn');
    const trayMenu = document.getElementById('win11TrayMenu');

    if (trayBtn && trayMenu) {
      trayBtn.addEventListener('click', (e) => {
        e.stopPropagation();
        const isOpen = trayMenu.classList.contains('active');
        if (isOpen) {
          trayMenu.classList.remove('active');
          trayBtn.classList.remove('active');
        } else {
          trayMenu.classList.add('active');
          trayBtn.classList.add('active');
          logEvent("TRAY", "Tray popup menu opened via user click.");
        }
      });

      document.addEventListener('click', (e) => {
        if (!trayMenu.contains(e.target) && e.target !== trayBtn && !trayBtn.contains(e.target)) {
          trayMenu.classList.remove('active');
          trayBtn.classList.remove('active');
        }
      });
    }

    // Delegated clicks for server items in tray menu
    document.addEventListener('click', (e) => {
      const item = e.target.closest('.tray-menu-item[data-port]');
      if (item) {
        const port = parseInt(item.getAttribute('data-port'), 10);
        openBrowserWithPort(port);
        if (trayMenu) trayMenu.classList.remove('active');
        if (trayBtn) trayBtn.classList.remove('active');
      }

      if (e.target.closest('#btnTrayOpenAll')) {
        openAllActiveServers();
        if (trayMenu) trayMenu.classList.remove('active');
        if (trayBtn) trayBtn.classList.remove('active');
      }
    });

    // Browser Window Buttons
    const btnWinClose = document.getElementById('btnWinClose');
    if (btnWinClose) {
      btnWinClose.addEventListener('click', () => {
        isBrowserOpen = false;
        const win = document.getElementById('browserWindow');
        if (win) win.classList.add('minimized');
        updateTaskbarDot();
        logEvent("ACTION", "Browser window closed.");
      });
    }

    const btnWinMin = document.getElementById('btnWinMin');
    if (btnWinMin) {
      btnWinMin.addEventListener('click', () => {
        isBrowserOpen = false;
        const win = document.getElementById('browserWindow');
        if (win) win.classList.add('minimized');
        updateTaskbarDot();
        logEvent("ACTION", "Browser window minimized to taskbar.");
      });
    }

    const btnWinMax = document.getElementById('btnWinMax');
    if (btnWinMax) {
      btnWinMax.addEventListener('click', () => {
        const win = document.getElementById('browserWindow');
        if (!win) return;
        isBrowserMaximized = !isBrowserMaximized;
        if (isBrowserMaximized) {
          win.classList.add('maximized');
        } else {
          win.classList.remove('maximized');
        }
      });
    }

    // Taskbar Chrome Icon Toggle
    const taskbarChromeIcon = document.getElementById('taskbarChromeIcon');
    if (taskbarChromeIcon) {
      taskbarChromeIcon.addEventListener('click', () => {
        const win = document.getElementById('browserWindow');
        if (!win) return;
        if (isBrowserOpen) {
          isBrowserOpen = false;
          win.classList.add('minimized');
        } else {
          isBrowserOpen = true;
          win.classList.remove('minimized');
          if (browserTabs.length === 0 && activeServers.length > 0) {
            openBrowserWithPort(activeServers[0].port);
          }
        }
        updateTaskbarDot();
      });
    }

    // Refresh simulation
    const btnRefresh = document.getElementById('trayRefreshBtn');
    if (btnRefresh) {
      btnRefresh.addEventListener('click', () => {
        // Random slight memory fluctuations to show it's live!
        activeServers.forEach(s => {
          if (s.memory.includes('MB')) {
            const base = parseInt(s.memory, 10);
            const delta = Math.floor(Math.random() * 5) - 2;
            s.memory = `${Math.max(10, base + delta)} MB`;
          }
        });
        renderTrayMenu();
        if (trayMenu) trayMenu.classList.remove('active');
        if (trayBtn) trayBtn.classList.remove('active');
        logEvent("KERNEL", "Enumerated TCP tables via GetExtendedTcpTable (elapsed: 1.2ms, CPU: 0.0%)");
        showToast("⚡ Refreshed in 1.2ms", "Native socket scan completed with 0% CPU overhead.");
      });
    }
  }

  // --- PLAYGROUND CONTROLS ---
  function setupPlaygroundControls() {
    // Preset buttons
    const presetMap = {
      'postgres': { port: 5432, name: "postgres_db", framework: "PostgreSQL", memory: "112 MB", pid: 18240, exe: "postgres.exe", category: "database", url: "http://localhost:5432" },
      'redis': { port: 6379, name: "cache_server", framework: "Redis", memory: "22 MB", pid: 7410, exe: "redis-server.exe", category: "database", url: "http://localhost:6379" },
      'rails': { port: 3000, name: "rails_app", framework: "Ruby on Rails", memory: "94 MB", pid: 29800, exe: "ruby.exe", category: "backend", url: "http://localhost:3000" },
      'springboot': { port: 8080, name: "spring_api", framework: "Spring Boot", memory: "240 MB", pid: 11044, exe: "javaw.exe", category: "backend", url: "http://localhost:8080" },
      'astro': { port: 4321, name: "blog_site", framework: "Astro", memory: "38 MB", pid: 14920, exe: "node.exe", category: "frontend", url: "http://localhost:4321" },
      'docker': { port: 80, name: "nginx_proxy", framework: "Docker Nginx", memory: "16 MB", pid: 3200, exe: "docker.exe", category: "backend", url: "http://localhost:80" }
    };

    document.querySelectorAll('.btn-preset-spawn').forEach(btn => {
      btn.addEventListener('click', () => {
        const type = btn.getAttribute('data-preset');
        const srv = presetMap[type];
        if (srv) {
          spawnServer(JSON.parse(JSON.stringify(srv)));
        }
      });
    });

    // Custom Spawn Form
    const customForm = document.getElementById('customSpawnForm');
    if (customForm) {
      customForm.addEventListener('submit', (e) => {
        e.preventDefault();
        const portInput = document.getElementById('spawnPortInput');
        const nameInput = document.getElementById('spawnNameInput');
        const fwInput = document.getElementById('spawnFwInput');
        const memInput = document.getElementById('spawnMemInput');

        const port = parseInt(portInput.value, 10);
        if (!port || port < 1 || port > 65535) {
          showToast("Invalid Port", "Please enter a valid port between 1 and 65535.", "warn");
          return;
        }

        const name = nameInput.value.trim() || `srv-${port}`;
        const framework = fwInput.value || "Custom API";
        const memory = `${parseInt(memInput.value, 10) || 32} MB`;
        const pid = Math.floor(Math.random() * 20000) + 4000;

        const success = spawnServer({
          port,
          name,
          framework,
          memory,
          pid,
          exe: `${framework.toLowerCase().replace(/[^a-z]/g, '')}.exe`,
          category: "backend",
          url: `http://localhost:${port}`
        });

        if (success) {
          portInput.value = '';
          nameInput.value = '';
        }
      });
    }
  }

  // --- PUBLIC WINDOW APIS FOR DOM HANDLERS ---
  window.simulator = {
    killPort: killPort,
    switchTab: (port) => {
      currentActiveTabPort = port;
      renderBrowserTabs();
      renderBrowserViewport();
    },
    closeTab: closeBrowserTab,
    copyUrl: copyUrl,
    copyAllMarkdown: copyAllMarkdown,
    copyAllPlainText: copyAllPlainText,
    showToast: showToast,
    openAllActive: openAllActiveServers,
    incrementVite: (val) => {
      viteCount += val;
      const el = document.getElementById('viteCounterDisplay');
      if (el) el.textContent = `count is ${viteCount}`;
    },
    resetVite: () => {
      viteCount = 0;
      const el = document.getElementById('viteCounterDisplay');
      if (el) el.textContent = `count is 0`;
    },
    toggleSwaggerBody: (id) => {
      const el = document.getElementById(`swaggerBody_${id}`);
      if (el) {
        el.style.display = el.style.display === 'none' ? 'block' : 'none';
      }
    },
    executeSwaggerHealth: () => {
      const box = document.getElementById('swaggerResponseBox');
      if (!box) return;
      box.textContent = "Executing GET /api/v1/health via local socket...";
      setTimeout(() => {
        box.textContent = JSON.stringify({
          status: "healthy",
          port: 8000,
          service: "api-service",
          uptime: "1h 15m 02s",
          memory_mb: 28.4,
          timestamp: new Date().toISOString()
        }, null, 2);
        showToast("Swagger 200 OK", "GET /api/v1/health returned status: healthy");
      }, 300);
    },
    sendOllamaMsg: () => {
      const input = document.getElementById('ollamaInput');
      const chatBox = document.getElementById('ollamaChatMessages');
      if (!input || !chatBox || !input.value.trim()) return;

      const userText = input.value.trim();
      input.value = '';

      // User bubble
      const userBubble = document.createElement('div');
      userBubble.className = 'chat-bubble user';
      userBubble.textContent = userText;
      chatBox.appendChild(userBubble);
      chatBox.scrollTop = chatBox.scrollHeight;

      // AI Response simulation
      setTimeout(() => {
        const aiBubble = document.createElement('div');
        aiBubble.className = 'chat-bubble ai';

        let reply = `Here's what I found regarding "${userText}": In modern Windows development, PortPeek allows you to monitor and manage all local socket listeners with zero CPU overhead.`;
        if (userText.toLowerCase().includes('3000') || userText.toLowerCase().includes('occupied') || userText.toLowerCase().includes('in use')) {
          reply = `Port 3000 is currently occupied by <b>node.exe (Next.js - PID 14208)</b>. You can terminate it in 1-click via PortPeek Tray -> Stop Process -> 3000.`;
        } else if (userText.toLowerCase().includes('port') || userText.toLowerCase().includes('python')) {
          reply = `You can inspect active TCP listeners in Python using \`socket.create_server(('127.0.0.1', port))\` or query Windows kernel tables directly like PortPeek does.`;
        }

        aiBubble.innerHTML = reply;
        chatBox.appendChild(aiBubble);
        chatBox.scrollTop = chatBox.scrollHeight;
      }, 500);
    },
    resetAll: () => {
      activeServers = JSON.parse(JSON.stringify(DEFAULT_SERVERS));
      renderTrayMenu();
      updateTaskbarDot();
      browserTabs = [];
      currentActiveTabPort = null;
      isBrowserOpen = false;
      const win = document.getElementById('browserWindow');
      if (win) win.classList.add('minimized');
      logEvent("TRAY", "Reset simulated server state to default 4 dev servers.");
      showToast("Reset Complete", "Restored default active dev servers (3000, 5173, 8000, 11434).");
    },
    killAll: () => {
      activeServers = [];
      renderTrayMenu();
      updateTaskbarDot();
      browserTabs = [];
      currentActiveTabPort = null;
      isBrowserOpen = false;
      const win = document.getElementById('browserWindow');
      if (win) win.classList.add('minimized');
      logEvent("KILL", "Killed all simulated dev listeners.");
      showToast("All Servers Stopped", "Terminated all local dev server processes.", "kill");
    },
    triggerCollision: () => {
      spawnServer({
        port: 3000,
        name: "rails_app",
        framework: "Ruby on Rails",
        memory: "82 MB",
        pid: 31092,
        exe: "ruby.exe",
        category: "backend",
        url: "http://localhost:3000"
      });
    }
  };

  // Helper Escape
  function escapeHtml(str) {
    if (!str) return '';
    return String(str)
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;');
  }

})();
