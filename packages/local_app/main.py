"""
Fast Python CCTV Processing Service
High-performance RTSP stream processing with AI recognition
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

# Configure logging
logger.add("logs/cctv_service.log", rotation="10 MB", level="INFO")

app = FastAPI(
    title="Fast CCTV Processing Service",
    description="High-performance RTSP stream processing with AI recognition",
    version="1.0.0"
)

# CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

class StreamProcessor:
    """High-performance stream processor with AI recognition"""
    
    def __init__(self):
        self.streams: Dict[str, cv2.VideoCapture] = {}
        self.latest_frames: Dict[str, np.ndarray] = {}
        self.frame_locks: Dict[str, threading.Lock] = {}
        self.processing_threads: Dict[str, threading.Thread] = {}
        self.running = True
        
        # Performance monitoring
        self.fps_counters: Dict[str, int] = {}
        self.last_fps_time: Dict[str, float] = {}
        
        logger.info("StreamProcessor initialized")
    
    def add_stream(self, stream_id: str, rtsp_url: str) -> bool:
        """Add a new RTSP stream"""
        try:
            # Create lock for this stream
            self.frame_locks[stream_id] = threading.Lock()
            
            # Open RTSP stream with optimized settings
            cap = cv2.VideoCapture(rtsp_url)
            cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)  # Minimize buffer
            cap.set(cv2.CAP_PROP_FPS, 30)  # Set FPS
            cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1920)  # Set resolution
            cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 1080)
            
            if not cap.isOpened():
                logger.error(f"Failed to open stream: {rtsp_url}")
                return False
            
            self.streams[stream_id] = cap
            self.fps_counters[stream_id] = 0
            self.last_fps_time[stream_id] = time.time()
            
            # Start processing thread for this stream
            thread = threading.Thread(
                target=self._process_stream,
                args=(stream_id,),
                daemon=True
            )
            thread.start()
            self.processing_threads[stream_id] = thread
            
            logger.info(f"Added stream {stream_id}: {rtsp_url}")
            return True
            
        except Exception as e:
            logger.error(f"Error adding stream {stream_id}: {e}")
            return False
    
    def remove_stream(self, stream_id: str) -> bool:
        """Remove a stream"""
        try:
            if stream_id in self.streams:
                self.streams[stream_id].release()
                del self.streams[stream_id]
            
            if stream_id in self.latest_frames:
                del self.latest_frames[stream_id]
            
            if stream_id in self.frame_locks:
                del self.frame_locks[stream_id]
            
            if stream_id in self.processing_threads:
                del self.processing_threads[stream_id]
            
            logger.info(f"Removed stream {stream_id}")
            return True
            
        except Exception as e:
            logger.error(f"Error removing stream {stream_id}: {e}")
            return False
    
    def _process_stream(self, stream_id: str):
        """Process a single stream in a separate thread"""
        cap = self.streams[stream_id]
        frame_count = 0
        
        while self.running and stream_id in self.streams:
            try:
                ret, frame = cap.read()
                if not ret or frame is None:
                    logger.warning(f"Failed to read frame from {stream_id}")
                    time.sleep(0.1)
                    continue
                
                # Update latest frame with thread safety
                with self.frame_locks[stream_id]:
                    self.latest_frames[stream_id] = frame.copy()
                
                # Update FPS counter
                frame_count += 1
                current_time = time.time()
                if current_time - self.last_fps_time[stream_id] >= 1.0:
                    self.fps_counters[stream_id] = frame_count
                    frame_count = 0
                    self.last_fps_time[stream_id] = current_time
                
                # Small delay to prevent excessive CPU usage
                time.sleep(0.01)
                
            except Exception as e:
                logger.error(f"Error processing stream {stream_id}: {e}")
                time.sleep(0.1)
    
    def get_latest_frame(self, stream_id: str) -> Optional[np.ndarray]:
        """Get the latest frame from a stream"""
        if stream_id not in self.frame_locks:
            return None
        
        with self.frame_locks[stream_id]:
            return self.latest_frames.get(stream_id, None).copy() if stream_id in self.latest_frames else None
    
    def get_stream_info(self, stream_id: str) -> Dict:
        """Get stream information and performance metrics"""
        if stream_id not in self.streams:
            return {"error": "Stream not found"}
        
        cap = self.streams[stream_id]
        return {
            "stream_id": stream_id,
            "is_opened": cap.isOpened(),
            "fps": self.fps_counters.get(stream_id, 0),
            "frame_width": int(cap.get(cv2.CAP_PROP_FRAME_WIDTH)),
            "frame_height": int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT)),
            "timestamp": datetime.now().isoformat()
        }
    
    def get_all_streams_info(self) -> List[Dict]:
        """Get information for all streams"""
        return [self.get_stream_info(stream_id) for stream_id in self.streams.keys()]

# Global stream processor instance
stream_processor = StreamProcessor()

@app.get("/")
async def root():
    """Root endpoint"""
    return {
        "service": "Fast CCTV Processing Service",
        "version": "1.0.0",
        "status": "running",
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
        "active_streams": len(stream_processor.streams)
    }

@app.post("/add_stream")
async def add_stream(stream_id: str, rtsp_url: str):
    """Add a new RTSP stream"""
    if stream_id in stream_processor.streams:
        return {"success": False, "message": "Stream already exists"}
    
    success = stream_processor.add_stream(stream_id, rtsp_url)
    if success:
        return {"success": True, "message": "Stream added successfully"}
    else:
        raise HTTPException(status_code=400, detail="Failed to add stream")

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
async def get_stream_frame(stream_id: str):
    """Get the latest frame from a stream as JPEG"""
    frame = stream_processor.get_latest_frame(stream_id)
    if frame is None:
        raise HTTPException(status_code=404, detail="Stream not found or no frame available")
    
    # Encode frame as JPEG
    _, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 85])
    
    return StreamingResponse(
        io.BytesIO(buffer.tobytes()),
        media_type="image/jpeg",
        headers={"Cache-Control": "no-cache"}
    )

@app.get("/stream/{stream_id}/info")
async def get_stream_info(stream_id: str):
    """Get detailed information about a stream"""
    info = stream_processor.get_stream_info(stream_id)
    if "error" in info:
        raise HTTPException(status_code=404, detail=info["error"])
    return info

@app.get("/discover")
async def discover_cameras():
    """Discover available cameras (placeholder for now)"""
    # This would typically scan the network for RTSP cameras
    # For now, return some example URLs
    return {
        "cameras": [
            {
                "id": "camera_1",
                "name": "Main Camera",
                "url": "rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_sub",
                "status": "available"
            }
        ]
    }

if __name__ == "__main__":
    # Create logs directory
    os.makedirs("logs", exist_ok=True)
    
    # Start the service
    logger.info("Starting Fast CCTV Processing Service...")
    uvicorn.run(
        app,
        host="127.0.0.1",
        port=8086,
        log_level="info",
        access_log=True
    )
