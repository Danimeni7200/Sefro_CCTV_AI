Write-Host "Starting Sefro CCTV AI Platform - Essential Services"
Write-Host "====================================================="

Write-Host ""
Write-Host "This script will start the essential services in order:"
Write-Host "1. Cloud API (port 9000)"
Write-Host "2. AI Service (port 8000)"
Write-Host "3. Cloud Web (port 3000)"
Write-Host ""

Write-Host "Press Ctrl+C if you want to cancel..."
Start-Sleep -Seconds 5

Write-Host ""
Write-Host "[1/3] Starting Cloud API service..."
Set-Location packages/cloud_api
Start-Process powershell -ArgumentList "-NoExit", "-Command", "python main.py" -WindowTitle "Cloud API"
Set-Location ../..

Start-Sleep -Seconds 3

Write-Host ""
Write-Host "[2/3] Starting AI Service..."
Set-Location packages/ai_service
Start-Process powershell -ArgumentList "-NoExit", "-Command", "python main.py" -WindowTitle "AI Service"
Set-Location ../..

Start-Sleep -Seconds 3

Write-Host ""
Write-Host "[3/3] Starting Cloud Web interface..."
Set-Location packages/cloud_web
Start-Process powershell -ArgumentList "-NoExit", "-Command", "npm run dev" -WindowTitle "Cloud Web"
Set-Location ../..

Write-Host ""
Write-Host "All services started!"
Write-Host ""
Write-Host "You can now access:"
Write-Host "- Cloud Web Interface: http://localhost:3000"
Write-Host "- AI Service Health: http://localhost:8000/healthz"
Write-Host "- Cloud API Health: http://localhost:9000/healthz"
Write-Host ""
Write-Host "Default credentials:"
Write-Host "- Username: admin"
Write-Host "- Password: admin123"
Write-Host "- API Token: DEV-TOKEN-CHANGE"
Write-Host ""
Write-Host "Press any key to exit..."
$Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")