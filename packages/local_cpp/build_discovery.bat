@echo off
echo Building Discovery & Streaming Service...
echo ======================================

cd packages\local_cpp

if not exist build_discovery mkdir build_discovery
cd build_discovery

cmake .. -DCMAKE_BUILD_TYPE=Release

cmake --build . --config Release

echo.
echo Build complete! Executable is at: packages\local_cpp\build_discovery\Release\discovery_only.exe
echo.
pause
