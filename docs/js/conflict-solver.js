/**
 * PortPeek Interactive Port Conflict Solver & Terminal Decoder
 * Pure Vanilla JavaScript
 */

(function () {
  'use strict';

  // --- PORT CONFLICT PROFILES DATABASE ---
  const CONFLICT_PROFILES = {
    3000: {
      port: 3000,
      framework: "Next.js / Node.js",
      exe: "node.exe",
      defaultPid: 14208,
      mem: "58 MB",
      errorSample: "Error: listen EADDRINUSE: address already in use :::3000",
      what: "Another Node.js instance or orphaned Next.js development server is currently holding the TCP socket on port 3000.",
      why: "Commonly happens when a terminal tab was closed without cleanly terminating `npm run dev` with Ctrl+C, or when VS Code leaves background watcher tasks running.",
      cmdCommands: [
        "netstat -ano | findstr :3000",
        "taskkill /F /PID 14208"
      ],
      psCommands: [
        "Get-NetTCPConnection -LocalPort 3000 | ForEach-Object { Stop-Process -Id $_.OwningProcess -Force }"
      ],
      safeConfigs: [
        {
          target: "Next.js (package.json)",
          file: "package.json",
          code: '"scripts": {\n  "dev": "next dev -p 3001"\n}'
        },
        {
          target: "Next.js CLI Override",
          file: "Terminal",
          code: "npx next dev -p 3001"
        },
        {
          target: "Express.js Server",
          file: "server.js",
          code: "const PORT = process.env.PORT || 3001;\napp.listen(PORT, () => console.log(`Server on ${PORT}`));"
        },
        {
          target: "Docker Compose Port Mapping",
          file: "docker-compose.yml",
          code: "services:\n  web:\n    ports:\n      - \"3001:3000\""
        }
      ]
    },
    5173: {
      port: 5173,
      framework: "Vite (React, Vue, Svelte)",
      exe: "node.exe",
      defaultPid: 8932,
      mem: "36 MB",
      errorSample: "Port 5173 is in use, trying another one...",
      what: "Vite detected that port 5173 is already listening and automatically fell back to 5174 or 5175.",
      why: "Vite automatically increments port numbers when 5173 is occupied. While this avoids a hard crash, it frequently breaks hardcoded backend CORS configs, API proxy headers, or OAuth redirect URIs.",
      cmdCommands: [
        "netstat -ano | findstr :5173",
        "taskkill /F /PID 8932"
      ],
      psCommands: [
        "Get-NetTCPConnection -LocalPort 5173 | ForEach-Object { Stop-Process -Id $_.OwningProcess -Force }"
      ],
      safeConfigs: [
        {
          target: "Vite Strict Port Config",
          file: "vite.config.ts",
          code: "export default defineConfig({\n  server: {\n    port: 5173,\n    strictPort: true // Prevents silent port bumps\n  }\n});"
        },
        {
          target: "Vite CLI Custom Port",
          file: "package.json",
          code: '"scripts": {\n  "dev": "vite --port 5174"\n}'
        }
      ]
    },
    8000: {
      port: 8000,
      framework: "FastAPI / Django / Python",
      exe: "python.exe",
      defaultPid: 21044,
      mem: "32 MB",
      errorSample: "OSError: [WinError 10048] Only one usage of each socket address is normally permitted",
      what: "A Python Uvicorn or Django runserver process failed to bind port 8000 because an existing process owns the socket.",
      why: "Occurs when Uvicorn reloads reload worker threads without releasing the master socket, or when a previous session crashed ungracefully in the background.",
      cmdCommands: [
        "netstat -ano | findstr :8000",
        "taskkill /F /PID 21044"
      ],
      psCommands: [
        "Get-NetTCPConnection -LocalPort 8000 | ForEach-Object { Stop-Process -Id $_.OwningProcess -Force }"
      ],
      safeConfigs: [
        {
          target: "Uvicorn CLI Port Override",
          file: "Terminal",
          code: "uvicorn main:app --port 8001 --reload"
        },
        {
          target: "Django Runserver Port",
          file: "Terminal",
          code: "python manage.py runserver 8001"
        }
      ]
    },
    5432: {
      port: 5432,
      framework: "PostgreSQL Database",
      exe: "postgres.exe",
      defaultPid: 18240,
      mem: "112 MB",
      errorSample: "FATAL: lock file \"postmaster.pid\" already exists",
      what: "PostgreSQL cannot bind port 5432 because another PostgreSQL instance or local Windows Service is running.",
      why: "Often happens when both a native Windows PostgreSQL service (installed via installer) and a Docker PostgreSQL container compete for port 5432.",
      cmdCommands: [
        "netstat -ano | findstr :5432",
        "taskkill /F /PID 18240"
      ],
      psCommands: [
        "Stop-Service postgresql*",
        "Get-NetTCPConnection -LocalPort 5432 | ForEach-Object { Stop-Process -Id $_.OwningProcess -Force }"
      ],
      safeConfigs: [
        {
          target: "Docker Compose Postgres Mapping",
          file: "docker-compose.yml",
          code: "services:\n  db:\n    image: postgres:16\n    ports:\n      - \"5433:5432\" # Host 5433 -> Container 5432"
        },
        {
          target: "DATABASE_URL Environment Variable",
          file: ".env",
          code: "DATABASE_URL=\"postgresql://postgres:postgres@localhost:5433/mydb\""
        }
      ]
    },
    8080: {
      port: 8080,
      framework: "Spring Boot / Tomcat / Vue CLI",
      exe: "javaw.exe",
      defaultPid: 11044,
      mem: "220 MB",
      errorSample: "Web server failed to start. Port 8080 was already in use.",
      what: "Embedded Tomcat / Spring Boot or Vue CLI could not initialize HTTP listener on port 8080.",
      why: "Port 8080 is the most commonly contested secondary web port across Java, Node, Docker, and proxy tools.",
      cmdCommands: [
        "netstat -ano | findstr :8080",
        "taskkill /F /PID 11044"
      ],
      psCommands: [
        "Get-NetTCPConnection -LocalPort 8080 | ForEach-Object { Stop-Process -Id $_.OwningProcess -Force }"
      ],
      safeConfigs: [
        {
          target: "Spring Boot application.properties",
          file: "src/main/resources/application.properties",
          code: "server.port=8081"
        },
        {
          target: "Vue CLI / Webpack config",
          file: "vue.config.js",
          code: "module.exports = {\n  devServer: {\n    port: 8081\n  }\n};"
        }
      ]
    },
    6379: {
      port: 6379,
      framework: "Redis Server",
      exe: "redis-server.exe",
      defaultPid: 7410,
      mem: "24 MB",
      errorSample: "Could not create server TCP listening socket *:6379: bind: Address already in use",
      what: "Redis server failed to start because another Redis instance or Memcached daemon owns port 6379.",
      why: "A Redis background service is already running on Windows (via WSL2 or Windows Service), blocking your local standalone or Docker redis.",
      cmdCommands: [
        "netstat -ano | findstr :6379",
        "taskkill /F /PID 7410"
      ],
      psCommands: [
        "Get-NetTCPConnection -LocalPort 6379 | ForEach-Object { Stop-Process -Id $_.OwningProcess -Force }"
      ],
      safeConfigs: [
        {
          target: "Docker Redis Port Map",
          file: "docker-compose.yml",
          code: "services:\n  redis:\n    image: redis:alpine\n    ports:\n      - \"6380:6379\""
        },
        {
          target: "REDIS_URL .env Config",
          file: ".env",
          code: "REDIS_URL=\"redis://localhost:6380/0\""
        }
      ]
    },
    4200: {
      port: 4200,
      framework: "Angular CLI",
      exe: "node.exe",
      defaultPid: 16400,
      mem: "64 MB",
      errorSample: "Port 4200 is already in use. Use '--port' to specify a different port.",
      what: "Angular CLI (ng serve) stopped because port 4200 is locked by another build task.",
      why: "Previous `ng serve` process didn't terminate properly when terminal was exited.",
      cmdCommands: [
        "netstat -ano | findstr :4200",
        "taskkill /F /PID 16400"
      ],
      psCommands: [
        "Get-NetTCPConnection -LocalPort 4200 | ForEach-Object { Stop-Process -Id $_.OwningProcess -Force }"
      ],
      safeConfigs: [
        {
          target: "Angular CLI Port Flag",
          file: "package.json",
          code: '"scripts": {\n  "start": "ng serve --port 4300"\n}'
        }
      ]
    },
    11434: {
      port: 11434,
      framework: "Ollama Local LLM",
      exe: "ollama.exe",
      defaultPid: 5412,
      mem: "1.2 GB",
      errorSample: "Error: listen tcp 127.0.0.1:11434: bind: address already in use",
      what: "Ollama background service is already running on port 11434.",
      why: "Ollama runs as an automatic Windows system startup task in the background tray.",
      cmdCommands: [
        "netstat -ano | findstr :11434",
        "taskkill /F /PID 5412"
      ],
      psCommands: [
        "Get-Process ollama* | Stop-Process -Force"
      ],
      safeConfigs: [
        {
          target: "Ollama Custom Host Environment Variable",
          file: "PowerShell",
          code: '$env:OLLAMA_HOST="127.0.0.1:11435"\nollama serve'
        }
      ]
    }
  };

  // --- INITIALIZATION ---
  document.addEventListener('DOMContentLoaded', () => {
    initSolver();
  });

  function initSolver() {
    const input = document.getElementById('solverInput');
    const btn = document.getElementById('btnSolve');
    const samplePills = document.querySelectorAll('.sample-pill');

    // Default to port 3000
    solveQuery("3000");

    if (btn && input) {
      btn.addEventListener('click', () => {
        solveQuery(input.value.trim());
      });

      input.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
          solveQuery(input.value.trim());
        }
      });
    }

    samplePills.forEach(pill => {
      pill.addEventListener('click', () => {
        const query = pill.getAttribute('data-query');
        if (input) input.value = query;
        solveQuery(query);
      });
    });
  }

  // --- SOLVER LOGIC & QUERY PARSER ---
  function solveQuery(query) {
    if (!query) query = "3000";

    const extracted = parseQuery(query);
    const profile = getProfileForPort(extracted.port, extracted.frameworkHint);

    renderDiagnosis(profile, query);
  }

  function parseQuery(raw) {
    const str = raw.trim();

    // 1. Direct number
    const numMatch = str.match(/\b(3000|5173|8000|8080|5432|6379|4200|11434|27017|9200|4321|8081|3306|\d{2,5})\b/);
    let port = numMatch ? parseInt(numMatch[1], 10) : 3000;

    let frameworkHint = null;
    const lower = str.toLowerCase();

    if (lower.includes('next') || lower.includes('eaddrinuse') || lower.includes(':::3000')) {
      port = 3000;
      frameworkHint = "Next.js / Node.js";
    } else if (lower.includes('vite') || lower.includes('5173')) {
      port = 5173;
      frameworkHint = "Vite";
    } else if (lower.includes('fastapi') || lower.includes('django') || lower.includes('10048') || lower.includes('uvicorn')) {
      port = 8000;
      frameworkHint = "FastAPI / Python";
    } else if (lower.includes('postgres') || lower.includes('postmaster.pid') || lower.includes('5432')) {
      port = 5432;
      frameworkHint = "PostgreSQL";
    } else if (lower.includes('spring') || lower.includes('tomcat') || lower.includes('8080')) {
      port = 8080;
      frameworkHint = "Spring Boot / Tomcat";
    } else if (lower.includes('redis') || lower.includes('6379')) {
      port = 6379;
      frameworkHint = "Redis";
    } else if (lower.includes('angular') || lower.includes('4200')) {
      port = 4200;
      frameworkHint = "Angular CLI";
    } else if (lower.includes('ollama') || lower.includes('11434')) {
      port = 11434;
      frameworkHint = "Ollama";
    }

    return { port, frameworkHint };
  }

  function getProfileForPort(port, hint) {
    if (CONFLICT_PROFILES[port]) {
      return CONFLICT_PROFILES[port];
    }

    // Generic fallback profile
    const randomPid = Math.floor(Math.random() * 25000) + 4000;
    return {
      port: port,
      framework: hint || `Custom App / Service on :${port}`,
      exe: "app.exe",
      defaultPid: randomPid,
      mem: "45 MB",
      errorSample: `Error: Port ${port} is already in use (EADDRINUSE)`,
      what: `A running process on your system is actively bound to TCP port ${port}.`,
      why: `Another application, dev server instance, or background service is occupying this local port.`,
      cmdCommands: [
        `netstat -ano | findstr :${port}`,
        `taskkill /F /PID ${randomPid}`
      ],
      psCommands: [
        `Get-NetTCPConnection -LocalPort ${port} | ForEach-Object { Stop-Process -Id $_.OwningProcess -Force }`
      ],
      safeConfigs: [
        {
          target: "Environment Variable Override",
          file: ".env",
          code: `PORT=${port + 1}`
        },
        {
          target: "Docker Container Port Map",
          file: "docker-compose.yml",
          code: `ports:\n  - "${port + 1}:${port}"`
        }
      ]
    };
  }

  // --- RENDER DIAGNOSIS UI ---
  function renderDiagnosis(p, originalQuery) {
    const container = document.getElementById('diagnosisContainer');
    if (!container) return;

    container.innerHTML = `
      <!-- TOP STATUS BANNER -->
      <div class="diag-banner">
        <div class="diag-icon-wrapper">
          <span>⚠️</span>
        </div>
        <div class="diag-body">
          <h2 class="diag-status-title">
            <span>Port Conflict Detected on</span>
            <span class="diag-port-badge">:${p.port}</span>
            <span style="font-size: 0.95rem; color: #94a3b8; font-weight: 500;">(${escapeHtml(p.framework)})</span>
          </h2>
          <p class="diag-summary-text">
            <strong>What Happened:</strong> ${escapeHtml(p.what)}<br/>
            <strong style="color:#cbd5e1; margin-top:4px; display:inline-block;">Root Cause:</strong> ${escapeHtml(p.why)}
          </p>

          <!-- Specs Bar -->
          <div class="process-specs-bar">
            <div class="spec-item">
              <span class="spec-label">Owning Process</span>
              <span class="spec-val" style="color: #38bdf8;">${escapeHtml(p.exe)}</span>
            </div>
            <div class="spec-item">
              <span class="spec-label">Process ID (PID)</span>
              <span class="spec-val" style="color: #f87171;">${p.defaultPid}</span>
            </div>
            <div class="spec-item">
              <span class="spec-label">Memory Footprint</span>
              <span class="spec-val">${p.mem}</span>
            </div>
            <div class="spec-item">
              <span class="spec-label">Socket State</span>
              <span class="spec-val" style="color: #22c55e;">LISTENING (0.0.0.0:${p.port})</span>
            </div>
          </div>
        </div>
      </div>

      <!-- SIDE BY SIDE COMPARISON -->
      <div class="compare-grid">
        
        <!-- CARD 1: THE MANUAL TERMINAL WAY -->
        <div class="compare-card manual">
          <div>
            <div class="card-header-row">
              <div class="card-header-title">
                <span style="color: #f87171;">❌</span>
                <span>The Manual Terminal Way</span>
              </div>
              <span class="card-header-badge badge-manual">Multi-Step Friction</span>
            </div>

            <p style="font-size:0.85rem; color:#94a3b8; margin-bottom:12px;">
              Requires finding the PID, memorizing flags, and forcing a process kill in admin terminal.
            </p>

            <div class="code-terminal-box">
              <div style="font-size:0.72rem; color:#64748b; margin-bottom:6px;">WINDOWS COMMAND PROMPT (CMD)</div>
              ${p.cmdCommands.map(cmd => `
                <div class="cmd-line">
                  <span><span class="prompt">&gt;</span>${escapeHtml(cmd)}</span>
                  <button class="btn-cmd-copy" onclick="window.conflictSolver.copyText('${escapeQuotes(cmd)}', this)">Copy</button>
                </div>
              `).join('')}
            </div>

            <div class="code-terminal-box">
              <div style="font-size:0.72rem; color:#64748b; margin-bottom:6px;">POWERSHELL (ONE-LINER)</div>
              ${p.psCommands.map(cmd => `
                <div class="cmd-line">
                  <span><span class="prompt">PS &gt;</span>${escapeHtml(cmd)}</span>
                  <button class="btn-cmd-copy" onclick="window.conflictSolver.copyText('${escapeQuotes(cmd)}', this)">Copy</button>
                </div>
              `).join('')}
            </div>
          </div>

          <div style="font-size:0.78rem; color:#ef4444; background:rgba(239,68,68,0.1); padding:8px 12px; border-radius:6px; border:1px solid rgba(239,68,68,0.2);">
            ⚠️ <b>High Friction:</b> Takes ~20-40 seconds of context switching away from your IDE.
          </div>
        </div>

        <!-- CARD 2: THE 1-CLICK PORTPEEK WAY -->
        <div class="compare-card portpeek">
          <div>
            <div class="card-header-row">
              <div class="card-header-title">
                <span style="color: #4ade80;">✅</span>
                <span>The 1-Click PortPeek Way</span>
              </div>
              <span class="card-header-badge badge-portpeek">Instant &lt; 1 Second</span>
            </div>

            <p style="font-size:0.85rem; color:#94a3b8; margin-bottom:14px;">
              Zero command line memorization. Terminate orphaned servers directly from your system tray.
            </p>

            <div class="portpeek-steps-list">
              <div class="step-card">
                <span class="step-num">1</span>
                <span class="step-text">Click <strong>PortPeek tray icon</strong> (or press <code>Win + Alt + P</code>)</span>
              </div>
              <div class="step-card">
                <span class="step-num">2</span>
                <span class="step-text">Hover over <strong>Stop Process ▶</strong></span>
              </div>
              <div class="step-card">
                <span class="step-num">3</span>
                <span class="step-text">Click <strong>Port :${p.port} (${escapeHtml(p.exe)})</strong></span>
              </div>
            </div>
          </div>

          <div>
            <button class="btn-demo-kill-action" id="btnSimulateKill" onclick="window.conflictSolver.simulateKill(${p.port}, '${escapeQuotes(p.exe)}', ${p.defaultPid})">
              <span>⚡ Simulate PortPeek 1-Click Kill (: ${p.port})</span>
            </button>
          </div>
        </div>

      </div>

      <!-- SAFE CONFIGURATION WORKAROUNDS -->
      <div class="safe-configs-section">
        <div class="safe-header">
          <h3>
            <span>🛡️ Safe Configuration Alternatives</span>
            <span style="font-size:0.75rem; color:var(--accent-cyan); background:rgba(56,189,248,0.1); padding:2px 8px; border-radius:12px;">Run Both Simultaneously</span>
          </h3>
          <p>Don't want to kill the existing process? Cleanly configure your framework to use an alternate port:</p>
        </div>

        <div class="config-snippets-grid">
          ${p.safeConfigs.map(c => `
            <div class="config-card">
              <div>
                <div class="config-card-header">
                  <span>${escapeHtml(c.target)}</span>
                  <span class="config-file-tag">${escapeHtml(c.file)}</span>
                </div>
                <pre class="config-code-pre">${escapeHtml(c.code)}</pre>
              </div>
              <button class="btn-cmd-copy" style="align-self:flex-start;" onclick="window.conflictSolver.copyText('${escapeQuotes(c.code)}', this)">📋 Copy Config Snippet</button>
            </div>
          `).join('')}
        </div>

        <!-- Security & OAuth Callout -->
        <div class="security-oauth-alert">
          <span style="font-size:1.2rem;">🔒</span>
          <div>
            <strong>Important CORS & OAuth Notice:</strong>
            If you change your local dev port (e.g. from <code>:3000</code> to <code>:3001</code>), make sure to add <code>http://localhost:3001/api/auth/callback</code> to your OAuth Provider's Allowed Redirect URIs (Google Cloud Console / GitHub Developer Settings) and update your backend CORS origin list!
          </div>
        </div>
      </div>
    `;
  }

  // --- PUBLIC API ---
  window.conflictSolver = {
    copyText: (text, btn) => {
      navigator.clipboard.writeText(text).then(() => {
        const originalText = btn.textContent;
        btn.textContent = "✓ Copied!";
        btn.style.color = "#22c55e";
        setTimeout(() => {
          btn.textContent = originalText;
          btn.style.color = "";
        }, 1800);
      });
    },
    simulateKill: (port, exe, pid) => {
      const btn = document.getElementById('btnSimulateKill');
      if (!btn) return;

      btn.disabled = true;
      btn.innerHTML = `<span>⏳ Terminating PID ${pid} via Win32 API...</span>`;
      btn.style.background = "#0284c7";

      setTimeout(() => {
        btn.innerHTML = `<span>✅ Process Terminated! Port :${port} is now FREE</span>`;
        btn.style.background = "#15803d";
        btn.style.borderColor = "#22c55e";

        setTimeout(() => {
          btn.disabled = false;
          btn.innerHTML = `<span>⚡ Simulate PortPeek 1-Click Kill (: ${port})</span>`;
          btn.style.background = "";
          btn.style.borderColor = "";
        }, 3000);
      }, 600);
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

  function escapeQuotes(str) {
    if (!str) return '';
    return String(str)
      .replace(/\\/g, '\\\\')
      .replace(/'/g, "\\'")
      .replace(/"/g, '\\"')
      .replace(/\n/g, '\\n')
      .replace(/\r/g, '');
  }

})();
