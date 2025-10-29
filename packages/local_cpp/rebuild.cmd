@echo off
REM Batch script to rebuild local_cpp client
setlocal enabledelayedexpansion

echo.
echo ========================================
echo Building C++ LPR Client
echo ========================================
echo.

REM Navigate to build directory
cd /d "%~dp0build_full"
if errorlevel 1 (
    echo Error: Could not navigate to build_full directory
    pause
    exit /b 1
)

echo Working directory: %cd%
echo.

REM Run CMake configure
echo Step 1: Configuring CMake...
cmake ..
if errorlevel 1 (
    echo CMake configuration failed!
    pause
    exit /b 1
)
echo CMake configuration successful!
echo.

REM Build Release
echo Step 2: Building Release configuration...
cmake --build . --config Release
if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)
echo Build successful!
echo.

REM Show next steps
echo ========================================
echo Build Complete!
echo ========================================
echo.
echo Next steps:
echo 1. Run the client:
echo    .\Release\local_cpp_client.exe
echo.
echo 2. In another terminal, test the health endpoint:
echo    curl http://127.0.0.1:8085/status
echo.
echo 3. Check for success indicators in console output:
echo    - [StreamReader] Successfully opened stream with backend: FFMPEG
echo    - [StreamReader] Stream properties - FPS: 30, Resolution: 1920x1080
echo    - Stats - FPS: 15.00, Processed: 15, Dropped: 0
echo.
pause
