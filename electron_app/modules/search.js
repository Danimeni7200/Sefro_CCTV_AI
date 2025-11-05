/**
 * Search and Filter Module
 */

import state from './state.js';
import { formatTime } from './ui.js';
import { showResultDetails } from './dashboard.js';
import { updateRecentResults } from './dashboard.js';
import { loadResultsTable } from './results.js';

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

export { filterResults };