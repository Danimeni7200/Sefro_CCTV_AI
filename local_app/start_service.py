"""
Startup script for the Fast CCTV Processing Service
Handles installation and startup
"""

import subprocess
import sys
import os
from pathlib import Path

def install_requirements():
    """Install required packages"""
    print("Installing required packages...")
    try:
        subprocess.check_call([sys.executable, "-m", "pip", "install", "-r", "requirements.txt"])
        print("[OK] Requirements installed successfully")
        return True
    except subprocess.CalledProcessError as e:
        print(f"[ERROR] Failed to install requirements: {e}")
        return False

def download_yolo_model():
    """Download YOLO model if not present"""
    model_path = "yolov8n.pt"
    if not os.path.exists(model_path):
        print("Downloading YOLO model...")
        try:
            from ultralytics import YOLO
            model = YOLO('yolov8n.pt')  # This will download the model
            print("[OK] YOLO model downloaded successfully")
        except Exception as e:
            print(f"[ERROR] Failed to download YOLO model: {e}")
            return False
    else:
        print("[OK] YOLO model already exists")
    return True

def create_directories():
    """Create necessary directories"""
    directories = ["logs", "models", "temp"]
    for directory in directories:
        os.makedirs(directory, exist_ok=True)
        print(f"[OK] Created directory: {directory}")

def main():
    """Main startup function"""
    print("=== Fast CCTV Processing Service Startup ===")
    
    # Create directories
    create_directories()
    
    # Install requirements
    if not install_requirements():
        print("Failed to install requirements. Exiting.")
        return False
    
    # Download YOLO model
    if not download_yolo_model():
        print("Failed to download YOLO model. Exiting.")
        return False
    
    print("\n[OK] Setup complete! Starting service...")
    
    # Start the service
    try:
        from enhanced_main import app
        import uvicorn
        
        uvicorn.run(
            app,
            host="127.0.0.1",
            port=8086,
            log_level="info"
        )
    except KeyboardInterrupt:
        print("\nService stopped by user")
    except Exception as e:
        print(f"Error starting service: {e}")

if __name__ == "__main__":
    main()
