/**
 * @file app_state.h
 * @brief NVS persistence, H1OS telemetry normalization, flow period validation, and alarm logic.
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#ifndef APP_STATE_H
#define APP_STATE_H

#include "esp_err.h"
#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Load persistent configuration from NVS (or initialize factory defaults).
 */
esp_err_t app_state_load_config(app_context_t *ctx);

/**
 * @brief Commit current configuration structure to NVS.
 */
esp_err_t app_state_save_config(const app_context_t *ctx);

/**
 * @brief Validate Flow A and Flow B intervals against all 6 H1OS MANDATE-02 invariants.
 */
bool app_state_validate_flow_periods(uint32_t flow_a_sec, uint32_t flow_b_sec);

/**
 * @brief Apply validated Flow A and Flow B intervals via Layer 4 h1os_set_flow_periods().
 */
esp_err_t app_state_apply_flow_periods(app_context_t *ctx, uint32_t flow_a_sec, uint32_t flow_b_sec);

/**
 * @brief Pull latest environmental & battery snapshot from H1OS via h1os_get_telemetry().
 */
esp_err_t app_state_refresh_telemetry(app_context_t *ctx);

/**
 * @brief Evaluate compound temperature and humidity thresholds with hysteresis.
 */
bool app_state_evaluate_alarms(app_context_t *ctx);

/**
 * @brief Format temperature value according to user's selected unit (C / F / K).
 */
float app_state_convert_temp(float temp_c, app_temp_unit_t unit, const char **out_unit_str);

/**
 * @brief Format vapor pressure value according to user's selected unit (hPa / kPa).
 */
float app_state_convert_vp(float vp_hpa, app_vp_unit_t unit, const char **out_unit_str);

#ifdef __cplusplus
}
#endif

#endif /* APP_STATE_H */
