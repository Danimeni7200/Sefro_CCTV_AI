import cv2
import time

# Reolink camera RTSP URLs
rtsp_urls = [
    "rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_sub",
    "rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_main",
    "rtsp://admin:test1234@192.168.4.252:554//Streaming/Channels/101",
    "rtsp://admin:test1234@192.168.4.252:554//Streaming/Channels/102"
]

def test_camera_stream(url):
    print(f"Testing: {url}")
    try:
        # Create VideoCapture object
        cap = cv2.VideoCapture(url)
        
        # Check if camera opened successfully
        if not cap.isOpened():
            print(f"  Failed to open {url}")
            return False
            
        # Try to read a frame
        ret, frame = cap.read()
        if ret:
            print(f"  Success! Camera is accessible.")
            print(f"  Frame size: {frame.shape}")
            cap.release()
            return True
        else:
            print(f"  Failed to read frame from {url}")
            cap.release()
            return False
            
    except Exception as e:
        print(f"  Error: {e}")
        return False

if __name__ == "__main__":
    print("Testing Reolink camera connections...")
    print("=" * 50)
    
    success_count = 0
    for url in rtsp_urls:
        if test_camera_stream(url):
            success_count += 1
        print()
    
    print("=" * 50)
    print(f"Test completed. {success_count}/{len(rtsp_urls)} URLs worked.")
    
    if success_count == 0:
        print("\nTroubleshooting tips:")
        print("1. Check if the camera is powered on")
        print("2. Verify the IP address is correct")
        print("3. Ensure the username and password are correct")
        print("4. Check if port 554 is open on the camera")
        print("5. Verify the camera's RTSP service is enabled")
        print("6. Try accessing the camera's web interface at http://192.168.4.252")