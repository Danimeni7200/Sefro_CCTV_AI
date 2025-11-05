"""
Optimized CCTV Service with WebSocket Streaming
High-performance local RTSP stream processing
"""

from fastapi import FastAPI, WebSocket, WebSocketDisconnect, HTTPException, Query
from fastapi.responses import StreamingResponse, JSONResponse
from fastapi.middleware.cors import CORSMiddleware
import uvicorn
import cv2
import numpy as np
import io
import os
import time
import threading
import queue
import asyncio
from datetime import datetime
from typing import Dict, List, Optional, Set

# Configure OpenCV for better RTSP performance
os.environ['OPENCV_FFMPEG_CAPTURE_OPTIONS'] = 'rtsp_transport;tcp|rtsp_flags;prefer_tcp|stimeout;60000000'

app = FastAPI(
    title="Optimized CCTV Service",
    description="High-performance local RTSP stream processing",
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

# Global stream processor
class OptimizedStreamProcessor:
    def __init__(self):
        self.streams: Dict[str, cv2.VideoCapture] = {}
        self.stream_urls: Dict[str, str] = {}
        self.latest_frames: Dict[str, np.ndarray] = {}
        self.frame_locks: Dict[str, threading.Lock] = {}
        self.processing_threads: Dict[str, threading.Thread] = {}
        self.running = True
        
        # Performance settings
        self.jpeg_quality = 40  # Lower quality for faster transmission
        self.max_fps = 30
        self.frame_queues: Dict[str, queue.Queue] = {}
        
        # WebSocket connections
        self.active_connections: Dict[str, Set[WebSocket]] = {}
        
        # Performance monitoring
        self.fps_counters: Dict[str, int] = {}
        self.last_fps_time: Dict[str, float] = {}
        
        # Reconnection tracking
        self.consecutive_failures: Dict[str, int] = {}
        self.max_consecutive_failures = 30
        
        print("OptimizedStreamProcessor initialized")
    
    def add_stream(self, stream_id: str, rtsp_url: str) -> bool:
        """Add a new RTSP stream"""
        try:
            # If stream already exists, return success
            if stream_id in self.streams:
                return True
            
            # Store URL for potential reconnection
            self.stream_urls[stream_id] = rtsp_url
            
            # Create lock for this stream
            self.frame_locks[stream_id] = threading.Lock()
            
            # Create frame queue for WebSocket streaming
            self.frame_queues[stream_id] = queue.Queue(maxsize=2)  # Only keep latest frames
            
            # Initialize active connections set
            self.active_connections[stream_id] = set()
            
            # Try to use hardware acceleration if available
            try:
                # Try CUDA first
                cap = cv2.VideoCapture(rtsp_url, cv2.CAP_FFMPEG)
                # Check if CUDA is available and set properties accordingly
                if cv2.__version__ >= '4.5.0' and hasattr(cv2, 'cuda') and cv2.cuda.getCudaEnabledDeviceCount() > 0:
                    print(f"CUDA acceleration available, using for stream {stream_id}")
                    # CUDA specific settings could be added here
            except:
                # Fall back to standard capture
                cap = cv2.VideoCapture(rtsp_url, cv2.CAP_FFMPEG)
            
            # Set optimized properties
            cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)  # Minimize buffer for lowest latency
            cap.set(cv2.CAP_PROP_FPS, 30)  # Target 30 FPS
            
            # Try to set timeout properties
            try:
                cap.set(cv2.CAP_PROP_OPEN_TIMEOUT_MSEC, 30000)  # 30 second open timeout
                cap.set(cv2.CAP_PROP_READ_TIMEOUT_MSEC, 5000)   # 5 second read timeout
            except:
                pass  # Some OpenCV versions don't support these properties
            
            if not cap.isOpened():
                print(f"Failed to open stream: {rtsp_url}")
                return False
            
            self.streams[stream_id] = cap
            self.fps_counters[stream_id] = 0
            self.last_fps_time[stream_id] = time.time()
            self.consecutive_failures[stream_id] = 0
            
            # Start processing thread for this stream
            thread = threading.Thread(
                target=self._process_stream,
                args=(stream_id,),
                daemon=True
            )
            thread.start()
            self.processing_threads[stream_id] = thread
            
            print(f"Added stream {stream_id}: {rtsp_url}")
            return True
            
        except Exception as e:
            print(f"Error adding stream {stream_id}: {e}")
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
            
            if stream_id in self.stream_urls:
                del self.stream_urls[stream_id]
            
            if stream_id in self.frame_queues:
                del self.frame_queues[stream_id]
            
            if stream_id in self.active_connections:
                # Close all WebSocket connections for this stream
                for connection in self.active_connections[stream_id]:
                    asyncio.create_task(connection.close())
                del self.active_connections[stream_id]
            
            print(f"Removed stream {stream_id}")
            return True
            
        except Exception as e:
            print(f"Error removing stream {stream_id}: {e}")
            return False
    
    def _reconnect_stream(self, stream_id: str) -> bool:
        """Reconnect a failed stream"""
        if stream_id not in self.stream_urls:
            print(f"Cannot reconnect {stream_id}: URL not stored")
            return False
        
        rtsp_url = self.stream_urls[stream_id]
        
        print(f"Attempting to reconnect stream {stream_id}...")
        
        try:
            # Release old capture
            if stream_id in self.streams:
                old_cap = self.streams[stream_id]
                old_cap.release()
                del self.streams[stream_id]
            
            # Wait a bit before reconnecting
            time.sleep(1)
            
            # Create new capture with same settings as add_stream
            cap = cv2.VideoCapture(rtsp_url, cv2.CAP_FFMPEG)
            cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
            cap.set(cv2.CAP_PROP_FPS, 30)
            
            if not cap.isOpened():
                print(f"Failed to reconnect stream {stream_id}")
                return False
            
            self.streams[stream_id] = cap
            self.consecutive_failures[stream_id] = 0
            
            print(f"Successfully reconnected stream {stream_id}")
            return True
            
        except Exception as e:
            print(f"Error reconnecting stream {stream_id}: {e}")
            return False
    
    def _process_stream(self, stream_id: str):
        """Process a single stream in a separate thread"""
        frame_count = 0
        last_successful_frame_time = time.time()
        reconnect_cooldown = 0
        last_frame_time = time.time()
        
        while self.running and stream_id in self.streams:
            try:
                cap = self.streams.get(stream_id)
                if cap is None:
                    time.sleep(0.5)
                    continue
                
                # Check if capture is still opened
                if not cap.isOpened():
                    if time.time() - reconnect_cooldown > 5:
                        if not self._reconnect_stream(stream_id):
                            reconnect_cooldown = time.time()
                    time.sleep(0.5)
                    continue
                
                # Check if too long since last successful frame
                if time.time() - last_successful_frame_time > 30:
                    if time.time() - reconnect_cooldown > 10:
                        if not self._reconnect_stream(stream_id):
                            reconnect_cooldown = time.time()
                        else:
                            last_successful_frame_time = time.time()
                    continue
                
                # Frame rate limiting
                current_time = time.time()
                if (current_time - last_frame_time) < (1.0 / self.max_fps):
                    time.sleep(0.001)
                    continue
                last_frame_time = current_time
                
                ret, frame = cap.read()
                if not ret or frame is None:
                    self.consecutive_failures[stream_id] = self.consecutive_failures.get(stream_id, 0) + 1
                    
                    if self.consecutive_failures[stream_id] >= self.max_consecutive_failures:
                        if time.time() - reconnect_cooldown > 10:
                            if not self._reconnect_stream(stream_id):
                                reconnect_cooldown = time.time()
                            else:
                                last_successful_frame_time = time.time()
                    
                    time.sleep(0.033)
                    continue
                
                # Successfully read frame - reset failure counter
                self.consecutive_failures[stream_id] = 0
                last_successful_frame_time = time.time()
                
                # Update latest frame with thread safety
                with self.frame_locks[stream_id]:
                    self.latest_frames[stream_id] = frame.copy()
                
                # Add frame to queue for WebSocket streaming
                try:
                    # Remove old frame if queue is full
                    if self.frame_queues[stream_id].full():
                        try:
                            self.frame_queues[stream_id].get_nowait()
                        except queue.Empty:
                            pass
                    
                    # Resize frame for better performance if needed
                    if frame.shape[1] > 1280:  # If width > 1280
                        scale = 1280 / frame.shape[1]
                        new_width = 1280
                        new_height = int(frame.shape[0] * scale)
                        frame = cv2.resize(frame, (new_width, new_height))
                    
                    # Encode frame as JPEG with lower quality
                    _, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, self.jpeg_quality])
                    
                    # Put encoded frame in queue
                    self.frame_queues[stream_id].put_nowait(buffer.tobytes())
                except queue.Full:
                    pass  # Skip frame if queue is full
                except Exception as e:
                    print(f"Error encoding frame: {e}")
                
                # Update FPS counter
                frame_count += 1
                current_time = time.time()
                if current_time - self.last_fps_time[stream_id] >= 1.0:
                    self.fps_counters[stream_id] = frame_count
                    frame_count = 0
                    self.last_fps_time[stream_id] = current_time
                
            except Exception as e:
                print(f"Error processing stream {stream_id}: {e}")
                time.sleep(0.5)
    
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
            "timestamp": datetime.now().isoformat(),
            "active_websocket_connections": len(self.active_connections.get(stream_id, set()))
        }
    
    def get_all_streams_info(self) -> List[Dict]:
        """Get information for all streams"""
        return [self.get_stream_info(stream_id) for stream_id in self.streams.keys()]
    
    async def register_websocket(self, stream_id: str, websocket: WebSocket):
        """Register a WebSocket connection for a stream"""
        if stream_id not in self.active_connections:
            self.active_connections[stream_id] = set()
        self.active_connections[stream_id].add(websocket)
    
    async def unregister_websocket(self, stream_id: str, websocket: WebSocket):
        """Unregister a WebSocket connection for a stream"""
        if stream_id in self.active_connections:
            self.active_connections[stream_id].discard(websocket)

# Global stream processor instance
stream_processor = OptimizedStreamProcessor()

@app.get("/")
async def root():
    """Root endpoint"""
    return {
        "service": "Optimized CCTV Service",
        "version": "2.0.0",
        "status": "running",
        "timestamp": datetime.now().isoformat()
    }

@app.get("/health")
async def health():
    """Health check endpoint"""
    import psutil
    return {
        "status": "healthy",
        "timestamp": datetime.now().isoformat(),
        "memory_usage": psutil.virtual_memory().percent,
        "cpu_usage": psutil.cpu_percent(),
        "active_streams": len(stream_processor.streams)
    }

@app.get("/discover")
async def discover(ip: str = "192.168.4.252", user: str = "admin", password: str = "test1234", brand: str = "reolink"):
    """Discover available cameras"""
    return {
        "candidates": [
            f"rtsp://{user}:{password}@{ip}:554/h264Preview_01_sub",
            f"rtsp://{user}:{password}@{ip}:554/h264Preview_01_main"
        ]
    }

from urllib.parse import unquote

@app.post("/add_stream")
async def add_stream(stream_id: str = Query(...), rtsp_url: str = Query(...), enable_ai: bool = Query(True)):
    """Add a stream to the service"""
    decoded_rtsp_url = unquote(rtsp_url)
    success = stream_processor.add_stream(stream_id, decoded_rtsp_url)
    if success:
        return {
            "success": True,
            "message": "Stream added successfully",
            "stream_id": stream_id,
            "rtsp_url": rtsp_url,
            "enable_ai": enable_ai
        }
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

@app.get("/stream/{stream_id}/info")
async def get_stream_info(stream_id: str):
    """Get detailed information about a stream"""
    info = stream_processor.get_stream_info(stream_id)
    if "error" in info:
        raise HTTPException(status_code=404, detail=info["error"])
    return info

@app.get("/stream/{stream_id}/frame")
async def get_stream_frame(stream_id: str, quality: int = None):
    """Get the latest frame from a stream as JPEG"""
    frame = stream_processor.get_latest_frame(stream_id)
    if frame is None:
        raise HTTPException(status_code=404, detail="Stream not found or no frame available")
    
    # Use requested quality or default
    jpeg_quality = quality if quality is not None else stream_processor.jpeg_quality
    
    # Encode frame as JPEG
    _, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, jpeg_quality])
    
    return StreamingResponse(
        io.BytesIO(buffer.tobytes()),
        media_type="image/jpeg",
        headers={"Cache-Control": "no-cache"}
    )

@app.websocket("/ws/stream/{stream_id}")
async def websocket_stream(websocket: WebSocket, stream_id: str):
    """WebSocket endpoint for streaming frames"""
    await websocket.accept()
    
    # Check if stream exists
    if stream_id not in stream_processor.streams:
        await websocket.send_text(f"Error: Stream {stream_id} not found")
        await websocket.close()
        return
    
    # Register WebSocket connection
    await stream_processor.register_websocket(stream_id, websocket)
    
    try:
        # Send initial stream info
        info = stream_processor.get_stream_info(stream_id)
        await websocket.send_json({"type": "info", "data": info})
        
        # Stream frames
        while True:
            # Check if stream still exists
            if stream_id not in stream_processor.streams:
                await websocket.send_text("Stream ended")
                break
            
            try:
                # Try to get frame from queue with timeout
                if stream_id in stream_processor.frame_queues:
                    try:
                        # Non-blocking get with timeout
                        frame_data = stream_processor.frame_queues[stream_id].get(timeout=1.0)
                        await websocket.send_bytes(frame_data)
                    except queue.Empty:
                        # No frame available, send heartbeat
                        await websocket.send_json({"type": "heartbeat"})
                else:
                    # Stream exists but no queue
                    await asyncio.sleep(0.1)
            except Exception as e:
                print(f"Error sending frame via WebSocket: {e}")
                await asyncio.sleep(0.1)
    
    except WebSocketDisconnect:
        # Client disconnected
        print(f"WebSocket client disconnected from stream {stream_id}")
    except Exception as e:
        print(f"WebSocket error: {e}")
    finally:
        # Unregister WebSocket connection
        await stream_processor.unregister_websocket(stream_id, websocket)

@app.get("/stream/{stream_id}/mjpeg")
async def mjpeg_stream(stream_id: str):
    """Stream as MJPEG (Motion JPEG)"""
    if stream_id not in stream_processor.streams:
        raise HTTPException(status_code=404, detail="Stream not found")
    
    async def generate_mjpeg():
        """Generate MJPEG stream"""
        # MJPEG header
        boundary = "frame"
        mjpeg_header = f"--{boundary}\r\nContent-Type: image/jpeg\r\n\r\n"
        
        while True:
            if stream_id not in stream_processor.streams:
                break
            
            frame = stream_processor.get_latest_frame(stream_id)
            if frame is not None:
                # Encode frame as JPEG
                _, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, stream_processor.jpeg_quality])
                frame_bytes = buffer.tobytes()
                
                # Yield MJPEG part
                yield (mjpeg_header + frame_bytes.decode('latin1') + "\r\n").encode('latin1')
            
            # Control frame rate
            await asyncio.sleep(1.0 / stream_processor.max_fps)
    
    return StreamingResponse(
        generate_mjpeg(),
        media_type="multipart/x-mixed-replace; boundary=frame",
        headers={"Cache-Control": "no-cache"}
    )

if __name__ == "__main__":
    # Create logs directory if it doesn't exist
    os.makedirs("logs", exist_ok=True)
    
    # Start the service
    print("Starting Optimized CCTV Service...")
    uvicorn.run(
        app,
        host="127.0.0.1",
        port=8091,
        log_level="info"
    )
