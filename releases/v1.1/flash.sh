#!/bin/bash

# TouchAxe V1.1 Firmware Flash Script
# Utilisation: ./flash.sh [port]

PORT=${1:-/dev/ttyUSB0}  # Port par défaut, ou COM19 sur Windows

echo "🔄 Flash TouchAxe V1.1 sur $PORT"
echo "================================="

# Vérifier si esptool est installé
if ! command -v esptool.py &> /dev/null; then
    echo "❌ esptool.py n'est pas installé. Installez-le avec: pip install esptool"
    exit 1
fi

# Flash le firmware
echo "📡 Flash du firmware..."
esptool.py --chip esp32s3 --port $PORT --baud 921600 write_flash \
    0x0000 bootloader.bin \
    0x8000 partitions.bin \
    0x10000 firmware.bin

if [ $? -eq 0 ]; then
    echo "✅ Mise à jour réussie !"
    echo "🔄 Redémarrage de l'appareil..."
else
    echo "❌ Erreur lors de la mise à jour"
    exit 1
fi