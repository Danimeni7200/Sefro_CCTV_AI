cmake_minimum_required(VERSION 3.10)
project(discovery_only_service)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Simple discovery only service
add_executable(discovery_only_updated discovery_only.cpp)

# Link libraries
target_link_libraries(discovery_only_updated PRIVATE
    ws2_32
)