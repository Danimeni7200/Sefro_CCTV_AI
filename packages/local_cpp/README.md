# LPR C++ Client

A high-performance C++ client for License Plate Recognition (LPR) that can process video streams and send frames to a Python AI service for analysis.

## Features

### Current Implementation (Simple Mode)
- ✅ **Image Processing**: Send individual images to AI service
- ✅ **Health Checks**: Verify AI service availability
- ✅ **Error Handling**: Robust error handling and retry logic
- ✅ **Configuration**: JSON-based configuration
- ✅ **Logging**: Structured logging with multiple levels

### Planned Features (Full Streaming Mode)
- 🔄 **Stream Processing**: RTSP/HTTP stream capture and processing
- 🔄 **Real-time Pipeline**: Multi-threaded processing pipeline
- 🔄 **Preprocessing**: Image enhancement, resizing, quality filtering
- 🔄 **Backpressure Handling**: Intelligent frame dropping strategies
- 🔄 **Health Monitoring**: HTTP server for health checks and metrics
- 🔄 **Hot Reload**: Configuration changes without restart

## Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Video Stream  │───▶│   C++ Client     │───▶│  Python AI      │
│  (RTSP/HTTP)    │    │                  │    │   Service       │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                              │
                              ▼
                       ┌──────────────────┐
                       │  Health Server   │
                       │  (Port 8085)     │
                       └──────────────────┘
```

## Quick Start

### Prerequisites

1. **Visual Studio 2022** with C++ development tools
2. **vcpkg** package manager
3. **Python AI Service** running on port 8000

### Installation

1. **Install dependencies via vcpkg:**
   ```bash
   vcpkg install curl:x64-windows
   ```

2. **Build the project:**
   ```bash
   cd packages/local_cpp
   mkdir build && cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
   cmake --build . --config Release
   ```

### Usage

#### Simple Mode (Current)
Process individual images:

```bash
# Basic usage
./local_cpp_client.exe image.jpg

# With custom camera ID and AI host
./local_cpp_client.exe image.jpg CAM01 http://127.0.0.1:8000
```

#### Full Streaming Mode (Planned)
Process video streams:

```bash
# Using configuration file
./local_cpp_client.exe config.json

# With custom config
./local_cpp_client.exe my_config.json
```

## Configuration

### Simple Mode
Command line arguments:
- `image_path`: Path to the image file to process
- `camera_id`: Camera identifier (default: "CAM01")
- `ai_host`: AI service URL (default: "http://127.0.0.1:8000")

### Full Streaming Mode
Configuration file (`config.json`):

```json
{
  "stream": {
    "url": "rtsp://admin:admin@192.168.1.100:554/stream1",
    "camera_id": "CAM01",
    "fps_cap": 15,
    "reconnect_delay_ms": 1000,
    "max_reconnect_attempts": -1,
    "use_hardware_decode": true
  },
  "ai_service": {
    "host": "http://127.0.0.1:8000",
    "timeout_ms": 5000,
    "retry_count": 3,
    "retry_delay_ms": 1000
  },
  "pipeline": {
    "queue_size": 32,
    "drop_policy": "drop_oldest",
    "max_inference_queue": 16
  },
  "preprocessing": {
    "target_width": 1280,
    "target_height": 720,
    "letterbox": true,
    "gamma": 1.0,
    "denoise": false,
    "sharpen": false,
    "quality_threshold": 0.3
  },
  "privacy": {
    "mask_plate_on_storage": false,
    "anonymize": false,
    "store_original_image": true
  },
  "logging": {
    "level": "INFO",
    "file": "logs/cpp_client.log",
    "rotate_daily": true,
    "console_output": true
  },
  "health": {
    "port": 8085,
    "bind_address": "0.0.0.0",
    "metrics_interval_ms": 1000
  }
}
```

## API Integration

The C++ client communicates with the Python AI service via HTTP multipart form data:

### Request Format
- **Endpoint**: `POST /infer`
- **Content-Type**: `multipart/form-data`
- **Fields**:
  - `image`: Image file (JPEG format)
  - `camera_id`: Camera identifier string

### Response Format
```json
{
  "plate_text": "ABC1234",
  "confidence": 0.95,
  "bbox": {
    "x1": 100, "y1": 50,
    "x2": 200, "y2": 80
  },
  "vehicle_color": "white",
  "vehicle_type": "sedan"
}
```

## Health Monitoring

### Health Check Endpoint
- **URL**: `http://localhost:8085/healthz`
- **Method**: GET
- **Response**: `200 OK` or `503 Service Unavailable`

### Metrics Endpoint
- **URL**: `http://localhost:8085/metrics`
- **Method**: GET
- **Format**: Prometheus text format

Example metrics:
```
cpp_client_fps 15.2
cpp_client_queue_size 8
cpp_client_ai_healthy 1
cpp_client_stream_connected 1
```

## Performance Characteristics

### Simple Mode
- **Latency**: ~50-100ms per image
- **Throughput**: Limited by network and AI service
- **Memory**: ~10MB base usage

### Full Streaming Mode (Planned)
- **Latency**: ~100-200ms end-to-end
- **Throughput**: 15-30 FPS (configurable)
- **Memory**: ~50-100MB with buffering
- **CPU**: 10-20% on modern hardware

## Error Handling

### Connection Errors
- Automatic retry with exponential backoff
- Configurable retry count and delays
- Graceful degradation on persistent failures

### Stream Errors
- Automatic reconnection to video streams
- Frame dropping on backpressure
- Quality-based frame filtering

### AI Service Errors
- Health monitoring and circuit breaker pattern
- Queue management during service outages
- Detailed error logging and metrics

## Development

### Building from Source

1. **Clone the repository**
2. **Install vcpkg dependencies:**
   ```bash
   vcpkg install curl:x64-windows
   vcpkg install opencv4:x64-windows  # For full streaming mode
   vcpkg install nlohmann-json:x64-windows  # For full streaming mode
   ```

3. **Configure and build:**
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
   cmake --build . --config Release
   ```

### Code Structure

```
src/
├── simple_main.cpp          # Simple image processing mode
├── main.cpp                 # Full streaming mode (planned)
├── config.hpp/cpp          # Configuration management
├── stream_reader.hpp/cpp   # Video stream capture
├── ring_buffer.hpp/cpp     # Lock-free frame queuing
├── preprocessor.hpp/cpp    # Image preprocessing
├── inference_client.hpp/cpp # AI service communication
├── logger.hpp/cpp          # Structured logging
├── health_server.hpp/cpp   # Health monitoring
├── pipeline.hpp/cpp        # Main processing pipeline
└── frame.hpp               # Data structures
```

## Troubleshooting

### Common Issues

1. **"AI service is not healthy"**
   - Ensure Python AI service is running on port 8000
   - Check network connectivity
   - Verify service responds to `/healthz`

2. **"Failed to initialize CURL"**
   - Check vcpkg installation
   - Verify CURL library is properly linked

3. **"Failed to open image"**
   - Check file path and permissions
   - Ensure image file exists and is readable

4. **Build errors**
   - Verify Visual Studio 2022 is installed
   - Check vcpkg toolchain is properly configured
   - Ensure all dependencies are installed

### Debug Mode

Enable debug logging by setting log level to "DEBUG" in configuration:

```json
{
  "logging": {
    "level": "DEBUG",
    "console_output": true
  }
}
```

## License

This project is part of the LPR (License Plate Recognition) system and follows the same license terms.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## Support

For issues and questions:
1. Check the troubleshooting section
2. Review the logs for error details
3. Create an issue with detailed information




A high-performance C++ client for License Plate Recognition (LPR) that can process video streams and send frames to a Python AI service for analysis.

## Features

### Current Implementation (Simple Mode)
- ✅ **Image Processing**: Send individual images to AI service
- ✅ **Health Checks**: Verify AI service availability
- ✅ **Error Handling**: Robust error handling and retry logic
- ✅ **Configuration**: JSON-based configuration
- ✅ **Logging**: Structured logging with multiple levels

### Planned Features (Full Streaming Mode)
- 🔄 **Stream Processing**: RTSP/HTTP stream capture and processing
- 🔄 **Real-time Pipeline**: Multi-threaded processing pipeline
- 🔄 **Preprocessing**: Image enhancement, resizing, quality filtering
- 🔄 **Backpressure Handling**: Intelligent frame dropping strategies
- 🔄 **Health Monitoring**: HTTP server for health checks and metrics
- 🔄 **Hot Reload**: Configuration changes without restart

## Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Video Stream  │───▶│   C++ Client     │───▶│  Python AI      │
│  (RTSP/HTTP)    │    │                  │    │   Service       │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                              │
                              ▼
                       ┌──────────────────┐
                       │  Health Server   │
                       │  (Port 8085)     │
                       └──────────────────┘
```

## Quick Start

### Prerequisites

1. **Visual Studio 2022** with C++ development tools
2. **vcpkg** package manager
3. **Python AI Service** running on port 8000

### Installation

1. **Install dependencies via vcpkg:**
   ```bash
   vcpkg install curl:x64-windows
   ```

2. **Build the project:**
   ```bash
   cd packages/local_cpp
   mkdir build && cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
   cmake --build . --config Release
   ```

### Usage

#### Simple Mode (Current)
Process individual images:

```bash
# Basic usage
./local_cpp_client.exe image.jpg

# With custom camera ID and AI host
./local_cpp_client.exe image.jpg CAM01 http://127.0.0.1:8000
```

#### Full Streaming Mode (Planned)
Process video streams:

```bash
# Using configuration file
./local_cpp_client.exe config.json

# With custom config
./local_cpp_client.exe my_config.json
```

## Configuration

### Simple Mode
Command line arguments:
- `image_path`: Path to the image file to process
- `camera_id`: Camera identifier (default: "CAM01")
- `ai_host`: AI service URL (default: "http://127.0.0.1:8000")

### Full Streaming Mode
Configuration file (`config.json`):

```json
{
  "stream": {
    "url": "rtsp://admin:admin@192.168.1.100:554/stream1",
    "camera_id": "CAM01",
    "fps_cap": 15,
    "reconnect_delay_ms": 1000,
    "max_reconnect_attempts": -1,
    "use_hardware_decode": true
  },
  "ai_service": {
    "host": "http://127.0.0.1:8000",
    "timeout_ms": 5000,
    "retry_count": 3,
    "retry_delay_ms": 1000
  },
  "pipeline": {
    "queue_size": 32,
    "drop_policy": "drop_oldest",
    "max_inference_queue": 16
  },
  "preprocessing": {
    "target_width": 1280,
    "target_height": 720,
    "letterbox": true,
    "gamma": 1.0,
    "denoise": false,
    "sharpen": false,
    "quality_threshold": 0.3
  },
  "privacy": {
    "mask_plate_on_storage": false,
    "anonymize": false,
    "store_original_image": true
  },
  "logging": {
    "level": "INFO",
    "file": "logs/cpp_client.log",
    "rotate_daily": true,
    "console_output": true
  },
  "health": {
    "port": 8085,
    "bind_address": "0.0.0.0",
    "metrics_interval_ms": 1000
  }
}
```

## API Integration

The C++ client communicates with the Python AI service via HTTP multipart form data:

### Request Format
- **Endpoint**: `POST /infer`
- **Content-Type**: `multipart/form-data`
- **Fields**:
  - `image`: Image file (JPEG format)
  - `camera_id`: Camera identifier string

### Response Format
```json
{
  "plate_text": "ABC1234",
  "confidence": 0.95,
  "bbox": {
    "x1": 100, "y1": 50,
    "x2": 200, "y2": 80
  },
  "vehicle_color": "white",
  "vehicle_type": "sedan"
}
```

## Health Monitoring

### Health Check Endpoint
- **URL**: `http://localhost:8085/healthz`
- **Method**: GET
- **Response**: `200 OK` or `503 Service Unavailable`

### Metrics Endpoint
- **URL**: `http://localhost:8085/metrics`
- **Method**: GET
- **Format**: Prometheus text format

Example metrics:
```
cpp_client_fps 15.2
cpp_client_queue_size 8
cpp_client_ai_healthy 1
cpp_client_stream_connected 1
```

## Performance Characteristics

### Simple Mode
- **Latency**: ~50-100ms per image
- **Throughput**: Limited by network and AI service
- **Memory**: ~10MB base usage

### Full Streaming Mode (Planned)
- **Latency**: ~100-200ms end-to-end
- **Throughput**: 15-30 FPS (configurable)
- **Memory**: ~50-100MB with buffering
- **CPU**: 10-20% on modern hardware

## Error Handling

### Connection Errors
- Automatic retry with exponential backoff
- Configurable retry count and delays
- Graceful degradation on persistent failures

### Stream Errors
- Automatic reconnection to video streams
- Frame dropping on backpressure
- Quality-based frame filtering

### AI Service Errors
- Health monitoring and circuit breaker pattern
- Queue management during service outages
- Detailed error logging and metrics

## Development

### Building from Source

1. **Clone the repository**
2. **Install vcpkg dependencies:**
   ```bash
   vcpkg install curl:x64-windows
   vcpkg install opencv4:x64-windows  # For full streaming mode
   vcpkg install nlohmann-json:x64-windows  # For full streaming mode
   ```

3. **Configure and build:**
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
   cmake --build . --config Release
   ```

### Code Structure

```
src/
├── simple_main.cpp          # Simple image processing mode
├── main.cpp                 # Full streaming mode (planned)
├── config.hpp/cpp          # Configuration management
├── stream_reader.hpp/cpp   # Video stream capture
├── ring_buffer.hpp/cpp     # Lock-free frame queuing
├── preprocessor.hpp/cpp    # Image preprocessing
├── inference_client.hpp/cpp # AI service communication
├── logger.hpp/cpp          # Structured logging
├── health_server.hpp/cpp   # Health monitoring
├── pipeline.hpp/cpp        # Main processing pipeline
└── frame.hpp               # Data structures
```

## Troubleshooting

### Common Issues

1. **"AI service is not healthy"**
   - Ensure Python AI service is running on port 8000
   - Check network connectivity
   - Verify service responds to `/healthz`

2. **"Failed to initialize CURL"**
   - Check vcpkg installation
   - Verify CURL library is properly linked

3. **"Failed to open image"**
   - Check file path and permissions
   - Ensure image file exists and is readable

4. **Build errors**
   - Verify Visual Studio 2022 is installed
   - Check vcpkg toolchain is properly configured
   - Ensure all dependencies are installed

### Debug Mode

Enable debug logging by setting log level to "DEBUG" in configuration:

```json
{
  "logging": {
    "level": "DEBUG",
    "console_output": true
  }
}
```

## License

This project is part of the LPR (License Plate Recognition) system and follows the same license terms.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## Support

For issues and questions:
1. Check the troubleshooting section
2. Review the logs for error details
3. Create an issue with detailed information


