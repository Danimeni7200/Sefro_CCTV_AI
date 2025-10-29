"""
Simple CCTV Service for Testing
"""

from fastapi import FastAPI
from fastapi.responses import StreamingResponse
import uvicorn
import cv2
import numpy as np
import io
import os
from datetime import datetime

app = FastAPI()

@app.get("/")
async def root():
    return {"message": "Simple CCTV Service Running"}

@app.get("/health")
async def health():
    return {"status": "healthy"}

@app.get("/discover")
async def discover(ip: str = "192.168.4.252", user: str = "admin", password: str = "test1234", brand: str = "reolink"):
    return {
        "candidates": [
            f"rtsp://{user}:{password}@{ip}:554/h264Preview_01_sub",
            f"rtsp://{user}:{password}@{ip}:554/h264Preview_01_main"
        ]
    }

@app.post("/add_stream")
async def add_stream(stream_id: str, rtsp_url: str, enable_ai: bool = True):
    """Add a stream to the service"""
    return {
        "success": True,
        "message": "Stream added successfully",
        "stream_id": stream_id,
        "rtsp_url": rtsp_url,
        "enable_ai": enable_ai
    }

@app.get("/stream/{stream_id}/info")
async def get_stream_info(stream_id: str):
    """Get stream information"""
    return {
        "stream_id": stream_id,
        "is_opened": True,
        "fps": 30,
        "frame_width": 640,
        "frame_height": 360,
        "timestamp": datetime.now().isoformat()
    }

@app.get("/stream/{stream_id}/frame")
async def get_stream_frame(stream_id: str):
    """Get a frame from the stream"""
    # For testing, return a simple test image
    import cv2
    import io
    
    # Create a test image
    img = cv2.imread("test_image.jpg") if os.path.exists("test_image.jpg") else None
    if img is None:
        # Create a simple test pattern
        img = np.zeros((360, 640, 3), dtype=np.uint8)
        cv2.putText(img, f"Stream: {stream_id}", (50, 180), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        cv2.putText(img, f"Time: {datetime.now().strftime('%H:%M:%S')}", (50, 220), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
    
    # Encode as JPEG
    _, buffer = cv2.imencode('.jpg', img)
    return StreamingResponse(io.BytesIO(buffer.tobytes()), media_type="image/jpeg")

if __name__ == "__main__":
    uvicorn.run(app, host="127.0.0.1", port=8089)
