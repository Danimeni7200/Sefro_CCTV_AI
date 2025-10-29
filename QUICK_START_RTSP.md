# Quick Start: Fix RTSP Stream Capture

## TL;DR - 5 Minute Fix

### 1. Rebuild (1 minute)
```bash
cd packages/local_cpp/build_full
cmake ..
cmake --build . --config Release
```

### 2. Run and Check Output (1 minute)
```bash
./Release/local_cpp_client.exe
```

**Look for:**
- ✅ `Successfully opened stream` → **DONE! Frames are being captured.**
- ❌ `Failed to open stream with any backend` → **Go to Step 3**

### 3. Find Correct RTSP URL (2 minutes)
```bash
# In another terminal, get candidate URLs:
curl "http://127.0.0.1:8085/discover?ip=192.168.4.252&user=admin&pass=test1234&brand=reolink"

# Test each with VLC:
vlc "rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_main"
```

### 4. Update and Restart (1 minute)
```bash
# Edit packages/local_cpp/config.json
# Change "url" to the working RTSP URL from VLC

# Restart:
./Release/local_cpp_client.exe
```

---

## Common RTSP URLs

| Camera Brand | URL |
|---|---|
| **Reolink** | `rtsp://admin:pass@IP:554/h264Preview_01_main` |
| **Hikvision** | `rtsp://admin:pass@IP:554/Streaming/Channels/101` |
| **Dahua** | `rtsp://admin:pass@IP:554/live/ch00_0` |
| **Generic** | `rtsp://admin:pass@IP:554/stream` |

---

## Troubleshooting

### Problem: Still 0 FPS after updating URL

**Check 1: Network**
```bash
ping 192.168.4.252
```
Should respond. If not, camera is unreachable.

**Check 2: VLC Test**
```bash
vlc "rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_main"
```
Should play video. If not, URL is wrong.

**Check 3: OpenCV Build**
```bash
cd packages/local_cpp/build_full
cmake .. -DWITH_FFMPEG=ON
```
Look for `-- FFMPEG: YES`. If NO, rebuild OpenCV with FFMPEG.

### Problem: "Backend timed out"

**Solution:** Network issue or wrong URL
1. Verify network: `ping 192.168.4.252`
2. Try different RTSP path
3. Check firewall (port 554)

### Problem: "OpenCV not built with FFMPEG"

**Solution:** Rebuild OpenCV
```bash
git clone https://github.com/opencv/opencv.git
cd opencv && mkdir build && cd build
cmake .. -DWITH_FFMPEG=ON -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release -j4
cmake --install .
```

---

## Verify It's Working

### Check 1: Console Output
```
[StreamReader] Successfully opened stream with backend: FFMPEG
[StreamReader] Stream properties - FPS: 30, Resolution: 1920x1080
Stats - FPS: 15.00, Processed: 15, Dropped: 0
```

### Check 2: Health Endpoint
```bash
curl http://127.0.0.1:8085/status
```
Should show: `"stream_connected": true`

### Check 3: Log File
```bash
tail -f logs/cpp_client.log
```
Should show frames being processed with increasing frame count.

---

## Need More Help?

- **Detailed troubleshooting:** See `RTSP_TROUBLESHOOTING.md`
- **Technical reference:** See `STREAM_READER_DIAGNOSTICS.md`
- **All changes explained:** See `STREAM_FIX_SUMMARY.md`

---

## Key Points

✅ **DO:**
- Test RTSP URL with VLC first
- Use /discover endpoint to find candidate URLs
- Check network connectivity (ping)
- Rebuild OpenCV with FFMPEG if needed

❌ **DON'T:**
- Guess RTSP URLs without testing
- Ignore console error messages
- Assume network is working without testing
- Use old OpenCV build without FFMPEG

---

## One-Liner Diagnostics

```bash
# Check if camera is reachable
ping 192.168.4.252

# Get candidate RTSP URLs
curl "http://127.0.0.1:8085/discover?ip=192.168.4.252&user=admin&pass=test1234&brand=reolink"

# Test RTSP URL with VLC
vlc "rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_main"

# Check stream status
curl http://127.0.0.1:8085/status

# View recent logs
tail -20 logs/cpp_client.log
```

---

## Success Checklist

- [ ] Rebuilt C++ client
- [ ] Console shows "Successfully opened stream"
- [ ] FPS > 0 in logs
- [ ] Health endpoint shows stream_connected: true
- [ ] Frames appearing in logs with increasing frame count
