# How to Rebuild and Run the Fixed C++ Client

## Quick Start

### Option 1: PowerShell (Recommended)

```powershell
cd c:\Users\Dani\Desktop\license_plate\packages\local_cpp
.\rebuild.ps1
```

### Option 2: Command Prompt (CMD)

```cmd
cd c:\Users\Dani\Desktop\license_plate\packages\local_cpp
rebuild.cmd
```

### Option 3: Manual Steps (PowerShell)

```powershell
# Navigate to build directory
cd c:\Users\Dani\Desktop\license_plate\packages\local_cpp\build_full

# Configure CMake
cmake ..

# Build Release
cmake --build . --config Release

# Run the client
.\Release\local_cpp_client.exe
```

---

## Step-by-Step Instructions

### Step 1: Open PowerShell or CMD

**PowerShell:**
- Press `Win + X` and select "Windows PowerShell" or "Terminal"

**CMD:**
- Press `Win + R`, type `cmd`, press Enter

### Step 2: Navigate to the Project

```powershell
cd c:\Users\Dani\Desktop\license_plate\packages\local_cpp
```

### Step 3: Run the Rebuild Script

**PowerShell:**
```powershell
.\rebuild.ps1
```

**CMD:**
```cmd
rebuild.cmd
```

### Step 4: Wait for Build to Complete

The build will take 2-5 minutes. You should see:
```
-- Build files have been written to: ...
[100%] Built target local_cpp_client
Build successful!
```

### Step 5: Run the Client

After the script completes, run:

**PowerShell:**
```powershell
.\Release\local_cpp_client.exe
```

**CMD:**
```cmd
.\Release\local_cpp_client.exe
```

---

## What to Look For

### Success Indicators (You should see these)

```
OpenCV version: 4.x.x
[StreamReader] Validating URL: rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_main
[StreamReader] Attempting backend: FFMPEG (ID: 0)
[StreamReader] Successfully opened stream with backend: FFMPEG
[StreamReader] Stream properties - FPS: 30, Resolution: 1920x1080
[StreamReader] Connected to stream: rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_main
Stats - FPS: 15.00, Processed: 15, Dropped: 0, Inferences: 0/0, Avg Latency: 0.0ms
```

### If You See This (Success!)
```
Stats - FPS: 15.00, Processed: 15, Dropped: 0
```
✅ **Frames are being captured!** The fix is working.

### If You See This (Problem)
```
Failed to open stream with any backend. Diagnostics:
```
❌ Still having issues. See troubleshooting below.

---

## Verification Steps

### Test 1: Check Stream Connection

Open another PowerShell/CMD window and run:

```powershell
curl http://127.0.0.1:8085/status
```

**Expected response:**
```json
{
  "stream_connected": true,
  "fps": 15.0,
  "queue_size": 5,
  "ai_healthy": false
}
```

### Test 2: Check Metrics

```powershell
curl http://127.0.0.1:8085/metrics
```

**Expected response:**
```
lpr_fps 15.0
lpr_queue_size 5
lpr_stream_connected 1
```

### Test 3: Check Logs

```powershell
Get-Content -Path "logs/cpp_client.log" -Tail 20
```

Should show frames being processed with increasing frame count.

---

## Troubleshooting

### Problem: "CMake not found"

**Solution:** CMake is not installed or not in PATH.

```powershell
# Check if CMake is installed
cmake --version

# If not found, install from: https://cmake.org/download/
```

### Problem: "Build failed"

**Solution:** Check the error message in the console output.

Common causes:
1. Missing dependencies (OpenCV, libcurl)
2. Compiler issues
3. Disk space

### Problem: "Still 0 FPS after rebuild"

**Solution:** The RTSP URL might still be wrong.

1. Verify config.json was updated:
```powershell
Get-Content packages\local_cpp\config.json | Select-String "url"
```

Should show: `"url": "rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_main"`

2. Try alternative Reolink stream:
```json
"url": "rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_sub"
```

3. Test with VLC:
```powershell
vlc "rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_main"
```

### Problem: "Connection refused" or "Network unreachable"

**Solution:** Check network connectivity.

```powershell
# Test if camera is reachable
ping 192.168.4.252

# Test RTSP port
Test-NetConnection -ComputerName 192.168.4.252 -Port 554
```

---

## PowerShell Execution Policy Issue

If you get an error like "cannot be loaded because running scripts is disabled":

```powershell
# Temporarily allow script execution
Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope Process

# Then run the script
.\rebuild.ps1
```

---

## Complete Workflow

```powershell
# 1. Navigate to project
cd c:\Users\Dani\Desktop\license_plate\packages\local_cpp

# 2. Rebuild
.\rebuild.ps1

# 3. Run client (in same window after build completes)
.\Release\local_cpp_client.exe

# 4. In another PowerShell window, verify
curl http://127.0.0.1:8085/status

# 5. Check logs
Get-Content -Path "logs/cpp_client.log" -Tail 20
```

---

## Expected Timeline

| Step | Time | What Happens |
|------|------|--------------|
| CMake configure | 10-30 sec | Checks dependencies, generates build files |
| Build | 2-5 min | Compiles C++ code |
| Total | 2-6 min | Ready to run |

---

## Success Checklist

- [ ] Rebuild script runs without errors
- [ ] Build completes with "Built target local_cpp_client"
- [ ] Client starts and shows OpenCV version
- [ ] Console shows "Successfully opened stream"
- [ ] FPS > 0 in stats output
- [ ] Health endpoint shows stream_connected: true
- [ ] Logs show frames being processed

---

## Next Steps After Success

1. **Monitor the stream:** Keep the client running and watch for consistent FPS
2. **Test AI inference:** Verify the AI service is receiving frames
3. **Check cloud sync:** If enabled, verify results are syncing to cloud API
4. **Monitor performance:** Watch for dropped frames or latency issues

---

## Support

If you encounter issues:

1. Check console output for specific error messages
2. Refer to `RTSP_TROUBLESHOOTING.md` for detailed diagnostics
3. Verify network connectivity: `ping 192.168.4.252`
4. Test RTSP URL with VLC
5. Check `STREAM_READER_DIAGNOSTICS.md` for technical details

---

## Files Provided

- `rebuild.ps1` - PowerShell rebuild script
- `rebuild.cmd` - CMD batch rebuild script
- `rebuild.bat` - Alternative batch script
- `RUN_REBUILD.md` - This guide

**Choose the script that matches your shell and run it!**
