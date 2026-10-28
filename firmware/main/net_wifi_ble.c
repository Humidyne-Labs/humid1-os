/**
 * @file net_wifi_ble.c
 * @brief Skeleton implementation of BLE Security 1 Wi-Fi Provisioning, Fast Wi-Fi Burst, and SNTP.
 */

#include "net_wifi_ble.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_random.h"
#include "h1os/h1os.h"

static const char *TAG = "net_wifi_ble";

esp_err_t net_ble_prov_start(app_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Generate 8-digit numeric Proof-of-Possession PIN via hardware TRNG (APP_03) */
    uint32_t rand_val = esp_random() % 100000000UL;
    snprintf(ctx->prov_pop_pin, sizeof(ctx->prov_pop_pin), "%08lu", (unsigned long)rand_val);

    ESP_LOGI(TAG, "[SKELETON] Starting BLE Security 1 Provisioning: Service=%s PoP=%s",
             ctx->attrs.device_uid, ctx->prov_pop_pin);

    /* TODO (Active Dev): Initialize wifi_prov_mgr with WIFI_PROV_SCHEME_BLE,
     * WIFI_PROV_EVENT_HANDLER_FREE_BTDM, and WIFI_PROV_SECURITY_1. */
    return ESP_OK;
}

esp_err_t net_ble_prov_format_qr_uri(const app_context_t *ctx, char *out_buf, size_t buf_len)
{
    if (ctx == NULL || out_buf == NULL || buf_len < 64U) {
        return ESP_ERR_INVALID_ARG;
    }

    int written = snprintf(out_buf, buf_len,
                           "{\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"ble\"}",
                           ctx->attrs.device_uid, ctx->prov_pop_pin);
    if (written < 0 || (size_t)written >= buf_len) {
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

esp_err_t net_wifi_connect_burst(app_context_t *ctx, uint32_t timeout_ms)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "[SKELETON] Fast Wi-Fi connect burst (timeout=%lu ms)", (unsigned long)timeout_ms);
    ctx->wifi_connected   = true;
    ctx->telemetry.rssi_dbm = -62;
    snprintf(ctx->ssid_str, sizeof(ctx->ssid_str), "Humidor_LAN");
    snprintf(ctx->ip_addr_str, sizeof(ctx->ip_addr_str), "192.168.1.100");
    return ESP_OK;
}

esp_err_t net_wifi_stop_burst(app_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (ctx->wifi_connected) {
        ESP_LOGI(TAG, "[SKELETON] Stopping Wi-Fi STA radio prior to deep sleep");
        ctx->wifi_connected = false;
    }
    return ESP_OK;
}

esp_err_t net_sntp_ensure_valid_time(app_context_t *ctx, uint32_t timeout_ms)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (h1os_rtc_is_time_valid()) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "[SKELETON] Synchronizing PCF85063A RTC via SNTP (timeout=%lu ms)",
             (unsigned long)timeout_ms);
    /* TODO (Active Dev): Run esp_netif_sntp_init() and call h1os_set_time() */
    return ESP_OK;
}
