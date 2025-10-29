# 🚀 CCTV Stream Recognition E2E Test Suite - START HERE

## ⚡ Quick Start (2 Minutes)

### Step 1: Open Terminal
- **Windows**: Press `Win + R`, type `cmd`, press Enter
- **Linux/macOS**: Open Terminal

### Step 2: Navigate to Tests
```bash
cd packages/local_cpp/tests
```

### Step 3: Run Diagnostics
```bash
python cctv_diagnostics.py
```

### Step 4: Review Results
- ✓ Green = Tests passed
- ✗ Red = Tests failed
- Follow recommendations

---

## 📊 What You'll See

### Success ✓
```
✓ PASS | DNS Resolution
✓ PASS | Ping Host
✓ PASS | Port Check (554)
✓ PASS | FFMPEG Support
✓ PASS | Stream Connection

Summary: 5/5 tests passed
→ Your CCTV stream should work!
```

### Failure ✗
```
✗ FAIL | Port Check (554)
       Port 554 is closed or unreachable

Recommendations:
• Verify RTSP port (554) is open on camera
• Check camera firewall settings
```

---

## 🎯 What This Does

This test suite diagnoses why your app can't connect to CCTV cameras:

1. **Network Tests** - Checks if camera is reachable
2. **URL Validation** - Verifies RTSP URL format
3. **OpenCV Tests** - Checks if FFMPEG is available
4. **Stream Tests** - Attempts actual connection
5. **Reporting** - Generates detailed report

---

## 📁 Files Included

```
packages/local_cpp/tests/
├── cctv_diagnostics.py          ← Python tool (run this!)
├── e2e_cctv_stream_test.cpp     ← C++ tests
├── run_diagnostics.bat          ← Windows launcher
├── run_diagnostics.ps1          ← PowerShell launcher
├── GETTING_STARTED.md           ← Quick start guide
├── QUICK_REFERENCE.md           ← Command reference
├── README.md                    ← Full documentation
├── TROUBLESHOOTING_GUIDE.md     ← Problem solving
└── INDEX.md                     ← Navigation guide
```

---

## 🔧 Common Fixes

### Issue: "Port 554 is closed"
```
1. Access camera web interface: http://192.168.4.252
2. Login with admin credentials
3. Find "Network" or "Stream" settings
4. Enable RTSP service
5. Save and reboot camera
```

### Issue: "Failed to ping camera"
```
1. Check camera is powered on
2. Check network cable is connected
3. Verify IP address: ping 192.168.4.252
```

### Issue: "OpenCV does NOT have FFMPEG support"
```
# Windows
vcpkg install ffmpeg:x64-windows
cd packages/local_cpp
rmdir /s /q build_full
mkdir build_full && cd build_full
cmake .. -DCMAKE_BUILD_TYPE=Release -DWITH_FFMPEG=ON
cmake --build . --config Release

# Linux
sudo apt-get install libffmpeg-dev libavcodec-dev libavformat-dev
cd packages/local_cpp
rm -rf build_full
mkdir build_full && cd build_full
cmake .. -DCMAKE_BUILD_TYPE=Release -DWITH_FFMPEG=ON
cmake --build . -j$(nproc)
```

### Issue: "Failed to open stream"
```
1. Verify RTSP URL is correct
2. Check camera credentials (username/password)
3. Test with VLC: vlc "rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_main"
```

---

## 📚 Documentation

| Document | Purpose | Time |
|----------|---------|------|
| **GETTING_STARTED.md** | Quick setup guide | 5 min |
| **QUICK_REFERENCE.md** | Command reference | 2 min |
| **README.md** | Full documentation | 15 min |
| **TROUBLESHOOTING_GUIDE.md** | Problem solving | 20 min |
| **INDEX.md** | Navigation guide | 5 min |

---

## ✅ Success Checklist

- [ ] Run: `python cctv_diagnostics.py`
- [ ] All tests pass (or understand failures)
- [ ] Fixed any issues
- [ ] Verified with VLC
- [ ] Application runs without errors
- [ ] Health endpoint responds: `curl http://localhost:8085/healthz`

---

## 🎓 Next Steps

### If All Tests Pass ✓
1. Run the application:
   ```bash
   cd packages/local_cpp/build_full/Release
   ./local_cpp_client.exe
   ```

2. Check health endpoint:
   ```bash
   curl http://localhost:8085/healthz
   ```

### If Tests Fail ✗
1. Read the recommendations in the output
2. Consult `TROUBLESHOOTING_GUIDE.md`
3. Fix the issue
4. Re-run diagnostics
5. Repeat until all tests pass

---

## 🆘 Quick Help

### Python Not Found
```
Install from: https://www.python.org/
Make sure to check "Add Python to PATH"
```

### OpenCV Not Installed
```bash
pip install opencv-python
```

### CMake Not Found
```
Install from: https://cmake.org/
```

---

## 💡 Pro Tips

1. **Save the report**: Diagnostic tool saves results to:
   - `cctv_diagnostics_report.txt` (human-readable)
   - `cctv_diagnostics_report.json` (machine-readable)

2. **Test with VLC**: Always verify RTSP URL works:
   ```bash
   vlc "rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_main"
   ```

3. **Check logs**: Application logs are in:
   ```
   packages/local_cpp/logs/cpp_client.log
   ```

4. **Verify network**: Use ping to check connectivity:
   ```bash
   ping 192.168.4.252
   ```

---

## 🚀 Ready?

```bash
cd packages/local_cpp/tests
python cctv_diagnostics.py
```

**That's it!** The tool will guide you through the rest.

---

## 📞 Need Help?

- **Quick lookup**: See `QUICK_REFERENCE.md`
- **Detailed help**: See `TROUBLESHOOTING_GUIDE.md`
- **Full guide**: See `README.md`
- **Navigation**: See `INDEX.md`

---

## 🎯 What Gets Tested

✓ Network connectivity (DNS, Ping, Port)
✓ RTSP URL format validation
✓ OpenCV installation and FFMPEG support
✓ Available video capture backends
✓ Actual RTSP stream connection
✓ Frame reading capability
✓ Multiple backend fallback
✓ Comprehensive diagnostics report

---

## ⏱️ Time Estimates

| Task | Time |
|------|------|
| Run diagnostics | 30 seconds |
| Review results | 2 minutes |
| Fix common issue | 5-15 minutes |
| Rebuild OpenCV | 10-30 minutes |
| Total (worst case) | ~1 hour |

---

## 🎉 Success Indicators

You'll know it's working when:
- ✓ All diagnostic tests pass
- ✓ VLC can play the RTSP stream
- ✓ Application starts without errors
- ✓ Health endpoint responds
- ✓ Frames are being processed
- ✓ Logs show successful inference

---

**Let's get started!** 🚀

```
cd packages/local_cpp/tests
python cctv_diagnostics.py
```

---

*For detailed information, see the documentation files in the tests directory.*
