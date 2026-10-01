/**
 * @file screen_dashboard.cpp
 * @brief Active Environmental Telemetry Dashboard Screen
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


static const char *TAG = "screen_dashboard";

void ui_show_dashboard_screen(const app_telemetry_data_t *telemetry, app_temp_unit_t unit, bool alarm_active)
{
    if (!telemetry) return;

    ESP_LOGI(TAG, "Rendering Dashboard: Temp=%.1fK, RH=%.1f%%, Batt=%u%%, Alarm=%d",
             telemetry->temp_k, telemetry->rh_pct, telemetry->battery_pct, alarm_active);

    bsp_lvgl_lock();

    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);

    // Set full white background
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // 1. Top Header Bar (White bg, 1px bottom border, black text)
    lv_obj_t *header = lv_obj_create(scr);
    lv_obj_set_size(header, 200, 22);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(header, lv_color_black(), 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_all(header, 2, 0);

    // Left: Time string
    char time_str[16] = {0};
    if (bsp_time_get_formatted(BSP_TIME_FMT_12H_MIN, time_str, sizeof(time_str)) != ESP_OK) {
        snprintf(time_str, sizeof(time_str), "--:--");
    }
    lv_obj_t *lbl_time = lv_label_create(header);
    lv_label_set_text(lbl_time, time_str);
    lv_obj_set_style_text_color(lbl_time, lv_color_black(), 0);
    lv_obj_align(lbl_time, LV_ALIGN_LEFT_MID, 4, 0);

    // Right: Battery & RSSI
    lv_obj_t *lbl_bat = lv_label_create(header);
    lv_label_set_text_fmt(lbl_bat, "%u%% | %ddBm", telemetry->battery_pct, telemetry->rssi_dbm);
    lv_obj_set_style_text_color(lbl_bat, lv_color_black(), 0);
    lv_obj_align(lbl_bat, LV_ALIGN_RIGHT_MID, -4, 0);

    // 2. Main Environmental Card (26 to 162px)
    lv_obj_t *card = lv_obj_create(scr);
    lv_obj_set_size(card, 192, 136);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 26);
    lv_obj_set_style_radius(card, 6, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_style_bg_color(card, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_black(), 0);
    lv_obj_set_style_border_width(card, alarm_active ? 3 : 2, 0);

    // Alert Banner Bar at top of card when alarm is active
    if (alarm_active) {
        lv_obj_t *banner = lv_obj_create(card);
        lv_obj_set_size(banner, 188, 22);
        lv_obj_align(banner, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_bg_color(banner, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(banner, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(banner, 0, 0);
        lv_obj_set_style_radius(banner, 0, 0);
        lv_obj_set_style_pad_all(banner, 2, 0);

        lv_obj_t *lbl_alert = lv_label_create(banner);
        lv_label_set_text(lbl_alert, "! THRESHOLD ALERT !");
        lv_obj_set_style_text_color(lbl_alert, lv_color_white(), 0);
        lv_obj_align(lbl_alert, LV_ALIGN_CENTER, 0, 0);
    }

    int rh_y = alarm_active ? 26 : 6;
    int rh_title_y = alarm_active ? 50 : 36;
    int line_y = alarm_active ? 68 : 58;
    int temp_y = alarm_active ? 74 : 66;
    int temp_title_y = alarm_active ? 98 : 96;

    // Humidity Display
    lv_obj_t *lbl_rh = lv_label_create(card);
    lv_label_set_text_fmt(lbl_rh, "%.1f %%", telemetry->rh_pct);
    lv_obj_set_style_text_color(lbl_rh, lv_color_black(), 0);
    lv_obj_align(lbl_rh, LV_ALIGN_TOP_MID, 0, rh_y);

    // Humidity Label Subtext
    lv_obj_t *lbl_rh_title = lv_label_create(card);
    lv_label_set_text(lbl_rh_title, "RELATIVE HUMIDITY");
    lv_obj_set_style_text_color(lbl_rh_title, lv_color_black(), 0);
    lv_obj_align(lbl_rh_title, LV_ALIGN_TOP_MID, 0, rh_title_y);

    // Horizontal Separator Line
    lv_obj_t *line = lv_obj_create(card);
    lv_obj_set_size(line, 160, 2);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, line_y);
    lv_obj_set_style_bg_color(line, lv_color_black(), 0);
    lv_obj_set_style_border_width(line, 0, 0);

    // Temperature Display
    float temp_val = telemetry->temp_f;
    const char *unit_symbol = "F";
    if (unit == APP_TEMP_UNIT_CELSIUS) {
        temp_val = telemetry->temp_c;
        unit_symbol = "C";
    } else if (unit == APP_TEMP_UNIT_KELVIN) {
        temp_val = telemetry->temp_k;
        unit_symbol = "K";
    }

    lv_obj_t *lbl_temp = lv_label_create(card);
    lv_label_set_text_fmt(lbl_temp, "%.1f %s", temp_val, unit_symbol);
    lv_obj_set_style_text_color(lbl_temp, lv_color_black(), 0);
    lv_obj_align(lbl_temp, LV_ALIGN_TOP_MID, 0, temp_y);

    // Temperature Label Subtext
    lv_obj_t *lbl_temp_title = lv_label_create(card);
    lv_label_set_text(lbl_temp_title, "TEMPERATURE");
    lv_obj_set_style_text_color(lbl_temp_title, lv_color_black(), 0);
    lv_obj_align(lbl_temp_title, LV_ALIGN_TOP_MID, 0, temp_title_y);

    // 3. Bottom Footer (166 to 200px)
    char date_str[24] = {0};
    if (bsp_time_get_date_formatted(BSP_DATE_FMT_MM_DD_YY_DOW, date_str, sizeof(date_str)) != ESP_OK) {
        snprintf(date_str, sizeof(date_str), "HUMID1-OS");
    }

    lv_obj_t *lbl_footer = lv_label_create(scr);
    lv_label_set_text_fmt(lbl_footer, "%s", date_str);
    lv_obj_set_style_text_color(lbl_footer, lv_color_black(), 0);
    lv_obj_align(lbl_footer, LV_ALIGN_TOP_MID, 0, 172);

    bsp_lvgl_unlock();
}
