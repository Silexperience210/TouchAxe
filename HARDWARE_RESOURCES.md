# 🔧 RÉCAPITULATIF DES RESSOURCES MATÉRIELLES - TouchAxe

## 🎯 **Vue d'ensemble du projet**
**TouchAxe** est un dashboard professionnel de monitoring Bitcoin mining pour ESP32-S3 avec écran tactile capacitif. Il permet de surveiller en temps réel plusieurs mineurs BitAxe ESP-Miner.

---

## 🖥️ **COMPOSANT PRINCIPAL - ESP32-S3**

### **Board utilisée**
- **Modèle** : `esp32-4827S043C` (Sunton)
- **MCU** : ESP32-S3 (Xtensa LX7 dual-core)
- **Fréquence CPU** : 240 MHz (maximum)
- **Architecture** : 32-bit RISC-V + Xtensa LX7
- **Cores utilisés** : 1 seul core (Core 1) pour stabilité

### **Mémoire**
- **Flash** : 8 MB (QIO mode, 80MHz)
- **PSRAM** : 8 MB (QSPI)
- **RAM** : 328 KB (utilisation: ~113 KB / 34.5%)
- **Partition** : `default_8MB.csv` (3.3 MB pour firmware)

### **Connectivité**
- **WiFi** : 802.11 b/g/n (2.4 GHz uniquement)
- **Bluetooth** : BLE 5.0 (non utilisé)
- **USB** : USB-C pour programmation et debug

---

## 📱 **ÉCRAN ET INTERFACE TACTILE**

### **Écran LCD**
- **Contrôleur** : ST7796
- **Résolution** : 480 × 272 pixels
- **Type** : IPS LCD couleur
- **Interface** : SPI (4-wire)
- **Fréquence SPI** : 10 MHz (réduit pour stabilité)

### **Brochage SPI Écran**
```cpp
#define TFT_MISO -1     // Non utilisé
#define TFT_MOSI 11     // SDA (MOSI)
#define TFT_SCLK 12     // SCL (SCK)
#define TFT_CS   10     // Chip Select
#define TFT_DC   13     // Data/Command
#define TFT_RST  14     // Reset
```

### **Contrôleur Tactile**
- **Modèle** : GT911 (TAMC_GT911)
- **Type** : Capacitif 5-point multi-touch
- **Interface** : I2C
- **Adresse I2C** : 0x5D (par défaut)
- **Fréquence de lecture** : 50 Hz (20ms)

### **Brochage I2C Tactile**
```cpp
#define GT911_SDA  19   // Data
#define GT911_SCL  20   // Clock
#define GT911_INT  18   // Interrupt
#define GT911_RST  38   // Reset
```

---

## 🔌 **BROCHAGE COMPLET ESP32-S3**

### **SPI Bus 1 (Écran)**
- GPIO 11 : MOSI (SDA)
- GPIO 12 : SCLK (SCL)
- GPIO 10 : CS (Chip Select)
- GPIO 13 : DC (Data/Command)
- GPIO 14 : RST (Reset)

### **I2C Bus (Tactile)**
- GPIO 19 : SDA (Data)
- GPIO 20 : SCL (Clock)
- GPIO 18 : INT (Interrupt)
- GPIO 38 : RST (Reset)

### **USB/UART (Debug)**
- GPIO 43 : TX
- GPIO 44 : RX
- GPIO 0  : BOOT (programmation)
- GPIO 0  : EN (reset)

---

## 📚 **LIBRAIRIES ET FRAMEWORKS**

### **Core Framework**
- **ESP-IDF** : 3.20017.241212 (Arduino)
- **PlatformIO** : Core pour compilation
- **Arduino Core** : Framework principal

### **Bibliothèques utilisées**
```ini
lib_deps =
    lvgl/lvgl@^9.4.0                    # GUI
    bblanchon/ArduinoJson@^7.0.4        # JSON parsing
    contrem/arduino-timer@^3.0.1        # Timers
    bodmer/TFT_eSPI@^2.5.43            # Display driver
    ESPAsyncWebServer                  # Web server
    AsyncTCP                          # TCP async
    TAMC_GT911                        # Touch driver
```

### **Configuration Build**
```cpp
build_flags =
    -D ARDUINO_USB_MODE=0
    -D ARDUINO_USB_CDC_ON_BOOT=0
    -D CORE_DEBUG_LEVEL=0              // Optimisé
    -D BOARD_HAS_PSRAM
    -mfix-esp32-psram-cache-issue
    -D LV_HOR_RES_MAX=480
    -D LV_VER_RES_MAX=272
    -D CONFIG_ESP32S3_SPIRAM_SUPPORT
    -D ARDUINO_EVENT_RUNNING_CORE=1
    -D ARDUINO_RUNNING_CORE=1          // Core 1 uniquement
    -O2                               // Optimisation niveau 2
```

---

## ⚡ **CONSOMMATION ET PERFORMANCES**

### **Utilisation Ressources**
- **Flash** : 1.5 MB / 3.3 MB (44.5%)
- **RAM** : 113 KB / 328 KB (34.5%)
- **CPU** : ~20-35% (avec debug level 0)
- **WiFi** : Connecté en station mode

### **Alimentation**
- **Tension** : 3.3V (interne)
- **Courant** : ~150-200mA (avec écran allumé)
- **Source** : USB-C ou batterie externe

---

## 🌐 **PÉRIPHÉRIQUES EXTERNES**

### **BitAxe ESP-Miners**
- **Connexion** : WiFi (même réseau local)
- **API** : HTTP REST (`/api/system/info`)
- **Fréquence polling** : Toutes les 15 secondes
- **Données collectées** :
  - Hashrate (GH/s)
  - Power consumption (W)
  - Best difficulty (string format)
  - Temperature (°C)
  - Shares accepted/rejected

### **Services Externes**
- **CoinGecko API** : Prix Bitcoin (USD)
- **NTP** : Synchronisation heure
- **mDNS** : Découverte réseau (optionnel)

---

## 🔧 **OUTILS DE DÉVELOPPEMENT**

### **IDE & Environnement**
- **VS Code** avec PlatformIO extension
- **Git** pour contrôle de version
- **GitHub** pour repository et CI/CD

### **Outils de Flash**
- **ESP Web Tools** : Flash via navigateur web
- **esptool.py** : Flash en ligne de commande
- **PlatformIO** : Build et upload intégré

### **Monitoring**
- **Serial Monitor** : Debug (115200 baud)
- **Web Interface** : Configuration WiFi
- **LVGL Simulator** : Debug GUI (optionnel)

---

## 📦 **FICHIERS BINAIRES**

### **Structure Firmware**
```
firmware/
├── bootloader.bin     (4 KB @ 0x1000)
├── partitions.bin     (4 KB @ 0x8000)
├── boot_app0.bin      (4 KB @ 0xe000)
└── firmware.bin       (1.5 MB @ 0x10000)
```

### **Manifest Web Flasher**
```json
{
  "name": "TouchAxe Mining Dashboard",
  "version": "1.0.0",
  "chipFamily": "ESP32-S3",
  "parts": [
    {"path": "bootloader.bin", "offset": 4096},
    {"path": "partitions.bin", "offset": 32768},
    {"path": "boot_app0.bin", "offset": 57344},
    {"path": "firmware.bin", "offset": 65536}
  ]
}
```

---

## 🎯 **COMPATIBILITÉ**

### **Boards Compatibles**
- **esp32-4827S043C** (recommandé)
- Autres ESP32-S3 avec écran 480×272 + GT911

### **Écrans Compatibles**
- ST7796 480×272 IPS
- Drivers TFT_eSPI compatibles

### **Contrôleurs Tactiles**
- GT911 (TAMC_GT911)
- Autres contrôleurs I2C capacitifs

---

## 🚀 **ROADMAP MATÉRIEL**

### **Améliorations Futures**
- **Écran OLED** : Version basse consommation
- **Batterie LiPo** : Autonomie prolongée
- **Capteurs** : Température, humidité ambiante
- **LED RGB** : Indicateurs d'état visuels
- **Buzzer** : Alertes sonores

### **Versions Alternatives**
- **ESP32-S2** : Moins cher, moins puissant
- **ESP32-C3** : RISC-V uniquement, WiFi uniquement
- **Raspberry Pi Pico** : Alternative non-ESP32

---

## 📋 **CHECKLIST DÉPLOIEMENT**

### **Matériel Requis**
- [x] ESP32-S3 board (esp32-4827S043C)
- [x] Écran 480×272 ST7796
- [x] Contrôleur tactile GT911
- [x] Câbles de connexion
- [x] Alimentation USB-C

### **Logiciel Configuré**
- [x] PlatformIO environment
- [x] Toutes les bibliothèques
- [x] Configuration pins
- [x] Build flags optimisés
- [x] Web flasher prêt

### **Testé et Validé**
- [x] Compilation réussie
- [x] Upload fonctionnel
- [x] Interface tactile responsive
- [x] WiFi connection
- [x] API BitAxe polling
- [x] Affichage Bitcoin price

---

**⚡ Powered by Silexperience** - *Ressources matérielles complètes pour TouchAxe v1.0.0*