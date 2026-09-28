/**
 * @file button_handler.h
 * @brief Tactile Button Event Dispatcher & Factory Reset Manager
 * 
 * Hardware Target:
 *  - BOOT Button: GPIO 0 (Long press triggers Factory Reset)
 *  - POWER Button: GPIO 18 (Click / Long press)
 * 
 * @attribution
 * - Humidyne Labs / Humiditron (2026)
 * 
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*app_factory_reset_cb_t)(void *user_data);

/**
 * @brief Initialize Application Button Handling Subsystem
 * 
 * Registers callbacks for BOOT button long press (Factory Reset) and POWER button.
 * 
 * @param reset_cb Callback invoked when user holds BOOT for >3 seconds
 * @param user_data Context pointer passed to reset callback
 * @return esp_err_t ESP_OK on success
 */
esp_err_t button_handler_init(app_factory_reset_cb_t reset_cb, void *user_data);

#ifdef __cplusplus
}
#endif
