/**
 * @file app_types.h
 * @brief Core data structures, server-driven telemetry contracts, and state enumerations for Humid1-OS.
 *
 * All temperature telemetry and server alarm thresholds use Kelvin (K) as the canonical
 * base unit per APP_02. Fixed-width <stdint.h> types are enforced per APP_08.
 */

#ifndef HUMID1_APP_TYPES_H
#define HUMID1_APP_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include "app_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Application top-level finite state machine states (APP_01 Section 4).
 */
typedef enum {
    APP_STATE_BOOT_INIT = 0,    /**< Cold boot / wake initialization and NVS inspection */
    APP_STATE_BLE_PROV,         /**< First-boot BLE GATT Security 1 Wi-Fi provisioning */
    APP_STATE_SNTP_SYNC,        /**< Wall-clock NTP synchronization to PCF85063A RTC */
    APP_STATE_TB_AUTO_PROV,     /**< Phase 1 ThingsBoard device registration (/provision/request) */
    APP_STATE_TB_CLAIMING,      /**< Phase 2 customer device claiming (v1/devices/me/claim) */
    APP_STATE_FLOW_B_SYNC,      /**< Unattended periodic Flow B wake (MQTTS sync + EPD update) */
    APP_STATE_INTERACTIVE,      /**< User button wake (AWAKE badge, 3-screen navigation, 30s timeout) */
    APP_STATE_OTA_UPDATE        /**< Dual-slot firmware download, verification, and reboot */
} app_fsm_state_t;

/**
 * @brief Active user screen index cycled via BOOT (GPIO 0) button taps (APP_04 Section 3).
 */
typedef enum {
    APP_SCREEN_DASHBOARD = 0,   /**< Screen 1: Main climate and connection dashboard */
    APP_SCREEN_DIAGNOSTICS,     /**< Screen 2: Tri-Node metrology, psychrometrics, and autonomy */
    APP_SCREEN_SYSTEM,          /**< Screen 3: Active server alarms, system state, and actions */
    APP_SCREEN_USER_COUNT,      /**< Total number of cycleable user screens (3) */
    APP_SCREEN_MODAL_PROV = 10, /**< Modal: BLE Wi-Fi Provisioning (QR + PIN + UID) */
    APP_SCREEN_MODAL_CLAIM,     /**< Modal: Customer Claiming (PIN + UID + 180s timer) */
    APP_SCREEN_MODAL_OTA        /**< Modal: OTA Firmware Update Progress */
} app_screen_id_t;

/**
 * @brief Display temperature unit preference (APP_04 Section 1).
 */
typedef enum {
    APP_TEMP_UNIT_FAHRENHEIT = 0, /**< Convert Kelvin to Fahrenheit: (K - 273.15) * 1.8 + 32 */
    APP_TEMP_UNIT_CELSIUS    = 1, /**< Convert Kelvin to Celsius: K - 273.15 */
    APP_TEMP_UNIT_KELVIN     = 2  /**< Display canonical Kelvin directly */
} app_temp_unit_t;

/**
 * @brief Bitmask flags for server-driven telemetry serialization (APP_02 Section 3.2).
 */
typedef enum {
    APP_TELE_MASK_RH            = (1U << 0), /**< "rh" (% relative humidity) */
    APP_TELE_MASK_TEMP_K        = (1U << 1), /**< "temp" (Kelvin) */
    APP_TELE_MASK_BATTERY       = (1U << 2), /**< "battery" (% state of charge) */
    APP_TELE_MASK_RSSI          = (1U << 3), /**< "rssi" (dBm) */
    APP_TELE_MASK_DEW_POINT_K   = (1U << 4), /**< "dew_point" (Kelvin) */
    APP_TELE_MASK_VBAT_MV       = (1U << 5), /**< "vbat_mv" (millivolts) */
    APP_TELE_MASK_AUTONOMY_DAYS = (1U << 6), /**< "autonomy_days" (days) */
    APP_TELE_MASK_CONFIDENCE    = (1U << 7), /**< "confidence" (% thermal equilibrium) */
    APP_TELE_MASK_DEFAULT       = 0x0FU      /**< Canonical 4-key ThingsBoard schema */
} app_telemetry_mask_t;

/**
 * @brief Server-driven alarm severity levels (APP_02 Section 5).
 */
typedef enum {
    APP_ALARM_SEV_NONE = 0,
    APP_ALARM_SEV_WARNING,
    APP_ALARM_SEV_CRITICAL
} app_alarm_severity_t;

/**
 * @brief Server-evaluated alarm state synchronized from ThingsBoard SHARED_SCOPE.
 */
typedef struct {
    bool                    active;         /**< True when server has an active alarm on this device */
    app_alarm_severity_t    severity;       /**< NONE, WARNING, or CRITICAL */
    char                    type[24];       /**< e.g., "RH Alarm", "Temp Alarm", "Battery Alarm" */
    char                    message[64];    /**< Formatted server description */
} app_alarm_state_t;

/**
 * @brief Server-configured alarm thresholds and hysteresis (temperatures in Kelvin).
 */
typedef struct {
    float rh_low_crit;          /**< Critical low RH% threshold */
    float rh_low_warn;          /**< Warning low RH% threshold */
    float rh_high_warn;         /**< Warning high RH% threshold */
    float rh_high_crit;         /**< Critical high RH% threshold */
    float temp_low_crit_k;      /**< Critical low temperature threshold (Kelvin) */
    float temp_low_warn_k;      /**< Warning low temperature threshold (Kelvin) */
    float temp_high_warn_k;     /**< Warning high temperature threshold (Kelvin) */
    float temp_high_crit_k;     /**< Critical high temperature threshold (Kelvin) */
    float batt_low_crit_pct;    /**< Critical low battery SOC% threshold */
    float batt_low_warn_pct;    /**< Warning low battery SOC% threshold */
    float rh_hysteresis;        /**< RH% hysteresis deadband */
    float temp_hysteresis_k;    /**< Temperature hysteresis deadband (Kelvin) */
    float batt_hysteresis_pct;  /**< Battery hysteresis deadband (%) */
} app_thresholds_t;

/**
 * @brief Unified server-driven telemetry frame populated from H1OS and Wi-Fi state.
 */
typedef struct {
    int64_t  timestamp_ms;      /**< Unix epoch timestamp in milliseconds */
    float    rh_pct;            /**< Compensated relative humidity (%) */
    float    temp_k;            /**< Tri-Node compensated ambient temperature (Kelvin) */
    float    dew_point_k;       /**< Psychrometric dew point temperature (Kelvin) */
    float    shtc3_raw_k;       /**< Raw SHTC3 sensor temperature (Kelvin) */
    float    mcu_die_k;         /**< ESP32-S3 internal die temperature (Kelvin) */
    float    epd_glass_k;       /**< SSD1681 panel glass temperature (Kelvin) */
    float    vapor_press_hpa;   /**< Actual vapor pressure (hPa) */
    float    sat_vp_hpa;        /**< Saturation vapor pressure (hPa) */
    float    abs_humidity_gm3;  /**< Absolute humidity (g/m^3) */
    float    autonomy_days;     /**< Projected remaining battery autonomy (days) */
    float    avg_current_ma;    /**< Rolling average system current draw (mA) */
    uint16_t vbat_mv;           /**< Calibrated battery voltage (mV) */
    uint8_t  battery_pct;       /**< LiPo state-of-charge (0..100%) */
    uint8_t  confidence_pct;    /**< Tri-Node thermal settling confidence (0..100%) */
    int8_t   rssi_dbm;          /**< Wi-Fi RSSI in dBm */
    bool     vbus_present;      /**< True if 5V USB power is connected */
    bool     thermal_settled;   /**< True if SAMPLE_FLAG_THERMAL_SETTLED is asserted */
} app_telemetry_frame_t;

/**
 * @brief Synchronized ThingsBoard Shared Attributes and persistent device configuration.
 */
typedef struct {
    uint32_t            flow_a_period_sec;      /**< Headless sensor sample interval (s) */
    uint32_t            flow_b_period_sec;      /**< Full wake & cloud sync interval (s) */
    uint32_t            telemetry_schema_mask;  /**< Bitmask of server-enabled telemetry keys */
    app_temp_unit_t     temp_unit;              /**< Display temperature unit (F, C, K) */
    bool                wifi_provisioned;       /**< True when Wi-Fi credentials exist in NVS */
    bool                device_claimed;         /**< True when customer claim is completed */
    bool                theme_dark;             /**< True for dark/inverted e-Paper theme */
    bool                sound_enabled;          /**< True when interactive audio cues are enabled */
    bool                auto_update_enabled;    /**< True when automatic FOTA on wake is enabled */
    bool                manual_ota_trigger;     /**< True when immediate FOTA is requested */
    bool                email_alerts_enabled;   /**< Server email alert state mirror */
    char                device_uid[20];         /**< Hardware UID: "HUMID1_A1B2C3" */
    char                device_nickname[32];    /**< Optional server-assigned nickname */
    char                tb_access_token[64];    /**< Permanent ThingsBoard device access token */
    char                target_fw_version[24];  /**< Target FOTA version string from server */
    char                target_fw_checksum[68]; /**< Target FOTA SHA-256 hex digest */
} app_config_attrs_t;

/**
 * @brief RTC Slow SRAM cache surviving deep sleep without NVS flash wear (APP_01 Section 5.2).
 */
typedef struct {
    uint32_t            magic;                  /**< Validation magic 0x48314150UL ("H1AP") */
    uint8_t             last_bssid[6];          /**< Cached AP BSSID for fast single-channel connect */
    uint8_t             last_channel;           /**< Cached Wi-Fi channel (1..13) */
    uint32_t            last_sntp_epoch;        /**< Unix epoch of last SNTP wall-clock sync */
    uint32_t            last_sync_epoch;        /**< Unix epoch of last successful MQTTS sync */
    uint16_t            consecutive_net_fails;  /**< Count of consecutive network failures */
    int32_t             last_err_code;          /**< Last recorded esp_err_t / protocol error */
    app_alarm_state_t   active_alarm;           /**< Cached server alarm state */
    app_thresholds_t    thresholds;             /**< Cached server alarm thresholds */
} app_rtc_cache_t;

/**
 * @brief Master runtime context passed to application modules and UI screens.
 */
typedef struct {
    app_fsm_state_t         fsm_state;          /**< Current state machine state */
    app_screen_id_t         active_screen;      /**< Currently displayed LVGL screen */
    bool                    is_interactive;     /**< True = AWAKE (button wake), False = SLEEP (RTC wake) */
    bool                    wifi_connected;     /**< True when STA has valid IPv4 address */
    uint8_t                 selected_action_idx;/**< Highlighted action item on Screen 3 (0..3) */
    uint16_t                claim_rem_sec;      /**< Remaining seconds on claiming countdown */
    char                    prov_pop_pin[12];   /**< 8-char TRNG Proof-of-Possession PIN */
    char                    claim_pin[10];      /**< 6-char TRNG ThingsBoard Claim PIN */
    char                    ip_addr_str[16];    /**< Assigned IPv4 address string */
    char                    ssid_str[33];       /**< Connected Wi-Fi SSID string */
    app_config_attrs_t      attrs;              /**< Persistent NVS & shared attributes */
    app_telemetry_frame_t   telemetry;          /**< Latest sensor & battery telemetry in Kelvin */
} app_context_t;

/* ============================================================================
 * Inline Temperature Unit Conversion Helpers (Kelvin Base Unit)
 * ========================================================================== */

/**
 * @brief Convert Celsius to canonical Kelvin.
 */
static inline float app_celsius_to_kelvin(float temp_c)
{
    return temp_c + APP_KELVIN_OFFSET_F;
}

/**
 * @brief Convert canonical Kelvin to user-selected display unit ('F', 'C', or 'K').
 */
static inline float app_kelvin_to_display(float temp_k, app_temp_unit_t unit)
{
    if (unit == APP_TEMP_UNIT_FAHRENHEIT) {
        return ((temp_k - APP_KELVIN_OFFSET_F) * 1.8f) + 32.0f;
    }
    if (unit == APP_TEMP_UNIT_CELSIUS) {
        return temp_k - APP_KELVIN_OFFSET_F;
    }
    return temp_k;
}

/**
 * @brief Return the unit suffix character ('F', 'C', or 'K') for UI formatting.
 */
static inline char app_temp_unit_char(app_temp_unit_t unit)
{
    if (unit == APP_TEMP_UNIT_FAHRENHEIT) {
        return 'F';
    }
    if (unit == APP_TEMP_UNIT_CELSIUS) {
        return 'C';
    }
    return 'K';
}

#ifdef __cplusplus
}
#endif

#endif /* HUMID1_APP_TYPES_H */
