# Use Ubuntu 20.04 as base image
FROM ubuntu:20.04

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    curl \
    pkg-config \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Install OpenCV from system packages to reduce size
RUN apt-get update && apt-get install -y \
    libopencv-dev \
    libopencv-core-dev \
    libopencv-imgproc-dev \
    libopencv-highgui-dev \
    libopencv-videoio-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source files
COPY src ./src
COPY CMakeLists.txt .

# Build the application
RUN mkdir build && cd build && \
    cmake .. && \
    cmake --build . --config Release

# Expose health port
EXPOSE 8085

# Copy config file
COPY config.json .

# Run the simple client by default
CMD ["./build/simple_cpp_client", "/app/test_image.jpg"]