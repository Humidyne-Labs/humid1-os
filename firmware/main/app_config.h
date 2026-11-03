/**
 * @file app_config.h
 * @brief Global compile-time constants, NVS keys, and H1OS v2.4.9-dev invariants for humid1-os.
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "h1os/h1os.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 1. Firmware & OTA Identity (ThingsBoard Native OTA Keys)
 * ========================================================================== */
#define APP_FW_TITLE                    "humid1-os"
#define APP_FW_VERSION                  "0.2.0-dev"
#define APP_HW_MODEL                    "WS-ESP32S3-EPD154-V2"
#define APP_OTA_MIN_BATTERY_SOC_PCT     25U
#define APP_OTA_MIN_BATTERY_MV          3650U

/* ============================================================================
 * 2. Flow Period Defaults & H1OS Silicon Boundaries (MANDATE-02)
 *    - Flow A: 60s .. 14400s (4h); if > 255s, must be an exact multiple of 60s
 *    - Flow B: 300s .. 86400s (24h); must be > Flow A; remainder 0 or >= 60s
 * ========================================================================== */
#define APP_DEFAULT_FLOW_A_SEC          H1OS_DEFAULT_FLOW_A_PERIOD_SEC  /* 180s (3 min)  */
#define APP_DEFAULT_FLOW_B_SEC          H1OS_DEFAULT_FLOW_B_PERIOD_SEC  /* 900s (15 min) */
#define APP_MIN_FLOW_A_SEC              H1OS_FLOW_A_MIN_SEC             /* 60s  (1 min)  */
#define APP_MAX_FLOW_A_SEC              H1OS_FLOW_A_MAX_SEC             /* 14400s (4 hr) */
#define APP_MIN_FLOW_B_SEC              H1OS_FLOW_B_MIN_SEC             /* 300s (5 min)  */
#define APP_MAX_FLOW_B_SEC              H1OS_FLOW_B_MAX_SEC             /* 86400s (24 hr)*/
#define APP_INTERACTIVE_TIMEOUT_SEC     H1OS_DEFAULT_INACTIVITY_TIMEOUT_SEC /* 30s      */
#define APP_ONBOARDING_HOLD_TIMEOUT_SEC 300U                                /* 5 min dedicated setup hold */
#define APP_ONBOARDING_KEEPALIVE_SEC    15U                                 /* 15s keep-alive timer reset */
#define APP_SYNC_WAIT_GATE_MS           0U
#define APP_SYNC_BARRIER_TIMEOUT_MS     5000U

/* ============================================================================
 * 3. Default Environmental Thresholds & Hysteresis
 * ========================================================================== */
#define APP_DEFAULT_TEMP_HIGH_C         23.0f
#define APP_DEFAULT_TEMP_LOW_C          16.0f
#define APP_DEFAULT_RH_HIGH_PCT         72.0f
#define APP_DEFAULT_RH_LOW_PCT          62.0f
#define APP_ALARM_HYSTERESIS_TEMP_C     0.5f
#define APP_ALARM_HYSTERESIS_RH_PCT     0.5f
#define APP_DEFAULT_NICKNAME            "Main Humidor"

/* ============================================================================
 * 4. NVS Storage Keys (Strictly <= 15 ASCII characters per ESP-IDF NVS limit)
 * ========================================================================== */
#define APP_NVS_KEY_PROVISIONED         "prov_done"
#define APP_NVS_KEY_CLAIMED             "tb_claimed"
#define APP_NVS_KEY_TB_TOKEN            "tb_token"
#define APP_NVS_KEY_CLAIM_SECRET        "claim_sec"
#define APP_NVS_KEY_NICKNAME            "nick_str"
#define APP_NVS_KEY_FLOW_A_SEC          "flow_a_s"
#define APP_NVS_KEY_FLOW_B_SEC          "flow_b_s"
#define APP_NVS_KEY_UNIT_TEMP           "unit_t"
#define APP_NVS_KEY_UNIT_VP             "unit_vp"
#define APP_NVS_KEY_THEME               "ui_theme"
#define APP_NVS_KEY_SOUND_EN            "snd_en"
#define APP_NVS_KEY_VOLUME_PCT          "vol_pct"
#define APP_NVS_KEY_ALERT_MODE          "alrt_mode"
#define APP_NVS_KEY_BATCH_EN            "batch_en"
#define APP_NVS_KEY_BATCH_RATIO         "batch_rat"
#define APP_NVS_KEY_TH_TEMP_HI          "th_t_hi"
#define APP_NVS_KEY_TH_TEMP_LO          "th_t_lo"
#define APP_NVS_KEY_TH_RH_HI            "th_rh_hi"
#define APP_NVS_KEY_TH_RH_LO            "th_rh_lo"

/* ============================================================================
 * 5. ThingsBoard Cloud & Network Timeouts
 * ========================================================================== */
#define APP_TB_BROKER_URI               "mqtts://humid1.com:8883"
#define APP_CLAIM_BASE_URL              "https://humid1.com/claim"
#define APP_WIFI_CONNECT_TIMEOUT_MS     5000U
#define APP_MQTT_CONNECT_TIMEOUT_MS     3500U
#define APP_SNTP_SYNC_TIMEOUT_MS        3000U
#define APP_SNTP_RESYNC_INTERVAL_SEC    86400UL

#define APP_TOPIC_TELEMETRY             "v1/devices/me/telemetry"
#define APP_TOPIC_ATTRIBUTES            "v1/devices/me/attributes"
#define APP_TOPIC_ATTRIBUTES_REQ        "v1/devices/me/attributes/request/1"
#define APP_TOPIC_ATTRIBUTES_RESP_PRE   "v1/devices/me/attributes/response/"
#define APP_TOPIC_RPC_REQ_SUB           "v1/devices/me/rpc/request/+"
#define APP_TOPIC_RPC_RESP_PREFIX       "v1/devices/me/rpc/response/"
#define APP_TOPIC_CLAIM                 "v1/devices/me/claim"

/* ============================================================================
 * 6. Static Buffer Capacities (Zero-Heap Steady-State Enforcement)
 * ========================================================================== */
#define APP_NICKNAME_MAX_LEN            24U
#define APP_TOKEN_MAX_LEN               64U
#define APP_CLAIM_SECRET_MAX_LEN        32U
#define APP_QR_PAYLOAD_MAX_LEN          128U
#define APP_MQTT_TX_BUF_SIZE            1024U
#define APP_MQTT_RX_BUF_SIZE            1024U
#define APP_HISTORY_EXPORT_MAX_SAMPLES  8U

/* ============================================================================
 * 7. Acoustic Feedback Parameters (Layer 4 h1os_audio_play_tone / chime)
 * ========================================================================== */
#define APP_AUDIO_BUTTON_CHIRP_HZ       1046U
#define APP_AUDIO_BUTTON_CHIRP_MS       50U
#define APP_AUDIO_CONFIRM_TONE_HZ       1318U
#define APP_AUDIO_CONFIRM_TONE_MS       80U
#define APP_AUDIO_DEFAULT_VOLUME_PCT    75U

/* ============================================================================
 * 8. 200x200 1-Bit e-Paper Layout Geometry
 * ========================================================================== */
#define APP_EPD_WIDTH                   200
#define APP_EPD_HEIGHT                  200
#define APP_EPD_SAFE_MARGIN_PX          6
#define APP_EPD_SAFE_WIDTH_PX           188
#define APP_QR_TARGET_SIZE_PX           123

#ifdef __cplusplus
}
#endif

#endif /* APP_CONFIG_H */
