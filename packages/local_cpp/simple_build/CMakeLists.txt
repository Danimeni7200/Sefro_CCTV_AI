cmake_minimum_required(VERSION 3.10)
project(simple_cpp_client)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set vcpkg paths
set(VCPKG_INSTALLED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg/installed/x64-windows")

# Find required packages
find_package(CURL CONFIG REQUIRED PATHS ${VCPKG_INSTALLED_DIR})

# Simple client
add_executable(simple_cpp_client ../src/simple_main.cpp)

# Link libraries
target_link_libraries(simple_cpp_client PRIVATE
    CURL::libcurl
)