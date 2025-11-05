/**
 * Modern Electron App - License Plate Recognition System
 * Refactored version with modular architecture
 */

// Import all modules using ES6 import syntax
import state from './modules/state.js';
import { initializeApp, setupEventListeners, applyTheme, setTheme } from './modules/main.js';
import { navigateTo } from './modules/navigation.js';
import { loadResults } from './modules/data.js';
import { showToast, updateConnectionStatus, updateCameraFilter, formatTime, formatDateTime } from './modules/ui.js';
import { updateDashboard, updateRecentResults, drawDetectionChart, drawCameraChart, showResultDetails, closeModal } from './modules/dashboard.js';
import { testConnection, saveSettings, resetSettings, startAutoRefresh } from './modules/settings.js';
import { updateLiveStreamView, extractCameraName, addStreamToLiveView, addStreamToPythonService, removeStream } from './modules/livestream.js';
import { performDiscover } from './modules/discover.js';
import { loadResultsTable, filterResultsTable, filterByCamera, filterByDate } from './modules/results.js';
import { filterResults } from './modules/search.js';
import { exportResults } from './modules/export.js';
import { previousPage, nextPage } from './modules/pagination.js';
import { startStream, stopStream, toggleFullscreenStream, captureSnapshot, showStreamInfo, changeStreamQuality } from './modules/streaming.js';

// Make functions globally available
window.navigateTo = navigateTo;
window.loadResults = loadResults;
window.showToast = showToast;
window.updateConnectionStatus = updateConnectionStatus;
window.updateCameraFilter = updateCameraFilter;
window.formatTime = formatTime;
window.formatDateTime = formatDateTime;
window.updateDashboard = updateDashboard;
window.updateRecentResults = updateRecentResults;
window.drawDetectionChart = drawDetectionChart;
window.drawCameraChart = drawCameraChart;
window.showResultDetails = showResultDetails;
window.closeModal = closeModal;
window.testConnection = testConnection;
window.saveSettings = saveSettings;
window.resetSettings = resetSettings;
window.startAutoRefresh = startAutoRefresh;
window.updateLiveStreamView = updateLiveStreamView;
window.extractCameraName = extractCameraName;
window.addStreamToLiveView = addStreamToLiveView;
window.addStreamToPythonService = addStreamToPythonService;
window.removeStream = removeStream;
window.performDiscover = performDiscover;
window.startStream = startStream;
window.stopStream = stopStream;
window.toggleFullscreenStream = toggleFullscreenStream;
window.captureSnapshot = captureSnapshot;
window.showStreamInfo = showStreamInfo;
window.changeStreamQuality = changeStreamQuality;
window.loadResultsTable = loadResultsTable;
window.filterResultsTable = filterResultsTable;
window.filterByCamera = filterByCamera;
window.filterByDate = filterByDate;
window.filterResults = filterResults;
window.exportResults = exportResults;
window.previousPage = previousPage;
window.nextPage = nextPage;
window.initializeApp = initializeApp;
window.setupEventListeners = setupEventListeners;
window.applyTheme = applyTheme;
window.setTheme = setTheme;

// Initialize the application when the DOM is loaded
document.addEventListener('DOMContentLoaded', function() {
  // Initialize the application directly
  initializeApp();
  setupEventListeners();
  applyTheme(state.theme);
  startAutoRefresh();
});

// Export for testing if needed
export {
  state,
  navigateTo,
  loadResults,
  showToast,
  updateConnectionStatus,
  updateCameraFilter,
  formatTime,
  formatDateTime,
  updateDashboard,
  updateRecentResults,
  drawDetectionChart,
  drawCameraChart,
  showResultDetails,
  closeModal,
  testConnection,
  saveSettings,
  resetSettings,
  startAutoRefresh,
  updateLiveStreamView,
  extractCameraName,
  addStreamToLiveView,
  addStreamToPythonService,
  removeStream,
  performDiscover,
  startStream,
  stopStream,
  toggleFullscreenStream,
  captureSnapshot,
  showStreamInfo,
  changeStreamQuality,
  loadResultsTable,
  filterResultsTable,
  filterByCamera,
  filterByDate,
  filterResults,
  exportResults,
  previousPage,
  nextPage,
  initializeApp,
  setupEventListeners,
  applyTheme,
  setTheme
};