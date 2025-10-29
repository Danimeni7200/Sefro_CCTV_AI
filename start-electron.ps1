Write-Host "Starting Sefro CCTV AI Platform - Electron Desktop App"
Write-Host "===================================================="

Write-Host ""
Write-Host "This script will start the Electron desktop application."
Write-Host "Make sure the essential services are running:"
Write-Host "- Cloud API (port 9000)"
Write-Host "- AI Service (port 8000)"
Write-Host ""

Write-Host "Installing dependencies (if needed)..."
Set-Location packages/electron_app
npm install
Write-Host ""

Write-Host "Starting Electron app..."
npm start

Write-Host ""
Write-Host "Electron app closed."
Set-Location ../..