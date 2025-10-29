# RTSP Stream Capture Fix - Summary

## Problem
The C++ edge client (packages/local_cpp) was failing to capture frames from RTSP streams, resulting in 0 FPS and no processed frames. The root cause was difficult to diagnose due to silent failures in OpenCV's VideoCapture.

## Root Causes Identified

1. **Incorrect RTSP URL** (Most Common)
   - Different camera brands use different RTSP paths
   - Current URL may not match the camera model
   - No validation or helpful error messages

2. **OpenCV Build Issues**
   - FFMPEG support may not be compiled in
   - No way to detect this at runtime

3. **Network/Timeout Issues**
   - VideoCapture.open() can hang indefinitely
   - No timeout protection
   - Silent failures without error messages

4. **Poor Error Diagnostics**
   - No detailed logging of what was attempted
   - No suggestions for troubleshooting
   - No information about OpenCV capabilities

## Changes Made

### 1. Enhanced stream_reader.cpp

**Added Features:**

- **OpenCV Build Information Logging**
  - Displays OpenCV version and build configuration on startup
  - Helps identify if FFMPEG/GStreamer are available

- **URL Validation**
  - Validates URL scheme (rtsp://, http://, https://, file://)
  - Rejects invalid URLs early with clear error messages

- **Backend Timeout Protection**
  - Prevents VideoCapture.open() from hanging indefinitely
  - 5-second timeout per backend attempt
  - Graceful fallback to next backend if timeout occurs

- **Detailed Connection Logging**
  - Logs each backend attempt with status
  - Reports stream properties (FPS, resolution) on successful connection
  - Provides actionable error messages with diagnostics

- **Comprehensive Error Messages**
  - Lists possible causes of connection failure
  - Suggests troubleshooting steps
  - Recommends using /discover endpoint or VLC for testing

**Example Console Output (Before):**
```
Stream reader started for: rtsp://admin:test1234@192.168.4.252:554/h264
Stats - FPS: 0.00, Processed: 0, Dropped: 0, Inferences: 0/0, Avg Latency: 0.0ms
```

**Example Console Output (After - Success):**
```
OpenCV version: 4.5.5
[StreamReader] Validating URL: rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_main
[StreamReader] Attempting backend: FFMPEG (ID: 0)
[StreamReader] Successfully opened stream with backend: FFMPEG
[StreamReader] Stream properties - FPS: 30, Resolution: 1920x1080
[StreamReader] Buffer size set to 1 (low latency mode)
[StreamReader] Connected to stream: rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_main
Stats - FPS: 15.00, Processed: 15, Dropped: 0, Inferences: 0/0, Avg Latency: 0.0ms
```

**Example Console Output (After - Failure with Diagnostics):**
```
[StreamReader] Validating URL: rtsp://admin:test1234@192.168.4.252:554/h264
[StreamReader] Attempting backend: FFMPEG (ID: 0)
[StreamReader] Failed to open with backend: FFMPEG
[StreamReader] Attempting backend: GStreamer (ID: 1)
[StreamReader] Backend GStreamer timed out (>5s)
[StreamReader] Attempting backend: ANY (ID: 2)
[StreamReader] Failed to open with backend: ANY
Failed to open stream with any backend. Diagnostics:
  URL: rtsp://admin:test1234@192.168.4.252:554/h264
  Possible causes:
    1. RTSP URL is incorrect (check camera brand-specific paths)
    2. OpenCV not built with FFMPEG support
    3. Network unreachable (firewall, IP, port 554)
    4. Camera credentials invalid (admin:test1234)
    5. Camera offline or stream not available
  Suggestions:
    - Use /discover endpoint to find correct RTSP URL
    - Test with VLC: vlc 'rtsp://admin:test1234@192.168.4.252:554/h264'
    - Check network connectivity: ping 192.168.4.252
    - Verify OpenCV build includes FFMPEG support
```

### 2. Documentation Files Created

#### RTSP_TROUBLESHOOTING.md
Comprehensive troubleshooting guide including:
- Root cause analysis
- Diagnostic steps (network, VLC, /discover endpoint, OpenCV build)
- Solution steps for each cause
- Common RTSP URL patterns by camera brand
- Advanced debugging techniques
- Quick checklist

#### STREAM_READER_DIAGNOSTICS.md
Detailed technical documentation including:
- Overview of improvements
- Configuration reference
- RTSP URL formats for all major camera brands
- Troubleshooting workflow
- Common issues and solutions
- Performance tuning recommendations
- Monitoring and health checks
- Advanced debugging

#### STREAM_FIX_SUMMARY.md (this file)
High-level summary of changes and how to use them

### 3. Updated config.json.example
Cleaned up and ready for use with proper documentation

## How to Use the Fix

### Step 1: Rebuild the C++ Client
```bash
cd packages/local_cpp/build_full
cmake ..
cmake --build . --config Release
```

### Step 2: Run and Check Console Output
```bash
./Release/local_cpp_client.exe 2>&1 | tee debug.log
```

### Step 3: Diagnose Based on Output

**If you see "Successfully opened stream":**
- ✅ Fix is working! Frames should be processing.
- Check logs for FPS > 0

**If you see "Failed to open stream with any backend":**
- Follow the suggestions in the error message
- Most likely: Wrong RTSP URL
- Use `/discover` endpoint to find correct URL
- Test with VLC before updating config.json

### Step 4: Fix the Issue

**Most Common Fix (Wrong RTSP URL):**
```bash
# 1. Get candidate URLs from discovery endpoint
curl "http://127.0.0.1:8085/discover?ip=192.168.4.252&user=admin&pass=test1234&brand=reolink"

# 2. Test each candidate with VLC
vlc "rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_main"

# 3. Update config.json with the working URL
# 4. Restart the client
```

**If OpenCV Lacks FFMPEG:**
```bash
# Rebuild OpenCV with FFMPEG support
git clone https://github.com/opencv/opencv.git
cd opencv && mkdir build && cd build
cmake .. -DWITH_FFMPEG=ON -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release -j4
cmake --install .

# Rebuild local_cpp
cd ../../packages/local_cpp
rm -rf build_full && mkdir build_full && cd build_full
cmake ..
cmake --build . --config Release
```

## Expected Results

### Before Fix
```
[2025-10-15 13:10:31.970] [INFO] Pipeline started successfully
[2025-10-15 13:10:32.983] [INFO] Stats - FPS: 0.00, Processed: 0, Dropped: 0, Inferences: 0/0, Avg Latency: 0.0ms
[2025-10-15 13:10:33.990] [INFO] Stats - FPS: 0.00, Processed: 0, Dropped: 0, Inferences: 0/0, Avg Latency: 0.0ms
```

### After Fix (with correct RTSP URL)
```
[StreamReader] Successfully opened stream with backend: FFMPEG
[StreamReader] Stream properties - FPS: 30, Resolution: 1920x1080
[2025-10-15 13:10:32.983] [INFO] Stats - FPS: 15.00, Processed: 15, Dropped: 0, Inferences: 0/0, Avg Latency: 0.0ms
[2025-10-15 13:10:33.990] [INFO] Stats - FPS: 15.00, Processed: 30, Dropped: 0, Inferences: 0/0, Avg Latency: 0.0ms
```

## Files Modified/Created

### Modified
- `packages/local_cpp/src/stream_reader.cpp` - Enhanced with diagnostics and timeout protection

### Created
- `RTSP_TROUBLESHOOTING.md` - Comprehensive troubleshooting guide
- `packages/local_cpp/STREAM_READER_DIAGNOSTICS.md` - Technical documentation
- `STREAM_FIX_SUMMARY.md` - This file

## Key Improvements

1. **Visibility**: Console output now clearly shows what's happening
2. **Diagnostics**: Detailed error messages with actionable suggestions
3. **Robustness**: Timeout protection prevents hanging
4. **Validation**: URL format validation catches errors early
5. **Documentation**: Multiple guides for different skill levels

## Next Steps

1. **Immediate**: Rebuild and test with current RTSP URL
2. **If still failing**: Follow troubleshooting guide to find correct URL
3. **If OpenCV issue**: Rebuild OpenCV with FFMPEG support
4. **Monitor**: Use health endpoint to verify stream is connected

## Support Resources

- **RTSP_TROUBLESHOOTING.md** - Start here for quick fixes
- **STREAM_READER_DIAGNOSTICS.md** - Detailed technical reference
- **Console output** - Now provides actionable diagnostics
- **/discover endpoint** - Automated RTSP URL discovery
- **VLC** - Best tool for testing RTSP URLs

## Questions?

Refer to the appropriate documentation:
- "How do I fix this?" → RTSP_TROUBLESHOOTING.md
- "What does this error mean?" → STREAM_READER_DIAGNOSTICS.md
- "How do I configure this?" → config.json.example + STREAM_READER_DIAGNOSTICS.md
- "What RTSP URL should I use?" → STREAM_READER_DIAGNOSTICS.md (RTSP URL Formats section)
