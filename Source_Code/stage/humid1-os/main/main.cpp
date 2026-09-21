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
 */

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"
#include "bsp/bsp.h"

static const char *TAG = "main";

// UI Label Handles
static lv_obj_t *lbl_title;
static lv_obj_t *lbl_time;
static lv_obj_t *lbl_sensors;
static lv_obj_t *lbl_power;
static lv_obj_t *lbl_storage;
static lv_obj_t *lbl_event;

// Button state tracking for edge detection
static bool s_boot_prev_state = false;
static bool s_pwr_prev_state = false;

/* =========================================================================
 * Audio Chime Synthesizer (880 Hz -> 1760 Hz)
 * ========================================================================= */
static void play_audio_chime(void) {
    const uint32_t sample_rate = 16000;
    const size_t tone_samples = sample_rate / 4; // 250ms per tone
    const size_t total_samples = tone_samples * 2;
    const size_t buf_size = total_samples * 2 * sizeof(int16_t); // Stereo 16-bit

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
			// Speaker is MONO
            *p++ = sample; // Left channel
            *p++ = sample; // Right channel
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
                lv_label_set_text(lbl_event, "Event: BOOT Key (Chime)");
            }
            play_audio_chime();
        }

        // Falling edge: POWER Pressed
        if (pwr_pressed && !s_pwr_prev_state) {
            ESP_LOGI(TAG, "POWER key pressed");
            if (lbl_event) {
                lv_label_set_text(lbl_event, "Event: POWER Key Pressed");
            }
        }

        s_boot_prev_state = boot_pressed;
        s_pwr_prev_state  = pwr_pressed;

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* =========================================================================
 * LVGL Periodic Telemetry Callback (Safe Display Refresh Context)
 * ========================================================================= */
static void ui_update_timer_cb(lv_timer_t *timer) {
    // 1. Environmental Sensor (SHTC3)
    bsp_shtc3_data_t env;
    if (bsp_shtc3_read(&env) == ESP_OK) {
        lv_label_set_text_fmt(lbl_sensors, "Temp: %.1f C  |  RH: %.1f %%", 
                              env.temperature_k - 273.15f, env.humidity_percent);
    }

    // 2. Real-Time Clock (PCF85063A)
    bsp_rtc_datetime_t dt;
    if (bsp_rtc_get_datetime(&dt) == ESP_OK) {
        lv_label_set_text_fmt(lbl_time, "%04d-%02d-%02d  %02d:%02d:%02d",
                              dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
    }

    // 3. Battery Voltage & Percentage
    uint32_t vbat_mv = 0;
    bsp_battery_get_voltage(&vbat_mv, NULL);
    uint8_t pct = bsp_battery_get_percentage();
    lv_label_set_text_fmt(lbl_power, "Battery: %lu mV (%d%%)", (unsigned long)vbat_mv, pct);

    // 4. MicroSD Storage Status
    if (bsp_sdcard_is_mounted()) {
        lv_label_set_text_fmt(lbl_storage, "SD Card: Mounted (%.1f GB)", bsp_sdcard_get_capacity_gb());
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
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    // Device Header
    char dev_name[32] = {0};
    bsp_get_device_name(dev_name, sizeof(dev_name));

    lbl_title = lv_label_create(scr);
    lv_label_set_text_fmt(lbl_title, "%s (Boot #%lu)", dev_name, (unsigned long)boot_count);
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

    // Periodic UI Refresh Timer (every 3s for e-Paper partial refresh)
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

    // 6. Initialize LVGL Port (Display only, touch bypassed)
    ESP_ERROR_CHECK(bsp_lvgl_init());

    // 7. Render Non-Touch UI
    create_non_touch_ui(boot_count);

    // 8. Start Background Tasks
    xTaskCreatePinnedToCore(button_monitor_task, "btn_task",  3 * 1024, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(bsp_lvgl_port_task,  "lvgl_task", 6 * 1024, NULL, 2, NULL, 1);
}