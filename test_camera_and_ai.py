import cv2
import requests
import time
import os

# Camera information
CAMERA_IP = "192.168.4.252"
USERNAME = "admin"
PASSWORD = "test1234"
CAMERA_ID = "CAM01"

# AI Service information
AI_SERVICE_URL = "http://127.0.0.1:8000"

# Reolink camera RTSP URLs
rtsp_urls = [
    f"rtsp://{USERNAME}:{PASSWORD}@{CAMERA_IP}:554/h264Preview_01_sub",
    f"rtsp://{USERNAME}:{PASSWORD}@{CAMERA_IP}:554/h264Preview_01_main",
    f"rtsp://{USERNAME}:{PASSWORD}@{CAMERA_IP}:554//Streaming/Channels/101",
    f"rtsp://{USERNAME}:{PASSWORD}@{CAMERA_IP}:554//Streaming/Channels/102"
]

def test_camera_connection():
    """Test connection to the camera and find a working RTSP URL"""
    print("Testing camera connections...")
    working_url = None
    
    for url in rtsp_urls:
        print(f"Trying: {url}")
        try:
            cap = cv2.VideoCapture(url)
            if cap.isOpened():
                ret, frame = cap.read()
                if ret:
                    print(f"  Success! Camera is accessible.")
                    print(f"  Frame size: {frame.shape}")
                    working_url = url
                    cap.release()
                    break
                else:
                    print(f"  Failed to read frame from {url}")
            else:
                print(f"  Failed to open {url}")
            cap.release()
        except Exception as e:
            print(f"  Error: {e}")
    
    return working_url

def capture_and_process_frame(rtsp_url):
    """Capture a frame from the camera and send it to the AI service"""
    print(f"\nCapturing frame from: {rtsp_url}")
    
    try:
        # Capture frame from camera
        cap = cv2.VideoCapture(rtsp_url)
        if not cap.isOpened():
            print("Failed to open camera stream")
            return False
            
        ret, frame = cap.read()
        if not ret:
            print("Failed to read frame from camera")
            cap.release()
            return False
            
        cap.release()
        print("Frame captured successfully")
        
        # Save frame to temporary file
        temp_file = "temp_frame.jpg"
        cv2.imwrite(temp_file, frame)
        print(f"Frame saved to {temp_file}")
        
        # Send frame to AI service
        print("Sending frame to AI service...")
        with open(temp_file, 'rb') as f:
            files = {'image': (temp_file, f, 'image/jpeg')}
            data = {'camera_id': CAMERA_ID}
            
            response = requests.post(
                f"{AI_SERVICE_URL}/infer",
                files=files,
                data=data
            )
            
        # Clean up temporary file
        os.remove(temp_file)
        
        if response.status_code == 200:
            result = response.json()
            print("AI service response:")
            print(f"  Plate: {result['plate_text']}")
            print(f"  Confidence: {result['confidence']}")
            print(f"  Vehicle color: {result['vehicle_color']}")
            return True
        else:
            print(f"AI service error: {response.status_code} - {response.text}")
            return False
            
    except Exception as e:
        print(f"Error: {e}")
        return False

def test_ai_service_health():
    """Test if AI service is healthy"""
    print("Testing AI service health...")
    try:
        response = requests.get(f"{AI_SERVICE_URL}/healthz")
        if response.status_code == 200:
            print("AI service is healthy")
            return True
        else:
            print(f"AI service health check failed: {response.status_code}")
            return False
    except Exception as e:
        print(f"Error connecting to AI service: {e}")
        return False

if __name__ == "__main__":
    print("Camera and AI Service Test")
    print("=" * 50)
    
    # Test AI service health
    if not test_ai_service_health():
        print("Cannot proceed without healthy AI service")
        exit(1)
    
    # Test camera connection
    working_url = test_camera_connection()
    if not working_url:
        print("\nNo working camera URLs found. Troubleshooting tips:")
        print("1. Check if the camera is powered on")
        print("2. Verify the IP address is correct")
        print("3. Ensure the username and password are correct")
        print("4. Check if port 554 is open on the camera")
        print("5. Verify the camera's RTSP service is enabled")
        print("6. Try accessing the camera's web interface at http://192.168.4.252")
        exit(1)
    
    # Capture and process frame
    print(f"\nUsing working URL: {working_url}")
    if capture_and_process_frame(working_url):
        print("\nTest completed successfully!")
    else:
        print("\nTest failed!")