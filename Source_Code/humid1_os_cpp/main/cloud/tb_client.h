/**
 * @file tb_client.h
 * @brief ThingsBoard IoT Framework Client Wrapper
 * 
 * Hardware Target:
 *  - ESP32-S3 MQTTS ThingsBoard Client pinned to Core 0
 * 
 * @attribution
 * - ThingsBoard.io Protocol Specifications
 * - Humidyne Labs / Humiditron (2026)
 * 
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "telemetry/sensor_reader.h"
#include "app_state.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*app_claim_complete_cb_t)(bool success, void *user_data);

/**
 * @brief Initialize ThingsBoard MQTTS Client Engine
 * 
 * @param broker_uri MQTTS Broker URI (or NULL for default in app_config.h)
 * @param access_token Optional access token (or NULL for claiming workflow)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t tb_client_init(const char *broker_uri, const char *access_token);

/**
 * @brief Wait until Connected to ThingsBoard Broker
 * 
 * @param timeout_ms Max time to wait in ms
 * @return esp_err_t ESP_OK if connected
 */
esp_err_t tb_client_wait_connected(uint32_t timeout_ms);

/**
 * @brief Check Connection Status
 * 
 * @return true if connected
 */
bool tb_client_is_connected(void);

/**
 * @brief Execute Device Claiming Workflow
 * 
 * Generates an unambiguous 6-character claiming PIN, publishes `v1/devices/me/claim`
 * to ThingsBoard, renders the claiming screen, and monitors for claiming completion.
 * 
 * @param duration_ms Claim window in milliseconds (e.g. 180,000 ms)
 * @param cb Callback invoked when claiming succeeds or expires
 * @param user_data Context pointer
 * @return esp_err_t ESP_OK on success
 */
esp_err_t tb_client_start_claiming(uint32_t duration_ms, app_claim_complete_cb_t cb, void *user_data);

/**
 * @brief Synchronously Publish Environmental Telemetry (QoS 1)
 * 
 * @param telemetry Pointer to telemetry snapshot
 * @param timeout_ms Max wait time for broker ACK
 * @return esp_err_t ESP_OK on confirmed receipt
 */
esp_err_t tb_client_send_telemetry(const app_telemetry_data_t *telemetry, uint32_t timeout_ms);

/**
 * @brief Synchronize Client and Shared Attributes with ThingsBoard Server
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t tb_client_sync_attributes(void);

/**
 * @brief Disconnect and Shutdown ThingsBoard Client
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t tb_client_disconnect(void);

#ifdef __cplusplus
}
#endif
