# Check project structure
Write-Host "Checking project structure..." -ForegroundColor Green
$sourceFiles = Get-ChildItem -Path "packages\local_cpp\src" -Filter "*.cpp" -Recurse
Write-Host "Found source files:" -ForegroundColor Yellow
$sourceFiles | ForEach-Object { Write-Host $_.FullName }

# Check CMake version
Write-Host "`nChecking CMake version..." -ForegroundColor Green
cmake --version

# Check OpenCV
Write-Host "`nChecking OpenCV..." -ForegroundColor Green
if (Test-Path env:OpenCV_DIR) {
    Write-Host "OpenCV_DIR: $env:OpenCV_DIR"
} else {
    Write-Host "OpenCV_DIR not set!" -ForegroundColor Red
}

# Check build directory
Write-Host "`nChecking build directory..." -ForegroundColor Green
if (Test-Path "packages\local_cpp\build") {
    Get-ChildItem "packages\local_cpp\build" | ForEach-Object { Write-Host $_.Name }
} else {
    Write-Host "Build directory not found!" -ForegroundColor Red
}

# Check dependencies
Write-Host "`nChecking dependencies..." -ForegroundColor Green
$deps = @(
    "C:\opencv\build\x64\vc16\bin\opencv_world480.dll",
    "C:\Program Files\curl\bin\libcurl.dll"
)
foreach ($dep in $deps) {
    if (Test-Path $dep) {
        Write-Host "$dep exists"
    } else {
        Write-Host "$dep not found!" -ForegroundColor Red
    }
}
