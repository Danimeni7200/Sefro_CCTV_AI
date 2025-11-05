/**
 * Main Application Module
 */

import state from './state.js';
import { navigateTo } from './navigation.js';
import { loadResults } from './data.js';
import { updateConnectionStatus, showToast } from './ui.js';
import { startAutoRefresh } from './settings.js';
import { updateLiveStreamView } from './livestream.js';
import { performDiscover } from './discover.js';
import { testConnection, saveSettings, resetSettings } from './settings.js';
import { filterResults } from './search.js';
import { exportResults } from './export.js';
import { previousPage, nextPage } from './pagination.js';
import { updateCameraFilter } from './ui.js';
import { loadResultsTable, filterResultsTable, filterByCamera, filterByDate } from './results.js';
import { closeModal } from './dashboard.js';

// Initialize the application
function initializeApp() {
  // Load initial data
  loadResults();
  updateConnectionStatus();
  
  // Set initial values
  document.getElementById('aiHostInput').value = state.aiHost;
  document.getElementById('refreshIntervalInput').value = state.refreshInterval / 1000;
  document.getElementById('resultsLimitInput').value = state.resultsLimit;
}

// Setup event listeners
function setupEventListeners() {
  // Navigation
  document.querySelectorAll('.nav-item').forEach(item => {
    item.addEventListener('click', (e) => {
      e.preventDefault();
      const page = item.dataset.page;
      navigateTo(page);
    });
  });

  // Header buttons
  document.getElementById('refreshBtn').addEventListener('click', () => {
    loadResults();
    showToast('بروزرسانی شد', 'success');
  });

  document.getElementById('notificationBtn').addEventListener('click', () => {
    showToast('3 اطلاع جدید', 'info');
  });

  // Search
  document.getElementById('searchInput').addEventListener('input', (e) => {
    filterResults(e.target.value);
  });

  // Results page filters
  document.getElementById('plateSearch').addEventListener('input', (e) => {
    filterResultsTable(e.target.value);
  });

  document.getElementById('cameraFilter').addEventListener('change', (e) => {
    filterByCamera(e.target.value);
  });

  document.getElementById('dateFilter').addEventListener('change', (e) => {
    filterByDate(e.value);
  });

  document.getElementById('exportBtn').addEventListener('click', exportResults);

  // Settings
  document.getElementById('testConnectionBtn').addEventListener('click', testConnection);
  document.getElementById('saveSettingsBtn').addEventListener('click', saveSettings);
  document.getElementById('resetSettingsBtn').addEventListener('click', resetSettings);

  // Theme selector
  document.querySelectorAll('.theme-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      const theme = btn.dataset.theme;
      setTheme(theme);
    });
  });

  // Discover form
  document.getElementById('discoverForm').addEventListener('submit', (e) => {
    e.preventDefault();
    performDiscover();
  });

  // Live stream page
  document.getElementById('goToDiscoverBtn').addEventListener('click', () => {
    navigateTo('discover');
  });
  
  // Refresh livestream button
  document.getElementById('refreshLivestreamBtn').addEventListener('click', () => {
    if (state.currentPage === 'livestream') {
      updateLiveStreamView();
      showToast('نمایش زنده بروزرسانی شد', 'success');
    }
  });

  // Modal
  document.getElementById('closeModalBtn').addEventListener('click', closeModal);
  document.getElementById('detailsModal').addEventListener('click', (e) => {
    if (e.target === document.getElementById('detailsModal')) {
      closeModal();
    }
  });

  // Pagination
  document.getElementById('prevBtn').addEventListener('click', previousPage);
  document.getElementById('nextBtn').addEventListener('click', nextPage);
}

// Apply theme
function applyTheme(theme) {
  if (theme === 'light') {
    document.documentElement.setAttribute('data-theme', 'light');
  } else {
    document.documentElement.removeAttribute('data-theme');
  }
}

// Set theme
function setTheme(theme) {
  state.theme = theme;
  localStorage.setItem('theme', theme);
  applyTheme(theme);

  // Update theme buttons
  document.querySelectorAll('.theme-btn').forEach(btn => {
    btn.classList.remove('active');
    if (btn.dataset.theme === theme) {
      btn.classList.add('active');
    }
  });
}

export { initializeApp, setupEventListeners, applyTheme, setTheme };