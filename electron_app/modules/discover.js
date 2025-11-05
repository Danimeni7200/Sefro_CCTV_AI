/**
 * Discover Module
 */

import state from './state.js';
import { showToast } from './ui.js';
import { navigateTo } from './navigation.js';
import { addStreamToLiveView } from './livestream.js';

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
    console.log(`Attempting to discover cameras: ${ip}, ${user}, ${brand}`);
    const result = await window.lpr.discover(ip, user, pass, brand);
    console.log('Discovery result:', result);
    
    // Check if we received a valid response
    if (!result) {
      showToast('خطا در دریافت پاسخ از سرور', 'error');
      return;
    }
    
    // Check for explicit error in response
    if (result.error) {
      showToast(`خطا در کشف: ${result.error}`, 'error');
      return;
    }
    
    // Check if IP is reachable (if this information is provided)
    if (result.ip_reachable === false) {
      showToast('آدرس IP دوربین در دسترس نیست. لطفاً اتصال شبکه را بررسی کنید', 'error');
      return;
    }
    
    if (result.candidates && result.candidates.length > 0) {
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
      showToast('آدرسی یافت نشد. لطفاً اطلاعات دوربین را بررسی کنید', 'warning');
    }
  } catch (error) {
    console.error('Discovery error:', error);
    showToast('خطا در کشف: ' + error.message, 'error');
  } finally {
    btn.disabled = false;
    btn.innerHTML = '<i class="fas fa-search"></i> شروع کشف';
  }
}

export { performDiscover };