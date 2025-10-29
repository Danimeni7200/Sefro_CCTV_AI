@echo off
echo Building Basic Discovery Service...
echo ==================================

cd packages\local_cpp

if not exist build_discovery mkdir build_discovery
cd build_discovery

cl /EHsc /MD /O2 /I. ..\discovery_only_basic.cpp /Fe:discovery_only.exe ws2_32.lib

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build complete! Executable: discovery_only.exe
    copy discovery_only.exe ..\
) else (
    echo Build failed!
)

cd ..
pause


