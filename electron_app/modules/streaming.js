/**
 * Streaming Module - Handles video stream control
 * Enhanced with WebSocket support for lower latency
 */

import state from './state.js';
import { showToast } from './ui.js';
import { addStreamToPythonService } from './livestream.js';
import { updateLiveStreamView } from './livestream.js';

// Maximum number of simultaneous active streams
const MAX_ACTIVE_STREAMS = 4;

// Stream management
const activeStreams = new Set();
const streamLastActivity = new Map();
const streamRefreshTimers = new Map();
const activeWebSockets = new Map(); // Track active WebSocket connections

// Quality levels for streaming
const QUALITY_LEVELS = [30, 50, 70, 90];
let currentQuality = 50;

// Stream mode - WebSocket preferred for lower latency
const STREAM_MODES = {
  WEBSOCKET: 'websocket',
  MJPEG: 'mjpeg',
  POLLING: 'polling'
};
let preferredMode = STREAM_MODES.WEBSOCKET; // Default to WebSocket

// Start streaming for a camera
async function startStream(cameraId, rtspUrl) {
  console.log('Starting stream for camera:', cameraId);
  
  // Make sure DOM is ready
  if (document.readyState !== 'complete') {
    console.log('DOM not ready, waiting...');
    await new Promise(resolve => setTimeout(resolve, 100));
  }
  
  const placeholder = document.getElementById(`stream-placeholder-${cameraId}`);
  const canvasElement = document.getElementById(`stream-${cameraId}`);
  const statusIndicator = document.getElementById(`stream-status-${cameraId}`);
  const videoElement = document.getElementById(`video-${cameraId}`);
  
  if (!placeholder || !canvasElement || !statusIndicator) {
    console.error('Could not find stream elements for camera:', cameraId);
    showToast('خطا در شروع جریان زنده', 'error');
    return;
  }
  
  // Check if we've reached the maximum active streams
  if (activeStreams.size >= MAX_ACTIVE_STREAMS && !activeStreams.has(cameraId)) {
    showToast(`حداکثر ${MAX_ACTIVE_STREAMS} جریان همزمان مجاز است. لطفاً برخی از جریان‌ها را متوقف کنید.`, 'warning');
    return;
  }
  
  // Add to active streams
  activeStreams.add(cameraId);
  
  // Hide placeholder and show video element
  placeholder.style.display = 'none';
  if (canvasElement) canvasElement.style.display = 'block';
  if (videoElement) videoElement.style.display = 'block';
  
  statusIndicator.textContent = 'در حال اتصال...';
  statusIndicator.className = 'status-indicator';
  
  // Update last activity time
  streamLastActivity.set(cameraId, Date.now());
  
  // First, add the stream to Python service (or check if it already exists)
  const addStreamUrl = `http://127.0.0.1:8091/add_stream?stream_id=${cameraId}&rtsp_url=${encodeURIComponent(rtspUrl)}&enable_ai=true`;
  let streamReady = false;
  
  try {
    console.log('Attempting to add stream to Python service:', addStreamUrl);
    const addResponse = await fetch(addStreamUrl, { 
      method: 'POST',
      timeout: 10000 // 10 second timeout
    });
    const addResult = await addResponse.json();
    
    if (addResult.success) {
      // Stream was successfully added
      console.log('Added stream to Python service:', addResult);
      streamReady = true;
    } else if (addResult.message && addResult.message.includes('already exists')) {
      // Stream already exists - this is fine, we can proceed
      console.log('Stream already exists, proceeding with playback:', cameraId);
      streamReady = true;
    } else {
      // Actual error
      throw new Error(addResult.message || `HTTP ${addResponse.status}`);
    }
  } catch (error) {
    console.error('Error adding stream to Python service:', error);
    // Only show error if it's a real problem (not "already exists")
    if (!error.message.includes('already exists')) {
      showToast('خطا در اضافه کردن جریان: ' + error.message, 'error');
      // Show error placeholder if add stream fails
      if (placeholder) {
        placeholder.innerHTML = `
          <i class="fas fa-exclamation-triangle"></i>
          <p>خطا در شروع جریان</p>
          <small>${error.message || 'اتصال به سرویس پردازش امکان‌پذیر نیست'}</small>
        `;
        placeholder.style.display = 'flex';
      }
      if (canvasElement) canvasElement.style.display = 'none';
      if (videoElement) videoElement.style.display = 'none';
      activeStreams.delete(cameraId);
      statusIndicator.textContent = 'خطا در اتصال';
      statusIndicator.className = 'status-indicator warning';
      return; // Stop here if add stream failed
    } else {
      // "Already exists" - treat as success
      streamReady = true;
    }
  }
  
  // Only proceed if stream is ready (either newly added or already exists)
  if (!streamReady) {
    activeStreams.delete(cameraId);
    return;
  }
  
  // Wait briefly for stream to initialize, then start MJPEG directly
  statusIndicator.textContent = 'در حال آماده‌سازی جریان...';
  statusIndicator.className = 'status-indicator';
  
  // Simple check: wait a moment for stream to open, then proceed
  let streamInitialized = false;
  let attempts = 0;
  const maxAttempts = 15; // Increased attempts
  
  // Check stream info endpoint (lighter than fetching frames)
  while (!streamInitialized && attempts < maxAttempts) {
    try {
      const infoUrl = `http://127.0.0.1:8091/stream/${cameraId}/info?t=${Date.now()}`;
      console.log('Checking stream info:', infoUrl);
      
      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 5000); // 5 second timeout
      
      const infoResponse = await fetch(infoUrl, {
        cache: 'no-cache',
        signal: controller.signal
      });
      
      clearTimeout(timeoutId);
      
      if (infoResponse.ok) {
        const info = await infoResponse.json();
        console.log('Stream info response:', info);
        if (info.is_opened) {
          streamInitialized = true;
          console.log(`Stream initialized (attempt ${attempts + 1})`);
          break;
        } else {
          console.log(`Stream not yet opened (attempt ${attempts + 1})`);
        }
      } else {
        console.log(`Stream info endpoint returned ${infoResponse.status} (attempt ${attempts + 1})`);
      }
    } catch (error) {
      console.log(`Error checking stream info (attempt ${attempts + 1}):`, error.message);
    }
    
    attempts++;
    // Wait 1 second between attempts
    await new Promise(resolve => setTimeout(resolve, 1000));
  }
  
  if (streamInitialized) {
    console.log('Stream confirmed ready, starting playback');
  } else {
    console.log('Stream initialization check complete, attempting MJPEG stream anyway');
    // Continue anyway - MJPEG might work even if info check failed
  }
  
  // Proceed with streaming based on preferred mode
  console.log(`Starting stream with mode: ${preferredMode}`);
  
  // Set up refresh timer to reconnect if stream is idle
  setupStreamRefreshTimer(cameraId);
  
  if (preferredMode === STREAM_MODES.WEBSOCKET) {
    // Try WebSocket first for lowest latency
    const wsUrl = `ws://127.0.0.1:8091/ws/stream/${cameraId}`;
    console.log('Using WebSocket URL:', wsUrl);
    
    try {
      setupWebSocketStream(cameraId, wsUrl, videoElement, canvasElement, statusIndicator);
    } catch (error) {
      console.error('WebSocket stream failed, falling back to MJPEG:', error);
      fallbackToMjpeg(cameraId, videoElement, canvasElement, placeholder, statusIndicator);
    }
  } else {
    // Use MJPEG as requested or as fallback
    fallbackToMjpeg(cameraId, videoElement, canvasElement, placeholder, statusIndicator);
  }
}

// Fallback to MJPEG streaming if WebSocket fails
function fallbackToMjpeg(cameraId, videoElement, canvasElement, placeholder, statusIndicator) {
  const mjpegUrl = `http://127.0.0.1:8091/stream/${cameraId}/mjpeg`;
  console.log('Using MJPEG URL:', mjpegUrl);
  
  // Try MJPEG stream
  if (videoElement) {
    setupMjpegStream(cameraId, mjpegUrl, videoElement, canvasElement, placeholder, statusIndicator);
  } else if (canvasElement) {
    // Fallback: Poll for frames if video element not available
    startFramePolling(cameraId, canvasElement, statusIndicator);
  }
}

// Set up WebSocket stream
function setupWebSocketStream(cameraId, wsUrl, videoElement, canvasElement, statusIndicator) {
  console.log(`Setting up WebSocket stream for ${cameraId} at ${wsUrl}`);
  
  // Close any existing WebSocket
  if (activeWebSockets.has(cameraId)) {
    const existingWs = activeWebSockets.get(cameraId);
    if (existingWs && existingWs.readyState !== WebSocket.CLOSED) {
      existingWs.close();
    }
  }
  
  try {
    // Create new WebSocket connection
    const ws = new WebSocket(wsUrl);
    activeWebSockets.set(cameraId, ws);
    
    // Set up canvas for rendering frames
    if (!canvasElement) {
      console.error('Canvas element not found for WebSocket stream');
      return false;
    }
    
    const ctx = canvasElement.getContext('2d');
    if (!ctx) {
      console.error('Could not get canvas context');
      return false;
    }
    
    // Show canvas, hide video element
    canvasElement.style.display = 'block';
    if (videoElement) videoElement.style.display = 'none';
    
    // WebSocket event handlers
    ws.onopen = () => {
      console.log(`WebSocket connection opened for ${cameraId}`);
      statusIndicator.textContent = 'در حال پخش (WebSocket)';
      statusIndicator.className = 'status-indicator connected';
      streamLastActivity.set(cameraId, Date.now());
      showToast('جریان زنده شروع شد (WebSocket)', 'success');
    };
    
    ws.onmessage = (event) => {
      // Update activity timestamp
      streamLastActivity.set(cameraId, Date.now());
      
      // Handle different message types
      if (event.data instanceof Blob) {
        // Binary frame data
        const blob = event.data;
        const url = URL.createObjectURL(blob);
        const img = new Image();
        
        img.onload = () => {
          // Ensure canvas dimensions match image
          if (canvasElement.width !== img.width || canvasElement.height !== img.height) {
            canvasElement.width = img.width;
            canvasElement.height = img.height;
          }
          
          // Draw image to canvas
          ctx.drawImage(img, 0, 0);
          URL.revokeObjectURL(url);
        };
        
        img.src = url;
      } else {
        // JSON message (info, heartbeat, etc.)
        try {
          const data = JSON.parse(event.data);
          if (data.type === 'info') {
            console.log(`Stream info for ${cameraId}:`, data.data);
          } else if (data.type === 'heartbeat') {
            // Just a keepalive, no action needed
          } else if (data.type === 'error') {
            console.error(`WebSocket error for ${cameraId}:`, data.message);
          }
        } catch (e) {
          console.warn('Non-JSON message received:', event.data);
        }
      }
    };
    
    ws.onerror = (error) => {
      console.error(`WebSocket error for ${cameraId}:`, error);
      statusIndicator.textContent = 'خطا در اتصال WebSocket';
      statusIndicator.className = 'status-indicator warning';
      
      // Fallback to MJPEG
      const mjpegUrl = `http://127.0.0.1:8091/stream/${cameraId}/mjpeg`;
      if (videoElement) {
        console.log('Falling back to MJPEG stream');
        setupMjpegStream(cameraId, mjpegUrl, videoElement, canvasElement, null, statusIndicator);
      }
    };
    
    ws.onclose = (event) => {
      console.log(`WebSocket connection closed for ${cameraId}:`, event.code, event.reason);
      
      // Remove from active WebSockets
      activeWebSockets.delete(cameraId);
      
      // Only attempt reconnect if stream is still active
      if (activeStreams.has(cameraId)) {
        console.log(`Attempting to reconnect WebSocket for ${cameraId}`);
        setTimeout(() => {
          // Try WebSocket first if preferred
          if (preferredMode === STREAM_MODES.WEBSOCKET) {
            const wsUrl = `ws://127.0.0.1:8091/ws/stream/${cameraId}`;
            console.log(`Trying WebSocket stream at ${wsUrl}`);
            
            const success = setupWebSocketStream(cameraId, wsUrl, videoElement, canvasElement, statusIndicator);
            
            if (success) {
              // Set up refresh timer
              setupStreamRefreshTimer(cameraId);
              return;
            }
            
            console.log('WebSocket stream setup failed, falling back to MJPEG');
          }
          
          // Reconnect with WebSocket
          const wsUrl = `ws://127.0.0.1:8091/ws/stream/${cameraId}`;
          const videoElement = document.getElementById(`video-${cameraId}`);
          const canvasElement = document.getElementById(`stream-${cameraId}`);
          const statusIndicator = document.getElementById(`stream-status-${cameraId}`);
          const placeholder = document.getElementById(`stream-placeholder-${cameraId}`);
          setupWebSocketStream(cameraId, wsUrl, videoElement, canvasElement, statusIndicator);
          // If WebSocket fails, try MJPEG as fallback
          if (!activeWebSockets.has(cameraId) || activeWebSockets.get(cameraId).readyState === WebSocket.CLOSED) {
            fallbackToMjpeg(cameraId, videoElement, canvasElement, placeholder, statusIndicator);
          }
        }, 2000);
      }
    };
    
    return true;
  } catch (error) {
    console.error(`Error setting up WebSocket for ${cameraId}:`, error);
    return false;
  }
}

// Set up MJPEG stream
function setupMjpegStream(cameraId, mjpegUrl, videoElement, canvasElement, placeholder, statusIndicator) {
  // Reset any existing handlers
  videoElement.onload = null;
  videoElement.onerror = null;
  
  // Set image to fill container exactly, matching borders
  videoElement.style.display = 'block';
  videoElement.style.position = 'absolute';
  videoElement.style.top = '0';
  videoElement.style.left = '0';
  videoElement.style.right = '0';
  videoElement.style.bottom = '0';
  videoElement.style.width = '100%';
  videoElement.style.height = '100%';
  videoElement.style.objectFit = 'contain';
  videoElement.style.objectPosition = 'center';
  videoElement.style.margin = '0';
  videoElement.style.padding = '0';
  videoElement.style.backgroundColor = '#000';
  // Ensure video element is above placeholder (z-index fix)
  videoElement.style.zIndex = '1';
  
  // Add load and error handlers with better error reporting
  videoElement.onload = () => {
    console.log('MJPEG stream loaded successfully for:', cameraId);
    statusIndicator.textContent = 'در حال پخش';
    statusIndicator.className = 'status-indicator connected';
    streamLastActivity.set(cameraId, Date.now());
    showToast('جریان زنده شروع شد', 'success');
  };
  
  videoElement.onerror = (event) => {
    console.error('Error loading MJPEG stream for:', cameraId, event);
    
    // Try fallback method immediately
    console.log('MJPEG failed, trying fallback method for:', cameraId);
    if (canvasElement) {
      // Hide video element and show canvas
      canvasElement.style.display = 'block';
      videoElement.style.display = 'none';
      
      // Start frame polling as fallback
      startFramePolling(cameraId, canvasElement, statusIndicator);
      
      // Update status to show we're using fallback
      statusIndicator.textContent = 'استفاده از روش جایگزین';
      statusIndicator.className = 'status-indicator connected';
    } else {
      // No fallback available
      statusIndicator.textContent = 'خطا در اتصال';
      statusIndicator.className = 'status-indicator warning';
      
      // Show error in placeholder
      const errorMessage = 'خطا در نمایش جریان - لطفاً دوباره تلاش کنید';
      if (placeholder) {
        placeholder.innerHTML = `
          <i class="fas fa-exclamation-triangle"></i>
          <p>${errorMessage}</p>
          <small>مشکل در اتصال به جریان MJPEG</small>
        `;
        placeholder.style.display = 'flex';
      }
      if (videoElement) videoElement.style.display = 'none';
      activeStreams.delete(cameraId);
      showToast(errorMessage, 'error');
    }
  };
  
  // Skip MJPEG and go directly to frame polling as a more reliable method
  console.log('Skipping MJPEG and using frame polling for:', cameraId);
  if (canvasElement) {
    canvasElement.style.display = 'block';
    videoElement.style.display = 'none';
    startFramePolling(cameraId, canvasElement, statusIndicator);
  }
}

// Set up refresh timer for a stream
function setupStreamRefreshTimer(cameraId) {
  // Clear any existing timer
  if (streamRefreshTimers.has(cameraId)) {
    clearInterval(streamRefreshTimers.get(cameraId));
  }
  
  // Set up new timer to check for inactivity
  const timer = setInterval(() => {
    const lastActivity = streamLastActivity.get(cameraId) || 0;
    const now = Date.now();
    const inactiveTime = now - lastActivity;
    
    // If stream has been inactive for more than 15 seconds, refresh it
    if (inactiveTime > 15000) {
      console.log(`Refreshing inactive stream ${cameraId}`);
      const videoElement = document.getElementById(`video-${cameraId}`);
      
      // Check if we're using WebSocket
      if (activeWebSockets.has(cameraId)) {
        // Close and reopen WebSocket
        const ws = activeWebSockets.get(cameraId);
        if (ws && ws.readyState !== WebSocket.CLOSED) {
          ws.close();
        }
        
        // Reconnect with WebSocket
        const wsUrl = `ws://127.0.0.1:8091/ws/stream/${cameraId}`;
        const videoElement = document.getElementById(`video-${cameraId}`);
        const canvasElement = document.getElementById(`stream-${cameraId}`);
        const statusIndicator = document.getElementById(`stream-status-${cameraId}`);
        setupWebSocketStream(cameraId, wsUrl, videoElement, canvasElement, statusIndicator);
      } else if (videoElement && videoElement.src) {
        // Force refresh by temporarily clearing and resetting the source
        const currentSrc = videoElement.src;
        videoElement.src = '';
        setTimeout(() => {
          videoElement.src = currentSrc;
          streamLastActivity.set(cameraId, now);
        }, 100);
      }
    }
  }, 5000); // Check every 5 seconds
  
  streamRefreshTimers.set(cameraId, timer);
}

// Stop streaming for a camera
function stopStream(cameraId) {
  console.log('Stopping stream for camera:', cameraId);
  
  const placeholder = document.getElementById(`stream-placeholder-${cameraId}`);
  const canvasElement = document.getElementById(`stream-${cameraId}`);
  const videoElement = document.getElementById(`video-${cameraId}`);
  const statusIndicator = document.getElementById(`stream-status-${cameraId}`);
  
  // Remove from active streams
  activeStreams.delete(cameraId);
  
  // Close WebSocket if active
  if (activeWebSockets.has(cameraId)) {
    const ws = activeWebSockets.get(cameraId);
    if (ws && ws.readyState !== WebSocket.CLOSED) {
      ws.close();
    }
    activeWebSockets.delete(cameraId);
  }
  
  // Clear refresh timer
  if (streamRefreshTimers.has(cameraId)) {
    clearInterval(streamRefreshTimers.get(cameraId));
    streamRefreshTimers.delete(cameraId);
  }
  
  // Show placeholder and hide video/canvas elements
  if (placeholder) {
    placeholder.innerHTML = `
      <i class="fas fa-video"></i>
      <p>در انتظار شروع جریان زنده...</p>
      <small>روی دکمه "شروع جریان" کلیک کنید</small>
    `;
    placeholder.style.display = 'flex';
  }
  if (canvasElement) canvasElement.style.display = 'none';
  if (videoElement) {
    videoElement.src = '';
    videoElement.style.display = 'none';
  }
  
  // Stop any polling intervals
  if (state.streamIntervals && state.streamIntervals[cameraId]) {
    const interval = state.streamIntervals[cameraId];
    if (typeof interval === 'object' && interval.stop) {
      interval.stop();
      if (interval.timeout) clearTimeout(interval.timeout);
    } else {
      clearTimeout(interval);
    }
    delete state.streamIntervals[cameraId];
  }
  
  // Update status
  if (statusIndicator) {
    statusIndicator.textContent = 'متوقف شده';
    statusIndicator.className = 'status-indicator';
  }
  
  showToast('جریان متوقف شد', 'info');
}

// Toggle fullscreen for a stream
async function toggleFullscreenStream(cameraId) {
  const streamContainer = document.getElementById(`video-${cameraId}`)?.closest('.stream-item') || 
                          document.getElementById(`stream-${cameraId}`)?.closest('.stream-item');
  
  if (!streamContainer) {
    console.error('Stream container not found for:', cameraId);
    return;
  }
  
  const videoElement = document.getElementById(`video-${cameraId}`);
  const canvasElement = document.getElementById(`stream-${cameraId}`);
  const fullscreenBtn = streamContainer.querySelector('.btn-small[onclick*="toggleFullscreenStream"]');
  
  try {
    // Check if already in fullscreen or has fullscreen class
    const isFullscreen = document.fullscreenElement === streamContainer || 
        document.webkitFullscreenElement === streamContainer || 
        document.mozFullScreenElement === streamContainer || 
        document.msFullscreenElement === streamContainer ||
        streamContainer.classList.contains('fullscreen');
        
    if (isFullscreen) {
      // Exit fullscreen with a small delay to prevent freezing
      streamContainer.classList.remove('fullscreen');
      document.body.classList.remove('fullscreen-active');
      
      setTimeout(async () => {
        if (document.fullscreenElement) {
          if (document.exitFullscreen) {
            await document.exitFullscreen();
          } else if (document.webkitExitFullscreen) {
            await document.webkitExitFullscreen();
          } else if (document.mozCancelFullScreen) {
            await document.mozCancelFullScreen();
          } else if (document.msExitFullscreen) {
            await document.msExitFullscreen();
          }
        }
        
        // Update button icon
        if (fullscreenBtn) {
          const icon = fullscreenBtn.querySelector('i');
          if (icon) icon.className = 'fas fa-expand';
        }
        
        showToast('خروج از حالت تمام صفحه', 'info');
      }, 100);
    } else {
      // Enter fullscreen
      streamContainer.classList.add('fullscreen');
      document.body.classList.add('fullscreen-active');
      
      // Try browser's native fullscreen API
      try {
        if (streamContainer.requestFullscreen) {
          await streamContainer.requestFullscreen();
        } else if (streamContainer.webkitRequestFullscreen) {
          await streamContainer.webkitRequestFullscreen();
        } else if (streamContainer.mozRequestFullScreen) {
          await streamContainer.mozRequestFullScreen();
        } else if (streamContainer.msRequestFullscreen) {
          await streamContainer.msRequestFullscreen();
        }
      } catch (fsError) {
        console.log('Using CSS fullscreen fallback');
        // We're already using CSS fallback, so no need to handle this error
      }
      
      // Update button icon
      if (fullscreenBtn) {
        const icon = fullscreenBtn.querySelector('i');
        if (icon) icon.className = 'fas fa-compress';
      }
      
      // Add quality selector in fullscreen mode
      addQualitySelector(streamContainer, cameraId);
      
      showToast('وارد حالت تمام صفحه شد', 'success');
    }
  } catch (error) {
    console.error('Error toggling fullscreen:', error);
    showToast('خطا در تغییر حالت نمایش: ' + error.message, 'error');
  }
}

// Add quality selector in fullscreen mode
function addQualitySelector(streamContainer, cameraId) {
  // Remove existing quality selector if any
  const existingSelector = streamContainer.querySelector('.quality-selector');
  if (existingSelector) {
    existingSelector.remove();
  }
  
  // Create quality selector
  const qualitySelector = document.createElement('div');
  qualitySelector.className = 'quality-selector active';
  
  // Add quality buttons
  QUALITY_LEVELS.forEach(quality => {
    const btn = document.createElement('button');
    btn.className = `quality-btn ${quality === currentQuality ? 'active' : ''}`;
    btn.textContent = quality;
    btn.onclick = () => changeStreamQuality(cameraId, quality);
    qualitySelector.appendChild(btn);
  });
  
  streamContainer.appendChild(qualitySelector);
  
  // Add event listener to hide quality selector when exiting fullscreen
  const hideSelector = () => {
    if (qualitySelector.parentNode) {
      qualitySelector.remove();
    }
    document.removeEventListener('fullscreenchange', hideSelector);
    document.removeEventListener('webkitfullscreenchange', hideSelector);
    document.removeEventListener('mozfullscreenchange', hideSelector);
    document.removeEventListener('MSFullscreenChange', hideSelector);
  };
  
  document.addEventListener('fullscreenchange', hideSelector);
  document.addEventListener('webkitfullscreenchange', hideSelector);
  document.addEventListener('mozfullscreenchange', hideSelector);
  document.addEventListener('MSFullscreenChange', hideSelector);
}

// Change stream quality
async function changeStreamQuality(cameraId, quality) {
  console.log(`Changing quality for ${cameraId} to ${quality}`);
  
  try {
    // If quality parameter is not provided, show quality selection menu
    if (quality === undefined) {
      const qualities = {
        '480p': 30,
        '720p': 50,
        '1080p': 90
      };
      
      const currentQualityValue = localStorage.getItem(`stream-quality-${cameraId}`) || '720p';
      
      // Create quality selection menu
      const qualityMenu = document.createElement('div');
      qualityMenu.className = 'quality-menu';
      qualityMenu.style.position = 'absolute';
      qualityMenu.style.bottom = '60px';
      qualityMenu.style.right = '10px';
      qualityMenu.style.background = 'rgba(20, 20, 30, 0.9)';
      qualityMenu.style.borderRadius = '8px';
      qualityMenu.style.padding = '8px';
      qualityMenu.style.zIndex = '1000'; // Higher z-index to ensure visibility
      qualityMenu.style.boxShadow = '0 4px 15px rgba(0, 0, 0, 0.3)';
      
      // Find the stream container
      const streamContainer = document.getElementById(`video-${cameraId}`)?.closest('.stream-item') || 
                              document.getElementById(`stream-${cameraId}`)?.closest('.stream-item');
      if (streamContainer) {
        streamContainer.appendChild(qualityMenu);
      } else {
        document.body.appendChild(qualityMenu);
      }
      
      Object.keys(qualities).forEach(q => {
        const option = document.createElement('div');
        option.className = 'quality-option';
        option.textContent = q;
        option.style.padding = '6px 12px';
        option.style.cursor = 'pointer';
        option.style.borderRadius = '4px';
        option.style.margin = '4px 0';
        
        if (q === currentQualityValue) {
          option.style.background = 'rgba(100, 100, 255, 0.3)';
        }
        
        option.addEventListener('mouseover', () => {
          option.style.background = 'rgba(100, 100, 255, 0.2)';
        });
        
        option.addEventListener('mouseout', () => {
          if (q !== currentQualityValue) {
            option.style.background = 'transparent';
          }
        });
        
        option.addEventListener('click', async (e) => {
          e.stopPropagation(); // Prevent event bubbling
          
          // Store the selected quality in localStorage
          localStorage.setItem(`stream-quality-${cameraId}`, q);
          
          // Send quality change request to backend without restarting stream
          const qualityUrl = `http://127.0.0.1:8091/set_quality?stream_id=${cameraId}&quality=${qualities[q]}`;
          try {
            const response = await fetch(qualityUrl, { method: 'POST' });
            const result = await response.json();
            
            if (result.success) {
              showToast(`کیفیت پخش به ${q} تغییر کرد`, 'success');
            } else {
              showToast(`خطا در تغییر کیفیت: ${result.message || 'خطای ناشناخته'}`, 'warning');
            }
          } catch (error) {
            console.error('Error changing quality:', error);
            showToast('خطا در تغییر کیفیت', 'error');
          }
          
          qualityMenu.remove();
        });
        
        qualityMenu.appendChild(option);
      });
      
      // Add close button
      const closeBtn = document.createElement('div');
      closeBtn.textContent = 'بستن';
      closeBtn.style.textAlign = 'center';
      closeBtn.style.padding = '6px';
      closeBtn.style.marginTop = '8px';
      closeBtn.style.borderTop = '1px solid rgba(255, 255, 255, 0.1)';
      closeBtn.style.cursor = 'pointer';
      closeBtn.addEventListener('click', (e) => {
        e.stopPropagation(); // Prevent event bubbling
        qualityMenu.remove();
      });
      qualityMenu.appendChild(closeBtn);
      
      // Close menu when clicking outside
      const closeMenu = (e) => {
        if (!qualityMenu.contains(e.target)) {
          qualityMenu.remove();
          document.removeEventListener('click', closeMenu);
        }
      };
      
      // Delay adding the event listener to prevent immediate closing
      setTimeout(() => {
        document.addEventListener('click', closeMenu);
      }, 100);
      
      return;
    }
    
    // This part is for direct quality change (not through menu)
    // Store the selected quality in localStorage
    localStorage.setItem(`stream-quality-${cameraId}`, quality);
    
    // Send quality change request to backend without restarting stream
    const qualityUrl = `http://127.0.0.1:8091/set_quality?stream_id=${cameraId}&quality=${quality}`;
    const response = await fetch(qualityUrl, { method: 'POST' });
    const result = await response.json();
    
    if (result.success) {
      showToast(`کیفیت پخش به ${quality} تغییر کرد`, 'success');
    } else {
      showToast(`خطا در تغییر کیفیت: ${result.message || 'خطای ناشناخته'}`, 'warning');
    }
  } catch (error) {
    console.error('Error changing quality:', error);
    showToast('خطا در تغییر کیفیت', 'error');
  }
}

// Listen for fullscreen changes to update button icon
document.addEventListener('fullscreenchange', updateFullscreenButtons);
document.addEventListener('webkitfullscreenchange', updateFullscreenButtons);
document.addEventListener('mozfullscreenchange', updateFullscreenButtons);
document.addEventListener('MSFullscreenChange', updateFullscreenButtons);

function updateFullscreenButtons() {
  const isFullscreen = !!(document.fullscreenElement || document.webkitFullscreenElement || 
                          document.mozFullScreenElement || document.msFullscreenElement);
  
  document.querySelectorAll('.stream-item').forEach(item => {
    const fullscreenBtn = item.querySelector('.btn-small[onclick*="toggleFullscreenStream"]');
    if (fullscreenBtn) {
      const icon = fullscreenBtn.querySelector('i');
      if (icon) {
        icon.className = isFullscreen ? 'fas fa-compress' : 'fas fa-expand';
      }
    }
  });
}

// Capture snapshot from stream
function captureSnapshot(cameraId) {
  const videoElement = document.getElementById(`video-${cameraId}`);
  const canvasElement = document.getElementById(`stream-${cameraId}`);
  
  // Try video element first, then canvas
  const element = videoElement?.src && videoElement.style.display !== 'none' ? videoElement : 
                  canvasElement?.style.display !== 'none' ? canvasElement : null;
  
  if (element) {
    // Create canvas from element
    const canvas = document.createElement('canvas');
    
    if (element instanceof HTMLImageElement) {
      // Video element (MJPEG stream)
      canvas.width = element.videoWidth || element.naturalWidth || 640;
      canvas.height = element.videoHeight || element.naturalHeight || 480;
    } else if (element instanceof HTMLCanvasElement) {
      // Canvas element (frame polling)
      canvas.width = element.width || 640;
      canvas.height = element.height || 480;
    } else {
      showToast('هیچ تصویری برای ثبت وجود ندارد', 'warning');
      return;
    }
    
    const ctx = canvas.getContext('2d');
    
    // Draw the current frame
    ctx.drawImage(element, 0, 0, canvas.width, canvas.height);
    
    canvas.toBlob((blob) => {
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = `snapshot_${cameraId}_${new Date().toISOString().replace(/[:.]/g, '-')}.png`;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      URL.revokeObjectURL(url);
      showToast('عکس ثبت شد', 'success');
    }, 'image/png');
  } else {
    showToast('هیچ تصویری برای ثبت وجود ندارد', 'warning');
  }
}

// Show stream information
function showStreamInfo(cameraId) {
  const camera = state.discoveredCameras.find(c => c.id === cameraId);
  if (!camera) {
    showToast('اطلاعات دوربین یافت نشد', 'error');
    return;
  }
  
  const info = `
نام دوربین: ${camera.name}
آدرس RTSP: ${camera.url}
تاریخ اضافه: ${new Date(camera.addedAt).toLocaleString('fa-IR')}
شناسه: ${camera.id}
  `;
  
  alert(info);
}

// Fallback: Poll for frames using canvas (only if MJPEG fails)
function startFramePolling(cameraId, canvasElement, statusIndicator) {
  console.log('Starting frame polling fallback for:', cameraId);
  
  // Prevent multiple polling instances for same stream
  if (!state.streamIntervals) {
    state.streamIntervals = {};
  }
  
  // Stop existing polling if any
  if (state.streamIntervals[cameraId]) {
    const existing = state.streamIntervals[cameraId];
    if (typeof existing === 'object' && existing.stop) {
      existing.stop();
      if (existing.timeout) clearTimeout(existing.timeout);
    } else {
      clearTimeout(existing);
    }
  }
  
  const ctx = canvasElement.getContext('2d');
  const frameUrl = `http://127.0.0.1:8091/stream/${cameraId}/frame`;
  
  // Optimized frame polling with faster refresh rate
  let errorCount = 0;
  let currentDelay = 100; // Start with 100ms (10 FPS) for smoother video
  let isPolling = true;
  let abortController = null;
  
  const pollFrame = async () => {
    if (!isPolling) return;
    
    try {
      const timestamp = Date.now();
      abortController = new AbortController();
      const timeoutId = setTimeout(() => {
        abortController.abort();
        console.log('Frame request timeout for:', cameraId);
      }, 3000); // 3 second timeout (reduced from 5s)
      
      // Reduced logging to improve performance
      const response = await fetch(`${frameUrl}?t=${timestamp}`, {
        cache: 'no-cache',
        signal: abortController.signal
      });
      
      clearTimeout(timeoutId);
      
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}`);
      }
      
      const blob = await response.blob();
      const imageUrl = URL.createObjectURL(blob);
      
      const img = new Image();
      img.onload = () => {
        // Optimized rendering - only update dimensions when needed
        if (!canvasElement.width || !canvasElement.height || 
            canvasElement.width !== canvasElement.parentElement.clientWidth ||
            canvasElement.height !== canvasElement.parentElement.clientHeight) {
          canvasElement.width = canvasElement.parentElement.clientWidth;
          canvasElement.height = canvasElement.parentElement.clientHeight;
        }
        
        // Fast image drawing with optimized calculations
        const aspectRatio = img.width / img.height;
        const canvasAspect = canvasElement.width / canvasElement.height;
        
        let drawWidth, drawHeight, drawX, drawY;
        
        if (canvasAspect > aspectRatio) {
          drawHeight = canvasElement.height;
          drawWidth = drawHeight * aspectRatio;
          drawX = (canvasElement.width - drawWidth) / 2;
          drawY = 0;
        } else {
          drawWidth = canvasElement.width;
          drawHeight = drawWidth / aspectRatio;
          drawX = 0;
          drawY = (canvasElement.height - drawHeight) / 2;
        }
        
        ctx.clearRect(0, 0, canvasElement.width, canvasElement.height);
        ctx.drawImage(img, drawX, drawY, drawWidth, drawHeight);
        
        URL.revokeObjectURL(imageUrl);
        
        // Reset error count on success
        errorCount = 0;
        currentDelay = 100; // Keep faster refresh rate
        
        if (statusIndicator) {
          statusIndicator.textContent = 'در حال پخش';
          statusIndicator.className = 'status-indicator connected';
        }
        
        // Schedule next poll only if still polling
        if (isPolling) {
          const nextTimeout = setTimeout(pollFrame, currentDelay);
          state.streamIntervals[cameraId] = nextTimeout;
        }
      };
      
      img.onerror = () => {
        URL.revokeObjectURL(imageUrl);
        errorCount++;
        // Faster recovery on errors
        currentDelay = Math.min(currentDelay * 1.2, 2000); // Max 2 seconds (reduced from 10s)
        if (isPolling) {
          const nextTimeout = setTimeout(pollFrame, currentDelay);
          state.streamIntervals[cameraId] = nextTimeout;
        }
      };
      
      img.src = imageUrl;
    } catch (error) {
      console.error('Error fetching frame for:', cameraId, error);
      
      // Handle abort errors specifically
      if (error.name === 'AbortError') {
        console.log('Frame request was aborted for:', cameraId);
      } else {
        // Ignore ERR_INSUFFICIENT_RESOURCES errors silently
        if (error.message && !error.message.includes('INSUFFICIENT_RESOURCES')) {
          console.error('Error fetching frame:', error);
        }
        
        errorCount++;
        
        // Exponential backoff on errors to prevent resource exhaustion
        currentDelay = Math.min(currentDelay * 2, 10000); // Max 10 seconds
        
        if (statusIndicator && errorCount < 3) {
          statusIndicator.textContent = 'خطا در اتصال';
          statusIndicator.className = 'status-indicator warning';
        }
      }
      
      // Schedule next poll with backoff only if still polling
      if (isPolling) {
        const nextTimeout = setTimeout(pollFrame, currentDelay);
        state.streamIntervals[cameraId] = nextTimeout;
      }
    }
  };
  
  // Store polling state for cleanup
  state.streamIntervals[cameraId] = {
    stop: () => { 
      isPolling = false; 
      if (abortController) {
        abortController.abort();
      }
    },
    timeout: setTimeout(pollFrame, currentDelay)
  };
  
  console.log('Started frame polling for:', cameraId);
}

// Toggle streaming mode between WebSocket and MJPEG
function toggleStreamingMode() {
  if (preferredMode === STREAM_MODES.WEBSOCKET) {
    preferredMode = STREAM_MODES.MJPEG;
    showToast('حالت پخش: MJPEG (سازگاری بیشتر)', 'info');
  } else {
    preferredMode = STREAM_MODES.WEBSOCKET;
    showToast('حالت پخش: WebSocket (تأخیر کمتر)', 'info');
  }
  
  // Save preference
  localStorage.setItem('preferredStreamMode', preferredMode);
  
  return preferredMode;
}

// Initialize preferred mode from localStorage if available
function initStreamingMode() {
  const savedMode = localStorage.getItem('preferredStreamMode');
  if (savedMode && Object.values(STREAM_MODES).includes(savedMode)) {
    preferredMode = savedMode;
    console.log(`Initialized streaming mode from storage: ${preferredMode}`);
  }
}

// Call initialization on module load
initStreamingMode();

// Make sure functions are available globally
window.startStream = startStream;
window.stopStream = stopStream;
window.toggleFullscreenStream = toggleFullscreenStream;
window.captureSnapshot = captureSnapshot;
window.showStreamInfo = showStreamInfo;
window.changeStreamQuality = changeStreamQuality;
window.toggleStreamingMode = toggleStreamingMode;

export { 
  startStream, 
  stopStream, 
  toggleFullscreenStream, 
  captureSnapshot, 
  showStreamInfo, 
  changeStreamQuality,
  toggleStreamingMode
};