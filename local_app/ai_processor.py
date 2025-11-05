"""
AI Processing Module for CCTV Recognition
High-performance object detection and recognition
"""

import cv2
import numpy as np
from ultralytics import YOLO
import torch
from typing import List, Dict, Tuple, Optional
import time
from loguru import logger
import asyncio
from concurrent.futures import ThreadPoolExecutor
import threading

class AIProcessor:
    """High-performance AI processor for CCTV recognition"""
    
    def __init__(self, model_path: str = "yolov8n.pt", device: str = "auto"):
        self.model_path = model_path
        self.device = self._get_optimal_device(device)
        self.model = None
        self.executor = ThreadPoolExecutor(max_workers=4)
        self.processing_queue = asyncio.Queue()
        self.results_cache = {}
        self.cache_lock = threading.Lock()
        
        # Performance metrics
        self.inference_times = []
        self.total_inferences = 0
        
        logger.info(f"AI Processor initialized with device: {self.device}")
    
    def _get_optimal_device(self, device: str) -> str:
        """Determine the optimal device for inference"""
        if device == "auto":
            if torch.cuda.is_available():
                return "cuda"
            elif hasattr(torch.backends, 'mps') and torch.backends.mps.is_available():
                return "mps"
            else:
                return "cpu"
        return device
    
    def load_model(self) -> bool:
        """Load the YOLO model"""
        try:
            self.model = YOLO(self.model_path)
            logger.info(f"Model loaded successfully: {self.model_path}")
            return True
        except Exception as e:
            logger.error(f"Failed to load model: {e}")
            return False
    
    def preprocess_frame(self, frame: np.ndarray) -> np.ndarray:
        """Preprocess frame for optimal inference"""
        # Resize if too large (maintain aspect ratio)
        height, width = frame.shape[:2]
        max_size = 1280
        
        if max(height, width) > max_size:
            scale = max_size / max(height, width)
            new_width = int(width * scale)
            new_height = int(height * scale)
            frame = cv2.resize(frame, (new_width, new_height), interpolation=cv2.INTER_LINEAR)
        
        return frame
    
    def detect_objects(self, frame: np.ndarray, confidence_threshold: float = 0.5) -> List[Dict]:
        """Detect objects in frame using YOLO"""
        if self.model is None:
            logger.warning("Model not loaded")
            return []
        
        # Add logging to check frame properties
        logger.debug(f"Processing frame with shape: {frame.shape if frame is not None else 'None'}")
        
        if frame is None or frame.size == 0:
            logger.warning("Empty or None frame provided for detection")
            return []
        
        start_time = time.time()
        
        try:
            # Preprocess frame
            processed_frame = self.preprocess_frame(frame)
            logger.debug(f"Preprocessed frame shape: {processed_frame.shape}")
            
            # Run inference
            results = self.model(processed_frame, conf=confidence_threshold, verbose=False)
            logger.debug(f"Model inference completed, results type: {type(results)}")
            
            # Process results
            detections = []
            for result in results:
                boxes = result.boxes
                if boxes is not None:
                    logger.debug(f"Found {len(boxes)} boxes in result")
                    for box in boxes:
                        # Get box coordinates
                        x1, y1, x2, y2 = box.xyxy[0].cpu().numpy()
                        confidence = box.conf[0].cpu().numpy()
                        class_id = int(box.cls[0].cpu().numpy())
                        class_name = self.model.names[class_id]
                        
                        detections.append({
                            "bbox": [int(x1), int(y1), int(x2), int(y2)],
                            "confidence": float(confidence),
                            "class_id": class_id,
                            "class_name": class_name,
                            "timestamp": time.time()
                        })
                else:
                    logger.debug("No boxes found in result")
            
            # Update performance metrics
            inference_time = time.time() - start_time
            self.inference_times.append(inference_time)
            self.total_inferences += 1
            
            # Keep only last 100 inference times for rolling average
            if len(self.inference_times) > 100:
                self.inference_times = self.inference_times[-100:]
            
            logger.debug(f"Detected {len(detections)} objects with confidence threshold {confidence_threshold}")
            return detections
            
        except Exception as e:
            logger.error(f"Error during object detection: {e}")
            import traceback
            logger.error(f"Traceback: {traceback.format_exc()}")
            return []
    
    def detect_license_plates(self, frame: np.ndarray) -> List[Dict]:
        """Specialized license plate detection"""
        # Use YOLO to detect vehicles first
        vehicle_detections = self.detect_objects(frame, confidence_threshold=0.3)
        
        # Filter for vehicles (car, truck, bus, motorcycle)
        vehicle_classes = [2, 3, 5, 7]  # COCO class IDs for vehicles
        vehicles = [det for det in vehicle_detections if det["class_id"] in vehicle_classes]
        
        license_plates = []
        for vehicle in vehicles:
            x1, y1, x2, y2 = vehicle["bbox"]
            
            # Extract vehicle region
            vehicle_roi = frame[y1:y2, x1:x2]
            
            # Look for license plate in vehicle region
            # This is a simplified approach - in production, use a dedicated LPR model
            plate_candidates = self._find_license_plate_candidates(vehicle_roi)
            
            for plate in plate_candidates:
                # Adjust coordinates to full frame
                plate["bbox"] = [
                    x1 + plate["bbox"][0],
                    y1 + plate["bbox"][1],
                    x1 + plate["bbox"][2],
                    y1 + plate["bbox"][3]
                ]
                plate["vehicle_bbox"] = vehicle["bbox"]
                plate["vehicle_confidence"] = vehicle["confidence"]
                license_plates.append(plate)
        
        return license_plates
    
    def _find_license_plate_candidates(self, vehicle_roi: np.ndarray) -> List[Dict]:
        """Find potential license plate regions in vehicle ROI"""
        candidates = []
        
        try:
            # Convert to grayscale
            gray = cv2.cvtColor(vehicle_roi, cv2.COLOR_BGR2GRAY)
            
            # Apply edge detection
            edges = cv2.Canny(gray, 50, 150)
            
            # Find contours
            contours, _ = cv2.findContours(edges, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            
            for contour in contours:
                # Get bounding rectangle
                x, y, w, h = cv2.boundingRect(contour)
                
                # Filter by aspect ratio (license plates are typically wider than tall)
                aspect_ratio = w / h
                if 2.0 <= aspect_ratio <= 6.0 and w > 50 and h > 10:
                    candidates.append({
                        "bbox": [x, y, x + w, y + h],
                        "confidence": 0.5,  # Placeholder confidence
                        "class_name": "license_plate",
                        "timestamp": time.time()
                    })
        
        except Exception as e:
            logger.error(f"Error finding license plate candidates: {e}")
        
        return candidates
    
    def draw_detections(self, frame: np.ndarray, detections: List[Dict]) -> np.ndarray:
        """Draw detection boxes on frame"""
        result_frame = frame.copy()
        
        for detection in detections:
            x1, y1, x2, y2 = detection["bbox"]
            confidence = detection["confidence"]
            class_name = detection["class_name"]
            
            # Draw bounding box
            cv2.rectangle(result_frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
            
            # Draw label
            label = f"{class_name}: {confidence:.2f}"
            label_size = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 2)[0]
            cv2.rectangle(result_frame, (x1, y1 - label_size[1] - 10), 
                         (x1 + label_size[0], y1), (0, 255, 0), -1)
            cv2.putText(result_frame, label, (x1, y1 - 5), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 0), 2)
        
        return result_frame
    
    def get_performance_stats(self) -> Dict:
        """Get performance statistics"""
        avg_inference_time = np.mean(self.inference_times) if self.inference_times else 0
        fps = 1.0 / avg_inference_time if avg_inference_time > 0 else 0
        
        return {
            "total_inferences": self.total_inferences,
            "average_inference_time": avg_inference_time,
            "estimated_fps": fps,
            "device": self.device,
            "model_loaded": self.model is not None
        }
    
    async def process_frame_async(self, frame: np.ndarray, detection_type: str = "objects") -> List[Dict]:
        """Asynchronously process a frame"""
        loop = asyncio.get_event_loop()
        
        if detection_type == "license_plates":
            result = await loop.run_in_executor(
                self.executor, 
                self.detect_license_plates, 
                frame
            )
        else:
            result = await loop.run_in_executor(
                self.executor, 
                self.detect_objects, 
                frame
            )
        
        return result

# Global AI processor instance
ai_processor = AIProcessor()

def initialize_ai_processor():
    """Initialize the AI processor"""
    return ai_processor.load_model()
