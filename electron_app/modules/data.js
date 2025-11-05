/**
 * Data Loading Module
 */

import state from './state.js';
import { showToast, updateConnectionStatus, updateCameraFilter } from './ui.js';
import { updateDashboard, updateRecentResults } from './dashboard.js';
import { updateLiveStreamView } from './livestream.js';
import { loadResultsTable } from './results.js';

async function loadResults() {
  try {
    const data = await window.lpr.fetchLatest(state.aiHost, state.resultsLimit);
    
    if (data && data.error) {
      // Check if it's a connection error (404 or network error)
      if (data.error.includes('404') || data.error.includes('fetch') || data.error.includes('network')) {
        showToast('سرویس اصلی در دسترس نیست. لطفاً فایل start-essential.bat را اجرا کنید تا سرویس‌های ضروری راه‌اندازی شوند.', 'error');
      } else {
        showToast('خطا در دریافت اطلاعات: ' + data.error, 'error');
      }
      state.isConnected = false;
      updateConnectionStatus();
      return;
    }

    state.results = data || [];
    state.isConnected = true;
    updateConnectionStatus();

    // Extract unique cameras
    state.cameras.clear();
    state.results.forEach(r => {
      if (r.camera_id) state.cameras.add(r.camera_id);
    });

    // Update camera filter
    updateCameraFilter();

    // Update dashboard if on dashboard page
    if (state.currentPage === 'dashboard') {
      updateDashboard();
    }

    // Update results table if on results page
    if (state.currentPage === 'results') {
      loadResultsTable();
    }
  } catch (error) {
    console.error('Error loading results:', error);
    showToast('خطا در بارگذاری اطلاعات. لطفاً فایل start-essential.bat را اجرا کنید تا سرویس‌های ضروری راه‌اندازی شوند.', 'error');
    state.isConnected = false;
    updateConnectionStatus();
  }
}

export { loadResults };