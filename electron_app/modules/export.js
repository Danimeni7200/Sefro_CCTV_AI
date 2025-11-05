/**
 * Export Module
 */

import state from './state.js';
import { formatDateTime, showToast } from './ui.js';

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

function exportResults() {
  const csv = convertToCSV(state.results);
  downloadCSV(csv, 'license_plate_results.csv');
  showToast('دانلود شروع شد', 'success');
}

export { convertToCSV, downloadCSV, exportResults };