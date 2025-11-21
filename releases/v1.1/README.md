# TouchAxe Firmware V1.1

## 🚀 Nouvelles fonctionnalités V1.1

### Améliorations de l'interface
- ✅ **Boutons Prev/Next** : Navigation tactile fiable entre les mineurs (remplace le swipe défaillant)
- ✅ **Debounce des boutons** : Élimination des déclenchements multiples (200ms de délai)
- ✅ **Centrage du container** : Interface parfaitement centrée avec bordures visibles
- ✅ **Icônes météo Font Awesome** : Affichage professionnel du temps (soleil, nuages, pluie, orage, neige)

### Corrections techniques
- ✅ **Power consumption symbol** : Correction de l'affichage (⚡ remplacé par LV_SYMBOL_CHARGE)
- ✅ **Optimisations UI** : Réduction des dimensions pour de meilleures performances

## 📁 Fichiers binaires

- `firmware.bin` - Firmware principal (1.4MB)
- `bootloader.bin` - Bootloader ESP32
- `partitions.bin` - Table de partitions

## 🔄 Mise à jour via esptool webusb

### Méthode recommandée (via navigateur web)
1. Ouvrez [esptool webusb flash](https://espressif.github.io/esptool-js/)
2. Connectez votre TouchAxe en USB
3. Cliquez sur "Connect"
4. Sélectionnez le fichier `firmware.bin`
5. Adresse de flash : `0x10000`
6. Cliquez sur "Program"

### Commande esptool (terminal)
```bash
esptool.py --chip esp32s3 --port COM19 --baud 921600 write_flash 0x10000 firmware.bin
```

## 📋 Informations techniques

- **Plateforme** : ESP32-S3
- **Résolution** : 480x272 pixels
- **Framework** : Arduino ESP32 v2.0.16
- **LVGL** : v9.4.0
- **Date de build** : 21 novembre 2025

## 🔙 Retour à la V1.0

Si vous souhaitez revenir à la version V1.0, utilisez les fichiers de la branche `main` avant les dernières modifications.

---
*TouchAxe - Dashboard Bitcoin Mining avec interface tactile avancée*