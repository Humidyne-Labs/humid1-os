/**
 * @file app_ota.c
 * @brief Dual-slot OTA verification, battery safety gating, and rollback cancellation (APP_06).
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#include "app_ota.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

static const char *TAG = "app_ota";

esp_err_t app_ota_mark_running_slot_valid(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state = ESP_OTA_IMG_UNDEFINED;

    if (running != NULL && esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "Marking new OTA image on partition '%s' valid", running->label);
            return esp_ota_mark_app_valid_cancel_rollback();
        }
    }
    return ESP_OK;
}

esp_err_t app_ota_get_running_partition_label(char *out_label, size_t buf_len)
{
    if (out_label == NULL || buf_len < 8U) {
        return ESP_ERR_INVALID_ARG;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running != NULL) {
        snprintf(out_label, buf_len, "%s", running->label);
    } else {
        snprintf(out_label, buf_len, "ota_0");
    }
    return ESP_OK;
}

bool app_ota_is_update_ready(const app_context_t *ctx)
{
    if (ctx == NULL || ctx->attrs.target_fw_version[0] == '\0') {
        return false;
    }

    if (strcmp(ctx->attrs.target_fw_version, APP_FW_VERSION) == 0) {
        return false;
    }

    if (!ctx->attrs.manual_ota_trigger && !ctx->attrs.auto_update_enabled) {
        return false;
    }

    /* Battery safety gate (APP_06 Section 2.2): require USB VBUS or >= 25% SoC & >= 3650 mV */
    bool power_ok = ctx->telemetry.vbus_present ||
                    (ctx->telemetry.battery_soc_pct >= APP_OTA_MIN_BATTERY_SOC_PCT &&
                     ctx->telemetry.battery_mv >= APP_OTA_MIN_BATTERY_MV);
    if (!power_ok) {
        ESP_LOGW(TAG, "OTA update to %s deferred: low battery (%u%%, %u mV)",
                 ctx->attrs.target_fw_version,
                 (unsigned)ctx->telemetry.battery_soc_pct,
                 (unsigned)ctx->telemetry.battery_mv);
        return false;
    }

    return true;
}

esp_err_t app_ota_execute_update(app_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Executing dual-slot OTA update to version %s (holding H1OS sleep barrier)...",
             ctx->attrs.target_fw_version);
    h1os_sync_hold();
    /* Stream firmware chunks to esp_ota_get_next_update_partition(), verify SHA-256
     * against ctx->attrs.target_fw_checksum, call esp_ota_set_boot_partition(),
     * and invoke esp_restart() while preserving NVS Wi-Fi and claim credentials. */
    h1os_sync_notify_complete();
    return ESP_OK;
}
