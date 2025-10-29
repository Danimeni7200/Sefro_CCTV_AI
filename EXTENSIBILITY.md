# LPR System Extensibility Guide

This document outlines how to extend the License Plate Recognition system with additional features.

## Architecture Overview

The system follows a hybrid-tiered architecture:
- **Edge Device**: C++ client for camera capture and image processing
- **Local Server**: Python FastAPI service for AI inference and logging
- **Cloud**: FastAPI backend + Next.js frontend for analytics and management

## Extension Points

### 1. Vehicle Type Detection

**Location**: `packages/ai_service/main.py`

**Current State**: Placeholder in `LPRResult` model
```python
vehicle_type: Optional[str] = None
```

**Extension Steps**:
1. Add vehicle detection model (YOLOv8, EfficientDet, etc.)
2. Update `dummy_detect_and_ocr()` function to include vehicle classification
3. Add vehicle type labels (car, truck, motorcycle, bus, etc.)
4. Update confidence scoring for multi-class detection

**Example Implementation**:
```python
def detect_vehicle_type(image_bytes: bytes) -> tuple[str, float]:
    # Load vehicle detection model
    # Run inference
    # Return vehicle_type, confidence
    return "car", 0.85
```

### 2. Vehicle Color Detection

**Location**: `packages/ai_service/main.py`

**Current State**: Placeholder in `LPRResult` model
```python
vehicle_color: Optional[str] = None
```

**Extension Steps**:
1. Add color classification model or use color histogram analysis
2. Define color categories (red, blue, white, black, silver, etc.)
3. Update inference pipeline to include color detection
4. Handle lighting variations and shadows

**Example Implementation**:
```python
def detect_vehicle_color(image_bytes: bytes, bbox: BBox) -> tuple[str, float]:
    # Crop vehicle region using bbox
    # Analyze color distribution
    # Return dominant_color, confidence
    return "white", 0.92
```

### 3. Anomaly Detection

**Location**: `packages/ai_service/main.py` + `packages/cloud_api/main.py`

**Extension Steps**:
1. Add anomaly detection service
2. Define anomaly types (suspicious behavior, unusual patterns, etc.)
3. Implement real-time monitoring
4. Add alert system for flagged events

**Example Implementation**:
```python
def detect_anomalies(lpr_results: List[LPRResult]) -> List[Anomaly]:
    anomalies = []
    # Check for repeated low confidence
    # Check for unusual plate patterns
    # Check for time-based anomalies
    return anomalies
```

### 4. Cloud Sync with Anonymization

**Location**: `packages/ai_service/main.py`

**Current State**: Basic logging to local JSONL file

**Extension Steps**:
1. Add selective cloud sync based on confidence thresholds
2. Implement data anonymization (hash plate numbers, remove PII)
3. Add retry logic and offline queuing
4. Implement data retention policies

**Example Implementation**:
```python
def anonymize_lpr_result(result: LPRResult) -> LPRResult:
    # Hash plate text
    # Remove or hash camera ID
    # Remove timestamps or add noise
    return anonymized_result

def should_sync_to_cloud(result: LPRResult) -> bool:
    # Check confidence threshold
    # Check for flagged anomalies
    # Check sync frequency limits
    return result.confidence > 0.8 or result.vehicle_type == "suspicious"
```

### 5. Real-time Analytics

**Location**: `packages/cloud_api/main.py` + `packages/cloud_web/`

**Extension Steps**:
1. Add WebSocket support for real-time updates
2. Implement streaming analytics (traffic patterns, peak hours)
3. Add dashboard widgets for live monitoring
4. Implement alert notifications

**Example Implementation**:
```python
@app.websocket("/ws/analytics")
async def websocket_analytics(websocket: WebSocket):
    await websocket.accept()
    while True:
        # Send real-time analytics data
        await websocket.send_json(analytics_data)
```

### 6. Model Retraining Pipeline

**Location**: `packages/cloud_api/main.py`

**Extension Steps**:
1. Add data collection endpoint for model feedback
2. Implement model versioning and A/B testing
3. Add retraining pipeline with new data
4. Implement model deployment automation

**Example Implementation**:
```python
@app.post("/feedback/lpr")
async def submit_feedback(feedback: LPRFeedback):
    # Store feedback for retraining
    # Trigger retraining if enough data collected
    # Update model version
    pass
```

## Configuration Extensions

### Environment Variables

Add new configuration options to respective `.env.example` files:

```bash
# AI Service
VEHICLE_DETECTION_ENABLED=true
VEHICLE_DETECTION_MODEL_PATH=models/vehicle_model.onnx
COLOR_DETECTION_ENABLED=true
ANOMALY_DETECTION_ENABLED=false

# Cloud API
ANALYTICS_RETENTION_DAYS=365
REALTIME_ANALYTICS_ENABLED=true
MODEL_RETRAINING_ENABLED=false
```

### Database Schema Extensions

For cloud database, add new tables:

```sql
-- Vehicle types
CREATE TABLE vehicle_types (
    id INTEGER PRIMARY KEY,
    type_name VARCHAR(50) NOT NULL,
    description TEXT
);

-- Anomalies
CREATE TABLE anomalies (
    id INTEGER PRIMARY KEY,
    camera_id VARCHAR(50),
    anomaly_type VARCHAR(100),
    confidence FLOAT,
    timestamp DATETIME,
    details JSON
);

-- Model versions
CREATE TABLE model_versions (
    id INTEGER PRIMARY KEY,
    model_type VARCHAR(50),
    version VARCHAR(20),
    accuracy FLOAT,
    created_at DATETIME,
    is_active BOOLEAN
);
```

## Testing Extensions

### Unit Tests

Add tests for new functionality:

```python
def test_vehicle_type_detection():
    # Test vehicle type detection accuracy
    pass

def test_color_detection():
    # Test color detection under different lighting
    pass

def test_anomaly_detection():
    # Test anomaly detection logic
    pass
```

### Integration Tests

Add end-to-end tests:

```python
def test_full_pipeline_with_extensions():
    # Test complete pipeline with all extensions
    pass
```

## Performance Considerations

### Optimization Strategies

1. **Model Optimization**: Use ONNX Runtime for faster inference
2. **Caching**: Implement Redis for frequently accessed data
3. **Batch Processing**: Process multiple images simultaneously
4. **GPU Acceleration**: Use CUDA for model inference

### Monitoring

Add performance monitoring:

```python
import time
from functools import wraps

def monitor_performance(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        start_time = time.time()
        result = func(*args, **kwargs)
        execution_time = time.time() - start_time
        # Log performance metrics
        return result
    return wrapper
```

## Security Considerations

### Data Privacy

1. **Anonymization**: Implement proper data anonymization
2. **Encryption**: Encrypt sensitive data in transit and at rest
3. **Access Control**: Implement role-based access control
4. **Audit Logging**: Log all data access and modifications

### Compliance

1. **GDPR**: Implement data protection measures
2. **Retention Policies**: Implement data retention and deletion
3. **Consent Management**: Add consent tracking for data collection

## Deployment Extensions

### Docker Configuration

Update Dockerfiles for new dependencies:

```dockerfile
# AI Service Dockerfile
FROM python:3.10-slim
RUN apt-get update && apt-get install -y \
    libgl1-mesa-glx \
    libglib2.0-0 \
    libsm6 \
    libxext6 \
    libxrender-dev \
    libgomp1
COPY requirements.txt .
RUN pip install -r requirements.txt
COPY . .
CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--port", "8000"]
```

### Kubernetes Deployment

Add Kubernetes manifests for scaling:

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: lpr-ai-service
spec:
  replicas: 3
  selector:
    matchLabels:
      app: lpr-ai-service
  template:
    metadata:
      labels:
        app: lpr-ai-service
    spec:
      containers:
      - name: ai-service
        image: lpr-ai-service:latest
        ports:
        - containerPort: 8000
        resources:
          requests:
            memory: "512Mi"
            cpu: "500m"
          limits:
            memory: "1Gi"
            cpu: "1000m"
```

## Conclusion

This extensibility guide provides a roadmap for enhancing the LPR system with additional features while maintaining the existing architecture. Each extension point is designed to be modular and independent, allowing for incremental development and deployment.

For questions or contributions, please refer to the main README.md file.

