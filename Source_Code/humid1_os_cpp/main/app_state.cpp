/**
 * @file app_state.cpp
 * @brief Application Configuration & Dynamic Server Attribute State Implementation
 * 
 * Uses BSP unambiguous key generation and persistent NVS storage.
 * 
 * @attribution
 * - Humidyne Labs / Humiditron (2026)
 * 
 * SPDX-License-Identifier: MIT
 */

#include "app_state.h"
#include "app_config.h"
#include <string.h>
#include "esp_log.h"
#include "bsp/bsp.h"

static const char *TAG = "app_state";

static app_state_t s_state;

esp_err_t app_state_init(void)
{
    memset(&s_state, 0, sizeof(s_state));

    // Dynamic Server defaults (overridden dynamically as server pushes shared attributes)
    s_state.sleep_interval_sec = APP_DEFAULT_SLEEP_SEC;
    s_state.temp_unit          = APP_TEMP_UNIT_FAHRENHEIT;
    s_state.sound_enabled      = true;

    // Default canonical Kelvin thresholds (rule chain compatible)
    s_state.thresholds.rh_low_critical      = 62.0f;
    s_state.thresholds.rh_low_warning       = 65.0f;
    s_state.thresholds.rh_high_warning      = 73.0f;
    s_state.thresholds.rh_high_critical     = 76.0f;
    s_state.thresholds.temp_low_critical_k  = 287.60f; // 58.0 °F
    s_state.thresholds.temp_low_warning_k   = 290.93f; // 64.0 °F
    s_state.thresholds.temp_high_warning_k  = 295.37f; // 72.0 °F
    s_state.thresholds.temp_high_critical_k = 297.04f; // 75.0 °F
    s_state.thresholds.batt_low_critical    = 15;
    s_state.thresholds.batt_low_warning     = 25;

    // Hardware Identification from BSP
    bsp_get_device_id(s_state.device_name, sizeof(s_state.device_name));


    // Check Provisioning status
    s_state.is_provisioned = app_state_has_wifi_credentials();

    // Check Claiming status
    uint32_t claimed_val = 0;
    if (bsp_nvs_get_u32("tb_claimed", &claimed_val) == ESP_OK && claimed_val == 1) {
        s_state.is_claimed = true;
    } else {
        s_state.is_claimed = false;
    }

    // Load persisted PoP PIN or generate using BSP unambiguous key generator
    if (bsp_nvs_get_str("prov_pop", s_state.pop_pin, sizeof(s_state.pop_pin)) != ESP_OK || strlen(s_state.pop_pin) < APP_PROV_POP_LEN) {
        bsp_generate_unambiguous_key(s_state.pop_pin, APP_PROV_POP_LEN, NULL);
        bsp_nvs_set_str("prov_pop", s_state.pop_pin);
    }

    // Load persisted server-synced parameters
    uint32_t saved_sleep = 0;
    if (bsp_nvs_get_u32("sleep_sec", &saved_sleep) == ESP_OK && saved_sleep >= APP_MIN_SLEEP_SEC && saved_sleep <= APP_MAX_SLEEP_SEC) {
        s_state.sleep_interval_sec = saved_sleep;
    }

    uint32_t saved_unit = 0;
    if (bsp_nvs_get_u32("temp_unit", &saved_unit) == ESP_OK && saved_unit <= APP_TEMP_UNIT_KELVIN) {
        s_state.temp_unit = (app_temp_unit_t)saved_unit;
    }

    ESP_LOGI(TAG, "State Loaded: Dev='%s', Prov=%d, Claimed=%d, Sleep=%lus, PoP='%s'",
             s_state.device_name, s_state.is_provisioned, s_state.is_claimed,
             (unsigned long)s_state.sleep_interval_sec, s_state.pop_pin);

    return ESP_OK;
}

app_state_t *app_state_get(void)
{
    return &s_state;
}

bool app_state_has_wifi_credentials(void)
{
    char ssid[33] = {0};
    esp_err_t ret = bsp_nvs_get_str("wifi_ssid", ssid, sizeof(ssid));
    return (ret == ESP_OK && strlen(ssid) > 0);
}

bool app_state_is_claimed(void)
{
    return s_state.is_claimed;
}

esp_err_t app_state_set_claimed(bool claimed)
{
    s_state.is_claimed = claimed;
    return bsp_nvs_set_u32("tb_claimed", claimed ? 1 : 0);
}

esp_err_t app_state_save_config(void)
{
    bsp_nvs_set_u32("sleep_sec", s_state.sleep_interval_sec);
    bsp_nvs_set_u32("temp_unit", (uint32_t)s_state.temp_unit);
    return ESP_OK;
}

esp_err_t app_state_factory_wipe(void)
{
    ESP_LOGW(TAG, "Executing Factory Wipe (NVS + Wi-Fi cache)...");
    bsp_wifi_invalidate_fast_cache();
    bsp_nvs_wipe_all();
    s_state.is_provisioned = false;
    s_state.is_claimed     = false;
    return ESP_OK;
}
