# TouchAxe Firmware Releases

## 📦 Versions disponibles

### V1.1 (Recommandée) ⭐
**Date** : 21 novembre 2025
**Nouvelles fonctionnalités** :
- ✅ Boutons Prev/Next pour navigation tactile entre mineurs
- ✅ Debounce des boutons (élimination des clics multiples)
- ✅ Centrage parfait du container avec bordures visibles
- ✅ Icônes météo Font Awesome professionnelles
- ✅ Correction de l'affichage de la consommation électrique

[Dossier V1.1](./v1.1/)

### V1.0 (Original)
Version originale du TouchAxe avec interface de base.

## 🚀 Mise à jour via esptool webusb

### Méthode la plus simple (Navigateur)
1. Ouvrez [esptool webusb](https://espressif.github.io/esptool-js/)
2. Connectez votre TouchAxe en USB
3. Cliquez sur "Connect"
4. Sélectionnez le fichier `firmware.bin` de la version souhaitée
5. Adresse de flash : `0x10000`
6. Cliquez sur "Program"

### Via script automatique
```bash
# Linux/Mac
cd releases/v1.1
./flash.sh [PORT]

# Windows
cd releases\v1.1
flash.bat [PORT]
```

### Via esptool manuel
```bash
esptool.py --chip esp32s3 --port COM19 --baud 921600 write_flash ^
    0x0000 bootloader.bin ^
    0x8000 partitions.bin ^
    0x10000 firmware.bin
```

## 📋 Informations techniques

- **Chip** : ESP32-S3
- **Fréquence** : 240MHz
- **RAM** : 320KB
- **Flash** : 16MB
- **Résolution** : 480x272 pixels
- **Tactile** : GT911 Capacitive

## 🔄 Rollback

Pour revenir à une version précédente :
1. Utilisez les fichiers binaires de l'ancienne version
2. Suivez la même procédure de flash

## ⚠️ Précautions

- Sauvegardez vos paramètres WiFi avant la mise à jour
- Assurez-vous que la batterie est chargée
- Ne débranchez pas pendant le flash

---
*TouchAxe - Dashboard Bitcoin Mining*