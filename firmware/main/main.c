/**
 * @file main.c
 * @brief HUMID1-OS Application Entry Point & H1OS v2.4.4-dev Lifecycle Integration
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#include "app_config.h"
#include "screens/screen_dashboard.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "h1os/h1os.h"
#include "h1os/h1os_lvgl.h"
#include "h1os/h1os_time.h"
#include "h1os/h1os_version.h"

#include <inttypes.h>
#include <stdio.h>
#include <time.h>

static const char *TAG = "humid1_app";

/* Persistent cycle counter across deep sleep in RTC Fast/Slow memory */
RTC_DATA_ATTR static uint32_t s_cycle_counter = 0;

static void update_display(void)
{
    h1os_telemetry_t telem = {0};
    esp_err_t err = h1os_get_telemetry(&telem);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Telemetry Snapshot: Temp=%.1f°F (%.1f°C), RH=%.1f%%, Batt=%u%% (%u mV)",
                 (double)telem.ambient_temp_f,
                 (double)telem.ambient_temp_c,
                 (double)telem.ambient_rh_pct,
                 telem.battery_percent,
                 telem.battery_mv);
    } else {
        ESP_LOGW(TAG, "h1os_get_telemetry returned error: %s", esp_err_to_name(err));
    }

    screen_dashboard_render(&telem, s_cycle_counter);
}

static void app_on_boot(void *user_data)
{
    (void)user_data;
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "HUMID1-OS Application %s", APP_VERSION_STR);
    ESP_LOGI(TAG, "Underlying Middleware: H1OS %s", h1os_version_get_string());
    ESP_LOGI(TAG, "Self-Hosted Cloud Target: %s", APP_TB_DEFAULT_BROKER_URI);
    ESP_LOGI(TAG, "=================================================");
}

static void app_on_wake(const h1os_wake_context_t *ctx, void *user_data)
{
    (void)user_data;
    s_cycle_counter++;

    ESP_LOGI(TAG, "Lifecycle: WAKE hook (Cycle #%" PRIu32 ", Cause bitmap=0x%08" PRIx32 ", Button=%u)",
             s_cycle_counter,
             ctx ? ctx->wakeup_causes : 0,
             ctx ? ctx->wake_button : 0);

    /* Update the primary humidor dashboard display */
    update_display();
}

static void app_on_pre_sleep(uint32_t sleep_sec, void *user_data)
{
    (void)user_data;
    ESP_LOGI(TAG, "Lifecycle: PRE-SLEEP hook (Interval: %" PRIu32 " s)", sleep_sec);
}

static void app_on_shutdown(void *user_data)
{
    (void)user_data;
    ESP_LOGI(TAG, "Lifecycle: SHUTDOWN hook executing");
}

void app_main(void)
{
    h1os_lifecycle_t lifecycle = {
        .session =
            {
                .inactivity_timeout_sec = 30,
                .clk_mode               = SYS_CLK_MODE_DYNAMIC,
                .flow_a_period_sec      = 60,
                .flow_b_period_sec      = APP_DEFAULT_SLEEP_SEC,
            },

        .presentation =
            {
                .chime_mask = (1 << H1OS_CHIME_BOOT) | (1 << H1OS_CHIME_SHUTDOWN),
                .on_chime   = NULL,
                .on_splash  = NULL,
                .volume_pct = 40,
            },

        .on_boot      = app_on_boot,
        .on_wake      = app_on_wake,
        .on_pre_sleep = app_on_pre_sleep,
        .on_shutdown  = app_on_shutdown,
        .user_data    = NULL,
    };

    esp_err_t err = h1os_start(&lifecycle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "h1os_start failed: %s", esp_err_to_name(err));
    }
}
