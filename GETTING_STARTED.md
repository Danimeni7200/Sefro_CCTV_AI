# Getting Started with Sefro CCTV AI Platform

This document explains how to quickly get started with the Sefro CCTV AI Platform.

## Quick Start

To run the essential services of the platform:

### Option 1: Using Batch Script (Windows)
Double-click on `start-essential.bat` or run from command prompt:
```cmd
start-essential.bat
```

### Option 2: Using PowerShell Script (Windows)
Run from PowerShell:
```powershell
.\start-essential.ps1
```

### Option 3: Manual Start
Follow the detailed instructions in [RUNNING_SERVICES.md](RUNNING_SERVICES.md)

## What Services Will Start?

The quick start scripts will launch these essential services in order:

1. **Cloud API** - Authentication and data management (port 9000)
2. **AI Service** - License plate recognition engine (port 8000)
3. **Cloud Web** - Web dashboard for viewing results (port 3000)

## Accessing the Platform

After starting the services:

1. Open your web browser and go to: http://localhost:3000
2. Log in with the default credentials:
   - Username: `admin`
   - Password: `admin123`
   - API Token: `DEV-TOKEN-CHANGE`

## Default Credentials Warning

For security, you should change the default credentials in production:
- Cloud API credentials are in `packages/cloud_api/main.py`
- Look for the `init_db()` function where the default admin user is created

## Next Steps

After verifying the essential services are working:

1. Check the health of each service:
   - AI Service: http://localhost:8000/healthz
   - Cloud API: http://localhost:9000/healthz

2. Try uploading an image for license plate recognition through the web interface

3. Run the Electron Desktop App:
   - Double-click on `start-electron.bat` to start the desktop application
   - The Electron app will connect to the AI Service to display recognition results

4. For advanced usage, see [RUNNING_SERVICES.md](RUNNING_SERVICES.md) for instructions on:
   - Running the Local C++ Client for video processing
   - Configuring services with environment variables
   - Setting up authentication properly

## Troubleshooting

If services don't start:

1. Make sure required ports (8000, 9000, 3000) are available
2. Verify Python and Node.js are installed
3. Check that all dependencies are installed:
   - For AI Service and Cloud API: `pip install -r requirements.txt`
   - For Cloud Web: `npm install`
4. Check the console windows for error messages

## Stopping Services

To stop the services, simply close the console windows that were opened by the start script.