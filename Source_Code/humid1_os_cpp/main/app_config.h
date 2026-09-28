/**
 * @file app_config.h
 * @brief Application-Wide Configuration Defaults & Constants
 * 
 * Target Board: Waveshare ESP32-S3 ePaper 1.54 V2
 * 
 * @attribution
 * - Humidyne Labs / Humiditron (2026)
 * 
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Wi-Fi & BLE Provisioning Configuration
 * ========================================================================= */
#define APP_PROV_SERVICE_PREFIX     "PROV_"
#define APP_PROV_POP_LEN            8
#define APP_WIFI_CONNECT_TIMEOUT_MS 8000
#define APP_WIFI_FAST_TIMEOUT_MS    4000

/* =========================================================================
 * ThingsBoard IoT Configuration & Server Credentials
 * ========================================================================= */
#define APP_TB_HOST                 "humid1.com"
#define APP_TB_PORT                 8883
#define APP_TB_PROVISION_KEY        "joz5hqceqbkzzft3qzaf"
#define APP_TB_PROVISION_SECRET     "d9xpuylns0pdlk2w70hc"
#define APP_TB_DEFAULT_BROKER_URI   "mqtts://" APP_TB_HOST ":8883"
#define APP_TB_CONNECT_TIMEOUT_MS   6000
#define APP_TB_CLAIM_DURATION_MS    (3 * 60 * 1000)  /* 3 minutes claim window */
#define APP_TB_CLAIM_TOKEN_LEN      6
#define APP_TB_TELEMETRY_TIMEOUT_MS 5000

/* =========================================================================
 * Power & Sleep Timing & Periodic Schedules
 * ========================================================================= */
#define APP_DEFAULT_SLEEP_SEC       900              /* 15 minutes default sleep */
#define APP_MIN_SLEEP_SEC           30               /* 30 seconds minimum       */
#define APP_MAX_SLEEP_SEC           86400            /* 24 hours maximum         */
#define APP_SNTP_SYNC_INTERVAL_SEC  (24 * 3600)      /* Periodic 24h SNTP sync interval */

/* =========================================================================
 * Button & Reset Timing
 * ========================================================================= */
#define APP_BUTTON_DEBOUNCE_MS      50
#define APP_BUTTON_LONG_PRESS_MS    3000             /* 3 seconds hold for factory wipe */

/* =========================================================================
 * Display & Refresh
 * ========================================================================= */
#define APP_DISPLAY_FULL_REFRESH_INTERVAL 10         /* Full OTP refresh every 10 partials */

#ifdef __cplusplus
}
#endif

