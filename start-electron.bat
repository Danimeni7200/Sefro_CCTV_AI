@echo off
echo Starting Sefro CCTV AI Platform - Electron Desktop App
echo ====================================================

echo.
echo This script will start the Electron desktop application.
echo Make sure the essential services are running:
echo - Cloud API (port 9000)
echo - AI Service (port 8000)
echo.

echo Installing dependencies (if needed)...
cd packages/electron_app
npm install
echo.

echo Starting Electron app...
npm start

echo.
echo Electron app closed.
cd ../..