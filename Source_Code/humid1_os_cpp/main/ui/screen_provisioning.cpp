/**
 * @file screen_provisioning.cpp
 * @brief BLE Wi-Fi Provisioning Screen with Concurrent QR Code & Pairing Key
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


static const char *TAG = "screen_prov";

void ui_show_provisioning_screen(const char *service_name, const char *pop_pin)
{
    ESP_LOGI(TAG, "Rendering Provisioning Screen: Name='%s', PoP='%s'",
             service_name ? service_name : "N/A", pop_pin ? pop_pin : "N/A");

    bsp_lvgl_lock();
    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);

    // Full white background
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);

    // 2. Render QR Code (Targeting max boundary --- px)
    // bsp_prov_render_qr_code automatically calculates the module requirement and snaps integer scaling.
    lv_obj_t *qr = bsp_prov_render_qr_code(scr, 140, pop_pin); //renders at 135 px, scans fine.
    if (qr) {
        lv_obj_align(qr, LV_ALIGN_TOP_MID, 0, 22);
    } else {
        ESP_LOGE(TAG, "Failed to render QR Code widget");
    }

    bsp_lvgl_unlock();
    bsp_delay_ms(1000); // Allow time for the screen to refresh before returning
}

/*
void ui_show_provisioning_screen(const char *service_name, const char *pop_pin)
{
    ESP_LOGI(TAG, "Rendering Provisioning Screen: Name='%s', PoP='%s'",
             service_name ? service_name : "N/A", pop_pin ? pop_pin : "N/A");

    bsp_lvgl_lock();
    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);

    // Full white background
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);

    // 1. Top Header Bar (200x20 px)
    lv_obj_t *header = lv_obj_create(scr);
    lv_obj_set_size(header, 200, 20);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE); // Remove scrollbars
    lv_obj_set_style_bg_color(header, lv_color_white(), 0);
    lv_obj_set_style_border_color(header, lv_color_black(), 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);

    lv_obj_t *lbl_title = lv_label_create(header);
    lv_label_set_text_fmt(lbl_title, "WI-FI SETUP | %s", service_name ? service_name : "PROV");
    lv_obj_set_style_text_color(lbl_title, lv_color_black(), 0);
    lv_obj_align(lbl_title, LV_ALIGN_CENTER, 0, 0);

    // 2. Render QR Code (Targeting max boundary 115 px)
    // bsp_prov_render_qr_code automatically calculates the module requirement and snaps integer scaling.
    lv_obj_t *qr = bsp_prov_render_qr_code(scr, 115, pop_pin);
    if (qr) {
        lv_obj_align(qr, LV_ALIGN_TOP_MID, 0, 22);
    } else {
        ESP_LOGE(TAG, "Failed to render QR Code widget");
    }

    // 3. Bottom Footer Text (Adjusted Y offsets to avoid overlap with QR bottom)
    lv_obj_t *lbl_pin = lv_label_create(scr);
    lv_label_set_text_fmt(lbl_pin, "PAIRING PIN: %s", pop_pin ? pop_pin : "--------");
    lv_obj_set_style_text_color(lbl_pin, lv_color_black(), 0);
    lv_obj_align(lbl_pin, LV_ALIGN_TOP_MID, 0, 145);

    lv_obj_t *lbl_hint = lv_label_create(scr);
    lv_label_set_text(lbl_hint, "Scan in App | Hold Boot: Reset");
    lv_obj_set_style_text_color(lbl_hint, lv_color_black(), 0);
    lv_obj_align(lbl_hint, LV_ALIGN_TOP_MID, 0, 168);
    
    bsp_lvgl_unlock();
}
*/