"""
Enhanced Fast Python CCTV Processing Service
With AI recognition and optimized performance
"""

import asyncio
import cv2
import numpy as np
from fastapi import FastAPI, HTTPException, BackgroundTasks
from fastapi.responses import StreamingResponse, JSONResponse
from fastapi.middleware.cors import CORSMiddleware
import uvicorn
from typing import Dict, List, Optional
import threading
import time
from datetime import datetime
import json
from loguru import logger
import psutil
import os
from pathlib import Path
import io
from urllib.parse import urlparse
import time
import threading
import concurrent.futures
from typing import Dict, List, Optional, Tuple
import numpy as np


# Configure logging
logger.add("logs/cctv_service.log", rotation="10 MB", level="INFO")

app = FastAPI(
    title="Enhanced Fast CCTV Processing Service",
    description="High-performance RTSP stream processing with AI recognition",
    version="2.0.0"
)

# CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

class EnhancedStreamProcessor:
    """Enhanced stream processor with AI recognition"""
    
    def __init__(self):
        self.streams: Dict[str, cv2.VideoCapture] = {}
        self.stream_urls: Dict[str, str] = {}  # Store RTSP URLs for reconnection
        self.latest_frames: Dict[str, np.ndarray] = {}
        self.latest_detections: Dict[str, List[Dict]] = {}
        self.frame_locks: Dict[str, threading.Lock] = {}
        self.processing_threads: Dict[str, threading.Thread] = {}
        self.running = True
        
        # Performance monitoring
        self.fps_counters: Dict[str, int] = {}
        self.last_fps_time: Dict[str, float] = {}
        self.detection_fps: Dict[str, int] = {}
        
        # AI processing settings
        self.ai_enabled = True
        self.detection_interval = 5  # Process every 5 frames
        self.frame_counters: Dict[str, int] = {}
        
        logger.info("Enhanced StreamProcessor initialized")
    
    def add_stream(self, stream_id: str, rtsp_url: str, enable_ai: bool = True) -> bool:
        """Add a new RTSP stream with optional AI processing"""
        try:
            # Create lock for this stream
            self.frame_locks[stream_id] = threading.Lock()
            
            # Open RTSP stream with optimized settings for low latency
            logger.info(f"Attempting to open RTSP stream: {rtsp_url}")
            cap = cv2.VideoCapture(rtsp_url)
            
            # Optimize for low latency
            cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)  # Minimize buffer
            cap.set(cv2.CAP_PROP_FPS, 30)  # Set FPS
            
            # Low latency RTSP settings
            cap.set(cv2.CAP_PROP_OPEN_TIMEOUT_MSEC, 8000)  # 8 second timeout
            cap.set(cv2.CAP_PROP_READ_TIMEOUT_MSEC, 8000)   # 8 second read timeout
            
            # Check if stream opened successfully
            if not cap.isOpened():
                logger.error(f"Failed to open stream: {rtsp_url}")
                return False
            
            logger.info(f"Successfully opened RTSP stream: {rtsp_url}")
            
            self.streams[stream_id] = cap
            self.stream_urls[stream_id] = rtsp_url  # Store URL for reconnection
            self.fps_counters[stream_id] = 0
            self.last_fps_time[stream_id] = time.time()
            self.frame_counters[stream_id] = 0
            self.latest_detections[stream_id] = []
            
            # Start processing thread for this stream
            thread = threading.Thread(
                target=self._process_stream,
                args=(stream_id, enable_ai),
                daemon=True
            )
            thread.start()
            self.processing_threads[stream_id] = thread
            
            logger.info(f"Added stream {stream_id}: {rtsp_url} (AI: {enable_ai})")
            return True
            
        except Exception as e:
            logger.error(f"Error adding stream {stream_id}: {e}")
            import traceback
            logger.error(f"Traceback: {traceback.format_exc()}")
            return False
    
    def remove_stream(self, stream_id: str) -> bool:
        """Remove a stream"""
        try:
            if stream_id in self.streams:
                self.streams[stream_id].release()
                del self.streams[stream_id]
            
            if stream_id in self.latest_frames:
                del self.latest_frames[stream_id]
            
            if stream_id in self.latest_detections:
                del self.latest_detections[stream_id]
            
            if stream_id in self.frame_locks:
                del self.frame_locks[stream_id]
            
            if stream_id in self.processing_threads:
                del self.processing_threads[stream_id]
            
            if stream_id in self.stream_urls:
                del self.stream_urls[stream_id]
            
            # Clean up counters
            for counter_dict in [self.fps_counters, self.last_fps_time, self.detection_fps, self.frame_counters]:
                if stream_id in counter_dict:
                    del counter_dict[stream_id]
            
            logger.info(f"Removed stream {stream_id}")
            return True
            
        except Exception as e:
            logger.error(f"Error removing stream {stream_id}: {e}")
            return False
    
    def _reconnect_stream(self, stream_id: str) -> bool:
        """Attempt to reconnect to a stream"""
        try:
            if stream_id not in self.stream_urls:
                logger.error(f"Stream URL not found for {stream_id}")
                return False
            
            rtsp_url = self.stream_urls[stream_id]
            logger.info(f"Attempting to reconnect to stream {stream_id} at {rtsp_url}")
            
            # Release the current connection if it exists
            if stream_id in self.streams:
                try:
                    logger.info(f"Releasing current stream connection for {stream_id}")
                    self.streams[stream_id].release()
                except Exception as e:
                    logger.warning(f"Error releasing current stream connection: {e}")
            
            # Wait a bit before reconnecting
            time.sleep(1.0)
            
            # Attempt to reopen the stream
            cap = cv2.VideoCapture(rtsp_url)
            cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)  # Minimize buffer
            cap.set(cv2.CAP_PROP_FPS, 30)  # Set FPS
            cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1920)  # Set resolution
            cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 1080)
            
            # Check if stream opened successfully
            if not cap.isOpened():
                logger.error(f"Failed to reconnect to stream: {rtsp_url}")
                return False
            
            # Test read a frame to ensure it's working
            ret, frame = cap.read()
            if not ret or frame is None:
                logger.warning(f"Stream opened but failed to read frame")
                cap.release()
                return False
            
            logger.info(f"Successfully tested frame read for stream {stream_id}")
            
            # Update the stream connection
            self.streams[stream_id] = cap
            logger.info(f"Successfully reconnected to stream {stream_id}")
            return True
            
        except Exception as e:
            logger.error(f"Error during reconnection attempt for stream {stream_id}: {e}")
            import traceback
            logger.error(f"Traceback: {traceback.format_exc()}")
            return False
    
    def _process_stream(self, stream_id: str, enable_ai: bool):
        """Process a single stream in a separate thread"""
        if stream_id not in self.streams:
            logger.error(f"Stream {stream_id} not found in streams dict")
            return
            
        cap = self.streams[stream_id]
        frame_count = 0
        detection_count = 0
        consecutive_failures = 0
        max_consecutive_failures = 10  # Maximum consecutive failures before reconnecting
        
        logger.info(f"Starting stream processing loop for {stream_id}")
        
        # Frame skipping for performance optimization
        frame_skip = 2  # Process every 3rd frame for AI
        frame_counter = 0
        
        while self.running and stream_id in self.streams:
            try:
                # Check if the capture is still valid
                if not cap.isOpened():
                    logger.warning(f"Stream capture closed for {stream_id}, attempting to reconnect...")
                    if self._reconnect_stream(stream_id):
                        cap = self.streams[stream_id]  # Get the new capture object
                        consecutive_failures = 0
                        logger.info(f"Successfully reconnected to stream {stream_id}")
                    else:
                        logger.error(f"Failed to reconnect to stream {stream_id}")
                        time.sleep(2.0)  # Longer delay if reconnection failed
                        continue
                
                ret, frame = cap.read()
                if not ret or frame is None:
                    consecutive_failures += 1
                    logger.warning(f"Failed to read frame from {stream_id} (failure #{consecutive_failures})")
                    
                    # If we have too many consecutive failures, try to reconnect
                    if consecutive_failures >= max_consecutive_failures:
                        logger.warning(f"Too many consecutive failures for {stream_id}, attempting to reconnect...")
                        # Try to reconnect to the stream
                        if self._reconnect_stream(stream_id):
                            cap = self.streams[stream_id]  # Get the new capture object
                            consecutive_failures = 0
                            logger.info(f"Successfully reconnected to stream {stream_id}")
                        else:
                            logger.error(f"Failed to reconnect to stream {stream_id}")
                            time.sleep(2.0)  # Longer delay if reconnection failed
                            continue
                    
                    time.sleep(0.033)  # ~30 FPS delay
                    continue
                else:
                    # Reset failure counter on successful frame read
                    consecutive_failures = 0
                
                # Update latest frame with thread safety (always update for live view)
                with self.frame_locks[stream_id]:
                    self.latest_frames[stream_id] = frame.copy()
                
                # AI processing optimization - only process every N frames
                if enable_ai and self.ai_enabled:
                    frame_counter += 1
                    if frame_counter % frame_skip == 0:
                        # Run AI detection in thread pool to avoid blocking
                        try:
                            import concurrent.futures
                            with concurrent.futures.ThreadPoolExecutor(max_workers=1) as executor:  # Reduced workers
                                future = executor.submit(self._run_ai_detection_sync, stream_id, frame.copy())
                                # Don't wait for completion to avoid blocking
                        except Exception as e:
                            logger.error(f"Error starting AI detection: {e}")
                        detection_count += 1
                
                # Update FPS counter
                frame_count += 1
                current_time = time.time()
                if current_time - self.last_fps_time[stream_id] >= 1.0:
                    self.fps_counters[stream_id] = frame_count
                    self.detection_fps[stream_id] = detection_count
                    frame_count = 0
                    detection_count = 0
                    self.last_fps_time[stream_id] = current_time
                
                # Minimal delay to prevent excessive CPU usage but keep it responsive
                time.sleep(0.005)  # 5ms delay (~200 FPS cap)
                
            except Exception as e:
                logger.error(f"Error processing stream {stream_id}: {e}")
                import traceback
                logger.error(f"Traceback: {traceback.format_exc()}")
                consecutive_failures += 1
                
                # If we have too many consecutive failures, try to reconnect
                if consecutive_failures >= max_consecutive_failures:
                    logger.warning(f"Too many consecutive failures for {stream_id} due to exceptions, attempting to reconnect...")
                    if self._reconnect_stream(stream_id):
                        cap = self.streams[stream_id]  # Get the new capture object
                        consecutive_failures = 0
                        logger.info(f"Successfully reconnected to stream {stream_id}")
                    else:
                        logger.error(f"Failed to reconnect to stream {stream_id}")
                        time.sleep(2.0)  # Longer delay if reconnection failed
                
                time.sleep(0.05)  # 50ms delay on error
    
    def _run_ai_detection_sync(self, stream_id: str, frame: np.ndarray):
        """Run AI detection on a frame (synchronous version)"""
        try:
            logger.debug(f"AI detection disabled for stream {stream_id} - no AI processor available")
            # AI detection is disabled since ai_processor is not available
            # Update latest detections with empty list
            with self.frame_locks[stream_id]:
                self.latest_detections[stream_id] = []
            
        except Exception as e:
            logger.error(f"Error in AI detection for stream {stream_id}: {e}")
            import traceback
            logger.error(f"Traceback: {traceback.format_exc()}")
    
    async def _run_ai_detection(self, stream_id: str, frame: np.ndarray):
        """Run AI detection on a frame"""
        try:
            # AI detection is disabled since ai_processor is not available
            # Update latest detections with empty list
            with self.frame_locks[stream_id]:
                self.latest_detections[stream_id] = []
            
        except Exception as e:
            logger.error(f"Error in AI detection for stream {stream_id}: {e}")
    
    def get_latest_frame(self, stream_id: str, with_detections: bool = False) -> Optional[np.ndarray]:
        """Get the latest frame from a stream"""
        if stream_id not in self.frame_locks:
            logger.warning(f"Stream {stream_id} not found in frame_locks")
            return None
        
        with self.frame_locks[stream_id]:
            frame = self.latest_frames.get(stream_id, None)
            if frame is None:
                logger.debug(f"No latest frame available for stream {stream_id}")
                return None
            
            logger.debug(f"Retrieved frame for stream {stream_id}: shape={frame.shape}")
            
            # Return frame copy without any additional processing (no AI)
            return frame.copy()
    
    def get_latest_detections(self, stream_id: str) -> List[Dict]:
        """Get the latest detections for a stream"""
        if stream_id not in self.frame_locks:
            return []
        
        with self.frame_locks[stream_id]:
            return self.latest_detections.get(stream_id, []).copy()
    
    def get_stream_info(self, stream_id: str) -> Dict:
        """Get stream information and performance metrics"""
        if stream_id not in self.streams:
            return {"error": "Stream not found"}
        
        cap = self.streams[stream_id]
        return {
            "stream_id": stream_id,
            "is_opened": cap.isOpened(),
            "fps": self.fps_counters.get(stream_id, 0),
            "detection_fps": self.detection_fps.get(stream_id, 0),
            "frame_width": int(cap.get(cv2.CAP_PROP_FRAME_WIDTH)),
            "frame_height": int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT)),
            "latest_detections_count": len(self.latest_detections.get(stream_id, [])),
            "ai_enabled": self.ai_enabled,
            "timestamp": datetime.now().isoformat()
        }
    
    def get_all_streams_info(self) -> List[Dict]:
        """Get information for all streams"""
        return [self.get_stream_info(stream_id) for stream_id in self.streams.keys()]

# Global stream processor instance
stream_processor = EnhancedStreamProcessor()

@app.on_event("startup")
async def startup_event():
    """Initialize service without AI processor"""
    logger.info("Starting service without AI processor")
    logger.info("Service initialized successfully")

@app.get("/")
async def root():
    """Root endpoint"""
    return {
        "service": "Enhanced Fast CCTV Processing Service",
        "version": "2.0.0",
        "status": "running",
        "ai_enabled": False,  # AI is disabled
        "timestamp": datetime.now().isoformat()
    }

@app.get("/health")
async def health_check():
    """Health check endpoint"""
    return {
        "status": "healthy",
        "timestamp": datetime.now().isoformat(),
        "memory_usage": psutil.virtual_memory().percent,
        "cpu_usage": psutil.cpu_percent(),
        "active_streams": len(stream_processor.streams),
        "ai_performance": {"ai_enabled": False}  # AI is disabled
    }

@app.delete("/remove_stream/{stream_id}")
async def remove_stream(stream_id: str):
    """Remove a stream"""
    success = stream_processor.remove_stream(stream_id)
    if success:
        return {"success": True, "message": "Stream removed successfully"}
    else:
        raise HTTPException(status_code=404, detail="Stream not found")

@app.get("/streams")
async def list_streams():
    """List all active streams"""
    return {
        "streams": stream_processor.get_all_streams_info(),
        "count": len(stream_processor.streams)
    }

@app.get("/stream/{stream_id}/frame")
async def get_stream_frame(stream_id: str, with_detections: bool = False):
    """Get the latest frame from a stream as JPEG"""
    # Add a small delay to prevent overwhelming the system
    await asyncio.sleep(0.01)
    
    frame = stream_processor.get_latest_frame(stream_id, with_detections)
    if frame is None:
        raise HTTPException(status_code=404, detail="Stream not found or no frame available")
    
    try:
        # Encode frame as JPEG with optimized settings for speed
        _, buffer = cv2.imencode('.jpg', frame, [
            cv2.IMWRITE_JPEG_QUALITY, 60,  # Reduced quality for speed
            cv2.IMWRITE_JPEG_OPTIMIZE, 0,  # Disable optimization for speed
            cv2.IMWRITE_JPEG_PROGRESSIVE, 0  # Disable progressive for speed
        ])
        
        return StreamingResponse(
            io.BytesIO(buffer.tobytes()),
            media_type="image/jpeg",
            headers={
                "Cache-Control": "no-cache, no-store, must-revalidate",
                "Pragma": "no-cache",
                "Expires": "0",
                "Access-Control-Allow-Origin": "*"
            }
        )
    except Exception as e:
        logger.error(f"Error encoding frame for stream {stream_id}: {e}")
        raise HTTPException(status_code=500, detail="Error encoding frame")

@app.get("/stream/{stream_id}/detections")
async def get_stream_detections(stream_id: str):
    """Get the latest detections for a stream"""
    detections = stream_processor.get_latest_detections(stream_id)
    if detections is None:
        raise HTTPException(status_code=404, detail="Stream not found")
    
    return {
        "stream_id": stream_id,
        "detections": detections,
        "count": len(detections),
        "timestamp": datetime.now().isoformat()
    }

@app.get("/stream/{stream_id}/info")
async def get_stream_info(stream_id: str):
    """Get detailed information about a stream"""
    info = stream_processor.get_stream_info(stream_id)
    if "error" in info:
        raise HTTPException(status_code=404, detail=info["error"])
    return info

@app.get("/discover")
async def discover_cameras(ip: str = "192.168.4.252", user: str = "admin", password: str = "test1234", brand: str = "reolink"):
    """Discover available cameras - compatible with Electron app"""
    # Generate candidate URLs based on the provided parameters
    candidates = []
    
    # Common Reolink RTSP paths
    rtsp_paths = [
        "/h264Preview_01_sub",
        "/h264Preview_01_main", 
        "/h264Preview_02_sub",
        "/h264Preview_02_main",
        "/cam/realmonitor?channel=1&subtype=0",
        "/cam/realmonitor?channel=1&subtype=1"
    ]
    
    for path in rtsp_paths:
        candidates.append(f"rtsp://{user}:{password}@{ip}:554{path}")
    
    return {
        "candidates": candidates,
        "ip": ip,
        "user": user,
        "brand": brand
    }

@app.post("/toggle_ai")
async def toggle_ai(enabled: bool):
    """Toggle AI processing on/off"""
    stream_processor.ai_enabled = enabled
    return {
        "success": True,
        "ai_enabled": enabled,
        "message": f"AI processing {'enabled' if enabled else 'disabled'}"
    }

@app.get("/ai_stats")
async def get_ai_stats():
    """Get AI processing statistics"""
    # AI is disabled since ai_processor is not available
    return {
        "ai_enabled": False,
        "message": "AI processing is disabled"
    }

@app.post("/add_stream")
async def add_stream(stream_id: str, rtsp_url: str, enable_ai: bool = True):
    """Add a new RTSP stream with optional AI processing"""
    # Validate RTSP URL
    if not rtsp_url or not rtsp_url.startswith('rtsp://'):
        raise HTTPException(status_code=400, detail="Invalid RTSP URL format. URL must start with 'rtsp://'")
    
    # Validate stream_id
    if not stream_id:
        raise HTTPException(status_code=400, detail="Stream ID is required")
    
    # Validate URL structure
    try:
        parsed_url = urlparse(rtsp_url)
        if not parsed_url.hostname:
            raise HTTPException(status_code=400, detail="Invalid RTSP URL: Missing hostname")
        
        # Check for incomplete IP addresses more accurately
        hostname = parsed_url.hostname
        if '.' in hostname:
            parts = hostname.split('.')
            # Check for obviously incomplete private network addresses
            if len(parts) >= 2:
                if parts[0] == '192' and parts[1] == '168' and len(parts) < 4:
                    raise HTTPException(status_code=400, detail="Invalid RTSP URL: Incomplete IP address")
                elif parts[0] == '10' and len(parts) < 4:
                    raise HTTPException(status_code=400, detail="Invalid RTSP URL: Incomplete IP address")
                elif parts[0] == '172' and len(parts) < 4:
                    raise HTTPException(status_code=400, detail="Invalid RTSP URL: Incomplete IP address")
            
    except HTTPException:
        # Re-raise HTTP exceptions
        raise
    except Exception as e:
        logger.error(f"Error parsing RTSP URL {rtsp_url}: {e}")
        raise HTTPException(status_code=400, detail=f"Invalid RTSP URL format: {str(e)}")
    
    if stream_id in stream_processor.streams:
        return {"success": False, "message": "Stream already exists"}
    
    success = stream_processor.add_stream(stream_id, rtsp_url, enable_ai)
    if success:
        return {"success": True, "message": "Stream added successfully"}
    else:
        raise HTTPException(status_code=400, detail="Failed to add stream. Please check the RTSP URL and network connectivity.")

if __name__ == "__main__":
    # Create logs directory
    os.makedirs("logs", exist_ok=True)
    
    # Start the service
    logger.info("Starting Enhanced Fast CCTV Processing Service...")
    uvicorn.run(
        app,
        host="127.0.0.1",
        port=8088,
        log_level="info",
        access_log=True
    )
