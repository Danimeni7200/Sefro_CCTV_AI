# PowerShell clean rebuild script for local_cpp
Write-Host "Cleaning and Rebuilding C++ LPR Client..." -ForegroundColor Cyan
Write-Host ""

$projectDir = "$PSScriptRoot"
$buildDir = "$projectDir\build_full"

Write-Host "Project directory: $projectDir" -ForegroundColor Green
Write-Host "Build directory: $buildDir" -ForegroundColor Green
Write-Host ""

# Step 1: Remove old build directory
Write-Host "Step 1: Cleaning old build files..." -ForegroundColor Yellow
if (Test-Path $buildDir) {
    Write-Host "Removing $buildDir" -ForegroundColor Yellow
    Remove-Item -Path $buildDir -Recurse -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1
    Write-Host "Old build directory removed!" -ForegroundColor Green
} else {
    Write-Host "No existing build directory found" -ForegroundColor Green
}
Write-Host ""

# Step 2: Create fresh build directory
Write-Host "Step 2: Creating fresh build directory..." -ForegroundColor Yellow
New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
Write-Host "Build directory created at: $buildDir" -ForegroundColor Green
Write-Host ""

# Step 3: Navigate to build directory
Set-Location $buildDir
Write-Host "Working directory: $(Get-Location)" -ForegroundColor Green
Write-Host ""

# Step 4: Run CMake configure
Write-Host "Step 3: Configuring CMake..." -ForegroundColor Yellow
cmake ..
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    Write-Host "Error code: $LASTEXITCODE" -ForegroundColor Red
    exit 1
}
Write-Host "CMake configuration successful!" -ForegroundColor Green
Write-Host ""

# Step 5: Build Release
Write-Host "Step 4: Building Release configuration..." -ForegroundColor Yellow
Write-Host "(This may take 2-5 minutes...)" -ForegroundColor Gray
cmake --build . --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    Write-Host "Error code: $LASTEXITCODE" -ForegroundColor Red
    exit 1
}
Write-Host "Build successful!" -ForegroundColor Green
Write-Host ""

# Step 6: Verify executable exists
$exePath = "$buildDir\Release\local_cpp_client.exe"
if (Test-Path $exePath) {
    Write-Host "Executable verified: $exePath" -ForegroundColor Green
} else {
    Write-Host "Warning: Executable not found at expected location" -ForegroundColor Yellow
    Write-Host "Searching for executable..." -ForegroundColor Yellow
    $found = Get-ChildItem -Path $buildDir -Name "local_cpp_client.exe" -Recurse
    if ($found) {
        Write-Host "Found at: $found" -ForegroundColor Green
    }
}
Write-Host ""

# Show next steps
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Clean Build Complete!" -ForegroundColor Green
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
Write-Host "Current directory: $(Get-Location)" -ForegroundColor Gray
Write-Host ""
