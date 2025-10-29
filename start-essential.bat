@echo off
echo Starting Sefro CCTV AI Platform - Essential Services
echo =====================================================

echo.
echo This script will start the essential services in order:
echo 1. Cloud API (port 9000)
echo 2. AI Service (port 8000)
echo 3. Cloud Web (port 3000)
echo.

echo Press Ctrl+C if you want to cancel...
timeout /t 5 /nobreak >nul

echo.
echo [1/3] Starting Cloud API service...
cd packages/cloud_api
start "Cloud API" cmd /k "python main.py"
cd ../..

timeout /t 3 /nobreak >nul

echo.
echo [2/3] Starting AI Service...
cd packages/ai_service
start "AI Service" cmd /k "python main.py"
cd ../..

timeout /t 3 /nobreak >nul

echo.
echo [3/3] Starting Cloud Web interface...
cd packages/cloud_web
start "Cloud Web" cmd /k "npm run dev"
cd ../..

echo.
echo All services started!
echo.
echo You can now access:
echo - Cloud Web Interface: http://localhost:3000
echo - AI Service Health: http://localhost:8000/healthz
echo - Cloud API Health: http://localhost:9000/healthz
echo.
echo Default credentials:
echo - Username: admin
echo - Password: admin123
echo - API Token: DEV-TOKEN-CHANGE
echo.
echo Press any key to exit...
pause >nul