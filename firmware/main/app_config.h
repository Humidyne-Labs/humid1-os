/**
 * @file app_config.h
 * @brief Compile-time constants, ThingsBoard endpoints, NVS keys, and timing defaults for Humid1-OS.
 *
 * Implements the architectural constants specified in APP_01 through APP_06.
 * "Documentation is Law" — do not modify without updating docs/architecture/.
 */

#ifndef HUMID1_APP_CONFIG_H
#define HUMID1_APP_CONFIG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 1. Firmware Identity & Versioning
 * ========================================================================== */
#define APP_FW_TITLE                        "humid1-os"
#define APP_FW_VERSION                      "0.1.0-dev"
#define APP_DEVICE_UID_PREFIX               "HUMID1_"

/* ============================================================================
 * 2. Wake Cadence & Timeout Constants (APP_01)
 * ========================================================================== */
#define APP_DEFAULT_FLOW_A_SEC              (60UL)      /**< Default headless sensor sample interval (s) */
#define APP_DEFAULT_FLOW_B_SEC              (900UL)     /**< Default cloud sync & EPD refresh interval (s) */
#define APP_MIN_FLOW_A_SEC                  (15UL)      /**< Minimum allowed Flow A period (s) */
#define APP_MAX_FLOW_A_SEC                  (3600UL)    /**< Maximum allowed Flow A period (s) */
#define APP_MIN_FLOW_B_SEC                  (60UL)      /**< Minimum allowed Flow B period (s) */
#define APP_MAX_FLOW_B_SEC                  (86400UL)   /**< Maximum allowed Flow B period (s) */
#define APP_INACTIVITY_TIMEOUT_SEC          (30UL)      /**< Hardcoded interactive mode sleep timeout (s) */

/* ============================================================================
 * 3. ThingsBoard MQTTS Cloud Endpoints & Provisioning Credentials (APP_02)
 * ========================================================================== */
#define APP_TB_BROKER_URI                   "mqtts://humid1.com:8883"
#define APP_TB_PROVISION_USER               "provision"
#define APP_TB_PROVISION_DEVICE_KEY         "joz5hqceqbkzzft3qzaf"
#define APP_TB_PROVISION_DEVICE_SECRET      "d9xpuylns0pdlk2w70hc"

#define APP_TB_TOPIC_PROV_REQ               "/provision/request"
#define APP_TB_TOPIC_PROV_RESP              "/provision/response"
#define APP_TB_TOPIC_CLAIM                  "v1/devices/me/claim"
#define APP_TB_TOPIC_TELEMETRY              "v1/devices/me/telemetry"
#define APP_TB_TOPIC_ATTRIBUTES             "v1/devices/me/attributes"
#define APP_TB_TOPIC_ATTR_REQ               "v1/devices/me/attributes/request/1"
#define APP_TB_TOPIC_ATTR_RESP_SUB          "v1/devices/me/attributes/response/+"
#define APP_TB_TOPIC_RPC_REQ_SUB            "v1/devices/me/rpc/request/+"
#define APP_TB_TOPIC_RPC_RESP_FMT           "v1/devices/me/rpc/response/%s"

#define APP_TB_CLAIM_DURATION_MS            (180000UL)  /**< 180-second customer claiming window */
#define APP_NET_CONNECT_TIMEOUT_MS          (5000UL)    /**< Bounded Wi-Fi / MQTTS wait timeout (ms) */
#define APP_SNTP_RESYNC_INTERVAL_SEC        (86400UL)   /**< Re-synchronize PCF85063A RTC every 24h */

/* ============================================================================
 * 4. NVS Namespace & Key Identifiers (APP_01 Section 5.1)
 * ========================================================================== */
#define APP_NVS_NAMESPACE                   "app_cfg"
#define APP_NVS_KEY_WIFI_PROV               "wifi_prov"
#define APP_NVS_KEY_CLAIMED                 "claimed"
#define APP_NVS_KEY_TB_TOKEN                "tb_token"
#define APP_NVS_KEY_NICKNAME                "nickname"
#define APP_NVS_KEY_TEMP_UNIT               "temp_unit"
#define APP_NVS_KEY_THEME_DARK              "theme_dark"
#define APP_NVS_KEY_SOUND_EN                "sound_en"
#define APP_NVS_KEY_AUTO_OTA                "auto_ota"
#define APP_NVS_KEY_FLOW_A_SEC              "flow_a_sec"
#define APP_NVS_KEY_FLOW_B_SEC              "flow_b_sec"

/* ============================================================================
 * 5. Display & OTA Safety Thresholds (APP_04 & APP_06)
 * ========================================================================== */
#define APP_UI_SCREEN_WIDTH_PX              (200U)
#define APP_UI_SCREEN_HEIGHT_PX             (200U)
#define APP_UI_SAFE_TEXT_WIDTH_PX           (188U)      /**< 200px minus 6px left/right margins */
#define APP_OTA_MIN_BATTERY_SOC_PCT         (25U)       /**< Minimum battery SOC% when not on USB */
#define APP_OTA_MIN_BATTERY_MV              (3650U)     /**< Minimum battery voltage (mV) for OTA */
#define APP_KELVIN_OFFSET_F                 (273.15f)   /**< 0 deg C in Kelvin */

#ifdef __cplusplus
}
#endif

#endif /* HUMID1_APP_CONFIG_H */
