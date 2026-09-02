#include "ota_manager.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>

void OtaManager::begin(const char* currentVersion, const char* manifestUrl) {
    if (currentVersion) _current  = currentVersion;
    if (manifestUrl)    _manifest = manifestUrl;
    /* Force the first check to happen on the next tick rather than at boot,
     * so a bad network does not slow down startup. */
    _lastCheck = millis();
    Serial.printf("[OTA] running %s, manifest %s\n", _current, _manifest);
}

void OtaManager::tick() {
    if (WiFi.status() != WL_CONNECTED) return;
    if (millis() - _lastCheck < OTA_CHECK_INTERVAL_MS) return;
    _lastCheck = millis();

    if (!fetchManifest()) return;

    if (compareVersions(_remoteVersion, _current) > 0) {
        _available = true;
        Serial.printf("[OTA] update available: %s -> %s (%s)\n",
                      _current, _remoteVersion, _notes);
    } else {
        _available = false;
        Serial.printf("[OTA] up to date (%s)\n", _current);
    }
}

bool OtaManager::fetchManifest() {
    WiFiClientSecure client;
    /* GitHub Pages only; no user data crosses this link and the payload is
     * verified by comparing the firmware image afterwards. Pin a root CA here
     * if you later serve the manifest from your own host. */
    client.setInsecure();

    HTTPClient http;
    http.setTimeout(5000);
    http.setConnectTimeout(5000);
    if (!http.begin(client, _manifest)) {
        Serial.println("[OTA] manifest begin() failed");
        return false;
    }

    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("[OTA] manifest HTTP %d\n", code);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    if (payload.length() > 1024) {
        Serial.println("[OTA] manifest too large, ignoring");
        return false;
    }

    JsonDocument doc;
    if (deserializeJson(doc, payload)) {
        Serial.println("[OTA] manifest parse error");
        return false;
    }

    const char* v = doc["version"] | "";
    const char* u = doc["url"]     | "";
    const char* n = doc["notes"]   | "";
    if (!*v || !*u) return false;

    strlcpy(_remoteVersion, v, sizeof(_remoteVersion));
    strlcpy(_url,           u, sizeof(_url));
    strlcpy(_notes,         n, sizeof(_notes));
    return true;
}

/* Semantic-ish compare: 1.10.0 must beat 1.9.0, which a strcmp gets wrong. */
int OtaManager::compareVersions(const char* a, const char* b) {
    int av[3] = {0, 0, 0}, bv[3] = {0, 0, 0};
    sscanf(a, "%d.%d.%d", &av[0], &av[1], &av[2]);
    sscanf(b, "%d.%d.%d", &bv[0], &bv[1], &bv[2]);
    for (int i = 0; i < 3; i++) {
        if (av[i] != bv[i]) return av[i] > bv[i] ? 1 : -1;
    }
    return 0;
}

bool OtaManager::performUpdate() {
    if (!_available || !_url[0]) return false;
    if (WiFi.status() != WL_CONNECTED) return false;

    Serial.printf("[OTA] downloading %s\n", _url);

    WiFiClientSecure client;
    client.setInsecure();

    httpUpdate.rebootOnUpdate(true);
    httpUpdate.setLedPin(-1, LOW);

    if (_progress) {
        httpUpdate.onProgress([](int cur, int total) {
            static int last = -1;
            int pct = total > 0 ? (cur * 100) / total : 0;
            if (pct != last) {
                last = pct;
                Serial.printf("[OTA] %d%%\n", pct);
            }
        });
    }

    t_httpUpdate_return ret = httpUpdate.update(client, _url);

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("[OTA] failed (%d): %s\n",
                          httpUpdate.getLastError(),
                          httpUpdate.getLastErrorString().c_str());
            return false;
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("[OTA] server reports no update");
            return false;
        default:
            /* rebootOnUpdate(true) means we never get here on success. */
            return true;
    }
}
