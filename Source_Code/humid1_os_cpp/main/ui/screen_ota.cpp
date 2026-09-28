/**
 * @file screen_ota.cpp
 * @brief OTA Firmware Update Progress Screen
 * 
 * Hardware Target:
 *  - Waveshare ESP32-S3 ePaper 1.54" V2 (200x200 1-bit Mono EPD)
 * 
 * @attribution
 * - Humidyne Labs / Humiditron (2026)
 * 
 * SPDX-License-Identifier: MIT
 */

#include "ui_manager.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "lvgl.h"
#include "bsp/bsp.h"


static const char *TAG = "screen_ota";

void ui_show_ota_screen(int progress_pct, const char *msg)
{
    ESP_LOGI(TAG, "Rendering OTA Progress Screen: %d%% - %s", progress_pct, msg ? msg : "");

    bsp_lvgl_lock();

    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);

    // Set full white background
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // 1. Top Header Bar (0 to 22px)
    lv_obj_t *header = lv_obj_create(scr);
    lv_obj_set_size(header, 200, 22);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_all(header, 2, 0);

    lv_obj_t *lbl_title = lv_label_create(header);
    lv_label_set_text(lbl_title, "FIRMWARE OTA UPDATE");
    lv_obj_set_style_text_color(lbl_title, lv_color_white(), 0);
    lv_obj_align(lbl_title, LV_ALIGN_CENTER, 0, 0);

    // 2. Center Card (28 to 140px)
    lv_obj_t *card = lv_obj_create(scr);
    lv_obj_set_size(card, 184, 106);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 28);
    lv_obj_set_style_bg_color(card, lv_color_white(), 0);
    lv_obj_set_style_border_color(card, lv_color_black(), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 6, 0);
    lv_obj_set_style_pad_all(card, 6, 0);

    lv_obj_t *lbl_pct = lv_label_create(card);
    lv_label_set_text_fmt(lbl_pct, "%d %%", progress_pct);
    lv_obj_set_style_text_color(lbl_pct, lv_color_black(), 0);
    lv_obj_align(lbl_pct, LV_ALIGN_CENTER, 0, -8);

    lv_obj_t *lbl_msg = lv_label_create(card);
    lv_label_set_text(lbl_msg, msg ? msg : "Transferring image...");
    lv_obj_set_style_text_color(lbl_msg, lv_color_black(), 0);
    lv_obj_align(lbl_msg, LV_ALIGN_BOTTOM_MID, 0, -2);

    // 3. Bottom Footer (144 to 200px)
    lv_obj_t *lbl_warn = lv_label_create(scr);
    lv_label_set_text(lbl_warn, "! DO NOT POWER OFF !");
    lv_obj_set_style_text_color(lbl_warn, lv_color_black(), 0);
    lv_obj_align(lbl_warn, LV_ALIGN_TOP_MID, 0, 148);

    lv_obj_t *lbl_sub = lv_label_create(scr);
    lv_label_set_text(lbl_sub, "Device will reboot when done");
    lv_obj_set_style_text_color(lbl_sub, lv_color_black(), 0);
    lv_obj_align(lbl_sub, LV_ALIGN_TOP_MID, 0, 172);

    bsp_lvgl_unlock();
}
