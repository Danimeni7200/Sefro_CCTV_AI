#!/usr/bin/env python3
"""
Simple test script to verify streaming concept
"""

import cv2
import threading
import time
import requests
from http.server import HTTPServer, BaseHTTPRequestHandler
import json
from urllib.parse import urlparse, parse_qs

# Global variables to store frames
frames = {}
frame_locks = {}

class StreamHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path.startswith('/stream/'):
            self.handle_stream_request()
        else:
            self.send_error(404)
            
    def handle_stream_request(self):
        # Extract stream ID
        stream_id = self.path[len('/stream/'):]
        if not stream_id:
            self.send_error(400)
            return
            
        # Check if we have a frame for this stream
        if stream_id not in frames:
            self.send_error(404)
            return
            
        # Get the frame
        with frame_locks.get(stream_id, threading.Lock()):
            frame_data = frames.get(stream_id)
            
        if not frame_data:
            self.send_error(404)
            return
            
        # Send the frame
        self.send_response(200)
        self.send_header('Content-Type', 'image/jpeg')
        self.send_header('Content-Length', str(len(frame_data)))
        self.end_headers()
        self.wfile.write(frame_data)

def capture_frames(stream_id, rtsp_url):
    """Capture frames from RTSP stream and store them"""
    if stream_id not in frame_locks:
        frame_locks[stream_id] = threading.Lock()
        
    cap = cv2.VideoCapture(rtsp_url)
    if not cap.isOpened():
        print(f"Failed to open {rtsp_url}")
        return
        
    print(f"Started capturing stream {stream_id}")
    
    while True:
        ret, frame = cap.read()
        if not ret:
            time.sleep(1)
            continue
            
        # Convert frame to JPEG
        _, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 80])
        
        # Store the frame
        with frame_locks[stream_id]:
            frames[stream_id] = buffer.tobytes()
            
        time.sleep(0.1)  # ~10 FPS

def main():
    # Start the HTTP server in a separate thread
    server = HTTPServer(('127.0.0.1', 8089), StreamHandler)
    server_thread = threading.Thread(target=server.serve_forever, daemon=True)
    server_thread.start()
    print("Streaming server started on http://127.0.0.1:8089")
    
    # For testing, let's simulate adding a stream
    # In a real implementation, this would be done via HTTP endpoints
    stream_id = "test_camera"
    rtsp_url = "rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_sub"
    
    # Start capturing frames
    capture_thread = threading.Thread(target=capture_frames, args=(stream_id, rtsp_url), daemon=True)
    capture_thread.start()
    
    print(f"Added stream {stream_id} with URL {rtsp_url}")
    print("You can view the stream at: http://127.0.0.1:8089/stream/test_camera")
    print("Press Ctrl+C to stop")
    
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Stopping...")
        server.shutdown()

if __name__ == "__main__":
    main()