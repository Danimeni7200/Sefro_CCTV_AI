# Running the Sefro CCTV AI Platform

This document explains how to run each component of the Sefro CCTV AI Platform and their dependencies.

## Component Overview

The platform consists of 5 main components:

1. **AI Service** (`@ai_service`) - Python AI inference service
2. **Cloud API** (`@cloud_api`) - Python backend API
3. **Cloud Web** (`@cloud_web`) - Next.js web frontend
4. **Electron App** (`@electron_app`) - Desktop application
5. **Local C++ Client** (`@local_cpp`) - C++ video processing pipeline

## Component Dependencies

```
Cloud Web ───► Cloud API ◄─── AI Service ◄─── Local C++ Client
                 ▲
                 │
            Electron App
```

- **Cloud Web** requires **Cloud API** to be running
- **AI Service** can run independently but is used by **Local C++ Client**
- **Local C++ Client** requires **AI Service** to be running
- **Electron App** can communicate with both **AI Service** and **Cloud API**

## Quick Start Scripts

For easier startup, the platform includes the following scripts:

- `start-essential.bat` / `start-essential.ps1` - Starts the essential services (Cloud API, AI Service, Cloud Web)
- `start-electron.bat` / `start-electron.ps1` - Starts the Electron desktop application
- `start-full-system.bat` / `start-full-system.ps1` - Starts all services including the Discovery Service for camera discovery in Electron app
- `start-full-system-with-cpp.bat` - Starts all services including the Discovery Service and provides information about the C++ client

## 1. AI Service (@ai_service)

### Description
The AI Service is a Python FastAPI application that performs license plate recognition using machine learning models.

### Dependencies
- Python 3.10+
- Dependencies listed in `requirements.txt`

### How to Run

1. Navigate to the AI service directory:
   ```bash
   cd packages/ai_service
   ```

2. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```

3. Run the service:
   ```bash
   python main.py
   ```
   
   Or using uvicorn directly:
   ```bash
   uvicorn main:app --host 0.0.0.0 --port 8000
   ```

### Configuration
The service can be configured using environment variables:
- `CONFIDENCE_THRESHOLD` - Minimum confidence for results (default: 0.7)
- `CLOUD_API_URL` - URL of the Cloud API service (default: http://127.0.0.1:9000)
- `CLOUD_SYNC_ENABLED` - Enable syncing to Cloud API (default: false)
- `CLOUD_API_TOKEN` - Authentication token for Cloud API

### Endpoints
- Health check: `http://localhost:8000/healthz`
- Inference: `http://localhost:8000/infer` (POST)
- Latest results: `http://localhost:8000/results/latest` (GET)

## 2. Cloud API (@cloud_api)

### Description
The Cloud API is a Python FastAPI backend that provides authentication, data storage, and serves as the central hub for the web interface.

### Dependencies
- Python 3.10+
- Dependencies listed in `requirements.txt`

### How to Run

1. Navigate to the Cloud API directory:
   ```bash
   cd packages/cloud_api
   ```

2. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```

3. Run the service:
   ```bash
   python main.py
   ```
   
   Or using uvicorn directly:
   ```bash
   uvicorn main:app --host 0.0.0.0 --port 9000
   ```

### Configuration
The service can be configured using environment variables:
- `JWT_SECRET` - Secret key for JWT token generation
- `DATABASE_URL` - Database connection URL (default: sqlite:///./cloud_lpr.db)

### Default Credentials
- Username: `admin`
- Password: `admin123`
- API Token: `DEV-TOKEN-CHANGE` (should be changed in production)

### Endpoints
- Health check: `http://localhost:9000/healthz`
- Authentication: `http://localhost:9000/auth/token` (POST)
- LPR data ingestion: `http://localhost:9000/ingest/lpr` (POST)
- Analytics: `http://localhost:9000/analytics/summary` (GET)

## 3. Cloud Web (@cloud_web)

### Description
The Cloud Web is a Next.js web application that provides a dashboard for viewing LPR results and system status.

### Dependencies
- Node.js 18+
- npm or yarn

### How to Run

1. Navigate to the Cloud Web directory:
   ```bash
   cd packages/cloud_web
   ```

2. Install dependencies:
   ```bash
   npm install
   ```

3. Run in development mode:
   ```bash
   npm run dev
   ```

4. Or build and run in production mode:
   ```bash
   npm run build
   npm run start
   ```

### Configuration
The web application connects to the Cloud API at `http://localhost:9000` by default.

### Access
- Web interface: `http://localhost:3000`

## 4. Electron App (@electron_app)

### Description
The Electron App is a desktop application that provides a local interface for monitoring and configuration.

### Dependencies
- Node.js 18+
- npm or yarn

### How to Run

1. Navigate to the Electron App directory:
   ```bash
   cd packages/electron_app
   ```

2. Install dependencies:
   ```bash
   npm install
   ```

3. Run the application:
   ```bash
   npm start
   ```

### Configuration
The Electron app can connect to:
- AI Service at `http://localhost:8000`
- Cloud API at `http://localhost:9000`

## 5. Local C++ Client (@local_cpp)

### Description
The Local C++ Client is a high-performance video processing pipeline that captures video streams, processes frames, and sends them to the AI Service for inference.

### Dependencies
- CMake 3.10+
- OpenCV 4.x
- CURL
- nlohmann/json

### How to Run

There are two versions of the C++ client:

#### Simple Version (for testing with static images)

The simple client has been built and is available as `simple_cpp_client.exe` in the `packages/local_cpp` directory.

1. Navigate to the Local C++ directory:
   ```bash
   cd packages/local_cpp
   ```

2. Run with an image file:
   ```bash
   .\simple_cpp_client.exe path/to/image.jpg [camera_id] [ai_service_url]
   ```

Example:
```bash
# Test with the provided test image
.\simple_cpp_client.exe test_image.jpg CAM01 http://127.0.0.1:8000
```

#### Full Pipeline Version (for processing video streams)

1. Navigate to the Local C++ directory:
   ```bash
   cd packages/local_cpp
   ```

2. Build the full client:
   ```bash
   mkdir build
   cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
   cmake --build . --config Release
   ```

3. Run the pipeline:
   ```bash
   ./cpp_client [config_file]
   ```

### Configuration
The full pipeline version uses `config.json` for configuration:
- Stream URL for RTSP camera
- AI Service URL (default: http://127.0.0.1:8000)
- Processing parameters
- Logging settings

## Running Components Together

### Minimum Setup (Recommended for getting started)
1. Start **Cloud API** first:
   ```bash
   cd packages/cloud_api
   python main.py
   ```

2. Start **AI Service**:
   ```bash
   cd packages/ai_service
   python main.py
   ```

3. Start **Cloud Web**:
   ```bash
   cd packages/cloud_web
   npm run dev
   ```

### Full Setup
1. Start **Cloud API** first
2. Start **AI Service**
3. Start **Cloud Web**
4. Start **Local C++ Client** (if you have a camera or video source)
5. Start **Electron App** (optional, for desktop monitoring)

### Health Checks
- AI Service: `http://localhost:8000/healthz`
- Cloud API: `http://localhost:9000/healthz`
- Cloud Web: `http://localhost:3000` (check visually)

## Authentication

### Cloud API Authentication
To authenticate with the Cloud API:

1. POST to `http://localhost:9000/auth/token` with form data:
   - username: `admin`
   - password: `admin123::DEV-TOKEN-CHANGE`
   
   Note: The password format is `password::api_token`

2. Use the returned JWT token in the `Authorization: Bearer <token>` header for subsequent requests.

### Example using curl:
```bash
curl -X POST "http://localhost:9000/auth/token" \
     -H "Content-Type: application/x-www-form-urlencoded" \
     -d "username=admin&password=admin123::DEV-TOKEN-CHANGE"
```

## Troubleshooting

### Common Issues

1. **Port conflicts**: Make sure ports 8000, 9000, and 3000 are available
2. **Missing dependencies**: Install all required dependencies for each component
3. **Connection refused**: Ensure dependent services are started before dependent components
4. **Permission errors**: Make sure you have appropriate permissions to access cameras or files

### Logs
- AI Service logs: Check console output or `packages/ai_service/logs/`
- Cloud API logs: Check console output
- Cloud Web logs: Check console output
- Local C++ logs: Check `packages/local_cpp/logs/`
- Electron App logs: Check console output

### Testing the Setup

1. Verify all services are running:
   - AI Service health: `curl http://localhost:8000/healthz`
   - Cloud API health: `curl http://localhost:9000/healthz`
   - Cloud Web: Open `http://localhost:3000` in browser

2. Test AI Service inference with a sample image:
   ```bash
   # Using Python (recommended)
   python -c "import requests; files = {'image': open('test_image.jpg', 'rb')}; data = {'camera_id': 'CAM01'}; response = requests.post('http://127.0.0.1:8000/infer', files=files, data=data); print(f'Status: {response.status_code}'); print(f'Response: {response.text}')"
   ```

3. Authenticate with Cloud API:
   ```bash
   curl -X POST "http://localhost:9000/auth/token" \
        -H "Content-Type: application/x-www-form-urlencoded" \
        -d "username=admin&password=admin123::DEV-TOKEN-CHANGE"
   ```