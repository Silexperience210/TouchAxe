# TouchAxe Firmware Update Script
# Compile, copy binaries, and update web flasher

Write-Host "`n========================================" -ForegroundColor Red
Write-Host "  TouchAxe Firmware Update Script" -ForegroundColor Red
Write-Host "========================================`n" -ForegroundColor Red

# 1. Compile firmware
Write-Host "[1/5] Compiling firmware..." -ForegroundColor Cyan
pio run
if ($LASTEXITCODE -ne 0) {
    Write-Host "❌ Compilation failed!" -ForegroundColor Red
    exit 1
}
Write-Host "✅ Compilation successful`n" -ForegroundColor Green

# 2. Copy binaries to webflasher
Write-Host "[2/5] Copying binaries to webflasher/firmware/..." -ForegroundColor Cyan
$binaries = @(
    @{src=".pio\build\esp32-4827S043C\bootloader.bin"; dest="webflasher\firmware\bootloader.bin"},
    @{src=".pio\build\esp32-4827S043C\partitions.bin"; dest="webflasher\firmware\partitions.bin"},
    @{src=".pio\build\esp32-4827S043C\firmware.bin"; dest="webflasher\firmware\firmware.bin"}
)

foreach ($bin in $binaries) {
    Copy-Item $bin.src $bin.dest -Force
    $size = (Get-Item $bin.dest).Length
    Write-Host "  ✓ $($bin.dest) ($([math]::Round($size/1KB, 2)) KB)" -ForegroundColor Gray
}
Write-Host "✅ Binaries copied`n" -ForegroundColor Green

# 3. Get firmware size
Write-Host "[3/5] Getting firmware information..." -ForegroundColor Cyan
$firmwareSize = (Get-Item "webflasher\firmware\firmware.bin").Length
$firmwareSizeKB = [math]::Round($firmwareSize/1KB, 2)
$buildDate = Get-Date -Format "yyyy-MM-dd"
Write-Host "  Firmware size: $firmwareSizeKB KB" -ForegroundColor Gray
Write-Host "  Build date: $buildDate" -ForegroundColor Gray
Write-Host "✅ Information retrieved`n" -ForegroundColor Green

# 4. Update docs folder if exists
if (Test-Path "docs") {
    Write-Host "[4/5] Updating docs folder..." -ForegroundColor Cyan
    Copy-Item -Recurse -Force "webflasher\*" "docs\"
    Write-Host "✅ docs folder updated`n" -ForegroundColor Green
} else {
    Write-Host "[4/5] docs folder not found, skipping..." -ForegroundColor Yellow
    Write-Host "  Run: New-Item -ItemType Directory -Path 'docs' -Force" -ForegroundColor Gray
    Write-Host "  Then: Copy-Item -Recurse -Force 'webflasher\*' 'docs\'" -ForegroundColor Gray
}

# 5. Summary
Write-Host "[5/5] Update Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Red
Write-Host "  Bootloader: ✓ Updated" -ForegroundColor Green
Write-Host "  Partitions: ✓ Updated" -ForegroundColor Green
Write-Host "  Firmware:   ✓ Updated ($firmwareSizeKB KB)" -ForegroundColor Green
Write-Host "  Build Date: $buildDate" -ForegroundColor Green
Write-Host "========================================`n" -ForegroundColor Red

# Next steps
Write-Host "📝 Next Steps:" -ForegroundColor Yellow
Write-Host "  1. Update version in webflasher/index.html" -ForegroundColor White
Write-Host "  2. Update version in webflasher/manifest.json" -ForegroundColor White
Write-Host "  3. Test locally: cd webflasher; python -m http.server 8000" -ForegroundColor White
Write-Host "  4. Commit: git add webflasher/ docs/" -ForegroundColor White
Write-Host "  5. Commit: git commit -m '📦 Update firmware vX.X.X'" -ForegroundColor White
Write-Host "  6. Push:   git push origin main`n" -ForegroundColor White

Write-Host "🚀 Ready to deploy!" -ForegroundColor Green
