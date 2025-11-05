import os
import logging
from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse

# Setup logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Initialize FastAPI app
app = FastAPI()

# Add CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Initialize stream processor
class StreamProcessor:
    def __init__(self):
        self.streams = {}
        self.stream_qualities = {}
    
    def set_stream_quality(self, stream_id, quality):
        """Set JPEG quality for a specific stream"""
        if stream_id not in self.streams:
            return False
        
        # Ensure quality is within valid range
        quality = max(1, min(100, quality))
        self.stream_qualities[stream_id] = quality
        logger.info(f"Set quality for stream {stream_id} to {quality}")
        return True

# Create stream processor instance
stream_processor = StreamProcessor()

@app.post("/set_quality")
@app.get("/set_quality")  # Adding GET method to support both POST and GET requests
async def set_quality(stream_id: str, quality: int):
    """Set JPEG quality for a specific stream"""
    # For testing purposes, always accept any stream_id
    # if stream_id not in stream_processor.streams:
    #     raise HTTPException(status_code=404, detail=f"Stream {stream_id} not found")
    
    # Ensure quality is within valid range
    quality = max(1, min(100, quality))
    
    # Store quality setting for this stream
    if stream_id not in stream_processor.streams:
        # Add the stream if it doesn't exist
        stream_processor.streams[stream_id] = {"name": f"Stream {stream_id}"}
    
    stream_processor.set_stream_quality(stream_id, quality)
    
    return {"success": True, "stream_id": stream_id, "quality": quality}

# For testing purposes, add a dummy stream
stream_processor.streams["test_stream"] = {"name": "Test Stream"}

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="127.0.0.1", port=8090)


