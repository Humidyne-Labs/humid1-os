/**
 * @file app_config.h
 * @brief Application Configuration & System Defaults for HUMID1-OS
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APP_VERSION_STR             "v2.0.0-dev"
#define APP_H1OS_TARGET_VERSION     "v2.4.4-dev"

/* Operational Duty Cycle Constants */
#define APP_DEFAULT_SLEEP_SEC       1800   /* 30 minutes */
#define APP_MIN_SLEEP_SEC           60     /* 1 minute minimum */
#define APP_MAX_SLEEP_SEC           86400  /* 24 hours maximum */

/* Self-Hosted ThingsBoard Custom Cloud Endpoint */
#define APP_TB_DEFAULT_BROKER_URI   "mqtts://thingsboard.humid1.com:8883"
#define APP_TB_DEFAULT_HTTP_URI     "https://thingsboard.humid1.com"
#define APP_TB_CONNECT_TIMEOUT_MS   10000
#define APP_TB_TELEMETRY_TIMEOUT_MS 5000
#define APP_TB_CLAIM_DURATION_MS    180000 /* 3-minute claiming window */

/* Network Time Sync (SNTP) */
#define APP_SNTP_SYNC_INTERVAL_SEC  86400  /* Sync time once every 24 hours */

/* Environmental Threshold Defaults (Safe Cigar Humidor Range: 65-72% RH, 65-70°F) */
#define APP_DEFAULT_RH_LOW_WARN     62.0f
#define APP_DEFAULT_RH_LOW_CRIT     58.0f
#define APP_DEFAULT_RH_HIGH_WARN    73.0f
#define APP_DEFAULT_RH_HIGH_CRIT    78.0f

#ifdef __cplusplus
}
#endif
