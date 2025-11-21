@echo off
REM TouchAxe V1.1 Firmware Flash Script pour Windows
REM Utilisation: flash.bat [port]

set PORT=%1
if "%PORT%"=="" set PORT=COM19

echo Flash TouchAxe V1.1 sur %PORT%
echo ================================

REM Vérifier si esptool est installé
esptool.py --version >nul 2>&1
if errorlevel 1 (
    echo esptool.py n'est pas installé. Installez-le avec: pip install esptool
    pause
    exit /b 1
)

echo Flash du firmware...
esptool.py --chip esp32s3 --port %PORT% --baud 921600 write_flash ^
    0x0000 bootloader.bin ^
    0x8000 partitions.bin ^
    0x10000 firmware.bin

if %errorlevel% equ 0 (
    echo Mise à jour réussie !
    echo Redémarrage de l'appareil...
) else (
    echo Erreur lors de la mise à jour
    pause
    exit /b 1
)

pause