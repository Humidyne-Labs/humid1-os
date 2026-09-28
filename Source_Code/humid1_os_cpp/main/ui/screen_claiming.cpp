/**
 * @file screen_claiming.cpp
 * @brief ThingsBoard Device Claiming Screen
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


static const char *TAG = "screen_claiming";

void ui_show_claiming_screen(const char *device_name, const char *claim_token, uint32_t remaining_sec)
{
    ESP_LOGI(TAG, "Rendering Claiming Screen: Device='%s', Token='%s', Rem=%lus",
             device_name ? device_name : "N/A", claim_token ? claim_token : "N/A", (unsigned long)remaining_sec);

    bsp_lvgl_lock();

    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);

    // Set full white background
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // 1. Top Header Bar (0 to 24px)
    lv_obj_t *header = lv_obj_create(scr);
    lv_obj_set_size(header, 200, 22);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_all(header, 2, 0);

    lv_obj_t *lbl_title = lv_label_create(header);
    lv_label_set_text_fmt(lbl_title, "CLAIMING | %s", device_name ? device_name : "HUMID1");
    lv_obj_set_style_text_color(lbl_title, lv_color_white(), 0);
    lv_obj_align(lbl_title, LV_ALIGN_CENTER, 0, 0);

    // 2. Claiming Card Box (28 to 140px)
    lv_obj_t *card = lv_obj_create(scr);
    lv_obj_set_size(card, 184, 106);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 28);
    lv_obj_set_style_bg_color(card, lv_color_white(), 0);
    lv_obj_set_style_border_color(card, lv_color_black(), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 6, 0);
    lv_obj_set_style_pad_all(card, 4, 0);

    lv_obj_t *lbl_box_title = lv_label_create(card);
    lv_label_set_text(lbl_box_title, "ENTER KEY IN DASHBOARD");
    lv_obj_set_style_text_color(lbl_box_title, lv_color_black(), 0);
    lv_obj_align(lbl_box_title, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *lbl_key = lv_label_create(card);
    lv_label_set_text_fmt(lbl_key, "%s", claim_token ? claim_token : "------");
    lv_obj_set_style_text_color(lbl_key, lv_color_black(), 0);
    lv_obj_align(lbl_key, LV_ALIGN_CENTER, 0, 6);

    // 3. Bottom Footer (144 to 200px)
    lv_obj_t *lbl_status = lv_label_create(scr);
    uint32_t min = remaining_sec / 60;
    uint32_t sec = remaining_sec % 60;
    lv_label_set_text_fmt(lbl_status, "Time Left: %02lu:%02lu", (unsigned long)min, (unsigned long)sec);
    lv_obj_set_style_text_color(lbl_status, lv_color_black(), 0);
    lv_obj_align(lbl_status, LV_ALIGN_TOP_MID, 0, 146);

    lv_obj_t *lbl_hint = lv_label_create(scr);
    lv_label_set_text(lbl_hint, "Waiting for Dashboard Claim...");
    lv_obj_set_style_text_color(lbl_hint, lv_color_black(), 0);
    lv_obj_align(lbl_hint, LV_ALIGN_TOP_MID, 0, 172);

    bsp_lvgl_unlock();
}
