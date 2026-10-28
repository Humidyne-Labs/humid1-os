/**
 * @file cloud_tb.h
 * @brief ThingsBoard MQTTS Client: Auto-Provisioning, Customer Claiming, Server-Driven Telemetry,
 *        Shared Attribute Synchronization, and Server-Managed Alarm Reception.
 *
 * Implements the protocol contracts specified in APP_02.
 */

#ifndef HUMID1_CLOUD_TB_H
#define HUMID1_CLOUD_TB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Execute Phase 1 ThingsBoard device auto-provisioning (/provision/request)
 *        using the Device Profile key/secret and store the assigned access token in NVS.
 *
 * @param[in,out] ctx        Pointer to the master application context.
 * @param[in]     timeout_ms Bounded MQTTS transaction timeout in milliseconds.
 * @return ESP_OK when tb_access_token is persisted in NVS.
 */
esp_err_t cloud_tb_auto_provision(app_context_t *ctx, uint32_t timeout_ms);

/**
 * @brief Generate a 6-character alphanumeric Claim PIN via hardware TRNG and publish
 *        the Phase 2 claiming initiation frame to v1/devices/me/claim (APP_02 Section 2.2).
 *
 * @param[in,out] ctx Pointer to the master application context.
 * @return ESP_OK on successful publish.
 */
esp_err_t cloud_tb_begin_claiming(app_context_t *ctx);

/**
 * @brief Execute a full Flow B / Interactive MQTTS synchronization burst:
 *        1. Connect to mqtts://humid1.com:8883 with esp_crt_bundle_attach
 *        2. Publish CLIENT_SCOPE attributes and server-driven telemetry (Kelvin base unit)
 *        3. Request and apply SHARED_SCOPE attributes (intervals, theme, alarm_state, OTA flags)
 *
 * @param[in,out] ctx        Pointer to the master application context.
 * @param[in]     timeout_ms Bounded MQTTS transaction timeout in milliseconds.
 * @return ESP_OK on successful synchronization.
 */
esp_err_t cloud_tb_sync_burst(app_context_t *ctx, uint32_t timeout_ms);

/**
 * @brief Serialize the unified server-driven telemetry JSON document into a caller-supplied
 *        static buffer according to ctx->attrs.telemetry_schema_mask (zero heap allocation).
 *
 * @param[in]  ctx     Pointer to the master application context.
 * @param[out] out_buf Destination character buffer.
 * @param[in]  buf_len Capacity of out_buf in bytes.
 * @return ESP_OK on success, ESP_ERR_INVALID_SIZE if buffer is too small.
 */
esp_err_t cloud_tb_format_telemetry_json(const app_context_t *ctx, char *out_buf, size_t buf_len);

#ifdef __cplusplus
}
#endif

#endif /* HUMID1_CLOUD_TB_H */
