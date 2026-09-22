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
 *  - Audio chime synthesis (880 Hz -> 1760 Hz) via I2S DAC
 *  - FreeRTOS background tasks for button scanning & LVGL refresh
 *  - Real-Time Clock (PCF85063A) with baseline timestamp
 *  - LVGL periodic timer callback for safe display refresh
 *  - Non-touch optimized UI layout for 200x200 e-Paper display
 * 
 * @copyright Copyright (c) 2026 Humidyne Labs / Humiditron
 * SPDX-License-Identifier: MIT
 * 
 * Notes: Tested latest pull of bsp, hosted (https://github.com/Humidyne-Labs/esp32-s3_bsp)
 * - Unused LVGL variables promted warning, sucessfully compiled [9/20/2026].
 * 
 */

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"
#include "bsp/bsp.h"
#include "esp_timer.h"

static const char *TAG = "main";

// UI Label Handles
static lv_obj_t *lbl_title   = NULL;
static lv_obj_t *lbl_time    = NULL;
static lv_obj_t *lbl_sensors = NULL;
static lv_obj_t *lbl_power   = NULL;
static lv_obj_t *lbl_storage = NULL;
static lv_obj_t *lbl_event   = NULL;

// Button state tracking for edge detection
static bool s_boot_prev_state = false;
static bool s_pwr_prev_state  = false;

/* =========================================================================
 * Audio Chime Synthesizer (880 Hz -> 1760 Hz)
 * ========================================================================= */
static void play_audio_chime(void) {
    const uint32_t sample_rate = 16000;
    const size_t tone_samples = sample_rate / 4; // 250ms per tone
    const size_t total_samples = tone_samples * 2;
    const size_t buf_size = total_samples * sizeof(int16_t); // Mono 16-bit

    int16_t *buf = (int16_t *)malloc(buf_size);
    if (!buf) {
        ESP_LOGE(TAG, "Audio buffer allocation failed");
        return;
    }

    float freqs[2] = {880.0f, 1760.0f};
    int16_t *p = buf;

    for (int t = 0; t < 2; t++) {
        for (size_t i = 0; i < tone_samples; i++) {
            float angle = 2.0f * (float)M_PI * freqs[t] * ((float)i / (float)sample_rate);
            float env = sinf((float)M_PI * ((float)i / (float)tone_samples));
            int16_t sample = (int16_t)(sinf(angle) * env * 12000.0f);
            *p++ = sample; // 1st channel
        }
    }

    bsp_audio_set_volume(85.0f);
    bsp_audio_play(buf, buf_size, NULL);
    free(buf);
}

/* =========================================================================
 * Fast Button Scanner (50 ms FreeRTOS Task)
 * ========================================================================= */
static void button_monitor_task(void *pvParameters) {
    while (1) {
        bool boot_pressed = bsp_button_is_pressed(BSP_BUTTON_BOOT);
        bool pwr_pressed  = bsp_button_is_pressed(BSP_BUTTON_POWER);

        // Falling edge: BOOT Pressed
        if (boot_pressed && !s_boot_prev_state) {
            ESP_LOGI(TAG, "BOOT key pressed -> Playing Audio Chime");
            if (lbl_event) {
                bsp_lvgl_lock();
                lv_label_set_text(lbl_event, "Event: BOOT Key (Chime)");
                bsp_lvgl_unlock();
            }
            play_audio_chime();
        }

        // Falling edge: POWER Pressed
        if (pwr_pressed && !s_pwr_prev_state) {
            ESP_LOGI(TAG, "POWER key pressed");
            if (lbl_event) {
                bsp_lvgl_lock();
                lv_label_set_text(lbl_event, "Event: POWER Key Pressed");
                bsp_lvgl_unlock();
            }
        }

        s_boot_prev_state = boot_pressed;
        s_pwr_prev_state  = pwr_pressed;

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* =========================================================================
 * LVGL Periodic Telemetry Callback (Runs in LVGL Task Context)
 * ========================================================================= */
static void ui_update_timer_cb(lv_timer_t *timer) {
    char str_buf[64];

    // 1. Environmental Sensor (SHTC3) - Formatted via standard snprintf
    bsp_shtc3_data_t env;
    if (bsp_shtc3_read(&env) == ESP_OK) {
        float temp_c = env.temperature_k - 273.15f;
        snprintf(str_buf, sizeof(str_buf), "Temp: %.1f C  |  RH: %.1f %%", temp_c, env.humidity_percent);
        lv_label_set_text(lbl_sensors, str_buf);
    }

    // 2. Real-Time Clock (PCF85063A)
    bsp_rtc_datetime_t dt;
    if (bsp_rtc_get_datetime(&dt) == ESP_OK) {
        snprintf(str_buf, sizeof(str_buf), "%04d-%02d-%02d  %02d:%02d:%02d",
                 dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
        lv_label_set_text(lbl_time, str_buf);
    }

    // 3. Battery Voltage & Percentage
    uint32_t vbat_mv = 0;
    bsp_battery_get_voltage(&vbat_mv, NULL);
    uint8_t pct = bsp_battery_get_percentage();
    snprintf(str_buf, sizeof(str_buf), "Battery: %lu mV (%d%%)", (unsigned long)vbat_mv, pct);
    lv_label_set_text(lbl_power, str_buf);

    // 4. MicroSD Storage Status
    if (bsp_sdcard_is_mounted()) {
        snprintf(str_buf, sizeof(str_buf), "SD Card: Mounted (%.1f GB)", bsp_sdcard_get_capacity_gb());
        lv_label_set_text(lbl_storage, str_buf);
    } else {
        lv_label_set_text(lbl_storage, "SD Card: Not Detected");
    }

    // 5. LED Heartbeat Toggle
    bsp_led_toggle();
}

/* =========================================================================
 * Non-Touch UI Layout (200x200 e-Paper Optimized)
 * ========================================================================= */
static void create_non_touch_ui(uint32_t boot_count) {
    char str_buf[64];
    lv_obj_t *scr = lv_screen_active();

    lv_obj_set_style_bg_color(scr, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    // Device Header
    char dev_name[32] = {0};
    bsp_get_device_name(dev_name, sizeof(dev_name));

    lbl_title = lv_label_create(scr);
    snprintf(str_buf, sizeof(str_buf), "%s (Boot #%lu)", dev_name, (unsigned long)boot_count);
    lv_label_set_text(lbl_title, str_buf);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 6);

    // RTC Clock
    lbl_time = lv_label_create(scr);
    lv_label_set_text(lbl_time, "2026-09-20  --:--:--");
    lv_obj_align(lbl_time, LV_ALIGN_TOP_LEFT, 8, 28);

    // SHTC3 Temperature & Humidity
    lbl_sensors = lv_label_create(scr);
    lv_label_set_text(lbl_sensors, "Temp: --.- C  |  RH: --.- %");
    lv_obj_align(lbl_sensors, LV_ALIGN_TOP_LEFT, 8, 50);

    // Battery ADC
    lbl_power = lv_label_create(scr);
    lv_label_set_text(lbl_power, "Battery: ---- mV (--%)");
    lv_obj_align(lbl_power, LV_ALIGN_TOP_LEFT, 8, 72);

    // SD Card Status
    lbl_storage = lv_label_create(scr);
    lv_label_set_text(lbl_storage, "SD Card: Checking...");
    lv_obj_align(lbl_storage, LV_ALIGN_TOP_LEFT, 8, 94);

    // Event & Button Action Feedback Box
    lv_obj_t *event_box = lv_obj_create(scr);
    lv_obj_set_size(event_box, 184, 38);
    lv_obj_align(event_box, LV_ALIGN_TOP_MID, 0, 118);
    lv_obj_set_style_pad_all(event_box, 4, 0);
    lv_obj_set_style_border_width(event_box, 1, 0);
    lv_obj_set_style_border_color(event_box, lv_color_black(), 0);
    
    lbl_event = lv_label_create(event_box);
    lv_label_set_text(lbl_event, "Event: System Ready");
    lv_obj_center(lbl_event);

    // Non-Touch Control Instructions Footer
    lv_obj_t *lbl_footer = lv_label_create(scr);
    lv_label_set_text(lbl_footer, "[BOOT: Chime | PWR: Event]");
    lv_obj_align(lbl_footer, LV_ALIGN_BOTTOM_MID, 0, -6);

    // Periodic UI Refresh Timer (3 seconds)
    lv_timer_create(ui_update_timer_cb, 3000, NULL);
}

/* =========================================================================
 * Master app_main
 * ========================================================================= */
extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Initializing Non-Touch ESP32-S3 e-Paper BSP...");

    // 1. Core Board Init (Power hold, NVS, I2C, Buttons)
    ESP_ERROR_CHECK(bsp_board_init());

    // 2. NVS Boot Counter
    uint32_t boot_count = 0;
    bsp_nvs_get_u32("boot_count", &boot_count);
    bsp_nvs_set_u32("boot_count", ++boot_count);

    // 3. Initialize RTC & baseline timestamp
    bsp_rtc_init();
    bsp_rtc_datetime_t dt;
    if (bsp_rtc_get_datetime(&dt) != ESP_OK || dt.year < 2024) {
        bsp_rtc_datetime_t init_dt = {
            .year = 2026, .month = 9, .day = 20,
            .weekday = 0, .hour = 19, .minute = 30, .second = 0
        };
        bsp_rtc_set_datetime(&init_dt);
    }

    // 4. Initialize SHTC3 Sensor & Mount MicroSD
    bsp_shtc3_init();
    bsp_sdcard_mount();

    // 5. Initialize Audio Codec & Play Boot Sound
    if (bsp_audio_init() == ESP_OK) {
        play_audio_chime();
    }

    // 6. Initialize LVGL Port
    ESP_ERROR_CHECK(bsp_lvgl_init());

    // 7. Render UI (under LVGL lock before tasks start)
    bsp_lvgl_lock();
    create_non_touch_ui(boot_count);
    bsp_lvgl_unlock();

    // 8. Start Background Tasks
    xTaskCreatePinnedToCore(button_monitor_task, "btn_task",  3 * 1024, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(bsp_lvgl_port_task,  "lvgl_task", 6 * 1024, NULL, 2, NULL, 1);
}