cmake_minimum_required(VERSION 3.10)
project(local_cpp_client)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set vcpkg paths
set(VCPKG_INSTALLED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows")
# Help CMake find vcpkg-provided packages that are transitive deps of OpenCV
set(Protobuf_DIR "${VCPKG_INSTALLED_DIR}/share/protobuf")
set(OpenCV_DIR "${VCPKG_INSTALLED_DIR}/share/opencv4")

# Find required packages (OpenCV optional)
find_package(OpenCV CONFIG QUIET PATHS ${VCPKG_INSTALLED_DIR})
if(OpenCV_FOUND)
    message(STATUS "Found OpenCV")
else()
    message(WARNING "OpenCV not found - will build minimal targets only")
endif()

# CURL is only needed for some targets, not all
find_package(CURL CONFIG PATHS ${VCPKG_INSTALLED_DIR})
if(CURL_FOUND)
    message(STATUS "Found CURL")
endif()

# JSON is needed for some targets
find_package(nlohmann_json CONFIG PATHS ${VCPKG_INSTALLED_DIR})
if(nlohmann_json_FOUND)
    message(STATUS "Found nlohmann_json")
endif()

# Main application (requires CURL, OpenCV, JSON)
if(OpenCV_FOUND AND CURL_FOUND AND nlohmann_json_FOUND)
    file(GLOB SOURCES "src/*.cpp")
    add_executable(cpp_client ${SOURCES})
else()
    message(WARNING "Skipping cpp_client: missing dependencies")
endif()

# Simple client (requires CURL)
if(CURL_FOUND)
    add_executable(simple_cpp_client src/simple_main.cpp)
    
    # Discovery test client
    add_executable(discovery_test src/discovery_test.cpp)
    
    # Standalone discovery service
    add_executable(discovery_service src/discovery_service.cpp src/health_server.cpp)
else()
    message(WARNING "Skipping simple_cpp_client, discovery_test, discovery_service: CURL not found")
endif()

# Simple discovery only service
if(OpenCV_FOUND)
    add_executable(discovery_only discovery_only.cpp)
else()
    message(WARNING "OpenCV not found - building minimal discovery_only without streaming support")
    add_executable(discovery_only discovery_only_basic.cpp)
endif()

# Streaming service (requires OpenCV)
if(OpenCV_FOUND)
    add_executable(streaming_service streaming_service.cpp)
endif()

# Link libraries (only if targets were created)
if(TARGET cpp_client)
    target_link_libraries(cpp_client PRIVATE
        opencv_core opencv_imgproc opencv_videoio opencv_highgui
        CURL::libcurl
        nlohmann_json::nlohmann_json
    )
endif()

if(TARGET simple_cpp_client)
    target_link_libraries(simple_cpp_client PRIVATE
        CURL::libcurl
    )
endif()

if(TARGET discovery_test)
    target_link_libraries(discovery_test PRIVATE
        CURL::libcurl
    )
endif()

if(TARGET discovery_service)
    target_link_libraries(discovery_service PRIVATE
        CURL::libcurl
    )
endif()

# discovery_only only needs OpenCV and ws2_32 (no CURL)
if(OpenCV_FOUND)
if(TARGET discovery_only)
    if(OpenCV_FOUND)
        target_link_libraries(discovery_only PRIVATE
            opencv_core opencv_imgproc opencv_videoio opencv_highgui
            ws2_32
        )
    else()
        target_link_libraries(discovery_only PRIVATE
            ws2_32
        )
    endif()
endif()
endif()

# streaming_service needs OpenCV
if(OpenCV_FOUND)
    if(TARGET streaming_service)
        target_link_libraries(streaming_service PRIVATE
            opencv_core opencv_imgproc opencv_videoio opencv_highgui
            ws2_32
        )
    endif()
endif()

# Stream test client
add_executable(stream_test_client stream_test_client.cpp)
target_link_libraries(stream_test_client PRIVATE
    ws2_32
)

# OpenCV stream test
if(OpenCV_FOUND)
    add_executable(test_opencv_stream test_opencv_stream.cpp)
    target_link_libraries(test_opencv_stream PRIVATE
        opencv_core opencv_videoio opencv_imgcodecs
    )
endif()

# Include directories
if(TARGET discovery_service)
    target_include_directories(discovery_service PRIVATE src)
endif()

# Compiler definitions
add_definitions(-DOPENCV_VERSION="${OpenCV_VERSION}")