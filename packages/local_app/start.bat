@echo off
echo Starting Fast CCTV Processing Service...
cd /d "%~dp0"
python start_service.py
pause
