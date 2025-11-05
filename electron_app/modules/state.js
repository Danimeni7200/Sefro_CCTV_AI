/**
 * State Management Module
 */

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
  autoRefreshTimer: null,
  streamIntervals: {},
  streamCleanup: {}
};

export default state;