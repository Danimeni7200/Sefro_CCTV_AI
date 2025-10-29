import cv2
import requests
import time
import os
import json
from datetime import datetime

# Camera information
CAMERA_IP = "192.168.4.252"
USERNAME = "admin"
PASSWORD = "test1234"
CAMERA_ID = "CAM01"

# AI Service information
AI_SERVICE_URL = "http://127.0.0.1:8000"

# Reolink camera RTSP URL (the one that worked in our test)
RTSP_URL = f"rtsp://{USERNAME}:{PASSWORD}@{CAMERA_IP}:554/h264Preview_01_sub"

# Processing settings
FRAME_INTERVAL = 1.0  # Process one frame per second
MAX_RETRIES = 3

def send_frame_to_ai(frame, camera_id):
    """Send a frame to the AI service for processing"""
    try:
        # Save frame to temporary file
        temp_file = "temp_frame.jpg"
        cv2.imwrite(temp_file, frame)
        
        # Send frame to AI service
        with open(temp_file, 'rb') as f:
            files = {'image': (temp_file, f, 'image/jpeg')}
            data = {'camera_id': camera_id}
            
            response = requests.post(
                f"{AI_SERVICE_URL}/infer",
                files=files,
                data=data,
                timeout=10
            )
        
        # Clean up temporary file
        os.remove(temp_file)
        
        if response.status_code == 200:
            return response.json()
        else:
            print(f"AI service error: {response.status_code} - {response.text}")
            return None
            
    except Exception as e:
        print(f"Error sending frame to AI service: {e}")
        return None

def process_camera_stream():
    """Continuously process frames from the camera"""
    print("Starting camera processing...")
    print(f"Connecting to: {RTSP_URL}")
    
    # Open camera stream
    cap = cv2.VideoCapture(RTSP_URL)
    
    if not cap.isOpened():
        print("Failed to open camera stream")
        return
    
    print("Camera connected successfully!")
    print("Press Ctrl+C to stop")
    
    frame_count = 0
    last_process_time = 0
    
    try:
        while True:
            # Read frame
            ret, frame = cap.read()
            if not ret:
                print("Failed to read frame from camera")
                time.sleep(1)
                continue
            
            frame_count += 1
            
            # Process frame at specified interval
            current_time = time.time()
            if current_time - last_process_time >= FRAME_INTERVAL:
                print(f"\nProcessing frame {frame_count}...")
                
                # Send to AI service
                result = send_frame_to_ai(frame, CAMERA_ID)
                
                if result:
                    print(f"  Plate detected: {result['plate_text']}")
                    print(f"  Confidence: {result['confidence']:.2f}")
                    print(f"  Vehicle color: {result['vehicle_color']}")
                else:
                    print("  No result from AI service")
                
                last_process_time = current_time
            
            # Display frame (optional - comment out if you don't want to see the video)
            cv2.imshow('Camera Feed', frame)
            
            # Break on 'q' key press or window close
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
                
    except KeyboardInterrupt:
        print("\nStopping camera processing...")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        cap.release()
        cv2.destroyAllWindows()

def test_ai_service():
    """Test if AI service is healthy"""
    print("Testing AI service health...")
    try:
        response = requests.get(f"{AI_SERVICE_URL}/healthz", timeout=5)
        if response.status_code == 200:
            print("✓ AI service is healthy")
            return True
        else:
            print(f"✗ AI service health check failed: {response.status_code}")
            return False
    except Exception as e:
        print(f"✗ Error connecting to AI service: {e}")
        return False

if __name__ == "__main__":
    print("Sefro CCTV AI Platform - Continuous Camera Processor")
    print("=" * 60)
    
    # Test AI service
    if not test_ai_service():
        print("Cannot proceed without healthy AI service")
        exit(1)
    
    print()
    
    # Start processing
    process_camera_stream()
    
    print("Camera processing stopped.")