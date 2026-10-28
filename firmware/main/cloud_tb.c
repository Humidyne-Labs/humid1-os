/**
 * @file cloud_tb.c
 * @brief Skeleton implementation of ThingsBoard MQTTS client and server-driven JSON serializer.
 */

#include "cloud_tb.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_random.h"
#include "app_state.h"

static const char *TAG = "cloud_tb";

static const char CLAIM_ALPHABET[] = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";

esp_err_t cloud_tb_auto_provision(app_context_t *ctx, uint32_t timeout_ms)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "[SKELETON] Phase 1 TB Auto-Provisioning on %s for %s (timeout=%lu ms)",
             APP_TB_TOPIC_PROV_REQ, ctx->attrs.device_uid, (unsigned long)timeout_ms);

    /* TODO (Active Dev): Connect to APP_TB_BROKER_URI with username "provision",
     * publish deviceName + provisionDeviceKey + provisionDeviceSecret, and parse credentialsValue. */
    return ESP_OK;
}

esp_err_t cloud_tb_begin_claiming(app_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Generate 6-character unambiguous alphanumeric Claim PIN via hardware TRNG */
    const size_t alpha_len = sizeof(CLAIM_ALPHABET) - 1U;
    for (size_t i = 0U; i < 6U; ++i) {
        uint32_t idx = esp_random() % (uint32_t)alpha_len;
        ctx->claim_pin[i] = CLAIM_ALPHABET[idx];
    }
    ctx->claim_pin[6]  = '\0';
    ctx->claim_rem_sec = (uint16_t)(APP_TB_CLAIM_DURATION_MS / 1000UL);

    ESP_LOGI(TAG, "[SKELETON] Phase 2 TB Claiming on %s: UID=%s PIN=%s window=%u s",
             APP_TB_TOPIC_CLAIM, ctx->attrs.device_uid, ctx->claim_pin,
             (unsigned int)ctx->claim_rem_sec);
    return ESP_OK;
}

esp_err_t cloud_tb_format_telemetry_json(const app_context_t *ctx, char *out_buf, size_t buf_len)
{
    if (ctx == NULL || out_buf == NULL || buf_len < 128U) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Canonical 4-key telemetry frame in Kelvin per APP_02 Section 3.2 */
    int n = snprintf(out_buf, buf_len,
                     "{\"rh\":%.2f,\"temp\":%.2f,\"battery\":%u,\"rssi\":%d",
                     (double)ctx->telemetry.rh_pct,
                     (double)ctx->telemetry.temp_k,
                     (unsigned int)ctx->telemetry.battery_pct,
                     (int)ctx->telemetry.rssi_dbm);
    if (n < 0 || (size_t)n >= buf_len - 2U) {
        return ESP_ERR_INVALID_SIZE;
    }

    size_t offset = (size_t)n;
    const uint32_t mask = ctx->attrs.telemetry_schema_mask;

    if ((mask & (uint32_t)APP_TELE_MASK_DEW_POINT_K) != 0U) {
        n = snprintf(out_buf + offset, buf_len - offset,
                     ",\"dew_point\":%.2f", (double)ctx->telemetry.dew_point_k);
        if (n > 0 && (offset + (size_t)n) < (buf_len - 2U)) {
            offset += (size_t)n;
        }
    }
    if ((mask & (uint32_t)APP_TELE_MASK_VBAT_MV) != 0U) {
        n = snprintf(out_buf + offset, buf_len - offset,
                     ",\"vbat_mv\":%u", (unsigned int)ctx->telemetry.vbat_mv);
        if (n > 0 && (offset + (size_t)n) < (buf_len - 2U)) {
            offset += (size_t)n;
        }
    }
    if ((mask & (uint32_t)APP_TELE_MASK_AUTONOMY_DAYS) != 0U) {
        n = snprintf(out_buf + offset, buf_len - offset,
                     ",\"autonomy_days\":%.1f", (double)ctx->telemetry.autonomy_days);
        if (n > 0 && (offset + (size_t)n) < (buf_len - 2U)) {
            offset += (size_t)n;
        }
    }
    if ((mask & (uint32_t)APP_TELE_MASK_CONFIDENCE) != 0U) {
        n = snprintf(out_buf + offset, buf_len - offset,
                     ",\"confidence\":%u", (unsigned int)ctx->telemetry.confidence_pct);
        if (n > 0 && (offset + (size_t)n) < (buf_len - 2U)) {
            offset += (size_t)n;
        }
    }

    out_buf[offset]     = '}';
    out_buf[offset + 1] = '\0';
    return ESP_OK;
}

esp_err_t cloud_tb_sync_burst(app_context_t *ctx, uint32_t timeout_ms)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char json_buf[256] = {0};
    esp_err_t err = cloud_tb_format_telemetry_json(ctx, json_buf, sizeof(json_buf));
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "[SKELETON] MQTTS Sync Burst (%lu ms): %s -> %s",
             (unsigned long)timeout_ms, APP_TB_TOPIC_TELEMETRY, json_buf);
    return ESP_OK;
}
