/**
 * @file app_state.h
 * @brief Application Configuration & Dynamic Server Attribute State
 * 
 * Dynamic and reactive dataset dictated by ThingsBoard Server attributes.
 * 
 * @attribution
 * - Humidyne Labs / Humiditron (2026)
 * 
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_TEMP_UNIT_FAHRENHEIT = 0,
    APP_TEMP_UNIT_CELSIUS,
    APP_TEMP_UNIT_KELVIN
} app_temp_unit_t;

typedef enum {
    APP_STAGE_PROVISIONING = 0,
    APP_STAGE_CLAIMING     = 1,
    APP_STAGE_RUN_LOOP     = 2,
    APP_STAGE_SHUTDOWN     = 3,
} app_stage_t;


typedef struct {
    uint32_t last_sntp_sync_ts;
    uint32_t wake_cycle_count;
} app_rtc_state_t;

typedef struct {
    float rh_low_critical;
    float rh_low_warning;
    float rh_high_warning;
    float rh_high_critical;
    float temp_low_critical_k;
    float temp_low_warning_k;
    float temp_high_warning_k;
    float temp_high_critical_k;
    uint8_t batt_low_critical;
    uint8_t batt_low_warning;
} app_alarm_thresholds_t;

typedef struct {
    bool                   is_provisioned;
    bool                   is_claimed;
    uint32_t               sleep_interval_sec;
    app_temp_unit_t        temp_unit;
    bool                   sound_enabled;
    char                   device_name[32];
    char                   pop_pin[16];
    char                   claim_key[16];
    app_alarm_thresholds_t thresholds;
} app_state_t;

/**
 * @brief Initialize Application State & Load Persistent Attributes
 */
esp_err_t app_state_init(void);

/**
 * @brief Retrieve Pointer to Global State Struct
 */
app_state_t *app_state_get(void);

/**
 * @brief Check if Wi-Fi credentials exist in NVS
 */
bool app_state_has_wifi_credentials(void);

/**
 * @brief Check if ThingsBoard device is claimed
 */
bool app_state_is_claimed(void);

/**
 * @brief Mark device claimed state in NVS
 */
esp_err_t app_state_set_claimed(bool claimed);

/**
 * @brief Save current server-synchronized configuration to NVS
 */
esp_err_t app_state_save_config(void);

/**
 * @brief Factory wipe (NVS + Wi-Fi Cache)
 */
esp_err_t app_state_factory_wipe(void);

#ifdef __cplusplus
}
#endif
