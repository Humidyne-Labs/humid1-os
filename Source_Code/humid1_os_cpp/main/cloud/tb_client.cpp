/**
 * @file tb_client.cpp
 * @brief ThingsBoard IoT Framework Client Wrapper Implementation
 * 
 * Reactive server-driven architecture supporting dynamic attributes, RPCs, and OTA updates.
 * 
 * @attribution
 * - ThingsBoard.io Protocol Specifications
 * - Humidyne Labs / Humiditron (2026)
 * 
 * SPDX-License-Identifier: MIT
 */

#include "tb_client.h"
#include "app_config.h"
#include "app_state.h"
#include "ui/ui_manager.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp/bsp.h"


static const char *TAG = "tb_client";

static app_claim_complete_cb_t s_claim_cb         = NULL;
static void                    *s_claim_user_data = NULL;
static TaskHandle_t            s_claim_task       = NULL;
static volatile bool           s_claim_active     = false;

static void on_tb_ota_cb(bsp_ota_status_t status, int progress_pct, const char *msg, void *user_data)
{
    ESP_LOGI(TAG, "OTA Progress: Status=%d, Progress=%d%%, Msg='%s'", (int)status, progress_pct, msg ? msg : "");
    ui_show_ota_screen(progress_pct, msg);
}

static void parse_dynamic_shared_attributes(const char *json_payload)
{
    if (!json_payload) return;

    cJSON *root = cJSON_Parse(json_payload);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse shared attributes JSON: %s", json_payload);
        return;
    }

    app_state_t *st = app_state_get();
    bool changed = false;

    // Support both direct attribute pushes and responses wrapped in {"shared":{...}}
    cJSON *obj = cJSON_GetObjectItem(root, "shared");
    if (!obj) obj = root;

    // 1. Dynamic Temperature Unit ("F", "C", "K")
    cJSON *item_unit = cJSON_GetObjectItem(obj, "temp_unit");
    if (cJSON_IsString(item_unit) && item_unit->valuestring) {
        if (strcmp(item_unit->valuestring, "C") == 0) {
            st->temp_unit = APP_TEMP_UNIT_CELSIUS;
        } else if (strcmp(item_unit->valuestring, "K") == 0) {
            st->temp_unit = APP_TEMP_UNIT_KELVIN;
        } else {
            st->temp_unit = APP_TEMP_UNIT_FAHRENHEIT;
        }
        changed = true;
    }

    // 2. Dynamic Sleep Interval (seconds or minutes)
    cJSON *item_sleep = cJSON_GetObjectItem(obj, "sleep_interval_sec");
    if (cJSON_IsNumber(item_sleep) && item_sleep->valueint >= (int)APP_MIN_SLEEP_SEC) {
        st->sleep_interval_sec = (uint32_t)item_sleep->valueint;
        changed = true;
    } else {
        cJSON *item_sleep_min = cJSON_GetObjectItem(obj, "sleep_interval_min");
        if (cJSON_IsNumber(item_sleep_min) && item_sleep_min->valueint > 0) {
            st->sleep_interval_sec = (uint32_t)(item_sleep_min->valueint * 60);
            changed = true;
        }
    }

    // 3. Dynamic Sound / Audio Setting
    cJSON *item_sound = cJSON_GetObjectItem(obj, "sound_enabled");
    if (cJSON_IsBool(item_sound)) {
        st->sound_enabled = cJSON_IsTrue(item_sound);
        changed = true;
    }

    // 4. Dynamic Alarm Thresholds
    cJSON *thresh = cJSON_GetObjectItem(obj, "alarm_thresholds");
    if (cJSON_IsObject(thresh)) {
        cJSON *rh_lc = cJSON_GetObjectItem(thresh, "rhLowCritical");
        cJSON *rh_lw = cJSON_GetObjectItem(thresh, "rhLowWarning");
        cJSON *rh_hw = cJSON_GetObjectItem(thresh, "rhHighWarning");
        cJSON *rh_hc = cJSON_GetObjectItem(thresh, "rhHighCritical");
        cJSON *t_lc  = cJSON_GetObjectItem(thresh, "tempLowCritical");
        cJSON *t_lw  = cJSON_GetObjectItem(thresh, "tempLowWarning");
        cJSON *t_hw  = cJSON_GetObjectItem(thresh, "tempHighWarning");
        cJSON *t_hc  = cJSON_GetObjectItem(thresh, "tempHighCritical");
        cJSON *b_lc  = cJSON_GetObjectItem(thresh, "batteryLowCritical");
        cJSON *b_lw  = cJSON_GetObjectItem(thresh, "batteryLowWarning");

        if (cJSON_IsNumber(rh_lc)) st->thresholds.rh_low_critical      = (float)rh_lc->valuedouble;
        if (cJSON_IsNumber(rh_lw)) st->thresholds.rh_low_warning       = (float)rh_lw->valuedouble;
        if (cJSON_IsNumber(rh_hw)) st->thresholds.rh_high_warning      = (float)rh_hw->valuedouble;
        if (cJSON_IsNumber(rh_hc)) st->thresholds.rh_high_critical     = (float)rh_hc->valuedouble;
        if (cJSON_IsNumber(t_lc))  st->thresholds.temp_low_critical_k  = (float)t_lc->valuedouble;
        if (cJSON_IsNumber(t_lw))  st->thresholds.temp_low_warning_k   = (float)t_lw->valuedouble;
        if (cJSON_IsNumber(t_hw))  st->thresholds.temp_high_warning_k  = (float)t_hw->valuedouble;
        if (cJSON_IsNumber(t_hc))  st->thresholds.temp_high_critical_k = (float)t_hc->valuedouble;
        if (cJSON_IsNumber(b_lc))  st->thresholds.batt_low_critical    = (uint8_t)b_lc->valueint;
        if (cJSON_IsNumber(b_lw))  st->thresholds.batt_low_warning     = (uint8_t)b_lw->valueint;
        changed = true;
    }

    if (changed) {
        ESP_LOGI(TAG, "Server Attributes Synced: Sleep=%lus, TempUnit=%d, Sound=%d",
                 (unsigned long)st->sleep_interval_sec, (int)st->temp_unit, st->sound_enabled);
        app_state_save_config();
    }

    cJSON_Delete(root);
}

static void on_tb_attr_cb(const char *json_payload, void *user_data)
{
    ESP_LOGI(TAG, "Received Shared Attributes from Server: %s", json_payload);
    parse_dynamic_shared_attributes(json_payload);

    // If server attributes arrive while in claiming state, device is confirmed claimed!
    if (s_claim_active) {
        ESP_LOGI(TAG, "Initial server dataset pushed! Device successfully claimed.");
        s_claim_active = false;
        app_state_set_claimed(true);
        if (s_claim_cb) {
            s_claim_cb(true, s_claim_user_data);
        }
    }
}

static void on_tb_rpc_cb(const char *request_id, const char *method, const char *params_json, void *user_data)
{
    ESP_LOGI(TAG, "RPC Request [%s]: Method='%s', Params='%s'", request_id, method, params_json);

    if (strcmp(method, "reboot") == 0) {
        bsp_tb_send_rpc_response(request_id, "{\"result\":\"ok\"}");
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
    } else if (strcmp(method, "shutdown") == 0 || strcmp(method, "powerOff") == 0) {
        bsp_tb_send_rpc_response(request_id, "{\"result\":\"shutting_down\"}");
        vTaskDelay(pdMS_TO_TICKS(500));
        bsp_lifecycle_power_off();
    } else if (strcmp(method, "ping") == 0) {
        bsp_tb_send_rpc_response(request_id, "{\"result\":\"pong\"}");
    } else if (strcmp(method, "beep") == 0 || strcmp(method, "event") == 0) {
        bsp_trigger_chime(BSP_CHIME_EVENT);
        bsp_tb_send_rpc_response(request_id, "{\"result\":\"beeped\"}");
    } else if (strcmp(method, "notify") == 0 || strcmp(method, "chime") == 0) {
        bsp_trigger_chime(BSP_CHIME_NOTIFY);
        bsp_tb_send_rpc_response(request_id, "{\"result\":\"chimed\"}");
    } else if (strcmp(method, "alarm") == 0 || strcmp(method, "alert") == 0) {
        bsp_trigger_chime(BSP_CHIME_ALARM);
        bsp_tb_send_rpc_response(request_id, "{\"result\":\"alarmed\"}");
    } else {
        bsp_tb_send_rpc_response(request_id, "{\"error\":\"unknown method\"}");
    }
}


static char s_normalized_broker_uri[128] = {0};

static const char *normalize_broker_uri(const char *uri)
{
    if (!uri || strlen(uri) == 0) {
        uri = APP_TB_DEFAULT_BROKER_URI;
    }

    if (strstr(uri, "://") != NULL) {
        // URI already contains scheme (e.g. mqtt:// or mqtts://)
        snprintf(s_normalized_broker_uri, sizeof(s_normalized_broker_uri), "%s", uri);
    } else {
        // Bare host supplied (e.g. "humid1.com"), normalize to mqtts://host:8883
        snprintf(s_normalized_broker_uri, sizeof(s_normalized_broker_uri), "mqtts://%s:%u", uri, APP_TB_PORT);
    }

    return s_normalized_broker_uri;
}

esp_err_t tb_client_init(const char *broker_uri, const char *access_token)
{
    const char *final_uri = normalize_broker_uri(broker_uri ? broker_uri : APP_TB_HOST);

    ESP_LOGI(TAG, "Initializing ThingsBoard Client -> Broker: '%s', Token: '%s', ProvKey: '%s'",
             final_uri,
             access_token ? access_token : "(none/claiming)",
             APP_TB_PROVISION_KEY);

    bsp_tb_config_t cfg = {
        .broker_uri   = final_uri,
        .access_token = access_token,
        .ca_cert_pem  = NULL, // uses system cert bundle
        .rpc_cb       = on_tb_rpc_cb,
        .attr_cb      = on_tb_attr_cb,
        .alarm_cb     = NULL,
        .ota_cb       = on_tb_ota_cb,
        .user_data    = NULL
    };

    return bsp_tb_init(&cfg);
}

esp_err_t tb_client_wait_connected(uint32_t timeout_ms)
{
    return bsp_tb_wait_connected(timeout_ms);
}

bool tb_client_is_connected(void)
{
    return bsp_tb_is_connected();
}

static void claim_monitor_task(void *pvParameters)
{
    uint32_t duration_sec = (uint32_t)(uintptr_t)pvParameters;
    app_state_t *st = app_state_get();

    for (uint32_t elapsed = 0; elapsed < duration_sec && s_claim_active; elapsed += 5) {
        uint32_t remaining = duration_sec - elapsed;
        ui_show_claiming_screen(st->device_name, st->claim_key, remaining);

        // Request shared attributes to poll claiming acceptance from rule chain
        bsp_tb_request_shared_attributes();
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    if (s_claim_active) {
        ESP_LOGW(TAG, "Claiming window expired!");
        s_claim_active = false;
        ui_show_message("CLAIM EXPIRED", "Please Restart to Retry");
        if (s_claim_cb) {
            s_claim_cb(false, s_claim_user_data);
        }
    }

    s_claim_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t tb_client_start_claiming(uint32_t duration_ms, app_claim_complete_cb_t cb, void *user_data)
{
    s_claim_cb        = cb;
    s_claim_user_data = user_data;
    s_claim_active    = true;

    app_state_t *st = app_state_get();

    // Auto-generate 6-character claim token via BSP
    esp_err_t ret = bsp_tb_claim_device_auto(st->claim_key, sizeof(st->claim_key), duration_ms);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to publish claiming token: %s", esp_err_to_name(ret));
        s_claim_active = false;
        return ret;
    }

    ESP_LOGI(TAG, "Device Claiming Token Published: %s", st->claim_key);

    ui_show_claiming_screen(st->device_name, st->claim_key, duration_ms / 1000);

    xTaskCreatePinnedToCore(claim_monitor_task, "tb_claim_task", 4096, (void *)(uintptr_t)(duration_ms / 1000), 4, &s_claim_task, 0);

    return ESP_OK;
}

esp_err_t tb_client_send_telemetry(const app_telemetry_data_t *telemetry, uint32_t timeout_ms)
{
    if (!telemetry) return ESP_ERR_INVALID_ARG;

    return bsp_tb_send_telemetry_sync(telemetry->temp_k, telemetry->rh_pct,
                                      telemetry->battery_pct, telemetry->rssi_dbm, timeout_ms);
}

esp_err_t tb_client_sync_attributes(void)
{
    bsp_tb_report_client_attributes();
    bsp_tb_request_shared_attributes();
    return ESP_OK;
}

esp_err_t tb_client_disconnect(void)
{
    s_claim_active = false;
    return bsp_tb_disconnect();
}
