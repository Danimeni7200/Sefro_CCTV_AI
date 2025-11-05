/**
 * Dashboard Module
 */

import state from './state.js';
import { formatTime, formatDateTime } from './ui.js';

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

export { updateDashboard, updateRecentResults, drawDetectionChart, drawCameraChart, showResultDetails, closeModal };