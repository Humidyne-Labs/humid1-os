/**
 * @file net_wifi_ble.h
 * @brief Wi-Fi STA burst manager, BLE GATT/SoftAP onboarding, and H1OS PCF85063A SNTP sync.
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#ifndef NET_WIFI_BLE_H
#define NET_WIFI_BLE_H

#include "esp_err.h"
#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize static synchronization primitives for networking.
 */
esp_err_t net_wifi_ble_init(app_context_t *ctx);

/**
 * @brief Connect Wi-Fi STA with strict timeout for Flow B telemetry sync.
 */
esp_err_t net_wifi_connect_burst(app_context_t *ctx, uint32_t timeout_ms);

/**
 * @brief Disconnect Wi-Fi STA and power down radio before deep sleep.
 */
esp_err_t net_wifi_disconnect_and_stop(app_context_t *ctx);

/**
 * @brief Synchronize system clock with NTP and commit UTC epoch to PCF85063A RTC via h1os_time.h.
 */
esp_err_t net_sntp_sync_rtc_if_needed(app_context_t *ctx);

/**
 * @brief Start BLE GATT / SoftAP onboarding service when unprovisioned.
 */
esp_err_t net_ble_provisioning_start(app_context_t *ctx);

/**
 * @brief Stop BLE provisioning and release RF resources.
 */
esp_err_t net_ble_provisioning_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* NET_WIFI_BLE_H */
