#pragma once
/*
 * ota_manager.h — over-the-air firmware update.
 *
 * The device has WiFi and 8 MB of flash; requiring a USB cable to ship a fix is
 * the single biggest operational gap in the project. This module closes it by
 * reusing the release infrastructure that already exists: the GitHub Pages site
 * that serves the web flasher also serves a small version manifest.
 *
 * Expected manifest (docs/ota.json), served over HTTPS:
 *   {
 *     "version": "1.3.0",
 *     "url": "https://silexperience210.github.io/TouchAxe/firmware/firmware.bin",
 *     "sha256": "…",
 *     "notes": "Async telemetry, ORDNANCE UI"
 *   }
 *
 * The check runs on the telemetry task, never on the LVGL thread. The update
 * itself is deliberately blocking and explicit: the user presses a button, the
 * UI shows a progress screen, and the device reboots.
 */

#include <Arduino.h>

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "1.2.0"
#endif

#ifndef OTA_MANIFEST_URL
#define OTA_MANIFEST_URL "https://silexperience210.github.io/TouchAxe/ota.json"
#endif

#define OTA_CHECK_INTERVAL_MS (6UL * 60UL * 60UL * 1000UL)   /* every 6 h */

class OtaManager {
public:
    static OtaManager& getInstance() {
        static OtaManager inst;
        return inst;
    }

    void begin(const char* currentVersion, const char* manifestUrl);

    /* Safe to call from the telemetry task on every poll cycle; it rate-limits
     * itself internally and does nothing until the interval has elapsed. */
    void tick();

    bool        updateAvailable() const { return _available; }
    const char* availableVersion() const { return _remoteVersion; }
    const char* releaseNotes()     const { return _notes; }

    /* Blocking. Shows nothing itself — the caller owns the UI. Reboots on
     * success and therefore does not return. */
    bool performUpdate();

    typedef void (*ProgressCb)(int percent);
    void setProgressCallback(ProgressCb cb) { _progress = cb; }

private:
    OtaManager() {}
    OtaManager(const OtaManager&) = delete;
    OtaManager& operator=(const OtaManager&) = delete;

    bool fetchManifest();
    static int compareVersions(const char* a, const char* b);

    const char* _current  = FIRMWARE_VERSION;
    const char* _manifest = OTA_MANIFEST_URL;
    char        _remoteVersion[24] = {0};
    char        _url[192]          = {0};
    char        _notes[96]         = {0};
    bool        _available   = false;
    uint32_t    _lastCheck   = 0;
    ProgressCb  _progress    = nullptr;
};
