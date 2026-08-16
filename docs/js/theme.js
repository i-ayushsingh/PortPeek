/**
 * PortPeek Theme Synchronization
 */
(function() {
  const savedTheme = localStorage.getItem('portpeek_theme') || 'dark';
  document.documentElement.setAttribute('data-theme', savedTheme);

  window.toggleTheme = function() {
    const current = document.documentElement.getAttribute('data-theme') || 'dark';
    const next = current === 'dark' ? 'light' : 'dark';
    document.documentElement.setAttribute('data-theme', next);
    localStorage.setItem('portpeek_theme', next);
  };
})();
