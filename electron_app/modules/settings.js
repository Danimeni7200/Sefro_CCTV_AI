/**
 * Settings Module
 */

import state from './state.js';
import { showToast } from './ui.js';
import { loadResults } from './data.js';

async function testConnection() {
  const host = document.getElementById('aiHostInput').value;
  const btn = document.getElementById('testConnectionBtn');
  
  btn.disabled = true;
  btn.innerHTML = '<i class="fas fa-spinner spin"></i> در حال تست...';

  try {
    const response = await fetch(`${host}/healthz`);
    if (response.ok) {
      showToast('اتصال موفق!', 'success');
      state.aiHost = host;
    } else {
      if (response.status === 404) {
        showToast('سرویس در دسترس نیست. لطفاً فایل start-essential.bat را اجرا کنید.', 'error');
      } else {
        showToast('سرویس پاسخ نداد. کد خطا: ' + response.status, 'error');
      }
    }
  } catch (error) {
    showToast('خطا در اتصال: لطفاً فایل start-essential.bat را اجرا کنید. خطا: ' + error.message, 'error');
  } finally {
    btn.disabled = false;
    btn.innerHTML = '<i class="fas fa-plug"></i> تست اتصال';
  }
}

function saveSettings() {
  const host = document.getElementById('aiHostInput').value;
  const interval = parseInt(document.getElementById('refreshIntervalInput').value) * 1000;
  const limit = parseInt(document.getElementById('resultsLimitInput').value);

  localStorage.setItem('ai_host', host);
  localStorage.setItem('refreshInterval', interval);
  localStorage.setItem('resultsLimit', limit);

  state.aiHost = host;
  state.refreshInterval = interval;
  state.resultsLimit = limit;

  // Restart auto-refresh
  clearInterval(state.autoRefreshTimer);
  startAutoRefresh();

  showToast('تنظیمات ذخیره شد', 'success');
}

function resetSettings() {
  if (confirm('آیا مطمئن هستید؟')) {
    localStorage.clear();
    location.reload();
  }
}

function startAutoRefresh() {
  state.autoRefreshTimer = setInterval(() => {
    loadResults();
  }, state.refreshInterval);
}

export { testConnection, saveSettings, resetSettings, startAutoRefresh };