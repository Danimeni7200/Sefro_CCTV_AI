@echo off
REM This script attempts to compile the discovery service and streaming service using available compilers

echo Checking for available compilers...

REM Try to use cl (Visual Studio compiler) if available
where cl >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Found Visual Studio compiler
    echo Compiling discovery service with cl...
    cl /std:c++17 /I"./vcpkg/installed/x64-windows/include" discovery_only.cpp /link /LIBPATH:"./vcpkg/installed/x64-windows/lib" opencv_core4.lib opencv_imgproc4.lib opencv_videoio4.lib opencv_highgui4.lib ws2_32.lib /OUT:discovery_only_updated.exe
    
    echo Compiling streaming service with cl...
    cl /std:c++17 /I"./vcpkg/installed/x64-windows/include" streaming_service.cpp /link /LIBPATH:"./vcpkg/installed/x64-windows/lib" opencv_core4.lib opencv_imgproc4.lib opencv_videoio4.lib opencv_highgui4.lib ws2_32.lib /OUT:streaming_service.exe
    goto end
)

REM Try to use g++ if available
where g++ >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Found g++ compiler
    echo Compiling discovery service with g++...
    g++ -std=c++17 -I./vcpkg/installed/x64-windows/include discovery_only.cpp -L./vcpkg/installed/x64-windows/lib -lopencv_core4 -lopencv_imgproc4 -lopencv_videoio4 -lopencv_highgui4 -lws2_32 -o discovery_only_updated.exe
    
    echo Compiling streaming service with g++...
    g++ -std=c++17 -I./vcpkg/installed/x64-windows/include streaming_service.cpp -L./vcpkg/installed/x64-windows/lib -lopencv_core4 -lopencv_imgproc4 -lopencv_videoio4 -lopencv_highgui4 -lws2_32 -o streaming_service.exe
    goto end
)

echo No compatible compiler found. Please install Visual Studio or MinGW.

:end
echo Build process completed.