# TouchAxe Web Flasher

[![Flash Firmware](https://img.shields.io/badge/Flash-Firmware-red?style=for-the-badge&logo=espressif)](https://yourusername.github.io/TouchAxe/)

## 🚀 Quick Start

Visit the web flasher: **https://yourusername.github.io/TouchAxe/**

1. Connect your ESP32-S3 Sunton 4.3" board via USB-C
2. Click "FLASH FIRMWARE" button
3. Select your device port
4. Wait for installation to complete
5. Connect to "TouchAxe-Setup" WiFi to configure

## ⚡ Features

- **Bitaxe API Integration** - Direct HTTP API communication for real-time monitoring
- **Swarm Monitoring** - Track up to 10 CyberHornet/Bitaxe devices in your mining fleet
- **Real-time Bitcoin Price** - Updates every 30 seconds from CoinGecko API
- **Satoshi Conversion** - Live 1$ = X Sats calculation
- **Multi-Device Dashboard** - Monitor hashrate, temperature, power, and uptime
- **Touch Navigation** - Swipe gestures with pagination (2 miners per page)
- **Auto-refresh** - Clock (1s), Bitcoin (30s), Hashrate (15s), Miners (10s)
- **NTP Time Sync** - Automatic time synchronization with timezone support
- **Web Portal** - Configure WiFi and Bitaxe devices via web interface
- **Futuristic UI** - Cyberpunk-style interface with animations

> **Powered by [Bitaxe](https://bitaxe.org)** - Open-source Bitcoin mining hardware.  
> This project integrates with the Bitaxe API to provide comprehensive swarm monitoring for your CyberHornet fleet.

## 📦 Firmware Details

- **Version:** 1.0.0
- **Build Date:** 2025-01-11
- **Target Board:** ESP32-S3 Sunton 4.3" RGB Display (480×272)
- **Flash Size:** 1,521,029 bytes (45.5%)
- **RAM Usage:** 112,992 bytes (34.5%)

## 🔧 Hardware Requirements

- **Board:** ESP32-S3 with 4.3" RGB LCD (Sunton ESP32-4827S043C)
- **Display:** 480×272 RGB565, 16-bit parallel interface
- **Touch:** GT911 capacitive touch controller
- **PSRAM:** 8MB required for LVGL buffers
- **Flash:** 16MB recommended

## 📋 Supported Browsers

The web flasher uses the Web Serial API and requires:
- ✅ Chrome 89+
- ✅ Edge 89+
- ✅ Opera 75+
- ❌ Firefox (not supported)
- ❌ Safari (not supported)

## 🎨 UI Screens

### Clock Screen (Main)
- Large time display (48pt)
- Date with NTP sync
- Bitcoin price (top right, 2 lines: BTC + price)
- Satoshi conversion (top left, 1 line: 1$ = X Sats)
- Total hashrate in GH/s (orange)
- Miners online count (green)
- Click left → Miners screen

### Miners Screen
- Display 2 miners per page with pagination
- UP/DOWN buttons with pulsing red glow effect
- Real-time stats: hashrate, temp, power, uptime
- Auto-refresh every 10 seconds
- Back button → Clock screen

### Welcome Screen
- Initial setup screen
- WiFi status display
- Auto-transition to Clock when connected

## 📡 API & Data Sources

- **Bitcoin Price:** CoinGecko API (free tier, 10-50 req/min)
- **Bitaxe Data:** Direct HTTP API calls to configured devices
- **Time Sync:** NTP servers (pool.ntp.org)

## 🔐 First Configuration

After flashing:

1. Board creates AP: **TouchAxe-Setup**
2. Connect to this WiFi
3. Open browser → http://192.168.4.1
4. Configure your WiFi network
5. Add your Bitaxe devices (IP + optional name)
6. Save and reboot

## 📁 File Structure

```
webflasher/
├── index.html          # Futuristic web flasher page
├── manifest.json       # ESP Web Tools manifest
├── firmware/
│   ├── bootloader.bin  # ESP32-S3 bootloader
│   ├── partitions.bin  # Partition table
│   └── firmware.bin    # Main application
└── README.md          # This file
```

## 🛠️ Development

To update the firmware:

1. Build with PlatformIO: `pio run`
2. Copy binaries to `webflasher/firmware/`:
   ```bash
   cp .pio/build/esp32-4827S043C/bootloader.bin webflasher/firmware/
   cp .pio/build/esp32-4827S043C/partitions.bin webflasher/firmware/
   cp .pio/build/esp32-4827S043C/firmware.bin webflasher/firmware/
   ```
3. Update version in `index.html` and `manifest.json`
4. Commit and push to GitHub
5. GitHub Pages will auto-deploy

## 📝 Deployment to GitHub Pages

1. Create repository: `TouchAxe`
2. Push webflasher folder to `gh-pages` branch
3. Enable GitHub Pages in repository settings
4. Source: `gh-pages` branch, `/` (root)
5. Visit: `https://yourusername.github.io/TouchAxe/`

Alternative (simpler):
- Push webflasher contents to `docs/` folder in main branch
- Enable Pages from `docs/` folder
- No separate branch needed

## 🐛 Troubleshooting

**Flash fails:**
- Check USB cable (must be data cable, not charge-only)
- Try different USB port
- Hold BOOT button while connecting
- Install CH340/CP2102 drivers if needed

**Web portal not accessible:**
- Wait 30s after boot for AP to start
- Check WiFi list for "TouchAxe-Setup"
- Try 192.168.4.1 in browser

**Display issues:**
- Check if bootloader and partitions match firmware
- Re-flash with "Erase all" option
- Verify board is genuine Sunton ESP32-4827S043C

## 📜 License

This project is open source. See main repository for license details.

## 🙏 Credits

- **[Bitaxe](https://bitaxe.org)** - Open-source Bitcoin mining hardware by @skot
- **LVGL** - Graphics library v9.4.0
- **ESP Web Tools** - Web-based flasher
- **CoinGecko** - Bitcoin price API

## ⚡ Support This Project

If you find TouchAxe useful, consider supporting development:

**Lightning:** `generousmarble35@zeuspay.com`

---

Made with ⚡ for the Bitcoin mining community
