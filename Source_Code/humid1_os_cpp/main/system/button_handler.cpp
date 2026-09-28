/**
 * @file button_handler.cpp
 * @brief Tactile Button Event Dispatcher & Factory Reset Manager Implementation
 * 
 * @attribution
 * - Humidyne Labs / Humiditron (2026)
 * 
 * SPDX-License-Identifier: MIT
 */

#include "button_handler.h"
#include "app_config.h"
#include "app_state.h"
#include "esp_log.h"
#include "esp_system.h"
#include "bsp/bsp.h"

static const char *TAG = "button_handler";

static app_factory_reset_cb_t s_reset_cb    = NULL;
static void                   *s_user_data  = NULL;

static void on_boot_button_event(bsp_button_t btn, bsp_button_event_t evt, void *user_data)
{
    if (btn != BSP_BUTTON_BOOT) return;

    if (evt == BSP_BUTTON_EVENT_LONG_PRESS) {
        ESP_LOGW(TAG, "BOOT Button Long Press Detected (>3s)! Triggering Factory Wipe...");

        // Play alert chime
        bsp_trigger_chime(BSP_CHIME_ALARM);

        // Wipe NVS and fast cache
        app_state_factory_wipe();

        if (s_reset_cb) {
            s_reset_cb(s_user_data);
        } else {
            ESP_LOGI(TAG, "Rebooting into Provisioning Mode...");
            esp_restart();
        }
    }
}

esp_err_t button_handler_init(app_factory_reset_cb_t reset_cb, void *user_data)
{
    s_reset_cb  = reset_cb;
    s_user_data = user_data;

    // Button interrupts and debouncing are already started by BSP during bsp_board_init_with_config.
    // Register long-press action handler for factory reset.
    bsp_button_register_cb(BSP_BUTTON_BOOT, BSP_BUTTON_EVENT_LONG_PRESS, on_boot_button_event, NULL);
    ESP_LOGI(TAG, "Button handler registered: BOOT long press (>%d ms) -> Factory Reset", APP_BUTTON_LONG_PRESS_MS);

    return ESP_OK;
}
