# Docker Setup for Sefro CCTV AI Platform

This Docker setup allows you to run the Sefro CCTV AI platform with a single command. We provide two configurations:

1. **Lite version** (`docker-compose-lite.yml`) - Core services only (AI Service, Cloud API, Cloud Web)
2. **Full version** (`docker-compose.yml`) - All services including the resource-intensive Local C++ Client

## Services Included

1. **AI Service** - Python FastAPI service running on port 8000
2. **Cloud API** - Python FastAPI backend running on port 9000
3. **Cloud Web** - Next.js frontend running on port 3000
4. **Local C++ Client** - C++ video processing client (resource-intensive)
5. **Electron App** - Desktop application (requires special setup for GUI)

## Prerequisites

- Docker installed on your system
- Docker Compose installed on your system

## Quick Start (Recommended: Lite Version)

To start the core services with one command:

```bash
docker-compose -f docker-compose-lite.yml up -d
```

## Full Version (Includes resource-intensive C++ client)

To start all services:

```bash
docker-compose up -d
```

This will build and start all services in detached mode.

## Accessing Services

Once the services are running, you can access them at:

- **AI Service Health Check**: http://localhost:8000/healthz
- **Cloud Web Interface**: http://localhost:3000
- **Cloud API Health Check**: http://localhost:9000/healthz

## Service Details

### AI Service
- Runs the license plate recognition AI model
- Accepts image uploads for processing
- Communicates with the Cloud API when enabled

### Cloud API
- Provides authentication and data storage
- Receives processed LPR data from AI Service
- Serves data to the Cloud Web interface

### Cloud Web
- Next.js frontend for viewing LPR results
- Dashboard for monitoring system status
- Analytics interface

### Local C++ Client
- Processes video streams from cameras
- Sends frames to the AI Service for processing
- Can handle RTSP streams

### Electron App
- Desktop application for local monitoring
- Note: Running GUI applications in Docker requires additional setup

## Configuration

Each service can be configured through environment variables in the docker-compose.yml file.

## Stopping Services

To stop lite version services:

```bash
docker-compose -f docker-compose-lite.yml down
```

To stop full version services:

```bash
docker-compose down
```

To stop and remove all data:

```bash
docker-compose down -v
```

To stop full version and remove all data:

```bash
docker-compose down -v
```

## Building Services Individually

If you need to rebuild a specific service:

For lite version:
```bash
docker-compose -f docker-compose-lite.yml build [service_name]
```

For full version:
```bash
docker-compose build [service_name]
```

For example:
```bash
docker-compose -f docker-compose-lite.yml build ai_service
```

## Troubleshooting

If you encounter issues:

1. Check that all ports are available
2. Ensure Docker has sufficient resources allocated
3. Check the logs for each service:
   
   For lite version:
   ```bash
   docker-compose -f docker-compose-lite.yml logs [service_name]
   ```
   
   For full version:
   ```bash
   docker-compose logs [service_name]
   ```

## Notes

- The Electron app requires special setup to display GUI elements when running in Docker
- The Local C++ client needs an image file to process; by default, it looks for test_image.jpg
- All services are connected through a Docker network for internal communication
- The Lite version excludes the resource-intensive Local C++ client, reducing the overall footprint significantly
- For development, we recommend starting with the Lite version and adding services as needed