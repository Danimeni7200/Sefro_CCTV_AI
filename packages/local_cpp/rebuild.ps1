# PowerShell rebuild script for local_cpp
Write-Host "Building C++ LPR Client..." -ForegroundColor Cyan
Write-Host ""

# Navigate to build directory
$buildDir = "$PSScriptRoot\build_full"
if (-not (Test-Path $buildDir)) {
    Write-Host "Error: build_full directory not found at $buildDir" -ForegroundColor Red
    exit 1
}

Set-Location $buildDir
Write-Host "Working directory: $(Get-Location)" -ForegroundColor Green
Write-Host ""

# Run CMake configure
Write-Host "Step 1: Configuring CMake..." -ForegroundColor Yellow
cmake ..
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    exit 1
}
Write-Host "CMake configuration successful!" -ForegroundColor Green
Write-Host ""

# Build Release
Write-Host "Step 2: Building Release configuration..." -ForegroundColor Yellow
cmake --build . --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}
Write-Host "Build successful!" -ForegroundColor Green
Write-Host ""

# Show next steps
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Build Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. Run the client:" -ForegroundColor White
Write-Host "   .\Release\local_cpp_client.exe" -ForegroundColor Cyan
Write-Host ""
Write-Host "2. In another PowerShell window, test the health endpoint:" -ForegroundColor White
Write-Host "   curl http://127.0.0.1:8085/status" -ForegroundColor Cyan
Write-Host ""
Write-Host "3. Check for success indicators in console output:" -ForegroundColor White
Write-Host "   - [StreamReader] Successfully opened stream with backend: FFMPEG" -ForegroundColor Green
Write-Host "   - [StreamReader] Stream properties - FPS: 30, Resolution: 1920x1080" -ForegroundColor Green
Write-Host "   - Stats - FPS: 15.00, Processed: 15, Dropped: 0" -ForegroundColor Green
Write-Host ""
