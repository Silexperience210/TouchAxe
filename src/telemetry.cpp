#include "telemetry.h"
#include "wifi_manager.h"
#include "ota_manager.h"
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

namespace {

/* ── published state, guarded by g_lock ──────────────────────────────────── */
SemaphoreHandle_t g_lock = nullptr;
RigSnapshot       g_snap[TELEMETRY_MAX_RIGS];
Aggregate         g_agg  = {};

uint16_t g_hist[TELEMETRY_MAX_RIGS][TELEMETRY_HISTORY_LEN] = {};
uint16_t g_histAgg[TELEMETRY_HISTORY_LEN] = {};
uint8_t  g_histCount = 0;      /* how many samples are valid (saturates)     */
uint8_t  g_histHead  = 0;      /* next write position                        */
uint32_t g_lastHistoryPush = 0;

TaskHandle_t     g_task     = nullptr;
QueueHandle_t    g_actions  = nullptr;
volatile bool    g_refreshRequested = false;

/* Thresholds. Tuned for Bitaxe-class hardware; change here, nowhere else. */
constexpr float TEMP_THERMAL_C = 70.0f;
constexpr float TEMP_LOAD_C    = 62.0f;
constexpr float HASH_FAULT_PCT = 0.80f;

struct ActionMsg { int index; Telemetry::Action action; };

/* A rig identity copied out of WifiManager so the poller never holds a
 * pointer into the manager's mutable String storage while doing I/O. */
struct RigTarget {
    char ip[40];
    bool valid;
};

void lock()   { if (g_lock) xSemaphoreTake(g_lock, portMAX_DELAY); }
void unlock() { if (g_lock) xSemaphoreGive(g_lock); }

void recomputeAggregate(Aggregate& agg, const RigSnapshot* snaps, int count) {
    agg = {};
    agg.totalCount = (uint8_t)count;
    for (int i = 0; i < count; i++) {
        if (!snaps[i].online || !snaps[i].stats.valid) continue;
        agg.onlineCount++;
        agg.hashrateGh += snaps[i].stats.hashrate;
        agg.powerW     += snaps[i].stats.power;
        if (snaps[i].stats.temp > agg.maxTempC)   agg.maxTempC = snaps[i].stats.temp;
        if (snaps[i].stats.bestDiff > agg.bestDiff) agg.bestDiff = snaps[i].stats.bestDiff;
    }
    if (agg.hashrateGh > 0.001f) {
        agg.efficiencyJTh = agg.powerW / (agg.hashrateGh / 1000.0f);
    }
}

void pushHistory(const RigSnapshot* snaps, int count, const Aggregate& agg) {
    for (int i = 0; i < TELEMETRY_MAX_RIGS; i++) {
        uint16_t v = (i < count && snaps[i].online)
                   ? (uint16_t)constrain(snaps[i].stats.hashrate, 0.0f, 65535.0f)
                   : 0;
        g_hist[i][g_histHead] = v;
    }
    g_histAgg[g_histHead] = (uint16_t)constrain(agg.hashrateGh, 0.0f, 65535.0f);
    g_histHead = (g_histHead + 1) % TELEMETRY_HISTORY_LEN;
    if (g_histCount < TELEMETRY_HISTORY_LEN) g_histCount++;
}

size_t copyHistory(const uint16_t* ring, uint16_t* out, size_t maxLen) {
    lock();
    size_t n = min((size_t)g_histCount, maxLen);
    /* oldest first */
    size_t start = (g_histHead + TELEMETRY_HISTORY_LEN - n) % TELEMETRY_HISTORY_LEN;
    for (size_t i = 0; i < n; i++) {
        out[i] = ring[(start + i) % TELEMETRY_HISTORY_LEN];
    }
    unlock();
    return n;
}

void drainActions() {
    ActionMsg msg;
    while (g_actions && xQueueReceive(g_actions, &msg, 0) == pdTRUE) {
        WifiManager* wifi = WifiManager::getInstance();
        BitaxeDevice* dev = wifi->getBitaxe(msg.index);
        if (!dev) continue;
        BitaxeAPI api;
        api.setDevice(dev->ip);
        bool ok = (msg.action == Telemetry::ACT_RESTART) ? api.restart() : api.reboot();
        Serial.printf("[TELEM] action %d on rig %d -> %s\n",
                      msg.action, msg.index, ok ? "OK" : "FAIL");
    }
}

void pollOnce() {
    WifiManager* wifi = WifiManager::getInstance();
    if (!wifi->isConnected()) return;

    int count = wifi->getBitaxeCount();
    if (count > TELEMETRY_MAX_RIGS) count = TELEMETRY_MAX_RIGS;

    /* 1. copy the targets out, fast, so we do no I/O while touching manager state */
    RigTarget targets[TELEMETRY_MAX_RIGS] = {};
    for (int i = 0; i < count; i++) {
        BitaxeDevice* dev = wifi->getBitaxe(i);
        if (!dev || dev->ip.length() == 0) continue;
        strlcpy(targets[i].ip, dev->ip.c_str(), sizeof(targets[i].ip));
        targets[i].valid = true;
    }

    /* 2. the slow part — entirely outside the lock */
    RigSnapshot local[TELEMETRY_MAX_RIGS];
    lock();
    memcpy(local, g_snap, sizeof(local));   /* keep failure counters */
    unlock();

    for (int i = 0; i < count; i++) {
        if (!targets[i].valid) { local[i].online = false; continue; }

        BitaxeAPI api;
        api.setDevice(String(targets[i].ip));
        BitaxeStats s;
        if (api.getStats(s)) {
            local[i].stats               = s;
            local[i].online              = true;
            local[i].updatedAt           = millis();
            local[i].consecutiveFailures = 0;
        } else {
            if (local[i].consecutiveFailures < 0xFFFF) local[i].consecutiveFailures++;
            /* One dropped packet is not an outage. Two in a row is. */
            if (local[i].consecutiveFailures >= 2) {
                local[i].online = false;
                local[i].stats.valid = false;
            }
        }
        /* let the poller yield between rigs so WiFi housekeeping can run */
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    for (int i = count; i < TELEMETRY_MAX_RIGS; i++) local[i] = RigSnapshot{};

    /* 3. publish */
    Aggregate agg;
    recomputeAggregate(agg, local, count);

    lock();
    memcpy(g_snap, local, sizeof(g_snap));
    g_agg = agg;
    if (millis() - g_lastHistoryPush >= 60000 || g_histCount == 0) {
        pushHistory(local, count, agg);
        g_lastHistoryPush = millis();
    }
    unlock();

    /* mirror online flags back for legacy UI code that reads BitaxeDevice */
    for (int i = 0; i < count; i++) {
        BitaxeDevice* dev = wifi->getBitaxe(i);
        if (dev) dev->online = local[i].online;
    }
}

void telemetryTask(void*) {
    Serial.println("[TELEM] poller started on core 0");
    for (;;) {
        drainActions();
        pollOnce();
        /* Cheap: rate-limits itself internally and only touches the network
         * once every OTA_CHECK_INTERVAL_MS. Runs here so it never blocks LVGL. */
        OtaManager::getInstance().tick();

        /* interruptible sleep: requestRefresh() cuts it short */
        uint32_t waited = 0;
        while (waited < TELEMETRY_POLL_MS && !g_refreshRequested) {
            vTaskDelay(pdMS_TO_TICKS(100));
            waited += 100;
            if (g_actions && uxQueueMessagesWaiting(g_actions) > 0) break;
        }
        g_refreshRequested = false;
    }
}

} // namespace

namespace Telemetry {

void start() {
    if (g_task) return;
    g_lock    = xSemaphoreCreateMutex();
    g_actions = xQueueCreate(8, sizeof(ActionMsg));
    memset(g_snap, 0, sizeof(g_snap));

    /* Core 0 hosts the WiFi stack; core 1 runs Arduino + LVGL. Pinning the
     * poller to core 0 keeps HTTP latency off the render loop entirely. */
    xTaskCreatePinnedToCore(telemetryTask, "telemetry", 10240, nullptr, 3, &g_task, 0);
}

void requestRefresh() { g_refreshRequested = true; }

bool get(int index, RigSnapshot& out) {
    if (index < 0 || index >= TELEMETRY_MAX_RIGS) return false;
    lock();
    out = g_snap[index];
    unlock();
    return out.stats.valid || out.online;
}

Aggregate aggregate() {
    lock();
    Aggregate a = g_agg;
    unlock();
    return a;
}

RigState state(int index) {
    RigSnapshot s;
    if (index < 0 || index >= TELEMETRY_MAX_RIGS) return RIG_OFFLINE;
    lock();
    s = g_snap[index];
    unlock();

    if (!s.online)                                  return RIG_OFFLINE;
    if (millis() - s.updatedAt > TELEMETRY_STALE_MS) return RIG_OFFLINE;
    if (s.stats.temp >= TEMP_THERMAL_C)             return RIG_THERMAL;
    if (s.stats.hashrate <= 0.1f)                   return RIG_FAULT;
    if (s.stats.temp >= TEMP_LOAD_C)                return RIG_LOAD;
    return RIG_NOMINAL;
}

const char* stateName(RigState s) {
    switch (s) {
        case RIG_NOMINAL: return "NOMINAL";
        case RIG_LOAD:    return "LOAD";
        case RIG_THERMAL: return "THERMAL";
        case RIG_FAULT:   return "FAULT";
        default:          return "OFFLINE";
    }
}

size_t history(int index, uint16_t* out, size_t maxLen) {
    if (index < 0 || index >= TELEMETRY_MAX_RIGS || !out) return 0;
    return copyHistory(g_hist[index], out, maxLen);
}

size_t historyAggregate(uint16_t* out, size_t maxLen) {
    if (!out) return 0;
    return copyHistory(g_histAgg, out, maxLen);
}

bool queueAction(int index, Action a) {
    if (!g_actions) return false;
    ActionMsg msg{index, a};
    return xQueueSend(g_actions, &msg, 0) == pdTRUE;
}

} // namespace Telemetry
