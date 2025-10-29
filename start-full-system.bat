@echo off
echo Starting Sefro CCTV AI Platform - Full System
echo =============================================

echo.
echo This script will start all services needed for the full system:
echo 1. Cloud API (port 9000)
echo 2. AI Service (port 8000)
echo 3. Cloud Web (port 3000)
echo 4. Discovery Service (port 8085)
echo.

echo Press Ctrl+C if you want to cancel...
timeout /t 3 /nobreak >nul

echo.
echo [1/4] Starting Cloud API service...
cd packages/cloud_api
start "Cloud API" cmd /k "python main.py"
cd ../..

timeout /t 3 /nobreak >nul

echo.
echo [2/4] Starting AI Service...
cd packages/ai_service
start "AI Service" cmd /k "python main.py"
cd ../..

timeout /t 3 /nobreak >nul

echo.
echo [3/4] Starting Cloud Web interface...
cd packages/cloud_web
start "Cloud Web" cmd /k "npm run dev"
cd ../..

timeout /t 3 /nobreak >nul

echo.
echo [4/4] Starting Discovery Service...
start "Discovery Service" cmd /k "python discovery_service.py"

echo.
echo All services started!
echo.
echo You can now access:
echo - Cloud Web Interface: http://localhost:3000
echo - AI Service Health: http://localhost:8000/healthz
echo - Cloud API Health: http://localhost:9000/healthz
echo - Discovery Service: http://localhost:8085/healthz
echo.
echo Default credentials:
echo - Username: admin
echo - Password: admin123
echo - API Token: DEV-TOKEN-CHANGE
echo.
echo To use the Electron app:
echo 1. Run start-electron.bat or navigate to packages/electron_app and run 'npm start'
echo 2. In the Electron app, go to Settings and verify AI Service Host is http://127.0.0.1:8000
echo 3. You can now use the 'کشف دوربین' (Camera Discovery) feature
echo.
echo Press any key to exit...
pause >nul