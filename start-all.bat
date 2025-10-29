@echo off
echo Starting Sefro CCTV AI Platform...

echo Make sure Docker is installed and running before executing this script.
echo Download Docker Desktop from: https://www.docker.com/products/docker-desktop

echo.
echo To start all services, run:
echo   docker-compose up -d

echo.
echo To stop all services, run:
echo   docker-compose down

echo.
echo Services will be available at:
echo   AI Service: http://localhost:8000
echo   Cloud API: http://localhost:9000
echo   Cloud Web: http://localhost:3000

pause