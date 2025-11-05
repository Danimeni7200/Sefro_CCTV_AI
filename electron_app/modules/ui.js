/**
 * UI Module
 */

import state from './state.js';

function showToast(message, type = 'info') {
  const container = document.getElementById('toastContainer');
  const toast = document.createElement('div');
  toast.className = `toast ${type}`;

  const icons = {
    success: 'fa-check-circle',
    error: 'fa-exclamation-circle',
    warning: 'fa-exclamation-triangle',
    info: 'fa-info-circle',
  };

  toast.innerHTML = `
    <i class="fas ${icons[type]} toast-icon"></i>
    <span class="toast-message">${message}</span>
    <button class="toast-close">&times;</button>
  `;

  container.appendChild(toast);

  const closeBtn = toast.querySelector('.toast-close');
  closeBtn.addEventListener('click', () => {
    toast.remove();
  });

  setTimeout(() => {
    toast.remove();
  }, 4000);
}

function updateConnectionStatus() {
  const statusDot = document.getElementById('statusDot');
  const statusText = document.getElementById('statusText');

  if (state.isConnected) {
    statusDot.classList.add('connected');
    statusText.textContent = 'متصل';
  } else {
    statusDot.classList.remove('connected');
    statusText.textContent = 'قطع شده';
  }
}

function updateCameraFilter() {
  const select = document.getElementById('cameraFilter');
  const currentValue = select.value;
  
  select.innerHTML = '<option value="">همه دوربین‌ها</option>';
  
  state.cameras.forEach(camera => {
    const option = document.createElement('option');
    option.value = camera;
    option.textContent = camera;
    select.appendChild(option);
  });

  select.value = currentValue;
}

function formatTime(dateString) {
  try {
    const date = new Date(dateString);
    return date.toLocaleTimeString('fa-IR', {
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit'
    });
  } catch (error) {
    return 'نامشخص';
  }
}

function formatDateTime(timestamp) {
  if (!timestamp) return 'نامشخص';
  const date = new Date(timestamp);
  return date.toLocaleString('fa-IR');
}

export { showToast, updateConnectionStatus, updateCameraFilter, formatTime, formatDateTime };