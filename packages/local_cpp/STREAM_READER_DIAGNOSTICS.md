# Stream Reader Diagnostics & RTSP Configuration

## Overview
The `stream_reader.cpp` component is responsible for capturing frames from RTSP/HTTP streams using OpenCV's VideoCapture. This document explains how to diagnose and fix stream capture failures.

## Recent Improvements

### Enhanced Diagnostics (Updated stream_reader.cpp)
The stream reader now includes:

1. **OpenCV Build Information Logging**
   - Displays OpenCV version and build configuration on startup
   - Shows whether FFMPEG, GStreamer, and other backends are available

2. **URL Validation**
   - Validates URL scheme (rtsp://, http://, https://, file://)
   - Rejects invalid URLs early with clear error messages

3. **Backend Timeout Protection**
   - Prevents VideoCapture.open() from hanging indefinitely
   - 5-second timeout per backend attempt
   - Graceful fallback to next backend if timeout occurs

4. **Detailed Connection Logging**
   - Logs each backend attempt with status
   - Reports stream properties (FPS, resolution) on successful connection
   - Provides actionable error messages with diagnostics

5. **Comprehensive Error Messages**
   - Lists possible causes of connection failure
   - Suggests troubleshooting steps
   - Recommends using /discover endpoint or VLC for testing

## Configuration

### Stream Configuration (config.json)

```json
{
  "stream": {
    "url": "rtsp://admin:password@192.168.x.x:554/path",
    "camera_id": "CAM01",
    "fps_cap": 15,
    "reconnect_delay_ms": 1000,
    "max_reconnect_attempts": -1,
    "use_hardware_decode": true
  }
}
```

**Parameters:**
- `url`: RTSP/HTTP stream URL (see RTSP URL Formats below)
- `camera_id`: Unique identifier for this camera (used in logs and results)
- `fps_cap`: Maximum frames per second to capture (15 is typical for LPR)
- `reconnect_delay_ms`: Initial delay before reconnect attempt (exponential backoff applied)
- `max_reconnect_attempts`: Maximum reconnect attempts (-1 = infinite)
- `use_hardware_decode`: Enable H.264 hardware decoding if available

## RTSP URL Formats

### Reolink Cameras
```
rtsp://admin:password@192.168.x.x:554/h264Preview_01_main    # Main stream
rtsp://admin:password@192.168.x.x:554/h264Preview_01_sub     # Sub stream (lower res)
```

### Hikvision Cameras
```
rtsp://admin:password@192.168.x.x:554/Streaming/Channels/101  # Main stream
rtsp://admin:password@192.168.x.x:554/Streaming/Channels/102  # Sub stream
```

### Dahua Cameras
```
rtsp://admin:password@192.168.x.x:554/live/ch00_0             # Channel 1
rtsp://admin:password@192.168.x.x:554/live/ch00_1             # Channel 2
```

### Uniview Cameras
```
rtsp://admin:password@192.168.x.x:554/media/video1
rtsp://admin:password@192.168.x.x:554/media/video2
```

### Axis Cameras
```
rtsp://admin:password@192.168.x.x:554/axis-media/media.amp?videocodec=h264
```

### Generic/Unknown
```
rtsp://admin:password@192.168.x.x:554/stream
rtsp://admin:password@192.168.x.x:554/live
rtsp://admin:password@192.168.x.x:554/h264
```

## Troubleshooting Workflow

### Step 1: Check Console Output
Run the client and examine console output for diagnostics:

```bash
cd packages/local_cpp/build_full
./Release/local_cpp_client.exe 2>&1 | tee debug.log
```

**Look for these key messages:**

✅ **Success indicators:**
```
OpenCV version: 4.x.x
[StreamReader] Successfully opened stream with backend: FFMPEG
[StreamReader] Stream properties - FPS: 30, Resolution: 1920x1080
[StreamReader] Connected to stream: rtsp://...
Stats - FPS: 15.00, Processed: 15, Dropped: 0
```

❌ **Failure indicators:**
```
[StreamReader] Failed to open with backend: FFMPEG
[StreamReader] Backend GStreamer timed out (>5s)
Failed to open stream with any backend. Diagnostics:
```

### Step 2: Verify Network Connectivity

```bash
# Test if camera is reachable
ping 192.168.x.x

# Test RTSP port (Windows)
Test-NetConnection -ComputerName 192.168.x.x -Port 554

# Test RTSP port (Linux/Mac)
nc -zv 192.168.x.x 554
```

### Step 3: Test RTSP URL with VLC

VLC is the gold standard for testing RTSP URLs:

```bash
# Install VLC if needed, then test:
vlc "rtsp://admin:password@192.168.x.x:554/h264"
```

**If VLC plays the stream:** The URL is correct; issue is likely OpenCV build or network.
**If VLC fails:** The URL is wrong; try alternatives or use /discover endpoint.

### Step 4: Use Discovery Endpoint

The health server includes a discovery endpoint to find candidate RTSP URLs:

```bash
# While client is running (even if stream fails):
curl "http://127.0.0.1:8085/discover?ip=192.168.x.x&user=admin&pass=password&brand=reolink"

# Response:
# {"success":true,"candidates":["rtsp://admin:password@192.168.x.x:554/h264Preview_01_sub","rtsp://admin:password@192.168.x.x:554/h264Preview_01_main",...]}
```

**Next steps:**
1. Copy each candidate URL
2. Test with VLC: `vlc "rtsp://..."`
3. When one works, update config.json
4. Restart the client

### Step 5: Verify OpenCV Build

Check if OpenCV was built with FFMPEG support:

```bash
cd packages/local_cpp/build_full
cmake .. -DWITH_FFMPEG=ON -DWITH_GSTREAMER=ON
```

**Look for in output:**
```
-- FFMPEG: YES
-- GStreamer: YES
```

If FFMPEG is missing, rebuild OpenCV:

```bash
# Download OpenCV
git clone https://github.com/opencv/opencv.git
cd opencv && mkdir build && cd build

# Configure with FFMPEG
cmake .. -DWITH_FFMPEG=ON -DWITH_GSTREAMER=ON -DCMAKE_BUILD_TYPE=Release

# Build (takes ~30 minutes)
cmake --build . --config Release -j4

# Install
cmake --install .

# Rebuild local_cpp
cd ../../packages/local_cpp
rm -rf build_full && mkdir build_full && cd build_full
cmake ..
cmake --build . --config Release
```

## Common Issues & Solutions

### Issue: "Failed to open stream with any backend"

**Possible causes (in order of likelihood):**

1. **Wrong RTSP URL** (60% of cases)
   - Solution: Test with VLC, use /discover endpoint, try alternative paths

2. **OpenCV not built with FFMPEG** (25% of cases)
   - Solution: Rebuild OpenCV with `-DWITH_FFMPEG=ON`

3. **Network unreachable** (10% of cases)
   - Solution: Verify ping, check firewall, verify credentials

4. **Camera offline or stream unavailable** (5% of cases)
   - Solution: Check camera web interface, restart camera

### Issue: "Backend FFMPEG timed out (>5s)"

**Cause:** VideoCapture.open() is hanging, likely due to network issue or invalid URL.

**Solutions:**
1. Verify network connectivity: `ping 192.168.x.x`
2. Check firewall: `Test-NetConnection -ComputerName 192.168.x.x -Port 554`
3. Try different RTSP path
4. Reduce timeout in code if needed (currently 5 seconds)

### Issue: FPS is 0, no frames processed

**Cause:** Stream connected but cap_.read() failing.

**Solutions:**
1. Check stream properties in console output
2. Verify frame resolution matches preprocessing config
3. Try disabling hardware decode: `"use_hardware_decode": false`
4. Check if stream is actually streaming (not paused)

### Issue: High latency or dropped frames

**Cause:** Preprocessing or inference queue full.

**Solutions:**
1. Increase queue sizes in config.json:
   ```json
   "pipeline": {
     "queue_size": 64,
     "max_inference_queue": 32
   }
   ```
2. Reduce fps_cap to lower processing load
3. Disable preprocessing features (denoise, sharpen)
4. Check AI service performance

## Performance Tuning

### For Low-Latency Scenarios
```json
{
  "stream": {
    "fps_cap": 5,
    "use_hardware_decode": true
  },
  "pipeline": {
    "queue_size": 8,
    "max_inference_queue": 4
  },
  "preprocessing": {
    "denoise": false,
    "sharpen": false,
    "quality_threshold": 0.5
  }
}
```

### For High-Throughput Scenarios
```json
{
  "stream": {
    "fps_cap": 30,
    "use_hardware_decode": true
  },
  "pipeline": {
    "queue_size": 64,
    "max_inference_queue": 32
  },
  "preprocessing": {
    "denoise": false,
    "sharpen": false,
    "quality_threshold": 0.3
  }
}
```

## Monitoring & Health Checks

### Health Endpoint
```bash
curl http://127.0.0.1:8085/status
```

**Response includes:**
```json
{
  "stream_connected": true,
  "fps": 15.0,
  "queue_size": 5,
  "ai_healthy": true
}
```

### Metrics Endpoint
```bash
curl http://127.0.0.1:8085/metrics
```

**Prometheus-format metrics:**
```
lpr_fps 15.0
lpr_queue_size 5
lpr_stream_connected 1
lpr_ai_healthy 1
```

## Advanced Debugging

### Enable Verbose Logging
In config.json:
```json
{
  "logging": {
    "level": "DEBUG",
    "console_output": true
  }
}
```

### Test with Sample Image
Use the simple client to test AI service without RTSP:
```bash
cd packages/local_cpp/build_full
./Release/simple_main.exe
```

### Check OpenCV Capabilities
Add to stream_reader.cpp:
```cpp
std::cout << "CAP_FFMPEG: " << cv::CAP_FFMPEG << std::endl;
std::cout << "CAP_GSTREAMER: " << cv::CAP_GSTREAMER << std::endl;
std::cout << "CAP_ANY: " << cv::CAP_ANY << std::endl;
```

## References

- **OpenCV VideoCapture:** https://docs.opencv.org/master/d8/dfe/classcv_1_1VideoCapture.html
- **RTSP Protocol:** https://en.wikipedia.org/wiki/Real_Time_Streaming_Protocol
- **VLC Media Player:** https://www.videolan.org/vlc/
- **Camera Manufacturer Docs:**
  - Reolink: https://support.reolink.com/
  - Hikvision: https://www.hikvision.com/
  - Dahua: https://www.dahuasecurity.com/

## Quick Checklist

- [ ] Network connectivity verified (ping camera)
- [ ] RTSP URL tested with VLC
- [ ] OpenCV built with FFMPEG support
- [ ] config.json updated with correct URL
- [ ] Client restarted
- [ ] Console shows "Successfully opened stream"
- [ ] Logs show FPS > 0
- [ ] Health endpoint shows stream_connected: true
