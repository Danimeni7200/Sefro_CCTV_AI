# Streaming Service Fix

## Problem
The Electron app was trying to call `POST http://127.0.0.1:8086/add_stream` but getting 404 errors.

## Root Cause
The `discovery_only.cpp` service has a bug in parameter parsing:
- The `handle_add_stream_request()` function expects the full path with query string
- But `handle_request()` already extracts the query string and passes only that
- This caused a mismatch where the function was looking for '?' in a string that already had it removed

## Solution

### 1. Fixed discovery_only.cpp
Changed the parameter handling functions to accept the extracted query string directly instead of looking for '?' again:
- `handle_add_stream_request(const std::string& query_string)` - now accepts query directly
- `handle_remove_stream_request(const std::string& query_string)` - now accepts query directly

### 2. Fixed CMakeLists.txt
Made OpenCV and CURL optional dependencies so discovery_only can build without CURL (which requires ZLIB):
- OpenCV is found and used if available
- discovery_only only needs OpenCV, not CURL
- Other targets still get CURL if found

## How to Use

### Build the Service
```batch
cd packages\local_cpp
build_discovery.bat
```

### Start the Service  
```batch
cd packages\local_cpp
start_discovery.bat
```

This will:
- Start `discovery_only.exe` on port 8086
- Enable:
  - POST /discover - Discover RTSP URLs for a camera
  - POST /add_stream - Add a stream to be served
  - POST /remove_stream - Remove a stream
  - GET /stream/{id} - Get the latest frame from a stream
  - GET /health - Health check

### Test the Discovery Endpoint
```bash
curl "http://127.0.0.1:8086/discover?ip=192.168.4.252&user=admin&pass=test1234&brand=reolink"
```

Expected response:
```json
{
  "success": true,
  "candidates": [
    "rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_sub",
    "rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_main",
    ...
  ]
}
```

### Test Adding a Stream
```bash
curl -X POST "http://127.0.0.1:8086/add_stream?id=stream_001&url=rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_sub"
```

Expected response:
```json
{
  "success": true,
  "message": "Stream added"
}
```

### Get Stream Frame (latest JPEG)
```bash
curl "http://127.0.0.1:8086/stream/stream_001" --output frame.jpg
```

## Electron App Integration

The Electron app will now:
1. Call `/discover` to get RTSP URLs
2. Automatically call `/add_stream` with the first discovered URL
3. Navigate to the live stream tab
4. The live stream tab can fetch frames from `/stream/{id}`

## CCTV Camera Info
- IP: 192.168.4.252
- Username: admin
- Password: test1234
- Brand: Reolink
- RTSP URL: rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_sub

## Next Steps
1. Run `start_discovery.bat` to start the service
2. Open the Electron app
3. Go to "کشف دوربین" (Discover Camera) tab
4. Enter camera info: IP=192.168.4.252, user=admin, pass=test1234, brand=Reolink
5. Click "شروع کشف" (Start Discovery)
6. App will automatically add the stream and navigate to live view
7. Stream should appear in the "مشاهده زنده دوربین‌ها" (Live View Cameras) tab

## Notes
- The service runs on port 8086 by default
- OpenCV is required for frame capture and JPEG encoding
- The service maintains the latest frame from each stream in memory
- Multiple streams can be added simultaneously
- Streams are automatically reconnected if they drop

## Troubleshooting
1. **Port 8086 already in use**: Kill any process using port 8086 or change the port in `discovery_only.cpp` line 500
2. **404 on add_stream**: Make sure the service is running on port 8086
3. **No frames appearing**: Check that the RTSP URL is valid and accessible (test with VLC)
4. **Build errors**: Make sure OpenCV is installed via vcpkg: `.\vcpkg\vcpkg.exe install opencv4:x64-windows`


