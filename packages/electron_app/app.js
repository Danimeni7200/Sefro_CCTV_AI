/**
 * Modern Electron App - License Plate Recognition System
 * Beautiful UI/UX with full functionality
 */

// ============================================================================
// STATE MANAGEMENT
// ============================================================================

const state = {
  currentPage: 'dashboard',
  aiHost: localStorage.getItem('ai_host') || 'http://127.0.0.1:8000',
  refreshInterval: parseInt(localStorage.getItem('refreshInterval') || '3000'),
  resultsLimit: parseInt(localStorage.getItem('resultsLimit') || '50'),
  theme: localStorage.getItem('theme') || 'dark',
  results: [],
  cameras: new Set(),
  discoveredCameras: JSON.parse(localStorage.getItem('discoveredCameras') || '[]'),
  isConnected: false,
  currentPage: 'dashboard',
  autoRefreshTimer: null,
};

// ============================================================================
// INITIALIZATION
// ============================================================================

document.addEventListener('DOMContentLoaded', () => {
  initializeApp();
  setupEventListeners();
  applyTheme(state.theme);
  startAutoRefresh();
});

function initializeApp() {
  // Load initial data
  loadResults();
  checkConnection();
  
  // Set initial values
  document.getElementById('aiHostInput').value = state.aiHost;
  document.getElementById('refreshIntervalInput').value = state.refreshInterval / 1000;
  document.getElementById('resultsLimitInput').value = state.resultsLimit;
}

// ============================================================================
// EVENT LISTENERS
// ============================================================================

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
    filterByDate(e.target.value);
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

// ============================================================================
// NAVIGATION
// ============================================================================

function navigateTo(page) {
  // Hide all pages
  document.querySelectorAll('.page').forEach(p => p.style.display = 'none');
  
  // Show selected page
  const pageElement = document.getElementById(`${page}-page`);
  if (pageElement) {
    pageElement.style.display = 'block';
    pageElement.classList.add('active');
  }

  // Update nav items
  document.querySelectorAll('.nav-item').forEach(item => {
    item.classList.remove('active');
    if (item.dataset.page === page) {
      item.classList.add('active');
    }
  });

  // Update header
  const titles = {
    dashboard: { title: 'داشبورد', subtitle: 'خوش آمدید به سامانه تشخیص پلاک' },
    results: { title: 'نتایج', subtitle: 'مشاهده تمام نتایج تشخیص شده' },
    settings: { title: 'تنظیمات', subtitle: 'تنظیمات برنامه و سرویس' },
    discover: { title: 'کشف دوربین', subtitle: 'کشف خودکار دوربین‌های RTSP' },
    livestream: { title: '👁️ مشاهده زنده دوربین‌ها', subtitle: 'مشاهده جریان زنده دوربین‌های کشف شده' },
  };

  const pageInfo = titles[page] || titles.dashboard;
  document.getElementById('pageTitle').textContent = pageInfo.title;
  document.getElementById('pageSubtitle').textContent = pageInfo.subtitle;

  state.currentPage = page;

  // Load page-specific data
  if (page === 'results') {
    loadResultsTable();
  } else if (page === 'dashboard') {
    updateDashboard();
  } else if (page === 'livestream') {
    updateLiveStreamView();
  }
}

// ============================================================================
// DATA LOADING
// ============================================================================

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

function updateDashboard() {
  // Update stats
  document.getElementById('totalPlates').textContent = state.results.length;
  document.getElementById('activeCameras').textContent = state.cameras.size;
  
  // Calculate accuracy
  const avgConfidence = state.results.length > 0
    ? (state.results.reduce((sum, r) => sum + (r.confidence || 0), 0) / state.results.length * 100).toFixed(1)
    : 0;
  document.getElementById('accuracy').textContent = avgConfidence + '%';
  
  // Processing speed (mock)
  document.getElementById('processingSpeed').textContent = '45ms';

  // Update recent results
  updateRecentResults();

  // Draw charts
  drawDetectionChart();
  drawCameraChart();
}

function updateRecentResults() {
  const container = document.getElementById('recentResultsList');
  container.innerHTML = '';

  const recentResults = state.results.slice(0, 5);
  
  recentResults.forEach(result => {
    const item = document.createElement('div');
    item.className = 'result-item';
    
    const confidence = (result.confidence * 100).toFixed(1);
    const confidencePercent = Math.min(100, confidence);
    
    item.innerHTML = `
      <div class="result-info">
        <div class="result-plate">${result.plate_text || 'نامشخص'}</div>
        <div class="result-meta">
          <span><i class="fas fa-camera"></i> ${result.camera_id || 'نامشخص'}</span>
          <span><i class="fas fa-clock"></i> ${formatTime(result.timestamp)}</span>
        </div>
      </div>
      <div class="result-confidence">
        <div class="confidence-bar">
          <div class="confidence-fill" style="width: ${confidencePercent}%"></div>
        </div>
        <span>${confidence}%</span>
      </div>
    `;
    
    item.addEventListener('click', () => showResultDetails(result));
    container.appendChild(item);
  });
}

function loadResultsTable() {
  const tbody = document.getElementById('resultsTableBody');
  tbody.innerHTML = '';

  state.results.forEach(result => {
    const tr = document.createElement('tr');
    const confidence = (result.confidence * 100).toFixed(1);
    
    tr.innerHTML = `
      <td>${formatDateTime(result.timestamp)}</td>
      <td>${result.camera_id || 'نامشخص'}</td>
      <td><span class="plate-text">${result.plate_text || 'نامشخص'}</span></td>
      <td><span class="confidence-badge">${confidence}%</span></td>
      <td>
        <small>
          x: ${(result.bbox?.x || 0).toFixed(0)},
          y: ${(result.bbox?.y || 0).toFixed(0)},
          w: ${(result.bbox?.w || 0).toFixed(0)},
          h: ${(result.bbox?.h || 0).toFixed(0)}
        </small>
      </td>
      <td>
        <div class="action-buttons">
          <button class="btn-small" title="مشاهده جزئیات">
            <i class="fas fa-eye"></i>
          </button>
          <button class="btn-small" title="دانلود">
            <i class="fas fa-download"></i>
          </button>
        </div>
      </td>
    `;
    
    tr.querySelector('.btn-small').addEventListener('click', () => showResultDetails(result));
    tbody.appendChild(tr);
  });
}

// ============================================================================
// FILTERING
// ============================================================================

function filterResults(query) {
  if (!query) {
    updateRecentResults();
    return;
  }

  const filtered = state.results.filter(r => 
    r.plate_text?.includes(query) || 
    r.camera_id?.includes(query)
  );

  const container = document.getElementById('recentResultsList');
  container.innerHTML = '';

  filtered.slice(0, 5).forEach(result => {
    const item = document.createElement('div');
    item.className = 'result-item';
    
    const confidence = (result.confidence * 100).toFixed(1);
    const confidencePercent = Math.min(100, confidence);
    
    item.innerHTML = `
      <div class="result-info">
        <div class="result-plate">${result.plate_text || 'نامشخص'}</div>
        <div class="result-meta">
          <span><i class="fas fa-camera"></i> ${result.camera_id || 'نامشخص'}</span>
          <span><i class="fas fa-clock"></i> ${formatTime(result.timestamp)}</span>
        </div>
      </div>
      <div class="result-confidence">
        <div class="confidence-bar">
          <div class="confidence-fill" style="width: ${confidencePercent}%"></div>
        </div>
        <span>${confidence}%</span>
      </div>
    `;
    
    item.addEventListener('click', () => showResultDetails(result));
    container.appendChild(item);
  });
}

function filterResultsTable(query) {
  const tbody = document.getElementById('resultsTableBody');
  const rows = tbody.querySelectorAll('tr');

  rows.forEach(row => {
    const plateText = row.querySelector('.plate-text')?.textContent || '';
    const visible = plateText.includes(query) || query === '';
    row.style.display = visible ? '' : 'none';
  });
}

function filterByCamera(cameraId) {
  const tbody = document.getElementById('resultsTableBody');
  const rows = tbody.querySelectorAll('tr');

  rows.forEach(row => {
    const cells = row.querySelectorAll('td');
    const rowCameraId = cells[1]?.textContent || '';
    const visible = !cameraId || rowCameraId === cameraId;
    row.style.display = visible ? '' : 'none';
  });
}

function filterByDate(date) {
  if (!date) {
    loadResultsTable();
    return;
  }

  const tbody = document.getElementById('resultsTableBody');
  const rows = tbody.querySelectorAll('tr');

  rows.forEach(row => {
    const cells = row.querySelectorAll('td');
    const rowDate = cells[0]?.textContent?.split(' ')[0] || '';
    const visible = rowDate === date;
    row.style.display = visible ? '' : 'none';
  });
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

// ============================================================================
// CHARTS
// ============================================================================

function drawDetectionChart() {
  const canvas = document.getElementById('detectionCanvas');
  if (!canvas) return;

  const ctx = canvas.getContext('2d');
  const width = canvas.parentElement.offsetWidth;
  const height = 300;
  
  canvas.width = width;
  canvas.height = height;

  // Mock data
  const data = [12, 19, 3, 5, 2, 3, 7, 15, 10, 8, 12, 14];
  const labels = ['1', '2', '3', '4', '5', '6', '7', '8', '9', '10', '11', '12'];

  // Draw bars
  const barWidth = width / data.length;
  const maxValue = Math.max(...data);
  const padding = 40;

  ctx.fillStyle = '#e5e7eb';
  ctx.font = '12px Vazirmatn';
  ctx.textAlign = 'center';

  data.forEach((value, index) => {
    const x = index * barWidth + barWidth / 2;
    const barHeight = (value / maxValue) * (height - padding * 2);
    const y = height - padding - barHeight;

    // Draw gradient bar
    const gradient = ctx.createLinearGradient(x - barWidth / 3, y, x - barWidth / 3, height - padding);
    gradient.addColorStop(0, '#667eea');
    gradient.addColorStop(1, '#764ba2');
    
    ctx.fillStyle = gradient;
    ctx.fillRect(x - barWidth / 3, y, barWidth * 0.6, barHeight);

    // Draw label
    ctx.fillStyle = '#9ca3af';
    ctx.fillText(labels[index], x, height - 10);
  });
}

function drawCameraChart() {
  const canvas = document.getElementById('cameraCanvas');
  if (!canvas) return;

  const ctx = canvas.getContext('2d');
  const width = canvas.parentElement.offsetWidth;
  const height = 300;
  
  canvas.width = width;
  canvas.height = height;

  // Mock data
  const cameras = Array.from(state.cameras).slice(0, 4);
  const data = cameras.map(() => Math.floor(Math.random() * 100) + 20);
  const colors = ['#667eea', '#764ba2', '#f5576c', '#43e97b'];

  // Draw pie chart
  const centerX = width / 2;
  const centerY = height / 2;
  const radius = Math.min(width, height) / 3;
  const total = data.reduce((a, b) => a + b, 0);

  let currentAngle = 0;

  data.forEach((value, index) => {
    const sliceAngle = (value / total) * Math.PI * 2;

    // Draw slice
    ctx.fillStyle = colors[index];
    ctx.beginPath();
    ctx.moveTo(centerX, centerY);
    ctx.arc(centerX, centerY, radius, currentAngle, currentAngle + sliceAngle);
    ctx.closePath();
    ctx.fill();

    // Draw label
    const labelAngle = currentAngle + sliceAngle / 2;
    const labelX = centerX + Math.cos(labelAngle) * (radius * 0.7);
    const labelY = centerY + Math.sin(labelAngle) * (radius * 0.7);

    ctx.fillStyle = '#ffffff';
    ctx.font = 'bold 12px Vazirmatn';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(((value / total) * 100).toFixed(0) + '%', labelX, labelY);

    currentAngle += sliceAngle;
  });
}

// ============================================================================
// SETTINGS
// ============================================================================

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

// ============================================================================
// DISCOVER
// ============================================================================

async function performDiscover() {
  const ip = document.getElementById('discoverIp').value;
  const user = document.getElementById('discoverUser').value;
  const pass = document.getElementById('discoverPass').value;
  const brand = document.getElementById('discoverBrand').value;

  if (!ip || !user || !pass) {
    showToast('لطفا تمام فیلدها را پر کنید', 'warning');
    return;
  }

  const btn = document.querySelector('#discoverForm button');
  btn.disabled = true;
  btn.innerHTML = '<i class="fas fa-spinner spin"></i> در حال کشف...';

  try {
    const result = await window.lpr.discover(ip, user, pass, brand);
    
    if (result && result.candidates && result.candidates.length > 0) {
      // Automatically add the FIRST discovered camera to live view (only one)
      // Check if this camera is already added
      const firstUrl = result.candidates[0];
      const alreadyAdded = state.discoveredCameras.some(c => c.url === firstUrl);
      
      if (!alreadyAdded) {
        addStreamToLiveView(firstUrl);
        showToast(`دوربین به نمایش زنده اضافه شد`, 'success');
      } else {
        showToast('این دوربین قبلا اضافه شده است', 'info');
      }
      
      // Automatically navigate to live stream tab after a short delay
      setTimeout(() => {
        navigateTo('livestream');
      }, 1500);
    } else {
      showToast('آدرسی یافت نشد', 'warning');
    }
  } catch (error) {
    showToast('خطا در کشف: ' + error.message, 'error');
  } finally {
    btn.disabled = false;
    btn.innerHTML = '<i class="fas fa-search"></i> شروع کشف';
  }
}

// Add stream to Python streaming service
async function addStreamToPythonService(streamId, rtspUrl, enableAI = true) {
  try {
    const response = await fetch(`http://127.0.0.1:8088/add_stream?stream_id=${streamId}&rtsp_url=${encodeURIComponent(rtspUrl)}&enable_ai=${enableAI}`, {
      method: 'POST'
    });
    
    if (!response.ok) {
      // If the endpoint doesn't exist, just log it and continue
      if (response.status === 404) {
        console.log('Python streaming service endpoint not available, continuing without it');
        return;
      }
      throw new Error(`HTTP ${response.status}`);
    }
    
    const result = await response.json();
    if (!result.success) {
      throw new Error(result.message || 'Failed to add stream');
    }
    
    console.log('Stream added to Python service:', streamId);
    return true;
  } catch (error) {
    console.error('Error adding stream to Python service:', error);
    // Don't show error toast for 404 errors since it's expected when streaming service is not available
    if (error.message !== 'HTTP 404') {
      showToast('خطا در اضافه کردن جریان: ' + error.message, 'error');
    }
    return false;
  }
}

// Add stream to live view
function addStreamToLiveView(url) {
  // Generate a unique ID for the stream
  const streamId = 'stream_' + Date.now();
  
  // Extract camera info from URL
  const cameraInfo = {
    id: streamId,
    url: url,
    name: extractCameraName(url),
    addedAt: new Date().toISOString()
  };

  // Add to discovered cameras
  state.discoveredCameras.push(cameraInfo);
  localStorage.setItem('discoveredCameras', JSON.stringify(state.discoveredCameras));

  // Add stream to Python service
  addStreamToPythonService(streamId, url, true).then(success => {
    if (success) {
      showToast('دوربین به نمایش زنده اضافه شد', 'success');
    } else {
      showToast('دوربین اضافه شد اما اتصال به سرویس پردازش با مشکل مواجه شد', 'warning');
    }
  });

  // If we're on the live stream page, update the view
  if (state.currentPage === 'livestream') {
    updateLiveStreamView();
  }
}

// Extract camera name from URL
function extractCameraName(url) {
  try {
    const urlObj = new URL(url);
    return urlObj.hostname;
  } catch (e) {
    return 'دوربین نامشخص';
  }
}

// Update live stream view
function updateLiveStreamView() {
  console.log('Updating live stream view with', state.discoveredCameras.length, 'cameras');
  
  const emptyState = document.getElementById('livestreamEmptyState');
  const streamGrid = document.getElementById('livestreamGrid');

  if (state.discoveredCameras.length === 0) {
    console.log('No cameras found, showing empty state');
    emptyState.style.display = 'block';
    streamGrid.style.display = 'none';
    return;
  }

  console.log('Showing stream grid with cameras');
  emptyState.style.display = 'none';
  streamGrid.style.display = 'grid';
  streamGrid.innerHTML = '';

  state.discoveredCameras.forEach((camera, index) => {
    console.log('Adding camera to stream grid:', camera);
    
    const streamItem = document.createElement('div');
    streamItem.className = 'stream-item';
    streamItem.innerHTML = `
      <div class="stream-header">
        <h4>${camera.name}</h4>
        <button class="btn-small" onclick="removeStream('${camera.id}')">
          <i class="fas fa-times"></i>
        </button>
      </div>
      <div class="stream-video-container">
        <div class="stream-video-wrapper">
          <!-- Canvas element for live stream -->
          <canvas id="stream-${camera.id}" class="live-stream-canvas"></canvas>
          <div id="stream-placeholder-${camera.id}" class="stream-placeholder">
            <i class="fas fa-video"></i>
            <p>در حال بارگذاری جریان زنده...</p>
            <small>${camera.name}</small>
          </div>
        </div>
      </div>
      <div class="stream-controls">
        <button class="btn btn-primary" onclick="startStream('${camera.id}', '${camera.url}')">
          <i class="fas fa-play"></i> شروع جریان
        </button>
        <button class="btn btn-secondary" onclick="stopStream('${camera.id}')">
          <i class="fas fa-stop"></i> توقف جریان
        </button>
      </div>
      <div class="stream-info">
        <span id="stream-status-${camera.id}" class="status-indicator">آماده</span>
        <span>اضافه شده: ${formatTime(camera.addedAt)}</span>
      </div>
    `;
    streamGrid.appendChild(streamItem);
  });
  
  console.log('Finished updating live stream view');
}

// Start streaming for a camera
function startStream(cameraId, rtspUrl) {
  console.log('Starting stream for camera:', cameraId);
  
  try {
    const placeholder = document.getElementById(`stream-placeholder-${cameraId}`);
    const canvasElement = document.getElementById(`stream-${cameraId}`);
    const statusIndicator = document.getElementById(`stream-status-${cameraId}`);
    
    // Check if elements exist
    if (!placeholder || !canvasElement || !statusIndicator) {
      console.error('Could not find stream elements for camera:', cameraId);
      showToast('خطا در شروع جریان زنده', 'error');
      return;
    }
    
    console.log('Found all elements for camera:', cameraId);
    
    // Hide placeholder and show canvas element
    placeholder.style.display = 'none';
    canvasElement.style.display = 'block';
    canvasElement.classList.add('streaming'); // Add streaming class for visual effect
    statusIndicator.textContent = 'در حال پخش';
    statusIndicator.className = 'status-indicator connected';
    
    console.log('Updated UI elements for camera:', cameraId);
    
    // Start live stream using canvas
    startLiveStream(cameraId, rtspUrl);
    
    showToast('شروع جریان زنده...', 'info');
  } catch (error) {
    console.error('Error starting stream:', error);
    showToast('خطا در شروع جریان زنده: ' + error.message, 'error');
  }
}

// Stop streaming for a camera
function stopStream(cameraId) {
  console.log('Stopping stream for camera:', cameraId);
  
  try {
    const placeholder = document.getElementById(`stream-placeholder-${cameraId}`);
    const canvasElement = document.getElementById(`stream-${cameraId}`);
    const statusIndicator = document.getElementById(`stream-status-${cameraId}`);
    
    // Check if elements exist
    if (!placeholder || !canvasElement || !statusIndicator) {
      console.error('Could not find stream elements for camera:', cameraId);
      showToast('خطا در توقف جریان زنده', 'error');
      return;
    }
    
    // Show placeholder and hide canvas element
    placeholder.style.display = 'flex';
    canvasElement.style.display = 'none';
    canvasElement.classList.remove('streaming'); // Remove streaming class
    statusIndicator.textContent = 'متوقف شده';
    statusIndicator.className = 'status-indicator';
    
    // Stop the stream animation frame
    if (state.streamIntervals && state.streamIntervals[cameraId]) {
      if (state.streamIntervals[cameraId].animationFrameId) {
        cancelAnimationFrame(state.streamIntervals[cameraId].animationFrameId);
      }
      delete state.streamIntervals[cameraId];
    }
    
    // Clear any pending timeouts
    if (state.streamIntervals && state.streamIntervals[cameraId + '_timeout']) {
      clearTimeout(state.streamIntervals[cameraId + '_timeout']);
      delete state.streamIntervals[cameraId + '_timeout'];
    }
    
    // Remove resize handler
    if (state.streamCleanup && state.streamCleanup[cameraId]) {
      window.removeEventListener('resize', state.streamCleanup[cameraId].resizeHandler);
      delete state.streamCleanup[cameraId];
    }
    
    // Clear canvas
    const ctx = canvasElement.getContext('2d');
    ctx.clearRect(0, 0, canvasElement.width, canvasElement.height);
    
    // Remove image reference
    delete canvasElement.currentImage;
    
    // Reset stream added flag so it can be restarted
    // This allows the stream to be re-added if restarted
    window.streamAddedFlags = window.streamAddedFlags || {};
    delete window.streamAddedFlags[cameraId];
    
    showToast('توقف جریان زنده', 'info');
  } catch (error) {
    console.error('Error stopping stream:', error);
    showToast('خطا در توقف جریان زنده: ' + error.message, 'error');
  }
}

// Start live stream using canvas for smoother playback
function startLiveStream(cameraId, rtspUrl) {
  console.log('Starting live stream for camera:', cameraId, 'from', rtspUrl);
  
  try {
    const canvasElement = document.getElementById(`stream-${cameraId}`);
    const statusIndicator = document.getElementById(`stream-status-${cameraId}`);
    
    // Check if element exists
    if (!canvasElement) {
      console.error('Could not find canvas element for camera:', cameraId);
      return;
    }
    
    // Get 2D context for drawing
    const ctx = canvasElement.getContext('2d');
    
    // Store resize handler for cleanup
    const resizeHandler = () => {
      // Redraw current frame if available
      if (canvasElement.currentImage) {
        const img = canvasElement.currentImage;
        const container = canvasElement.parentElement;
        const containerWidth = container.clientWidth;
        const containerHeight = container.clientHeight;
        
        // Calculate aspect ratio
        const aspectRatio = img.width / img.height;
        let drawWidth, drawHeight, drawX, drawY;
        
        if (containerWidth / containerHeight > aspectRatio) {
          // Container is wider than image aspect ratio
          drawHeight = containerHeight;
          drawWidth = containerHeight * aspectRatio;
          drawX = (containerWidth - drawWidth) / 2;
          drawY = 0;
        } else {
          // Container is taller than image aspect ratio
          drawWidth = containerWidth;
          drawHeight = containerWidth / aspectRatio;
          drawX = 0;
          drawY = (containerHeight - drawHeight) / 2;
        }
        
        // Set canvas dimensions to match container
        canvasElement.width = containerWidth;
        canvasElement.height = containerHeight;
        
        // Clear canvas
        ctx.clearRect(0, 0, canvasElement.width, canvasElement.height);
        
        // Draw image on canvas with proper aspect ratio
        ctx.drawImage(img, drawX, drawY, drawWidth, drawHeight);
      }
    };
    
    // Add resize listener
    window.addEventListener('resize', resizeHandler);
    
    // Initialize stream intervals object if it doesn't exist
    if (!state.streamIntervals) {
      state.streamIntervals = {};
    }
    
    // Store cleanup function
    if (!state.streamCleanup) {
      state.streamCleanup = {};
    }
    
    // Store resize handler for cleanup
    state.streamCleanup[cameraId] = {
      resizeHandler: resizeHandler
    };
    
    // Clear any existing interval for this camera
    if (state.streamIntervals[cameraId]) {
      if (state.streamIntervals[cameraId].animationFrameId) {
        cancelAnimationFrame(state.streamIntervals[cameraId].animationFrameId);
      }
      delete state.streamIntervals[cameraId];
    }
    
    // Track connection state
    let connectionFailures = 0;
    const maxConnectionFailures = 20; // Increased threshold
    let lastFrameTime = Date.now();
    const frameTimeout = 5000; // 5 seconds timeout
    
    // Add stream to Python service only once per session
    window.streamAddedFlags = window.streamAddedFlags || {};
    let streamAdded = window.streamAddedFlags[cameraId] || false;
    
    // Track pending requests to prevent overlapping
    let pendingRequest = false;
    
    // Fetch actual frame from Python service
    const fetchFrame = async () => {
      // Prevent overlapping requests
      if (pendingRequest) {
        return;
      }
      
      try {
        pendingRequest = true;
        
        // Add the stream to Python service only once
        if (!streamAdded) {
          console.log('Adding stream to Python service:', cameraId);
          const addStreamUrl = `http://127.0.0.1:8088/add_stream?stream_id=${cameraId}&rtsp_url=${encodeURIComponent(rtspUrl)}&enable_ai=true`;
          const addResponse = await fetch(addStreamUrl, { 
            method: 'POST',
            headers: {
              'Content-Type': 'application/json'
            },
            // Don't abort this request
          });
          const addResult = await addResponse.json();
          console.log('Added stream to Python service:', addResult);
          
          // Mark stream as added (even if it already exists)
          streamAdded = true;
          window.streamAddedFlags[cameraId] = true;
        }
        
        // Then fetch frames from the stream endpoint
        const timestamp = Date.now();
        const frameUrl = `http://127.0.0.1:8088/stream/${cameraId}/frame?t=${timestamp}`;
        
        // Fetch the image as blob with longer timeout
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), 5000); // 5 second timeout
        
        const response = await fetch(frameUrl, { 
          signal: controller.signal,
          cache: 'no-cache',
          headers: {
            'Cache-Control': 'no-cache',
            'Pragma': 'no-cache'
          }
        });
        clearTimeout(timeoutId);
        
        if (!response.ok) {
          throw new Error(`HTTP ${response.status}`);
        }
        
        const blob = await response.blob();
        const imageUrl = URL.createObjectURL(blob);
        
        // Create image object to draw on canvas
        const img = new Image();
        img.onload = () => {
          // Update last frame time
          lastFrameTime = Date.now();
          
          // Update status indicator to show active
          if (statusIndicator) {
            statusIndicator.textContent = 'در حال پخش';
            statusIndicator.className = 'status-indicator connected';
          }
          
          // Store image reference for resize handling
          canvasElement.currentImage = img;
          
          // Set canvas dimensions to match container while maintaining aspect ratio
          const container = canvasElement.parentElement;
          const containerWidth = container.clientWidth;
          const containerHeight = container.clientHeight;
          
          // Calculate aspect ratio
          const aspectRatio = img.width / img.height;
          let drawWidth, drawHeight, drawX, drawY;
          
          if (containerWidth / containerHeight > aspectRatio) {
            // Container is wider than image aspect ratio
            drawHeight = containerHeight;
            drawWidth = containerHeight * aspectRatio;
            drawX = (containerWidth - drawWidth) / 2;
            drawY = 0;
          } else {
            // Container is taller than image aspect ratio
            drawWidth = containerWidth;
            drawHeight = containerWidth / aspectRatio;
            drawX = 0;
            drawY = (containerHeight - drawHeight) / 2;
          }
          
          // Set canvas dimensions to match container
          canvasElement.width = containerWidth;
          canvasElement.height = containerHeight;
          
          // Clear canvas
          ctx.clearRect(0, 0, canvasElement.width, canvasElement.height);
          
          // Draw image on canvas with proper aspect ratio
          ctx.drawImage(img, drawX, drawY, drawWidth, drawHeight);
          
          // Clean up object URL
          URL.revokeObjectURL(imageUrl);
          
          // Reset connection failures on successful load
          connectionFailures = 0;
          console.log('Frame loaded and drawn successfully');
          
          // Release pending request flag
          pendingRequest = false;
        };
        
        img.onerror = (event) => {
          console.error('Failed to load frame image:', event);
          URL.revokeObjectURL(imageUrl);
          connectionFailures++;
          
          // Check if we haven't received a frame in a while
          const timeSinceLastFrame = Date.now() - lastFrameTime;
          if (timeSinceLastFrame > frameTimeout) {
            console.warn(`No frames received for ${timeSinceLastFrame}ms, treating as connection failure`);
            connectionFailures += 2; // Treat as multiple failures
            // Update status indicator to show problem
            if (statusIndicator) {
              statusIndicator.textContent = 'مشکل در اتصال';
              statusIndicator.className = 'status-indicator warning';
            }
          }
          
          // Release pending request flag
          pendingRequest = false;
          
          // If we have too many connection failures, stop the stream
          if (connectionFailures >= maxConnectionFailures) {
            console.error('Too many connection failures, stopping stream');
            // Update status indicator to show error
            if (statusIndicator) {
              statusIndicator.textContent = 'قطع شده';
              statusIndicator.className = 'status-indicator';
            }
            stopStream(cameraId);
            showToast('اتصال به دوربین قطع شده است. لطفاً اتصال را بررسی کنید.', 'error');
            return;
          }
        };
        
        img.src = imageUrl;
        
        console.log('Fetching frame from:', frameUrl);
      } catch (error) {
        // Release pending request flag
        pendingRequest = false;
        
        // Don't count abort errors as connection failures
        if (error.name !== 'AbortError') {
          console.error('Error fetching stream frame:', error);
          connectionFailures++;
        } else {
          console.warn('Request aborted, not counting as failure');
          return; // Don't process abort errors further
        }
        
        // Check if we haven't received a frame in a while
        const timeSinceLastFrame = Date.now() - lastFrameTime;
        if (timeSinceLastFrame > frameTimeout) {
          console.warn(`No frames received for ${timeSinceLastFrame}ms, treating as connection failure`);
          connectionFailures += 2; // Treat as multiple failures
          // Update status indicator to show problem
          if (statusIndicator) {
            statusIndicator.textContent = 'مشکل در اتصال';
            statusIndicator.className = 'status-indicator warning';
          }
        }
        
        // If we have too many connection failures, stop the stream
        if (connectionFailures >= maxConnectionFailures) {
          console.error('Too many connection failures, stopping stream');
          // Update status indicator to show error
          if (statusIndicator) {
            statusIndicator.textContent = 'قطع شده';
            statusIndicator.className = 'status-indicator';
          }
          stopStream(cameraId);
          showToast('اتصال به دوربین قطع شده است. لطفاً اتصال را بررسی کنید.', 'error');
          return;
        }
      }
    };
    
    // Use requestAnimationFrame for optimal rendering with rate limiting
    let lastFetch = 0;
    const minInterval = 60; // Minimum 60ms between fetches (~16 FPS max)
    
    const updateStream = () => {
      const now = Date.now();
      if (now - lastFetch >= minInterval) {
        fetchFrame();
        lastFetch = now;
      }
      if (state.streamIntervals[cameraId]) {
        state.streamIntervals[cameraId].animationFrameId = requestAnimationFrame(updateStream);
      }
    };
    
    // Start the streaming loop
    state.streamIntervals[cameraId] = { animationFrameId: requestAnimationFrame(updateStream) };
    
    console.log('Started live stream for camera:', cameraId);
  } catch (error) {
    console.error('Error starting live stream:', error);
  }
}

// ============================================================================
// CONNECTION STATUS
// ============================================================================

function checkConnection() {
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

function updateConnectionStatus() {
  checkConnection();
}

// ============================================================================
// THEME
// ============================================================================

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

function applyTheme(theme) {
  if (theme === 'light') {
    document.documentElement.setAttribute('data-theme', 'light');
  } else {
    document.documentElement.removeAttribute('data-theme');
  }
}

// ============================================================================
// MODAL
// ============================================================================

function showResultDetails(result) {
  const modal = document.getElementById('detailsModal');
  const body = document.getElementById('modalBody');

  const confidence = (result.confidence * 100).toFixed(2);

  body.innerHTML = `
    <div style="display: grid; gap: 1rem;">
      <div>
        <h4 style="color: var(--text-secondary); margin-bottom: 0.5rem;">پلاک</h4>
        <p style="font-size: 1.5rem; font-weight: 700; color: var(--primary); font-family: 'Courier New', monospace;">
          ${result.plate_text || 'نامشخص'}
        </p>
      </div>
      <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 1rem;">
        <div>
          <h4 style="color: var(--text-secondary); margin-bottom: 0.5rem;">دوربین</h4>
          <p>${result.camera_id || 'نامشخص'}</p>
        </div>
        <div>
          <h4 style="color: var(--text-secondary); margin-bottom: 0.5rem;">زمان</h4>
          <p>${formatDateTime(result.timestamp)}</p>
        </div>
      </div>
      <div>
        <h4 style="color: var(--text-secondary); margin-bottom: 0.5rem;">اطمینان</h4>
        <div style="display: flex; align-items: center; gap: 1rem;">
          <div style="flex: 1; height: 8px; background: var(--bg-hover); border-radius: 4px; overflow: hidden;">
            <div style="height: 100%; width: ${confidence}%; background: linear-gradient(90deg, var(--primary), var(--secondary));"></div>
          </div>
          <span style="font-weight: 700;">${confidence}%</span>
        </div>
      </div>
      <div>
        <h4 style="color: var(--text-secondary); margin-bottom: 0.5rem;"> موقعیت (Bounding Box)</h4>
        <p style="font-family: 'Courier New', monospace; font-size: 0.875rem;">
          X: ${(result.bbox?.x || 0).toFixed(2)}<br>
          Y: ${(result.bbox?.y || 0).toFixed(2)}<br>
          Width: ${(result.bbox?.w || 0).toFixed(2)}<br>
          Height: ${(result.bbox?.h || 0).toFixed(2)}
        </p>
      </div>
    </div>
  `;

  modal.classList.add('active');
}

function closeModal() {
  document.getElementById('detailsModal').classList.remove('active');
}

// ============================================================================
// NOTIFICATIONS
// ============================================================================

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

// ============================================================================
// EXPORT
// ============================================================================

function exportResults() {
  const csv = convertToCSV(state.results);
  downloadCSV(csv, 'license_plate_results.csv');
  showToast('دانلود شروع شد', 'success');
}

function convertToCSV(data) {
  const headers = ['زمان', 'دوربین', 'پلاک', 'اطمینان', 'X', 'Y', 'Width', 'Height'];
  const rows = data.map(r => [
    formatDateTime(r.timestamp),
    r.camera_id || '',
    r.plate_text || '',
    (r.confidence * 100).toFixed(2),
    (r.bbox?.x || 0).toFixed(2),
    (r.bbox?.y || 0).toFixed(2),
    (r.bbox?.w || 0).toFixed(2),
    (r.bbox?.h || 0).toFixed(2),
  ]);

  let csv = headers.join(',') + '\n';
  rows.forEach(row => {
    csv += row.map(cell => `"${cell}"`).join(',') + '\n';
  });

  return csv;
}

function downloadCSV(csv, filename) {
  const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
  const link = document.createElement('a');
  const url = URL.createObjectURL(blob);

  link.setAttribute('href', url);
  link.setAttribute('download', filename);
  link.style.visibility = 'hidden';

  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
}

// ============================================================================
// PAGINATION
// ============================================================================

let currentPage = 1;
const itemsPerPage = 20;

function previousPage() {
  if (currentPage > 1) {
    currentPage--;
    loadResultsTable();
    updatePaginationInfo();
  }
}

function nextPage() {
  const maxPage = Math.ceil(state.results.length / itemsPerPage);
  if (currentPage < maxPage) {
    currentPage++;
    loadResultsTable();
    updatePaginationInfo();
  }
}

function updatePaginationInfo() {
  const maxPage = Math.ceil(state.results.length / itemsPerPage);
  document.getElementById('paginationInfo').textContent = `صفحه ${currentPage} از ${maxPage}`;
  
  document.getElementById('prevBtn').disabled = currentPage === 1;
  document.getElementById('nextBtn').disabled = currentPage === maxPage;
}

// ============================================================================
// AUTO REFRESH
// ============================================================================

function startAutoRefresh() {
  state.autoRefreshTimer = setInterval(() => {
    loadResults();
  }, state.refreshInterval);
}

// ============================================================================
// UTILITIES
// ============================================================================

// Remove stream from live view
function removeStream(cameraId) {
  console.log('Removing stream for camera:', cameraId);
  
  try {
    // Remove from discovered cameras
    state.discoveredCameras = state.discoveredCameras.filter(c => c.id !== cameraId);
    localStorage.setItem('discoveredCameras', JSON.stringify(state.discoveredCameras));
    
    // Stop any running streams
    if (state.streamIntervals && state.streamIntervals[cameraId]) {
      clearInterval(state.streamIntervals[cameraId]);
      delete state.streamIntervals[cameraId];
    }
    
    // Update the view
    if (state.currentPage === 'livestream') {
      updateLiveStreamView();
    }
    
    showToast('دوربین حذف شد', 'success');
  } catch (error) {
    console.error('Error removing stream:', error);
    showToast('خطا در حذف دوربین', 'error');
  }
}

// Copy to clipboard utility
function copyToClipboard(text) {
  navigator.clipboard.writeText(text).then(() => {
    showToast('کپی شد', 'success');
  }).catch(() => {
    showToast('خطا در کپی', 'error');
  });
}

function formatTime(timestamp) {
  if (!timestamp) return 'نامشخص';
  const date = new Date(timestamp);
  return date.toLocaleTimeString('fa-IR');
}

function formatDateTime(timestamp) {
  if (!timestamp) return 'نامشخص';
  const date = new Date(timestamp);
  return date.toLocaleString('fa-IR');
}
