/**
 * Livestream Module
 */

import state from './state.js';
import { showToast, formatTime } from './ui.js';

function updateLiveStreamView() {
  console.log('Updating live stream view with', state.discoveredCameras.length, 'cameras');
  
  const emptyState = document.getElementById('livestreamEmptyState');
  const streamGrid = document.getElementById('livestreamGrid');

  if (state.discoveredCameras.length === 0) {
    console.log('No cameras found, showing empty state');
    emptyState.style.display = 'flex';
    streamGrid.style.display = 'none';
    return;
  }

  console.log('Showing stream grid with cameras');
  emptyState.style.display = 'none';
  streamGrid.style.display = 'grid';
  streamGrid.innerHTML = '';
  
  // Stream controls container (without toggle mode button)
  const streamButtonsContainer = document.createElement('div');
  streamButtonsContainer.className = 'stream-buttons-container';
  streamGrid.appendChild(streamButtonsContainer);

  state.discoveredCameras.forEach((camera, index) => {
    console.log('Adding camera to stream grid:', camera);
    
    const streamItem = document.createElement('div');
    streamItem.className = 'stream-item';
    streamItem.id = `stream-item-${camera.id}`;
    streamItem.innerHTML = `
      <div class="stream-header">
        <h4><i class="fas fa-video"></i> <span title="${camera.name}">${camera.name}</span></h4>
        <div class="stream-header-actions">
          <button class="btn-small" onclick="toggleFullscreenStream('${camera.id}')" title="تمام صفحه">
            <i class="fas fa-expand"></i>
          </button>
          <button class="btn-small" onclick="removeStream('${camera.id}')" title="حذف دوربین">
            <i class="fas fa-times"></i>
          </button>
        </div>
      </div>
      <div class="stream-container">
        <img id="video-${camera.id}" class="live-stream-video">
        <!-- Canvas element for live stream (fallback) -->
        <canvas id="stream-${camera.id}" class="live-stream-canvas"></canvas>
        <div id="stream-placeholder-${camera.id}" class="stream-placeholder">
          <i class="fas fa-video"></i>
          <p>در انتظار شروع جریان زنده...</p>
          <small>روی دکمه "شروع جریان" کلیک کنید</small>
        </div>
      </div>
      <div class="stream-controls">
        <div class="control-group">
          <button class="btn btn-primary" onclick="startStream('${camera.id}', '${camera.url}')" title="شروع نمایش زنده">
            <i class="fas fa-play"></i> شروع جریان
          </button>
          <button class="btn btn-secondary" onclick="stopStream('${camera.id}')" title="توقف نمایش زنده">
            <i class="fas fa-stop"></i> توقف جریان
          </button>
        </div>
        <div class="control-group">
          <button class="btn btn-secondary" onclick="toggleFullscreenStream('${camera.id}')" title="تمام صفحه">
            <i class="fas fa-expand"></i> تمام صفحه
          </button>
          <button class="btn btn-secondary" onclick="captureSnapshot('${camera.id}')" title="ثبت عکس">
            <i class="fas fa-camera"></i> ثبت عکس
          </button>
          <button class="btn btn-secondary" onclick="changeStreamQuality('${camera.id}')" title="کیفیت">
            <i class="fas fa-sliders-h"></i> کیفیت
          </button>
          <button class="btn btn-secondary" onclick="showStreamInfo('${camera.id}')" title="اطلاعات جریان">
            <i class="fas fa-info-circle"></i> اطلاعات
          </button>
        </div>
      </div>
      <div class="stream-info">
        <span id="stream-status-${camera.id}" class="status-indicator">آماده</span>
        <span><i class="far fa-clock"></i> اضافه شده: ${formatTime(camera.addedAt)}</span>
      </div>
    `;
    streamGrid.appendChild(streamItem);
  });
  
  console.log('Finished updating live stream view');
}

function extractCameraName(url) {
  try {
    const urlObj = new URL(url);
    return urlObj.hostname;
  } catch (e) {
    return 'دوربین نامشخص';
  }
}

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

async function addStreamToPythonService(streamId, rtspUrl, enableAI = true) {
  try {
    const response = await fetch(`http://127.0.0.1:8089/add_stream?stream_id=${streamId}&rtsp_url=${encodeURIComponent(rtspUrl)}&enable_ai=${enableAI}`, {
      method: 'POST'
    });
    
    if (!response.ok) {
      // If the endpoint doesn't exist, just log it and continue
      if (response.status === 404) {
        console.log('Python streaming service endpoint not available, continuing without it');
        return true; // Return true to indicate success even if endpoint is not available
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
    if (error.message && !error.message.includes('404')) {
      showToast('خطا در اتصال به سرویس پردازش: ' + error.message, 'error');
    }
    return true; // Return true to continue even if there's an error
  }
}

async function removeStream(cameraId) {
  console.log('Removing stream for camera:', cameraId);
  
  try {
    // First, exit fullscreen if the stream is in fullscreen mode
    const streamContainer = document.getElementById(`video-${cameraId}`)?.closest('.stream-item') || 
                            document.getElementById(`stream-${cameraId}`)?.closest('.stream-item');
    
    if (streamContainer) {
      const isFullscreen = !!(document.fullscreenElement === streamContainer || 
                              document.webkitFullscreenElement === streamContainer || 
                              document.mozFullScreenElement === streamContainer || 
                              document.msFullscreenElement === streamContainer);
      
      if (isFullscreen) {
        console.log('Exiting fullscreen before removing stream');
        try {
          if (document.exitFullscreen) {
            await document.exitFullscreen();
          } else if (document.webkitExitFullscreen) {
            await document.webkitExitFullscreen();
          } else if (document.mozCancelFullScreen) {
            await document.mozCancelFullScreen();
          } else if (document.msExitFullscreen) {
            await document.msExitFullscreen();
          }
          // Wait a bit for fullscreen to fully exit
          await new Promise(resolve => setTimeout(resolve, 100));
        } catch (fsError) {
          console.warn('Error exiting fullscreen:', fsError);
        }
      }
    }
    
    // Stop the stream first
    if (typeof window.stopStream === 'function') {
      window.stopStream(cameraId);
    }
    
    // Remove from discovered cameras
    state.discoveredCameras = state.discoveredCameras.filter(c => c.id !== cameraId);
    localStorage.setItem('discoveredCameras', JSON.stringify(state.discoveredCameras));
    
    // Stop any running streams
    if (state.streamIntervals && state.streamIntervals[cameraId]) {
      clearInterval(state.streamIntervals[cameraId]);
      delete state.streamIntervals[cameraId];
    }
    
    // Wait a bit for cleanup
    await new Promise(resolve => setTimeout(resolve, 50));
    
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

// Make sure functions are available globally
window.updateLiveStreamView = updateLiveStreamView;
window.extractCameraName = extractCameraName;
window.addStreamToLiveView = addStreamToLiveView;
window.addStreamToPythonService = addStreamToPythonService;
window.removeStream = removeStream;

export { updateLiveStreamView, extractCameraName, addStreamToLiveView, addStreamToPythonService, removeStream };