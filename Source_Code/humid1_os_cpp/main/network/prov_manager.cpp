/**
 * @file prov_manager.cpp
 * @brief BLE GATT Wi-Fi Provisioning Engine Implementation
 * 
 * @attribution
 * - Humidyne Labs / Humiditron (2026)
 * 
 * SPDX-License-Identifier: MIT
 */

#include "prov_manager.h"
#include "app_config.h"
#include "app_state.h"
#include "ui/ui_manager.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp/bsp.h"


static const char *TAG = "prov_manager";

static app_prov_success_cb_t s_success_cb    = NULL;
static void                  *s_user_data    = NULL;
static char                  s_serv_name[48] = {0};

static void on_bsp_prov_event(bsp_prov_event_t event, void *event_data, void *user_data)
{
    app_state_t *st = app_state_get();

    switch (event) {
    case BSP_PROV_EVENT_STARTED:
        ESP_LOGI(TAG, "BLE Provisioning Advertising Active. Service: %s, PoP: %s", s_serv_name, st->pop_pin);
        ui_show_provisioning_screen(s_serv_name, st->pop_pin);
        break;

    case BSP_PROV_EVENT_CRED_RECEIVED:
        ESP_LOGI(TAG, "Wi-Fi credentials received. Attempting connection...");
        ui_show_message("CONNECTING", "Verifying Wi-Fi Credentials...");
        break;

    case BSP_PROV_EVENT_CRED_SUCCESS:
        ESP_LOGI(TAG, "Wi-Fi associated and verified successfully!");
        bsp_trigger_chime(BSP_CHIME_NOTIFY);
        st->is_provisioned = true;
        ui_show_message("SUCCESS", "Wi-Fi Connected Successfully!");
        vTaskDelay(pdMS_TO_TICKS(1500));
        if (s_success_cb) {
            s_success_cb(s_user_data);
        }
        break;

    case BSP_PROV_EVENT_CRED_FAILED:
        ESP_LOGE(TAG, "Wi-Fi association failed!");
        bsp_trigger_chime(BSP_CHIME_ALARM);
        ui_show_message("FAILED", "Incorrect Password / Timeout\nRe-opening Provisioning...");
        vTaskDelay(pdMS_TO_TICKS(2000));
        ui_show_provisioning_screen(s_serv_name, st->pop_pin);
        break;


    case BSP_PROV_EVENT_FINISHED:
        ESP_LOGI(TAG, "BLE Provisioning ended.");
        break;

    default:
        break;
    }
}

esp_err_t prov_manager_start(app_prov_success_cb_t success_cb, void *user_data)
{
    s_success_cb = success_cb;
    s_user_data  = user_data;

    app_state_t *st = app_state_get();
    snprintf(s_serv_name, sizeof(s_serv_name), "%s%s", APP_PROV_SERVICE_PREFIX, st->device_name);

    ESP_LOGI(TAG, "Starting BLE Provisioning Service: %s with PoP: %s", s_serv_name, st->pop_pin);

    // Initial greeting screen
    ui_show_provisioning_screen(s_serv_name, st->pop_pin);

    esp_err_t ret = bsp_prov_start(s_serv_name, st->pop_pin, on_bsp_prov_event, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start BSP provisioning: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

void prov_manager_stop(void)
{
    bsp_prov_stop();
}

bool prov_manager_is_running(void)
{
    return bsp_prov_is_running();
}
