/**
 * @file main.cpp
 * @brief ESP32-S3 Touch ePaper BSP - Comprehensive Demonstration Application
 * 
 * Demonstrates:
 *  - Board master initialization & Power hold
 *  - Hardware eFuse Unique Device ID & BLE Advertising Name
 *  - NVS persistent parameter storage (Wi-Fi SSID/Passkey)
 *  - SHTC3 sensor readings in Kelvin & Celsius
 *  - Battery ADC voltage & percentage
 *  - MicroSD Card VFS mounting
 *  - LVGL v9 e-Paper GUI & Touch panel interaction
 * 
 * @copyright Copyright (c) 2026 Humidyne Labs / Humiditron
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bsp/bsp.h"
#include "bsp/bsp_audio.h"

static const char *TAG = "main_app";

static void audio_test_task(void *arg)
{
    ESP_LOGI(TAG, "Initializing BSP audio hardware...");
    ESP_ERROR_CHECK(bsp_audio_init());
    ESP_ERROR_CHECK(bsp_audio_set_volume(100.0f));

    const uint32_t sample_rate = 16000;
    const uint32_t tone_hz = 1000;
    const size_t samples_per_channel = sample_rate / 10; // 100 ms tone
    static int16_t tone[samples_per_channel * 2]; // stereo interleaved: L,R,L,R,...
    constexpr float kPi = 3.14159265358979323846f;

    for (size_t i = 0; i < samples_per_channel; ++i) {
        const float time_s = static_cast<float>(i) / static_cast<float>(sample_rate);
        const float sample = 0.25f * sinf(2.0f * kPi * static_cast<float>(tone_hz) * time_s);
        const int16_t value = static_cast<int16_t>(sample * 32767.0f);
        tone[i * 2 + 0] = value; // left
        tone[i * 2 + 1] = value; // right
    }

    size_t bytes_written = 0;
    ESP_LOGI(TAG, "Playing 100 ms 1 kHz stereo test tone...");
    esp_err_t ret = bsp_audio_play(tone, sizeof(tone), &bytes_written);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Audio test complete: %zu bytes written", bytes_written);
    } else {
        ESP_LOGE(TAG, "Audio test failed: %s", esp_err_to_name(ret));
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
    bsp_audio_pa_enable(false);
    vTaskDelete(NULL);
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting Unified BSP Demo Application...");

    /* Initialize core board hardware (Power hold, LED, NVS, shared I2C bus) */
    ESP_ERROR_CHECK(bsp_board_init());

    /* Print Hardware Unique Device ID & Default Device Name */
    char dev_id[32] = {0};
    char dev_name[32] = {0};
    if (bsp_get_device_id(dev_id, sizeof(dev_id)) == ESP_OK) {
        ESP_LOGI(TAG, "Device Hardware Unique ID: %s", dev_id);
    }
    if (bsp_get_device_name(dev_name, sizeof(dev_name)) == ESP_OK) {
        ESP_LOGI(TAG, "BLE/Device Name: %s", dev_name);
    }

    /* Demonstrate NVS Parameter Storage (Saving & Reading Provisioning Credentials) */
    char wifi_ssid[32] = {0};
    if (bsp_nvs_get_str("wifi_ssid", wifi_ssid, sizeof(wifi_ssid)) != ESP_OK) {
        ESP_LOGI(TAG, "No Wi-Fi SSID found in NVS. Saving default SSID...");
        bsp_nvs_set_str("wifi_ssid", "MyHomeWiFi");
        bsp_nvs_set_str("wifi_passkey", "SecretPasscode123");
        strncpy(wifi_ssid, "MyHomeWiFi", sizeof(wifi_ssid));
    }
    ESP_LOGI(TAG, "NVS Stored Wi-Fi SSID: %s", wifi_ssid);

    /* Initialize SHTC3 Environmental Sensor */
    bsp_shtc3_init();

    /* Initialize MicroSD Card */
    if (bsp_sdcard_mount() == ESP_OK) {
        ESP_LOGI(TAG, "MicroSD Card mounted. Capacity: %.2f GB", bsp_sdcard_get_capacity_gb());
    } else {
        ESP_LOGW(TAG, "No MicroSD Card detected or mount failed");
    }

    /* Initialize LVGL v9 GUI port (e-Paper Display + Touch Panel) */
    ESP_LOGI(TAG, "Starting LVGL initialization...");
    bool lvgl_ready = (bsp_lvgl_init() == ESP_OK);
    if (!lvgl_ready) {
        ESP_LOGE(TAG, "LVGL initialization failed");
    }

    /* Create sample LVGL UI widget */
    lv_obj_t *scr = lv_screen_active();
    lv_obj_t *label = lv_label_create(scr);
    char label_buf[64];
    snprintf(label_buf, sizeof(label_buf), "BSP Running!\n%s", dev_name);
    lv_label_set_text(label, label_buf);
    lv_obj_center(label);

    if (lvgl_ready) {
        xTaskCreate(bsp_lvgl_port_task, "lvgl_task", 4096, NULL, 5, NULL);
    }

    xTaskCreate(audio_test_task, "audio_test_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Initialization complete. Entering telemetry loop...");

    while (1) {
        /* Read battery voltage & level */
        uint32_t v_mv = 0;
        uint8_t batt_pct = bsp_battery_get_percentage();
        if (bsp_battery_get_voltage(&v_mv, NULL) == ESP_OK) {
            ESP_LOGI(TAG, "Battery: %lu mV (%u%%)", v_mv, batt_pct);
        }

        /* Read SHTC3 Sensor (Kelvin, Celsius & Humidity) */
        bsp_shtc3_data_t sensor_data;
        if (bsp_shtc3_read(&sensor_data) == ESP_OK) {
            float temperature_c = sensor_data.temperature_k - 273.15f;
            float temperature_f = temperature_c * 9.0f / 5.0f + 32.0f;
            ESP_LOGI(TAG, "Temp: %.2f K (%.2f C / %.2f F), Humidity: %.2f %%",
                     sensor_data.temperature_k, temperature_c, temperature_f,
                     sensor_data.humidity_percent);
        }

        /* Toggle status LED */
        bsp_led_toggle();

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
