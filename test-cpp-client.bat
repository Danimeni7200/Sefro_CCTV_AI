@echo off
echo Testing Sefro C++ Client with Sample Image
echo =========================================

echo.
echo 1. Checking if AI service is running...
curl -s http://127.0.0.1:8000/healthz | findstr "ok" >nul
if %errorlevel% == 0 (
    echo    AI Service is running and healthy
) else (
    echo    ERROR: AI Service is not running or not healthy
    echo    Please start the AI service first:
    echo    cd packages/ai_service
    echo    python main.py
    echo.
    exit /b 1
)

echo.
echo 2. Testing C++ client with sample image...
cd /d "%~dp0packages\local_cpp"

if exist simple_cpp_client.exe (
    echo    Found simple_cpp_client.exe
) else (
    echo    ERROR: simple_cpp_client.exe not found
    echo    Please build the C++ client first
    exit /b 1
)

if exist test_image.jpg (
    echo    Found test_image.jpg
) else (
    echo    Creating test image...
    python -c "import numpy as np; import cv2; img = np.zeros((100, 200, 3), dtype=np.uint8); img[:, :] = [255, 255, 255]; cv2.putText(img, 'Test', (50, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 0), 2); cv2.imwrite('test_image.jpg', img); print('Test image created')"
)

echo.
echo 3. Running inference...
echo    Command: .\simple_cpp_client.exe test_image.jpg CAM01 http://127.0.0.1:8000
echo.
.\simple_cpp_client.exe test_image.jpg CAM01 http://127.0.0.1:8000

echo.
echo 4. Done!
echo.
echo If you see inference results above, the C++ client is working correctly.
echo If not, check that all services are running properly.