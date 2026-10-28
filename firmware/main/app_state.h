/**
 * @file app_state.h
 * @brief NVS persistence, RTC fast-cache management, and dynamic Flow A/B interval synchronization.
 *
 * Implements the state persistence and factory reset rules specified in APP_01 and APP_03.
 */

#ifndef HUMID1_APP_STATE_H
#define HUMID1_APP_STATE_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize application context, derive hardware UID (HUMID1_<MAC>),
 *        validate RTC Slow SRAM cache, and load persistent NVS settings.
 *
 * @param[out] ctx Pointer to the master application context.
 * @return ESP_OK on success, or an esp_err_t code on NVS failure.
 */
esp_err_t app_state_init(app_context_t *ctx);

/**
 * @brief Obtain a pointer to the persistent RTC Slow SRAM cache.
 *
 * @return Pointer to the RTC_DATA_ATTR cache structure.
 */
app_rtc_cache_t *app_state_get_rtc_cache(void);

/**
 * @brief Save modified configuration attributes to the "app_cfg" NVS namespace
 *        and invoke nvs_commit() (APP-PIT-05).
 *
 * @param[in] ctx Pointer to the master application context.
 * @return ESP_OK on success, or an esp_err_t code on NVS failure.
 */
esp_err_t app_state_save_nvs(const app_context_t *ctx);

/**
 * @brief Dynamically apply updated Flow A and Flow B wake intervals to both NVS
 *        and the active H1OS orchestrator state (MANDATE-02).
 *
 * @param[in,out] ctx        Pointer to the master application context.
 * @param[in]     flow_a_sec New Flow A headless sample period in seconds (15..3600).
 * @param[in]     flow_b_sec New Flow B cloud sync period in seconds (60..86400).
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if out of bounds.
 */
esp_err_t app_state_apply_flow_periods(app_context_t *ctx, uint32_t flow_a_sec, uint32_t flow_b_sec);

/**
 * @brief Populate the unified telemetry frame in Kelvin from H1OS sensor and battery APIs.
 *
 * @param[in,out] ctx Pointer to the master application context.
 * @return ESP_OK on success.
 */
esp_err_t app_state_refresh_telemetry(app_context_t *ctx);

/**
 * @brief Clear Wi-Fi credentials (and optionally ThingsBoard claim state) in NVS
 *        and reboot into BLE provisioning mode (APP_03 Section 5).
 *
 * @param[in,out] ctx           Pointer to the master application context.
 * @param[in]     factory_reset If true, also clears tb_token, claimed flag, and nickname.
 * @return ESP_OK prior to system restart.
 */
esp_err_t app_state_reset_credentials(app_context_t *ctx, bool factory_reset);

#ifdef __cplusplus
}
#endif

#endif /* HUMID1_APP_STATE_H */
