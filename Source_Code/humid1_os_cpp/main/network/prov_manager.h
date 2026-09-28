/**
 * @file prov_manager.h
 * @brief BLE GATT Wi-Fi Provisioning Engine & Event Orchestration
 * 
 * Hardware Target:
 *  - ESP32-S3 BLE GATT Provisioning
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

typedef void (*app_prov_success_cb_t)(void *user_data);

/**
 * @brief Start BLE GATT Wi-Fi Provisioning Service
 * 
 * Sets up the service name (`PROV_<DEVICE_ID>`), passes the 8-character PoP PIN,
 * renders the interactive QR screen, and listens for incoming Wi-Fi credentials.
 * 
 * @param success_cb Callback invoked when credentials are successfully received & verified
 * @param user_data Custom context pointer
 * @return esp_err_t ESP_OK on success
 */
esp_err_t prov_manager_start(app_prov_success_cb_t success_cb, void *user_data);

/**
 * @brief Stop Provisioning Service and release BLE memory
 */
void prov_manager_stop(void);

/**
 * @brief Check if BLE provisioning is currently running
 * 
 * @return true if running
 */
bool prov_manager_is_running(void);

#ifdef __cplusplus
}
#endif
