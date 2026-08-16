/**
 * PortPeek Website Interactive Logic — Apple & Arc Polish
 * Fast OmniSearch, Developer-First Port Prioritization, Clipboard Helpers, View Switcher
 */

// Developer Priority Order for Ports Directory
const POPULAR_DEV_PORT_ORDER = [
  3000, 5173, 8000, 5432, 6379, 11434, 8080, 4200, 4321, 8081, 
  5500, 5000, 5001, 3001, 1337, 4000, 7860, 8501, 8001, 9090, 
  9229, 27017, 1433, 9000, 22, 80, 443
];

document.addEventListener('DOMContentLoaded', () => {
  initMobileNav();
  initPortsDirectory();
});

// --- MOBILE NAVIGATION ---
function initMobileNav() {
  const toggle = document.getElementById('navToggle');
  const navLinks = document.getElementById('navLinks');
  if (toggle && navLinks) {
    toggle.addEventListener('click', () => {
      navLinks.classList.toggle('open');
    });
  }
}

// --- PORTS DIRECTORY ENGINE ---
function initPortsDirectory() {
  const gridContainer = document.getElementById('portsGrid');
  const tableBody = document.getElementById('portsTableBody');
  const searchInput = document.getElementById('portSearchInput');
  const filterBtns = document.querySelectorAll('.filter-btn');
  const btnViewGrid = document.getElementById('btnViewGrid');
  const btnViewTable = document.getElementById('btnViewTable');
  const tableWrap = document.getElementById('portsTableWrap');
  const countAll = document.getElementById('countAll');

  if (typeof DEV_PORTS_DATA === 'undefined') return;

  if (countAll) {
    countAll.textContent = DEV_PORTS_DATA.length;
  }

  // Developer prioritized sort: common dev ports first, then ascending port number
  const sortedPortsData = [...DEV_PORTS_DATA].sort((a, b) => {
    const idxA = POPULAR_DEV_PORT_ORDER.indexOf(a.port);
    const idxB = POPULAR_DEV_PORT_ORDER.indexOf(b.port);
    
    if (idxA !== -1 && idxB !== -1) return idxA - idxB;
    if (idxA !== -1) return -1;
    if (idxB !== -1) return 1;
    return a.port - b.port;
  });

  let currentFilter = 'all';
  let currentQuery = '';
  let activeView = 'grid'; // 'grid' | 'table'

  function render() {
    const filtered = sortedPortsData.filter(item => {
      const matchCat = (currentFilter === 'all' || item.category === currentFilter);
      const query = currentQuery.toLowerCase().trim();
      const matchQuery = !query ||
        item.port.toString().includes(query) ||
        (item.name && item.name.toLowerCase().includes(query)) ||
        (item.framework && item.framework.toLowerCase().includes(query)) ||
        (item.commonUsage && item.commonUsage.toLowerCase().includes(query)) ||
        (item.description && item.description.toLowerCase().includes(query)) ||
        (item.command && item.command.toLowerCase().includes(query)) ||
        (item.ianaStatus && item.ianaStatus.toLowerCase().includes(query));

      return matchCat && matchQuery;
    });

    // Render Bento Grid View
    if (gridContainer) {
      if (filtered.length === 0) {
        gridContainer.innerHTML = `
          <div style="grid-column: 1 / -1; text-align: center; padding: 64px 20px; color: var(--text-muted); background: var(--bg-surface); border: 1px solid var(--border-default); border-radius: var(--radius-xl);">
            <div style="font-size: 2rem; margin-bottom: 8px;">🔍</div>
            <h3 style="font-size: 1.2rem; color: var(--text-primary); margin-bottom: 6px;">No matching ports found</h3>
            <p style="font-size: 0.9rem;">Try searching for "3000", "Vite", "FastAPI", "Postgres", "Redis", or "Ollama"</p>
          </div>
        `;
      } else {
        gridContainer.innerHTML = filtered.map(p => {
          const detailUrl = `${p.port}/index.html`;
          const localhostUrl = p.localhostUrl || `http://localhost:${p.port}`;
          const safeCmd = p.startCommand || p.command || `npm run dev`;
          const safeFramework = p.primaryFramework || p.framework || p.name;
          const safeTag = p.tag || p.category.toUpperCase();
          const categoryColor = getCategoryColor(p.category);

          return `
            <div class="port-card" data-category="${p.category}">
              <div>
                <div class="card-top">
                  <div class="card-port-num">${p.port}</div>
                  <span class="card-tag" style="border-color: ${categoryColor}40; color: ${categoryColor}; background: ${categoryColor}15;">
                    ${escapeHtml(safeTag)}
                  </span>
                </div>
                <h3 class="card-title">${escapeHtml(safeFramework)}</h3>
                <p class="card-desc">${escapeHtml(p.description)}</p>
              </div>

              <div>
                <div class="card-cmd-box">
                  <span class="cmd-text" title="${escapeHtml(safeCmd)}">$ ${escapeHtml(safeCmd)}</span>
                  <button class="btn-copy-cmd" onclick="copyCommand('${escapeQuotes(safeCmd)}', this)" title="Copy command" aria-label="Copy command">
                    <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect width="14" height="14" x="8" y="8" rx="2" ry="2"/><path d="M4 16c-1.1 0-2-.9-2-2V4c0-1.1.9-2 2-2h10c1.1 0 2 .9 2 2"/></svg>
                  </button>
                </div>

                <div class="card-actions-row">
                  <span style="font-size: 0.8rem; font-family: var(--font-mono); color: var(--text-muted);">${escapeHtml(p.protocol || 'TCP')}</span>
                  <a href="${detailUrl}" class="link-deep-dive">Deep Dive Guide →</a>
                </div>
              </div>
            </div>
          `;
        }).join('');
      }
    }

    // Render Table View
    if (tableBody) {
      if (filtered.length === 0) {
        tableBody.innerHTML = `<tr><td colspan="7" style="text-align: center; padding: 40px; color: var(--text-muted);">No matching ports found</td></tr>`;
      } else {
        tableBody.innerHTML = filtered.map(p => {
          const detailUrl = `${p.port}/index.html`;
          const safeCmd = p.startCommand || p.command || `npm run dev`;
          const safeFramework = p.primaryFramework || p.framework || p.name;

          return `
            <tr>
              <td style="font-family: var(--font-mono); font-weight: 800; color: var(--accent-cyan); font-size: 1.1rem;">${p.port}</td>
              <td style="font-weight: 700; color: var(--text-primary);">${escapeHtml(safeFramework)}</td>
              <td><span style="text-transform: capitalize; font-size: 0.84rem;">${escapeHtml(p.category)}</span></td>
              <td><code>${escapeHtml(p.protocol || 'TCP')}</code></td>
              <td><span style="font-size: 0.8rem; color: var(--text-muted);">${escapeHtml(p.ianaStatus || 'Registered / Dynamic')}</span></td>
              <td><code style="font-size: 0.82rem; color: var(--text-primary);">$ ${escapeHtml(safeCmd)}</code></td>
              <td><a href="${detailUrl}" style="color: var(--accent-cyan); font-weight: 700; font-size: 0.88rem;">Guide →</a></td>
            </tr>
          `;
        }).join('');
      }
    }
  }

  function getCategoryColor(cat) {
    switch (cat) {
      case 'frontend': return '#38bdf8';
      case 'backend': return '#a855f7';
      case 'database': return '#f59e0b';
      case 'ai': return '#ec4899';
      case 'system': return '#10b981';
      default: return '#0078d4';
    }
  }

  // Live Search listener
  if (searchInput) {
    searchInput.addEventListener('input', (e) => {
      currentQuery = e.target.value;
      render();
    });
  }

  // Filter Buttons
  filterBtns.forEach(btn => {
    btn.addEventListener('click', () => {
      filterBtns.forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      currentFilter = btn.getAttribute('data-filter') || 'all';
      render();
    });
  });

  // View Switcher (Grid vs Table)
  if (btnViewGrid && btnViewTable && tableWrap && gridContainer) {
    btnViewGrid.addEventListener('click', () => {
      btnViewGrid.classList.add('active');
      btnViewTable.classList.remove('active');
      gridContainer.style.display = 'grid';
      tableWrap.style.display = 'none';
      activeView = 'grid';
    });

    btnViewTable.addEventListener('click', () => {
      btnViewTable.classList.add('active');
      btnViewGrid.classList.remove('active');
      gridContainer.style.display = 'none';
      tableWrap.style.display = 'block';
      activeView = 'table';
    });
  }

  // Initial Render
  render();
}

// Helper: Escape HTML
function escapeHtml(str) {
  if (!str) return '';
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#039;');
}

// Helper: Escape Quotes
function escapeQuotes(str) {
  if (!str) return '';
  return String(str).replace(/'/g, "\\'");
}

// Global 1-click Copy Helper with Toast feedback
window.copyCommand = function(text, btnElement) {
  if (!navigator.clipboard) return;
  navigator.clipboard.writeText(text).then(() => {
    if (btnElement) {
      const origHtml = btnElement.innerHTML;
      btnElement.innerHTML = `<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#22c55e" stroke-width="2.5"><polyline points="20 6 9 17 4 12"/></svg>`;
      setTimeout(() => {
        btnElement.innerHTML = origHtml;
      }, 1800);
    }
  });
};
