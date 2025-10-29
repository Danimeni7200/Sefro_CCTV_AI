# RTSP Stream Capture Troubleshooting Guide

## Problem Summary
The C++ edge client (packages/local_cpp) fails to capture frames from an RTSP stream, resulting in 0 FPS and no processed frames. The stream_reader.cpp component cannot connect to the camera.

## Root Causes (in order of likelihood)

### 1. **Incorrect RTSP URL** (Most Common)
Different camera brands use different RTSP paths. The URL in config.json may not match your camera model.

**Current URL in config.json:**
```
rtsp://admin:test1234@192.168.4.252:554/h264
```

**Common RTSP paths by brand:**

| Brand | Path Examples |
|-------|---|
| **Reolink** | `/h264Preview_01_main`, `/h264Preview_01_sub`, `/h264` |
| **Hikvision** | `/Streaming/Channels/101`, `/Streaming/Channels/102` |
| **Dahua** | `/live/ch00_0`, `/live/ch00_1` |
| **Uniview** | `/media/video1`, `/media/video2` |
| **Axis** | `/axis-media/media.amp?videocodec=h264` |
| **Amcrest** | `/rtsp/channel=1`, `/rtsp/channel=2` |
| **TP-Link** | `/stream/profile1`, `/stream/profile2` |
| **Generic** | `/stream`, `/live`, `/video` |

### 2. **OpenCV Not Built with FFMPEG Support**
RTSP streams require FFMPEG backend in OpenCV. If your build lacks this, VideoCapture will fail silently.

**Check OpenCV build info:**
```bash
cd packages/local_cpp/build_full
# Look for FFMPEG in the output
cmake .. -DWITH_FFMPEG=ON
```

### 3. **Network/Firewall Issues**
- Camera IP unreachable (192.168.4.252)
- Port 554 blocked by firewall
- Credentials invalid (admin:test1234)
- Camera offline or stream not available

### 4. **Silent Failures in VideoCapture**
OpenCV's VideoCapture.open() can fail silently without error messages, making diagnosis difficult.

---

## Diagnostic Steps

### Step 1: Verify Network Connectivity
```bash
# Test if camera is reachable
ping 192.168.4.252

# Test RTSP port (554)
telnet 192.168.4.252 554
# or on Windows:
Test-NetConnection -ComputerName 192.168.4.252 -Port 554
```

**Expected:** Connection succeeds or times out (not "Connection refused")

### Step 2: Test RTSP URL with VLC
VLC is the best tool to validate RTSP URLs before using them in code.

```bash
# Install VLC if not present
# Then test the URL:
vlc "rtsp://admin:test1234@192.168.4.252:554/h264"
```

**Expected:** Video plays in VLC window

**If VLC fails:**
- Try alternative paths from the table above
- Check camera web interface for correct RTSP path
- Verify credentials are correct

### Step 3: Use the /discover Endpoint
The C++ client includes a discovery endpoint to find candidate RTSP URLs.

```bash
# Start the C++ client (it will fail to connect but health server runs)
cd packages/local_cpp/build_full
./Release/local_cpp_client.exe

# In another terminal, call the discovery endpoint:
curl "http://127.0.0.1:8085/discover?ip=192.168.4.252&user=admin&pass=test1234&brand=reolink"

# Response example:
# {"success":true,"candidates":["rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_sub","rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_main",...]}
```

**Next steps:**
1. Copy each candidate URL
2. Test with VLC: `vlc "rtsp://..."`
3. When one works, update config.json with that URL
4. Restart the C++ client

### Step 4: Check OpenCV Build
Verify that your OpenCV build includes FFMPEG support.

```bash
# On Windows, check the build output:
cd packages/local_cpp/build_full
cmake .. -DWITH_FFMPEG=ON -DWITH_GSTREAMER=ON

# Look for:
# -- FFMPEG: YES
# -- GStreamer: YES
```

If FFMPEG is missing, rebuild OpenCV with FFMPEG enabled:
```bash
# Download OpenCV source
git clone https://github.com/opencv/opencv.git
cd opencv
mkdir build && cd build

# Configure with FFMPEG
cmake .. -DWITH_FFMPEG=ON -DWITH_GSTREAMER=ON -DCMAKE_BUILD_TYPE=Release

# Build and install
cmake --build . --config Release
cmake --install .
```

### Step 5: Check Console Output
The updated stream_reader.cpp now logs detailed diagnostics. Run the client and check console output:

```bash
cd packages/local_cpp/build_full
./Release/local_cpp_client.exe 2>&1 | tee debug.log
```

**Look for:**
- `OpenCV version: ...` - confirms OpenCV is loaded
- `[StreamReader] Validating URL: ...` - URL validation
- `[StreamReader] Attempting backend: FFMPEG ...` - backend attempts
- `[StreamReader] Successfully opened stream ...` - success indicator
- `Failed to open stream with any backend. Diagnostics:` - detailed error

---

## Solution Steps

### Option A: Fix RTSP URL (Most Likely)

1. **Identify your camera brand** (check camera web interface or manual)

2. **Use /discover endpoint** to get candidate URLs:
   ```bash
   curl "http://127.0.0.1:8085/discover?ip=192.168.4.252&user=admin&pass=test1234&brand=reolink"
   ```

3. **Test each candidate with VLC:**
   ```bash
   vlc "rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_main"
   ```

4. **Update config.json** with the working URL:
   ```json
   {
     "stream": {
       "url": "rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_main",
       ...
     }
   }
   ```

5. **Restart the C++ client:**
   ```bash
   cd packages/local_cpp/build_full
   ./Release/local_cpp_client.exe
   ```

### Option B: Rebuild OpenCV with FFMPEG

If OpenCV lacks FFMPEG support:

```bash
# 1. Download OpenCV
git clone https://github.com/opencv/opencv.git
cd opencv && mkdir build && cd build

# 2. Configure with FFMPEG
cmake .. -DWITH_FFMPEG=ON -DWITH_GSTREAMER=ON -DCMAKE_BUILD_TYPE=Release

# 3. Build (this takes ~30 minutes)
cmake --build . --config Release -j4

# 4. Install
cmake --install .

# 5. Rebuild local_cpp
cd ../../packages/local_cpp
rm -rf build_full
mkdir build_full && cd build_full
cmake ..
cmake --build . --config Release
```

### Option C: Check Network/Firewall

```bash
# 1. Verify camera is reachable
ping 192.168.4.252

# 2. Check if port 554 is open
# Windows:
Test-NetConnection -ComputerName 192.168.4.252 -Port 554

# Linux/Mac:
nc -zv 192.168.4.252 554

# 3. Verify credentials
# Try accessing camera web interface:
# http://192.168.4.252 (or https://192.168.4.252)
# Login with admin:test1234
```

---

## Expected Behavior After Fix

Once the RTSP URL is correct and OpenCV has FFMPEG support:

1. **Console output shows:**
   ```
   [StreamReader] Successfully opened stream with backend: FFMPEG
   [StreamReader] Stream properties - FPS: 30, Resolution: 1920x1080
   [StreamReader] Connected to stream: rtsp://...
   ```

2. **Logs show frames being processed:**
   ```
   [2025-10-15 13:10:32.983] [INFO] Stats - FPS: 15.00, Processed: 15, Dropped: 0, Inferences: 0/0, Avg Latency: 0.0ms
   [2025-10-15 13:10:33.990] [INFO] Stats - FPS: 15.00, Processed: 30, Dropped: 0, Inferences: 0/0, Avg Latency: 0.0ms
   ```

3. **Health endpoint shows stream connected:**
   ```bash
   curl http://127.0.0.1:8085/status
   # Response includes: "stream_connected": true
   ```

---

## Common RTSP URL Patterns

### Reolink
```
rtsp://admin:password@192.168.x.x:554/h264Preview_01_main
rtsp://admin:password@192.168.x.x:554/h264Preview_01_sub
```

### Hikvision
```
rtsp://admin:password@192.168.x.x:554/Streaming/Channels/101
rtsp://admin:password@192.168.x.x:554/Streaming/Channels/102
```

### Dahua
```
rtsp://admin:password@192.168.x.x:554/live/ch00_0
rtsp://admin:password@192.168.x.x:554/live/ch00_1
```

### Generic/Unknown
```
rtsp://admin:password@192.168.x.x:554/stream
rtsp://admin:password@192.168.x.x:554/live
rtsp://admin:password@192.168.x.x:554/h264
```

---

## Advanced Debugging

### Enable Verbose OpenCV Logging
```cpp
// Add to main.cpp before creating StreamReader:
cv::setLogLevel(cv::LOG_LEVEL_VERBOSE);
```

### Test with Simple Client
Use the simple test client to isolate issues:
```bash
cd packages/local_cpp/build_full
./Release/simple_main.exe
# This tests image upload to AI service without RTSP
```

### Check OpenCV Capabilities
```cpp
// Add to stream_reader.cpp to list available backends:
std::cout << "Available backends:" << std::endl;
std::cout << "  CAP_FFMPEG: " << cv::CAP_FFMPEG << std::endl;
std::cout << "  CAP_GSTREAMER: " << cv::CAP_GSTREAMER << std::endl;
std::cout << "  CAP_ANY: " << cv::CAP_ANY << std::endl;
```

---

## Support Resources

- **OpenCV RTSP Documentation:** https://docs.opencv.org/master/d8/dfe/classcv_1_1VideoCapture.html
- **RTSP URL Format:** https://en.wikipedia.org/wiki/Real_Time_Streaming_Protocol
- **VLC Media Player:** https://www.videolan.org/vlc/
- **Camera Manufacturer RTSP Docs:**
  - Reolink: https://support.reolink.com/hc/en-us/articles/900004435586
  - Hikvision: https://www.hikvision.com/en/
  - Dahua: https://www.dahuasecurity.com/

---

## Quick Checklist

- [ ] Network connectivity verified (ping camera IP)
- [ ] RTSP URL tested with VLC
- [ ] OpenCV built with FFMPEG support
- [ ] config.json updated with correct RTSP URL
- [ ] C++ client restarted
- [ ] Console shows "Successfully opened stream"
- [ ] Logs show FPS > 0 and frames being processed
- [ ] Health endpoint shows stream_connected: true
