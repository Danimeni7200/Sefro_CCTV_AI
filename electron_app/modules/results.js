/**
 * Results Module
 */

import state from './state.js';
import { formatDateTime, formatTime } from './ui.js';
import { showResultDetails } from './dashboard.js';

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

export { loadResultsTable, filterResultsTable, filterByCamera, filterByDate };