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
 *  - etc....
 * 
 * @copyright Copyright (c) 2026 Humidyne Labs / Humiditron
 * SPDX-License-Identifier: MIT
 * 
 * @observations 
 *      (9/23/2026) When initalizing the RTC, PA_EN and EPD_3V3_EN 
 *      pins must NOT be uninitialized. Dispite the external pull-ups, 
 *      the RTC will not respond to I2C commands if these pins are 'floating'.
 *      The working theory is i2c diode clamping, grounding the i2c lines.
 *      OR, some sort of backfeeding issue / supply rail capcatiance inrush brownout.
 *      (initalizing PA_EN and EPD_3V3_EN before initializing the RTC fixed the issue)
 *      =================================================================================
 *      (9/24/2026) I had one heck of a time with getting binary graphics to display,
 *      the solution I went with was a custom decoder.
 *      I tried the built in LVGL decoder, but for some reason the get_decoder_data(dsc); function
 *      was returning all 0xFF's. so... idk if I forgot to 'enable' a flag? or if it dosen't
 *      nativly do 1bit images? idk 😕. I may test this further, in the future.
 *      ================================================================================= 
 */

#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"
#include "bsp/bsp.h"
#include "esp_timer.h"
#include "esp_lv_fs.h"
#include "esp_mmap_assets.h"
#include "mmap_generate_storage.h"

static const char *TAG = "main";

// UI Label Handles
static lv_obj_t *lbl_title   = NULL;
static lv_obj_t *lbl_time    = NULL;
static lv_obj_t *lbl_sensors = NULL;
static lv_obj_t *lbl_power   = NULL;
static lv_obj_t *lbl_storage = NULL;
static lv_obj_t *lbl_event   = NULL;

static lv_timer_t *s_ui_timer           = NULL;
static SemaphoreHandle_t s_shutdown_sem = NULL;

/* =========================================================================
 * Audio Chime Synthesizer (880 Hz -> 1760 Hz)
 * ========================================================================= */
static void play_audio_chime(void) 
{
    const uint32_t sample_rate = 16000;
    const size_t tone_samples  = sample_rate / 4; // 250ms per tone
    const size_t total_samples = tone_samples * 2;
    const size_t buf_size      = total_samples * sizeof(int16_t); // Mono 16-bit

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
 * Application Pre-Shutdown Callback
 * ========================================================================= */
static void shutdown_task(void *arg) {
    ESP_LOGI("APP", "Executing safe shutdown routine...");

    bsp_lvgl_lock();
    if (s_ui_timer) {
        lv_timer_delete(s_ui_timer);
        s_ui_timer = NULL;
    }
    lv_obj_t *scr = lv_screen_active();

    // Clear old labels and set white background
    lv_obj_clean(scr);
    lbl_title = lbl_time = lbl_sensors = lbl_power = lbl_storage = lbl_event = NULL;

    lv_obj_set_style_bg_color(scr, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    // Create and center shutdown image
    lv_obj_t *img = lv_image_create(scr);
    lv_image_set_src(img, "S:space_cat.bin");    
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    lv_refr_now(NULL); // Will now execute instantly with 0 dynamic allocations!
    bsp_lvgl_unlock();

    ESP_LOGI("APP", "Waiting 3.5s for e-Paper waveform refresh to complete...");
    vTaskDelay(pdMS_TO_TICKS(2000)); // Holds power while bsp_lvgl_port_task flushes EPD

    // Signal app_pre_shutdown that power can now be cut
    xSemaphoreGive(s_shutdown_sem);
    vTaskDelete(NULL);
}

static void app_pre_shutdown(void *user_data) 
{
    ESP_LOGI("APP", "Power off triggered -> Running shutdown display sequence...");    
    if (s_shutdown_sem == NULL) {
        s_shutdown_sem = xSemaphoreCreateBinary();
    }

    // Spawn shutdown task
    xTaskCreate(shutdown_task, "shutdown_task", 6 * 1024, NULL, 5, NULL);

    // Block here to prevent BSP from de-asserting BSP_GPIO_BAT_CTRL prematurely
    xSemaphoreTake(s_shutdown_sem, portMAX_DELAY);
    ESP_LOGI("APP", "e-Paper refresh complete. Releasing power latch.");
}

/* =========================================================================
 * Fast Button Scanner
 * ========================================================================= */
static void on_boot_button(bsp_button_t btn, bsp_button_event_t evt, void *arg) 
{
    if (evt == BSP_BUTTON_EVENT_SINGLE_CLICK) 
    {
        ESP_LOGI(TAG, "BOOT key pressed");
        if (lbl_event) {
            bsp_lvgl_lock();
            lv_label_set_text(lbl_event, "Event: BOOT Key Pressed");
            bsp_lvgl_unlock();
        }
    } 
    else if (evt == BSP_BUTTON_EVENT_LONG_PRESS) 
    {
        ESP_LOGI(TAG, "BOOT key pressed -> Playing Audio Chime");
        if (lbl_event) {
            bsp_lvgl_lock();
            lv_label_set_text(lbl_event, "Event: BOOT Key (Chime)");
            bsp_lvgl_unlock();
        }
        play_audio_chime();
    }
}

/* =========================================================================
 * LVGL Periodic Telemetry Callback (Runs in LVGL Task Context)
 * ========================================================================= */
static void ui_update_timer_cb(lv_timer_t *timer) 
{
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
 * Non-Touch UI Layout (200x200 e-Paper)
 * ========================================================================= */
static void create_non_touch_ui(uint32_t boot_count) 
{
    char str_buf[64];
    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr); // Clean up old object data.
    
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
    
    char label_txt[64];
    lbl_event = lv_label_create(event_box);
    esp_reset_reason_t reason = esp_reset_reason();
    snprintf(label_txt, sizeof(label_txt), "Code: %d", (int)reason);
    lv_label_set_text(lbl_event, label_txt);
    lv_obj_center(lbl_event);

    // Non-Touch Control Instructions Footer
    lv_obj_t *lbl_footer = lv_label_create(scr);
    lv_label_set_text(lbl_footer, "[BOOT: Chime | PWR: Event]");
    lv_obj_align(lbl_footer, LV_ALIGN_BOTTOM_MID, 0, -6);

    // Periodic UI Refresh Timer (3 seconds)
    if(s_ui_timer == NULL) {
        s_ui_timer = lv_timer_create(ui_update_timer_cb, 3000, NULL);
    }
}

/* =============================================================================
 * Launch space_cat into low earth orbit ... C A T S ... I N ... S P A C E ...
 * ============================================================================= */
static void image_test(bsp_button_t btn, bsp_button_event_t evt, void *arg) 
{
    if (evt == BSP_BUTTON_EVENT_SINGLE_CLICK) {
        ESP_LOGI(TAG, "POWER key pressed");
        if (lbl_event) {
            bsp_lvgl_lock();
            lv_label_set_text(lbl_event, "Event: POWER Key Pressed");
            bsp_lvgl_unlock();
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        app_pre_shutdown(NULL);                     // Trigger off screen
        uint32_t boot_count = 0;                    // Update the screens boot_counter
        bsp_nvs_get_u32("boot_count", &boot_count); // Get real boot count
        bsp_lvgl_lock();
        create_non_touch_ui(boot_count);
        bsp_lvgl_unlock();
    }
}

/* =========================================================================
 * Master app_main
 * ========================================================================= */
extern "C" void app_main(void) {
    // 1. Core Board Init (Power hold, NVS, I2C, Buttons)
    ESP_ERROR_CHECK(bsp_board_init());

    // 1.b Register custom event hooks
    bsp_button_register_cb(BSP_BUTTON_BOOT,  BSP_BUTTON_EVENT_SINGLE_CLICK, on_boot_button, NULL);
    bsp_button_register_cb(BSP_BUTTON_BOOT,  BSP_BUTTON_EVENT_LONG_PRESS,   on_boot_button, NULL);
    bsp_button_register_cb(BSP_BUTTON_POWER, BSP_BUTTON_EVENT_SINGLE_CLICK, image_test,     NULL);
    bsp_power_register_shutdown_cb(app_pre_shutdown, NULL);

    // 2. NVS Boot Counter
    uint32_t boot_count = 0;
    bsp_nvs_get_u32("boot_count", &boot_count);
    bsp_nvs_set_u32("boot_count", ++boot_count);

    // 3. Initialize RTC & baseline timestamp
    if(bsp_rtc_init() == ESP_OK) {
        // If RTC is stopped or not responding yet, retry once after board peripherals are up
        ESP_LOGI(TAG, "Checking RTC status...");
        bool is_running = false;
        if (bsp_rtc_is_running(&is_running) != ESP_OK || !is_running) {
            ESP_LOGW(TAG, "RTC power-loss or uninitialized. Programming default datetime...");
            bsp_rtc_datetime_t init_time = {
                .year = 2026, .month = 9, .day = 23,
                .weekday = 3, .hour = 12, .minute = 0, .second = 0,
            };
            // Ensure write succeeds
            for (int retry = 0; retry < 3; retry++) {
                if (bsp_rtc_set_datetime(&init_time) == ESP_OK) {
                   ESP_LOGI(TAG, "RTC datetime set successfully.");
                   break;
                }
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
        ESP_LOGI(TAG, "RTC initialized and running.");
    } else {
        ESP_LOGE(TAG, "RTC initialization failed. Continuing without RTC.");
    }

    // 4. Initialize SHTC3 Sensor
    if(bsp_shtc3_init() != ESP_OK) {
        ESP_LOGE(TAG, "SHTC3 sensor initialization failed. Continuing without sensor.");
    }

    // 4.b Mount MicroSD Card (if present)
    bsp_sdcard_mount();

    // 5. Initialize Audio Codec & Play Boot Sound
    if (bsp_audio_init() == ESP_OK) {
        play_audio_chime();
    }

    // 6. Initialize LVGL Port
    if(bsp_lvgl_init() != ESP_OK) {
        ESP_LOGE(TAG, "LVGL init Failed.");
    }

    // 7. Render UI (under LVGL lock before tasks start)
    bsp_lvgl_lock();
    create_non_touch_ui(boot_count);
    bsp_lvgl_unlock();

    /* 7.b Initialize Partition and Register Decoder Callbacks */
    if(init_drive("storage", 'S', MMAP_STORAGE_FILES, MMAP_STORAGE_CHECKSUM) != ESP_OK) {
        ESP_LOGE(TAG, "Drive init Failed.");
    }

    // 8. Start Background Tasks
    xTaskCreatePinnedToCore(bsp_lvgl_port_task, "lvgl_task", 12 * 1024, NULL, 2, NULL, 1);
}