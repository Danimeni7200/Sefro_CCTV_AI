@echo off
echo Starting Discovery & Streaming Service...
echo =====================================

cd packages\local_cpp

if not exist discovery_only.exe (
    echo Error: discovery_only.exe not found!
    echo Please build it first by running: build_discovery.bat
    pause
    exit /b 1
)

echo Starting discovery service on port 8086...
echo.
start "Discovery Service" cmd /k "discovery_only.exe"

timeout /t 2 /nobreak >nul

echo.
echo Discovery & Streaming Service is now running on port 8086
echo.
echo Endpoints:
echo - Discovery: POST http://127.0.0.1:8086/discover?ip=X&user=Y&pass=Z&brand=B
echo - Add Stream: POST http://127.0.0.1:8086/add_stream?id=X&url=Y
echo - Remove Stream: POST http://127.0.0.1:8086/remove_stream?id=X
echo - Stream Frame: GET http://127.0.0.1:8086/stream/X
echo - Health: GET http://127.0.0.1:8086/health
echo.
pause


