/**
 * Navigation Module
 */

import state from './state.js';
import { loadResultsTable } from './results.js';
import { updateDashboard } from './dashboard.js';
import { updateLiveStreamView } from './livestream.js';

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

export { navigateTo };