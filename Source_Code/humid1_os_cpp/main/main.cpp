/**
 * @file main.cpp
 * @brief ESP32-S3 ePaper HUMID1-OS User Application
 * 
 * Hardware Target: Waveshare ESP32-S3 ePaper 1.54" V2 (200x200 1-bit Mono EPD)
 * 
 * Architecture & Flow:
 *  - Native BSP v1.4.0 lifecycle integration (bsp_app_start, bsp_lifecycle_get_stage/set_stage)
 *  - Single master include manifest (bsp/bsp.h)
 *  - Automatic peripheral & chime dispatching handled by BSP
 *  - Clean Shutdown Lifecycle Hook (on_shutdown) & Low-Power Stand-down
 *  - Automatic PCF85063A RTC <-> POSIX time synchronization with periodic 24h SNTP updates
 *  - Dual-slot OTA image validation guard (bsp_ota_mark_valid)
 *  - Reactive server-driven dataset & ThingsBoard Rule Chain integration
 *  - BLE GATT Provisioning with concurrent QR Code & PoP PIN display
 *  - Factory Reset via BOOT button (>3s long press)
 *  - Auto-generated 6-character Device Claiming workflow
 * 
 * @attribution
 * - Humidyne Labs / Humiditron (2026)
 * 
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "bsp/bsp.h"
#include "app_config.h"
#include "app_state.h"
#include "system/button_handler.h"
#include "telemetry/sensor_reader.h"
#include "network/prov_manager.h"
#include "cloud/tb_client.h"
#include "ui/ui_manager.h"

static const char *TAG = "humid1_main";

// Forward declarations
static void on_prov_success(void *user_data);
static void on_claim_complete(bool success, void *user_data);
static void run_telemetry_cycle(void);

static void on_factory_reset(void *user_data)
{
    ESP_LOGW(TAG, "Factory Reset Triggered -> Cleared NVS & Rebooting");
    bsp_lifecycle_set_stage(APP_STAGE_PROVISIONING);
    bsp_trigger_chime(BSP_CHIME_NOTIFY);
    ui_show_message("FACTORY RESET", "Credentials Cleared\nRebooting...");
    vTaskDelay(pdMS_TO_TICKS(1500));
    esp_restart();
}

static void on_prov_success(void *user_data)
{
    ESP_LOGI(TAG, "Wi-Fi Provisioned successfully! Advancing lifecycle stage...");
    prov_manager_stop();
    bsp_trigger_chime(BSP_CHIME_NOTIFY);

    app_state_t *st = app_state_get();
    if (!st->is_claimed) {
        bsp_lifecycle_set_stage(APP_STAGE_CLAIMING);
        ESP_LOGI(TAG, "Device unclaimed. Starting ThingsBoard Claiming...");
        esp_err_t ret = tb_client_init(APP_TB_DEFAULT_BROKER_URI, NULL);
        if (ret == ESP_OK && tb_client_wait_connected(APP_TB_CONNECT_TIMEOUT_MS) == ESP_OK) {
            tb_client_start_claiming(APP_TB_CLAIM_DURATION_MS, on_claim_complete, NULL);
            return;
        }
    } else {
        bsp_lifecycle_set_stage(APP_STAGE_RUN_LOOP);
    }

    run_telemetry_cycle();
}

static void on_claim_complete(bool success, void *user_data)
{
    if (success) {
        ESP_LOGI(TAG, "Device claimed successfully in ThingsBoard!");
        bsp_lifecycle_set_stage(APP_STAGE_RUN_LOOP);
        bsp_trigger_chime(BSP_CHIME_NOTIFY);
        ui_show_message("CLAIM SUCCESS", "Device Linked to Dashboard!");
        vTaskDelay(pdMS_TO_TICKS(1500));
        run_telemetry_cycle();
    } else {
        ESP_LOGW(TAG, "Device claim expired or failed.");
        bsp_trigger_chime(BSP_CHIME_ALARM);
        ui_show_message("CLAIM TIMEOUT", "Hold BOOT to Reset\nor Power Cycle to Retry");
    }
}

static bool evaluate_alarms(const app_telemetry_data_t *telem, const app_alarm_thresholds_t *th)
{
    if (!telem || !th) return false;

    // Relative Humidity limits
    if (telem->rh_pct < th->rh_low_critical || telem->rh_pct > th->rh_high_critical) {
        return true;
    }
    // Canonical Kelvin Temperature limits
    if (telem->temp_k < th->temp_low_critical_k || telem->temp_k > th->temp_high_critical_k) {
        return true;
    }
    // Battery limits
    if (telem->battery_pct <= th->batt_low_critical) {
        return true;
    }

    return false;
}

static void check_and_sync_sntp_periodic(void)
{
    app_rtc_state_t rtc_state = {};
    bsp_lifecycle_load_state(&rtc_state, sizeof(rtc_state));

    time_t now = 0;
    time(&now);

    bool need_sntp = false;
    if (rtc_state.last_sntp_sync_ts == 0) {
        need_sntp = true;
    } else if (now > (time_t)rtc_state.last_sntp_sync_ts && 
               (now - (time_t)rtc_state.last_sntp_sync_ts) >= APP_SNTP_SYNC_INTERVAL_SEC) {
        need_sntp = true;
    }

    if (need_sntp) {
        ESP_LOGI(TAG, "Periodic SNTP Sync Window (Every 24h) -> Syncing time via NTP...");
        if (bsp_time_sntp_sync(3000) == ESP_OK) {
            time(&now);
            rtc_state.last_sntp_sync_ts = (uint32_t)now;
            rtc_state.wake_cycle_count++;
            bsp_lifecycle_save_state(&rtc_state, sizeof(rtc_state));
            ESP_LOGI(TAG, "SNTP Sync complete, hardware RTC updated");
        } else {
            ESP_LOGW(TAG, "SNTP sync timed out, relying on hardware RTC");
        }
    } else {
        rtc_state.wake_cycle_count++;
        bsp_lifecycle_save_state(&rtc_state, sizeof(rtc_state));
        ESP_LOGD(TAG, "Skipping SNTP sync (Hardware RTC is source of truth; cycle=%lu)",
                 (unsigned long)rtc_state.wake_cycle_count);
    }
}

static void run_telemetry_cycle(void)
{
    app_state_t *st = app_state_get();
    ESP_LOGI(TAG, "Starting Active Telemetry Cycle...");

    // 1. Sample Sensor & Battery Readings
    app_telemetry_data_t telem;
    sensor_reader_sample(&telem);

    // 2. Connect Wi-Fi (Uses <400ms fast RTC cache)
    if (!bsp_wifi_is_connected()) {
        esp_err_t ret = bsp_wifi_connect_from_nvs(APP_WIFI_FAST_TIMEOUT_MS);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Fast Wi-Fi connect failed, retrying with standard timeout...");
            ret = bsp_wifi_connect_from_nvs(APP_WIFI_CONNECT_TIMEOUT_MS);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Wi-Fi connection failed! Sleeping until next wake...");
                // Still update display with latest sensor data
                ui_show_dashboard_screen(&telem, st->temp_unit, false);

                // Enter sleep
                bsp_sleep_config_t sleep_cfg = {
                    .mode           = BSP_SLEEP_MODE_DEEP,
                    .duration_sec   = st->sleep_interval_sec,
                    .wake_sources   = BSP_WAKE_SRC_ALL,
                    .next_init_mode = BSP_INIT_MODE_FAST
                };
                bsp_lifecycle_enter_sleep(&sleep_cfg);
                return;
            }
        }
    }

    // Refresh RSSI with active connection
    sensor_reader_sample(&telem);

    // 3. Periodic SNTP Network Time Synchronization (24-hour interval)
    check_and_sync_sntp_periodic();

    // 4. Connect ThingsBoard & Publish Synchronous QoS 1 Telemetry
    esp_err_t ret = tb_client_init(APP_TB_DEFAULT_BROKER_URI, NULL);
    if (ret == ESP_OK && tb_client_wait_connected(APP_TB_CONNECT_TIMEOUT_MS) == ESP_OK) {
        // Synchronous QoS 1 delivery (guarantees delivery before sleep)
        tb_client_send_telemetry(&telem, APP_TB_TELEMETRY_TIMEOUT_MS);

        // Sync client & shared attributes (receives dynamic dataset from server)
        tb_client_sync_attributes();

        // Brief delay for any incoming OTA / server attribute dispatch
        vTaskDelay(pdMS_TO_TICKS(500));
        tb_client_disconnect();
    } else {
        ESP_LOGW(TAG, "ThingsBoard connection timed out, continuing...");
    }

    // Disconnect Wi-Fi before sleep to conserve battery
    bsp_wifi_disconnect();

    // 5. Evaluate Server Thresholds & Update ePaper Display
    bool alarm_active = evaluate_alarms(&telem, &st->thresholds);
    if (alarm_active && st->sound_enabled) {
        ESP_LOGW(TAG, "Critical Alarm Active! Triggering Alarm Chime Warble...");
        bsp_trigger_chime(BSP_CHIME_ALARM);
    }

    ui_show_dashboard_screen(&telem, st->temp_unit, alarm_active);

    // 6. Enter Deep Sleep using BSP Lifecycle Engine
    uint32_t sleep_sec = st->sleep_interval_sec;
    if (sleep_sec < APP_MIN_SLEEP_SEC) sleep_sec = APP_MIN_SLEEP_SEC;
    if (sleep_sec > APP_MAX_SLEEP_SEC) sleep_sec = APP_MAX_SLEEP_SEC;

    ESP_LOGI(TAG, "Scheduling Deep Sleep for %lu seconds via BSP...", (unsigned long)sleep_sec);
    bsp_sleep_config_t sleep_cfg = {
        .mode           = BSP_SLEEP_MODE_DEEP,
        .duration_sec   = sleep_sec,
        .wake_sources   = BSP_WAKE_SRC_ALL,
        .next_init_mode = BSP_INIT_MODE_FAST
    };

    bsp_lifecycle_enter_sleep(&sleep_cfg);
}

static void app_on_cold_boot(void *user_data)
{
    ESP_LOGI(TAG, "=== Initializing Cold Boot Lifecycle ===");

    // Register button handler for BOOT long-press factory wipe
    button_handler_init(on_factory_reset, NULL);

    // Initialize application state
    app_state_init();
    app_state_t *st = app_state_get();

    // Apply audio volume setting
    if (!st->sound_enabled) {
        bsp_audio_set_volume(0.0f);
    }

    // Evaluate Provisioning Status
    if (!st->is_provisioned) {
        ESP_LOGI(TAG, "No Wi-Fi credentials -> Starting BLE Provisioning");
        bsp_lifecycle_set_stage(APP_STAGE_PROVISIONING);
        prov_manager_start(on_prov_success, NULL);
        return; // Stays awake during provisioning
    }

    // Attempt Wi-Fi Connection
    esp_err_t ret = bsp_wifi_connect_from_nvs(APP_WIFI_CONNECT_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to connect Wi-Fi! Falling back to Provisioning...");
        bsp_lifecycle_set_stage(APP_STAGE_PROVISIONING);
        prov_manager_start(on_prov_success, NULL);
        return;
    }

    // Evaluate Claiming Status
    if (!st->is_claimed) {
        ESP_LOGI(TAG, "Device unclaimed -> Starting ThingsBoard Claiming Flow");
        bsp_lifecycle_set_stage(APP_STAGE_CLAIMING);
        ret = tb_client_init(APP_TB_DEFAULT_BROKER_URI, NULL);
        if (ret == ESP_OK && tb_client_wait_connected(APP_TB_CONNECT_TIMEOUT_MS) == ESP_OK) {
            tb_client_start_claiming(APP_TB_CLAIM_DURATION_MS, on_claim_complete, NULL);
            return; // Stays awake during claiming window
        } else {
            ESP_LOGE(TAG, "Failed to connect to ThingsBoard for claiming!");
            ui_show_message("SERVER ERROR", "Cannot Connect to ThingsBoard\nRetrying shortly...");
        }
    }

    // Advance to active telemetry run loop
    bsp_lifecycle_set_stage(APP_STAGE_RUN_LOOP);
    run_telemetry_cycle();
}

static void app_on_wake(const bsp_wake_context_t *ctx, void *user_data)
{
    ESP_LOGI(TAG, "=== Waking from Sleep (Cycle: %lu, Stage: %u) ===",
             (unsigned long)(ctx ? ctx->deep_sleep_count : 0),
             ctx ? ctx->app_stage : 0);

    // Register button handler
    button_handler_init(on_factory_reset, NULL);

    // Load state
    app_state_init();
    app_state_t *st = app_state_get();

    // Apply audio volume setting
    if (!st->sound_enabled) {
        bsp_audio_set_volume(0.0f);
    }

    uint8_t stage = ctx ? ctx->app_stage : bsp_lifecycle_get_stage();

    // Check if device needs provisioning
    if (stage == APP_STAGE_PROVISIONING || !st->is_provisioned) {
        bsp_lifecycle_set_stage(APP_STAGE_PROVISIONING);
        prov_manager_start(on_prov_success, NULL);
        return;
    }

    // Check if device is in claiming flow
    if (stage == APP_STAGE_CLAIMING || !st->is_claimed) {
        bsp_lifecycle_set_stage(APP_STAGE_CLAIMING);
        esp_err_t ret = bsp_wifi_connect_from_nvs(APP_WIFI_CONNECT_TIMEOUT_MS);
        if (ret == ESP_OK) {
            ret = tb_client_init(APP_TB_DEFAULT_BROKER_URI, NULL);
            if (ret == ESP_OK && tb_client_wait_connected(APP_TB_CONNECT_TIMEOUT_MS) == ESP_OK) {
                tb_client_start_claiming(APP_TB_CLAIM_DURATION_MS, on_claim_complete, NULL);
                return;
            }
        }
    }

    // Normal active telemetry cycle
    bsp_lifecycle_set_stage(APP_STAGE_RUN_LOOP);
    run_telemetry_cycle();
}

static void app_on_before_sleep(bsp_sleep_mode_t mode, uint32_t duration_sec, void *user_data)
{
    ESP_LOGI(TAG, "App Stand-down Hook: Entering %s for %lu seconds",
             (mode == BSP_SLEEP_MODE_DEEP) ? "Deep Sleep" : "Light Sleep",
             (unsigned long)duration_sec);
    tb_client_disconnect();
    bsp_wifi_disconnect();
}

static void app_on_shutdown(void *user_data)
{
    ESP_LOGW(TAG, "=== Application Shutdown Hook Triggered ===");
    bsp_lifecycle_set_stage(APP_STAGE_SHUTDOWN);

    // Teardown networks
    tb_client_disconnect();
    bsp_wifi_disconnect();

    // Play shutdown chime
    bsp_trigger_chime(BSP_CHIME_SHUTDOWN);

    // Display Space Cat image so it persists on the bi-stable e-Paper in zero-power state
    ui_show_shutdown_screen();
}


static const bsp_app_lifecycle_t s_app_lifecycle = {
    .on_cold_boot    = app_on_cold_boot,
    .on_wake         = app_on_wake,
    .on_before_sleep = app_on_before_sleep,
    .on_shutdown     = app_on_shutdown,
    .user_data       = NULL,
};

extern "C" void app_main(void)
{
    const esp_app_desc_t *app_desc = bsp_ota_get_app_desc();
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "  HUMID1-OS User Application v%s (%s)", 
             app_desc ? app_desc->version : bsp_get_version(),
             app_desc ? app_desc->project_name : "humid1-os");
    ESP_LOGI(TAG, "  Target: Waveshare ESP32-S3 Touch-ePaper-1.54 V2");
    ESP_LOGI(TAG, "==================================================");

    // 1. Confirm running firmware image is valid (OTA rollback guard)
    bsp_ota_mark_valid();

    // 2. Start BSP Application Engine with Lifecycle Hooks
    bsp_app_start(&s_app_lifecycle);
}
