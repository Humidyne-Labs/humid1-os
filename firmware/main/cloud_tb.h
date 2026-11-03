/**
 * @file cloud_tb.h
 * @brief ThingsBoard CE MQTTS client, zero-heap telemetry serializer, RPC handler, and OTA engine.
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#ifndef CLOUD_TB_H
#define CLOUD_TB_H

#include "esp_err.h"
#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Execute bounded ThingsBoard MQTTS telemetry publish, attribute sync, and RPC check.
 *
 * Runs on Core 0 inside the H1OS on_sync() lifecycle callback while Core 1 drives LVGL.
 */
esp_err_t cloud_tb_sync_burst(app_context_t *ctx);

/**
 * @brief Format JSON telemetry frame into caller-supplied static buffer (zero heap allocation).
 */
int cloud_tb_format_telemetry_json(const app_context_t *ctx, char *buf, size_t buf_size);

/**
 * @brief Format client-side attributes JSON into caller-supplied static buffer.
 */
int cloud_tb_format_client_attributes_json(const app_context_t *ctx, char *buf, size_t buf_size);

/**
 * @brief Dispatch an incoming ThingsBoard server-side RPC request JSON frame.
 */
esp_err_t cloud_tb_handle_rpc_json(app_context_t *ctx, int request_id, const char *json_str);

/**
 * @brief Verify current OTA image health and cancel rollback if boot succeeded.
 */
esp_err_t cloud_tb_verify_ota_image(void);

#ifdef __cplusplus
}
#endif

#endif /* CLOUD_TB_H */
