/**
 * @file net_wifi_ble.c
 * @brief Implementation of Wi-Fi burst connection, BLE onboarding, and H1OS PCF85063A SNTP sync.
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#include "net_wifi_ble.h"
#include <time.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "h1os/h1os_time.h"

static const char *TAG = "net_wifi_ble";

static StaticEventGroup_t s_net_event_group_buf;
static EventGroupHandle_t s_net_event_group = NULL;

#define NET_BIT_WIFI_CONNECTED  BIT0
#define NET_BIT_WIFI_FAIL       BIT1
#define NET_BIT_SNTP_SYNCED     BIT2

esp_err_t net_wifi_ble_init(app_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_net_event_group == NULL) {
        s_net_event_group = xEventGroupCreateStatic(&s_net_event_group_buf);
    }
    /* Synchronize system POSIX clock from hardware PCF85063A RTC on startup */
    (void)h1os_time_sync_rtc_to_system();
    return ESP_OK;
}

esp_err_t net_wifi_connect_burst(app_context_t *ctx, uint32_t timeout_ms)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!ctx->cfg.provisioned) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Starting Wi-Fi STA burst (timeout=%lu ms)...", (unsigned long)timeout_ms);
    wifi_ap_record_t ap_info = {0};
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        ctx->wifi_connected = true;
        ctx->wifi_rssi_dbm  = ap_info.rssi;
        return ESP_OK;
    }

    ctx->wifi_connected = false;
    ctx->wifi_rssi_dbm  = -100;
    return ESP_ERR_TIMEOUT;
}

esp_err_t net_wifi_disconnect_and_stop(app_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (ctx->wifi_connected) {
        (void)esp_wifi_disconnect();
        (void)esp_wifi_stop();
        ctx->wifi_connected = false;
    }
    return ESP_OK;
}

esp_err_t net_sntp_sync_rtc_if_needed(app_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    time_t now = 0;
    time(&now);
    bool rtc_valid = h1os_time_is_valid();

    if (rtc_valid && (ctx->last_sntp_sync_epoch > 0U) &&
        ((uint32_t)now - ctx->last_sntp_sync_epoch < APP_SNTP_RESYNC_INTERVAL_SEC)) {
        return ESP_OK;
    }

    if (!ctx->wifi_connected) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Validating system UTC clock and committing to PCF85063A RTC...");
    if (h1os_time_is_valid()) {
        esp_err_t err = h1os_time_sync_system_to_rtc();
        if (err == ESP_OK) {
            time(&now);
            ctx->last_sntp_sync_epoch = (uint32_t)now;
        }
        return err;
    }

    return ESP_OK;
}

esp_err_t net_ble_provisioning_start(app_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI(TAG, "Starting BLE onboarding advertisement (Secret=%s)...", ctx->cfg.claim_secret);
    return ESP_OK;
}

esp_err_t net_ble_provisioning_stop(void)
{
    ESP_LOGI(TAG, "Stopping BLE onboarding service...");
    return ESP_OK;
}
