/**
 * @file app_types.h
 * @brief Shared data structures, enumerations, and state models for humid1-os.
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include "app_config.h"
#include "h1os/h1os.h"
#include "h1os/h1os_lvgl.h"
#include "h1os/h1os_time.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Application UI screen identifiers (200x200 1-Bit e-Paper).
 */
typedef enum {
    SCREEN_DASHBOARD   = 0, /**< Screen 1: Primary environmental telemetry & status.       */
    SCREEN_DIAGNOSTICS = 1, /**< Screen 2: Psychrometric matrix, Tri-Node & battery stats. */
    SCREEN_SYSTEM      = 2, /**< Screen 3: System controls, theme polarity, mute, storage. */
    SCREEN_ONBOARDING  = 3, /**< Screen 4: Out-of-box QR code & BLE/claim onboarding.      */
    SCREEN_COUNT       = 4
} app_screen_id_t;

/**
 * @brief Temperature display unit enumeration.
 */
typedef enum {
    APP_TEMP_UNIT_CELSIUS    = 0, /**< Degrees Celsius (°C) - Default.    */
    APP_TEMP_UNIT_FAHRENHEIT = 1, /**< Degrees Fahrenheit (°F).           */
    APP_TEMP_UNIT_KELVIN     = 2  /**< Absolute Kelvin (K).               */
} app_temp_unit_t;

/**
 * @brief Vapor pressure display unit enumeration.
 */
typedef enum {
    APP_VP_UNIT_HPA = 0, /**< Hectopascals (hPa) - Default. */
    APP_VP_UNIT_KPA = 1  /**< Kilopascals (kPa).            */
} app_vp_unit_t;

/**
 * @brief 1-Bit e-Paper visual polarity theme.
 */
typedef enum {
    APP_THEME_LIGHT = 0, /**< Black text on White background (Default). */
    APP_THEME_DARK  = 1  /**< White text on Black background.           */
} app_theme_t;

/**
 * @brief Telemetry transmission mode (Wake-Send-Sleep vs Optional VBAT-Extending Batch Push).
 */
typedef enum {
    APP_TELEM_MODE_IMMEDIATE = 0, /**< Default: Wake -> Send current settled snapshot -> Sleep on every Flow B. */
    APP_TELEM_MODE_BATCH     = 1  /**< Optional VBAT Saver: Accumulate RTC ring-buffer samples & push every Nth Flow B. */
} app_telem_mode_t;

/**
 * @brief Threshold alarm trigger condition mode.
 */
typedef enum {
    APP_ALERT_MODE_ANY_TEMP      = 0, /**< Trigger if Temp exceeds high or low limit.                */
    APP_ALERT_MODE_ANY_RH        = 1, /**< Trigger if RH exceeds high or low limit.                  */
    APP_ALERT_MODE_EITHER_OR     = 2, /**< Trigger if EITHER Temp OR RH exceeds limits (Default).    */
    APP_ALERT_MODE_BOTH_REQUIRED = 3  /**< Trigger ONLY if BOTH Temp AND RH exceed limits together.  */
} app_alert_mode_t;

/**
 * @brief Persistent configuration model stored in NVS via h1os_nvs_*.
 */
typedef struct {
    bool             provisioned;                           /**< True if Wi-Fi credentials are saved.      */
    bool             claimed;                               /**< True if ThingsBoard device is claimed.    */
    char             tb_token[APP_TOKEN_MAX_LEN];           /**< ThingsBoard device access token.          */
    char             claim_secret[APP_CLAIM_SECRET_MAX_LEN];/**< Factory/Dynamic claim secret key.         */
    char             nickname[APP_NICKNAME_MAX_LEN];        /**< User-assigned humidor nickname.           */
    uint32_t         flow_a_sec;                            /**< Headless sensor burst interval (seconds). */
    uint32_t         flow_b_sec;                            /**< Display & cloud sync interval (seconds).  */
    app_temp_unit_t  temp_unit;                             /**< Active temperature unit (C / F / K).      */
    app_vp_unit_t    vp_unit;                               /**< Active vapor pressure unit (hPa / kPa).   */
    app_theme_t      theme;                                 /**< Active 1-bit UI polarity theme.           */
    bool             sound_enabled;                         /**< True if acoustic feedback is enabled.     */
    uint8_t          volume_pct;                            /**< Master speaker volume (0..100%).          */
    app_alert_mode_t alert_mode;                            /**< Compound alarm evaluation mode.           */
    bool             batch_telemetry_en;                    /**< False=Wake-Send-Sleep, True=Batch History.*/
    uint8_t          batch_flow_b_ratio;                    /**< Flow B cycles per Wi-Fi burst in batch mode.*/
    float            temp_high_c;                           /**< High temperature alarm threshold (°C).    */
    float            temp_low_c;                            /**< Low temperature alarm threshold (°C).     */
    float            rh_high_pct;                           /**< High relative humidity threshold (%RH).   */
    float            rh_low_pct;                            /**< Low relative humidity threshold (%RH).    */
} app_nvs_config_t;

/**
 * @brief Normalized environmental & hardware telemetry snapshot derived from h1os_telemetry_t.
 */
typedef struct {
    /* Primary Settled Environmental Metrics */
    float                ambient_temp_c;
    float                ambient_temp_f;
    float                ambient_temp_k;
    float                ambient_rh_pct;
    float                dew_point_c;
    float                dew_point_f;
    float                dew_point_k;
    float                vapor_pressure_hpa;
    float                sat_vapor_pressure_hpa;
    float                absolute_humidity_gm3;

    /* Tri-Node & Window Diagnostics */
    float                shtc3_raw_temp_c;
    float                mcu_die_temp_c;
    float                epd_glass_temp_c;
    float                trinode_temp_c;
    float                trinode_delta_c;
    float                window_low_c;
    float                window_high_c;
    uint8_t              confidence_pct;
    bool                 thermally_settled;
    uint16_t             valid_trend;

    /* Power & Autonomy Metrics */
    float                autonomy_days;
    float                avg_current_ma;
    uint16_t             battery_mv;
    uint8_t              battery_soc_pct;
    bool                 vbus_present;
    h1os_battery_state_t battery_state;

    /* Monotonic Counters & Flow Intervals */
    uint32_t             sample_count;
    uint32_t             boot_count;
    uint32_t             wake_count;
    uint32_t             flow_b_count;
    uint32_t             button_wake_count;
    uint32_t             flow_a_period_sec;
    uint32_t             flow_b_period_sec;

    /* Alarm Evaluation State */
    bool                 alarm_temp_high;
    bool                 alarm_temp_low;
    bool                 alarm_rh_high;
    bool                 alarm_rh_low;
    bool                 compound_alarm_active;
} app_telemetry_snapshot_t;

/**
 * @brief ThingsBoard Native OTA shared attribute state (APP_06).
 */
typedef struct {
    char target_fw_title[32];
    char target_fw_version[24];
    char target_fw_checksum[68];
    bool auto_update_enabled;
    bool manual_ota_trigger;
} app_ota_attrs_t;

/**
 * @brief Master runtime context for humid1-os (Strictly static allocation in .bss).
 */
typedef struct {
    app_nvs_config_t         cfg;
    app_ota_attrs_t          attrs;
    app_telemetry_snapshot_t telemetry;
    app_screen_id_t          active_screen;
    uint8_t                  system_menu_index;
    h1os_wake_state_t        wake_state;
    bool                     is_interactive_wake;
    bool                     in_onboarding_mode;
    uint32_t                 onboarding_elapsed_sec;
    bool                     wifi_connected;
    bool                     mqtt_connected;
    bool                     ota_in_progress;
    bool                     sync_ui_dirty;
    int8_t                   wifi_rssi_dbm;
    uint32_t                 last_sntp_sync_epoch;
    lv_obj_t                *root_screen;
    lv_group_t              *input_group;
} app_context_t;

#ifdef __cplusplus
}
#endif

#endif /* APP_TYPES_H */
