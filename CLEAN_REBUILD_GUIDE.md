# Clean Rebuild Guide - Fix CMake Cache Conflict

## Problem

You encountered a CMake error:
```
_add_executable cannot create target "local_cpp_client" because another
target with the same name already exists.
```

This happens when the CMake cache is corrupted or has conflicting configurations.

## Solution

Use the **clean rebuild script** which:
1. ✅ Removes the old build directory completely
2. ✅ Creates a fresh build directory
3. ✅ Runs CMake from scratch
4. ✅ Builds the project cleanly

---

## Quick Start

### Option 1: PowerShell (Recommended)

```powershell
cd c:\Users\Dani\Desktop\license_plate\packages\local_cpp
.\clean_rebuild.ps1
```

### Option 2: Command Prompt (CMD)

```cmd
cd c:\Users\Dani\Desktop\license_plate\packages\local_cpp
clean_rebuild.cmd
```

### Option 3: Manual Clean Rebuild (PowerShell)

```powershell
# Navigate to project
cd c:\Users\Dani\Desktop\license_plate\packages\local_cpp

# Remove old build
Remove-Item -Path build_full -Recurse -Force

# Create fresh build directory
mkdir build_full
cd build_full

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

### Step 2: Navigate to Project

```powershell
cd c:\Users\Dani\Desktop\license_plate\packages\local_cpp
```

### Step 3: Run Clean Rebuild Script

**PowerShell:**
```powershell
.\clean_rebuild.ps1
```

**CMD:**
```cmd
clean_rebuild.cmd
```

### Step 4: Wait for Build to Complete

The script will:
1. Remove old build directory (5-10 seconds)
2. Create fresh build directory (1 second)
3. Configure CMake (10-30 seconds)
4. Build Release (2-5 minutes)

**Total time: 3-6 minutes**

### Step 5: Run the Client

After the script completes, it will show:
```
Current directory: C:\Users\Dani\Desktop\license_plate\packages\local_cpp\build_full
```

Run the client:
```powershell
.\Release\local_cpp_client.exe
```

---

## What to Look For

### Success Indicators

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

### Problem: "cannot be loaded because running scripts is disabled"

**Solution:** Run this first:

```powershell
Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope Process
```

Then run the script again.

### Problem: "Build failed" with other errors

**Solution:** Check the error message in the console output.

Common causes:
1. Missing dependencies (OpenCV, libcurl)
2. Compiler issues
3. Disk space
4. Antivirus blocking build process

Try:
1. Disable antivirus temporarily
2. Check disk space: `Get-Volume`
3. Verify dependencies are installed

### Problem: Still 0 FPS after clean rebuild

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

## What the Clean Rebuild Does

### Before (Corrupted Cache)
```
build_full/
├── CMakeCache.txt (corrupted)
├── CMakeFiles/ (old configuration)
└── ... (conflicting files)
```

### After (Fresh Build)
```
build_full/
├── CMakeCache.txt (fresh)
├── CMakeFiles/ (new configuration)
├── Release/
│   └── local_cpp_client.exe (newly built)
└── ... (clean files)
```

---

## Complete Workflow

```powershell
# 1. Navigate to project
cd c:\Users\Dani\Desktop\license_plate\packages\local_cpp

# 2. Clean rebuild
.\clean_rebuild.ps1

# 3. Wait for build to complete (3-6 minutes)

# 4. Run client (script will be in build_full directory)
.\Release\local_cpp_client.exe

# 5. In another PowerShell window, verify
curl http://127.0.0.1:8085/status

# 6. Check logs
Get-Content -Path "logs/cpp_client.log" -Tail 20
```

---

## Expected Timeline

| Step | Time | What Happens |
|------|------|--------------|
| Remove old build | 5-10 sec | Deletes build_full directory |
| Create new build | 1 sec | Creates fresh build_full directory |
| CMake configure | 10-30 sec | Checks dependencies, generates build files |
| Build | 2-5 min | Compiles C++ code |
| **Total** | **3-6 min** | Ready to run |

---

## Success Checklist

- [ ] Clean rebuild script runs without errors
- [ ] Old build directory is removed
- [ ] Fresh build directory is created
- [ ] CMake configuration succeeds
- [ ] Build completes with "Built target local_cpp_client"
- [ ] Executable exists at Release/local_cpp_client.exe
- [ ] Client starts and shows OpenCV version
- [ ] Console shows "Successfully opened stream"
- [ ] FPS > 0 in stats output
- [ ] Health endpoint shows stream_connected: true
- [ ] Logs show frames being processed

---

## Files Provided

- `clean_rebuild.ps1` - PowerShell clean rebuild script
- `clean_rebuild.cmd` - CMD batch clean rebuild script
- `CLEAN_REBUILD_GUIDE.md` - This guide

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

## Summary

| Aspect | Status |
|--------|--------|
| CMake cache conflict | ✅ Fixed by clean rebuild |
| Build directory | ✅ Fresh and clean |
| Configuration | ✅ Fresh CMake configuration |
| Compilation | ✅ Clean build from source |
| Ready to run | ✅ Yes |

**Run the clean rebuild script and you're good to go!**
