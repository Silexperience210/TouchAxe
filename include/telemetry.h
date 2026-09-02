#pragma once
/*
 * telemetry.h — non-blocking rig telemetry.
 *
 * WHY THIS EXISTS
 * ---------------
 * Before this module, stats were fetched with synchronous HTTPClient calls made
 * from an lv_timer callback — i.e. from the LVGL thread. Each unreachable rig
 * stalled the whole UI for the HTTP timeout (2 s). Four offline rigs meant four
 * seconds of frozen screen and dead touch.
 *
 * Now: one FreeRTOS task pinned to core 0 does all the network I/O. It publishes
 * an immutable snapshot behind a mutex. The UI only ever *reads* that snapshot,
 * which takes microseconds and can never block.
 *
 * RULE: nothing in this file may call an LVGL function, and nothing in the UI
 * may call BitaxeAPI directly. That separation is the whole point.
 */

#include <Arduino.h>
#include "bitaxe_api.h"

#define TELEMETRY_MAX_RIGS      10
#define TELEMETRY_HISTORY_LEN   60   /* 60 samples ≈ 60 min at 60 s cadence */
#define TELEMETRY_POLL_MS       5000
#define TELEMETRY_STALE_MS      30000

struct RigSnapshot {
    BitaxeStats stats;
    bool        online;
    uint32_t    updatedAt;      /* millis() of last successful poll */
    uint16_t    consecutiveFailures;
};

struct Aggregate {
    float    hashrateGh;        /* sum over online rigs        */
    float    powerW;
    float    efficiencyJTh;     /* powerW / (hashrateGh/1000)  */
    float    maxTempC;
    uint32_t bestDiff;
    uint8_t  onlineCount;
    uint8_t  totalCount;
};

/* State is derived in exactly ONE place so every screen agrees. */
enum RigState : uint8_t {
    RIG_NOMINAL = 0,
    RIG_LOAD,
    RIG_THERMAL,
    RIG_FAULT,
    RIG_OFFLINE
};

namespace Telemetry {

/* Call once from setup(), after WiFi manager init. Spawns the poller. */
void start();

/* Ask the poller to skip its remaining sleep and refresh now. Returns
 * immediately — the fresh data lands in the snapshot a moment later. */
void requestRefresh();

/* Snapshot readers. All non-blocking, safe from the LVGL thread. */
bool         get(int index, RigSnapshot& out);
Aggregate    aggregate();
RigState     state(int index);
const char*  stateName(RigState s);

/* Rolling hashrate history for the sparkline, oldest first.
 * Returns the number of valid samples written into `out`. */
size_t history(int index, uint16_t* out, size_t maxLen);
size_t historyAggregate(uint16_t* out, size_t maxLen);

/* Queue a control action for the poller task to execute off the UI thread.
 * Returns false if the queue is full. */
enum Action : uint8_t { ACT_RESTART, ACT_REBOOT };
bool queueAction(int index, Action a);

} // namespace Telemetry
