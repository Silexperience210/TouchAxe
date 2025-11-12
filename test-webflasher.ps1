# Test Web Flasher Locally
# Quick test script to serve the webflasher folder

Write-Host "`n⚡ TouchAxe Web Flasher - Local Test Server`n" -ForegroundColor Red

$port = 8000

Write-Host "Starting server on port $port..." -ForegroundColor Cyan
Write-Host "URL: http://localhost:$port`n" -ForegroundColor Green

Write-Host "📝 Instructions:" -ForegroundColor Yellow
Write-Host "  1. Open Chrome, Edge, or Opera browser" -ForegroundColor White
Write-Host "  2. Navigate to http://localhost:$port" -ForegroundColor White
Write-Host "  3. Test the flash button (requires ESP32-S3 connected)" -ForegroundColor White
Write-Host "  4. Press Ctrl+C to stop the server`n" -ForegroundColor White

# Change to webflasher directory and start server
Set-Location webflasher
python -m http.server $port
