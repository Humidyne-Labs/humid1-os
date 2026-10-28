/**
 * @file screen_dashboard.c
 * @brief Humidor Environmental Telemetry Dashboard Screen Implementation
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#include "screen_dashboard.h"
#include "h1os/h1os_lvgl.h"
#include <stdio.h>

void screen_dashboard_render(const h1os_telemetry_t *telem, uint32_t cycle_count)
{
    h1os_lvgl_lock(100);

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    /* Header Bar */
    lv_obj_t *header = lv_label_create(scr);
    lv_label_set_text(header, "HUMID1-OS");
    lv_obj_align(header, LV_ALIGN_TOP_LEFT, 6, 6);

    /* Battery / Status in Header */
    lv_obj_t *lbl_bat = lv_label_create(scr);
    if (telem->is_usb_powered) {
        lv_label_set_text(lbl_bat, LV_SYMBOL_CHARGE " USB");
    } else {
        lv_label_set_text_fmt(lbl_bat, LV_SYMBOL_BATTERY_FULL " %u%%", telem->battery_percent);
    }
    lv_obj_align(lbl_bat, LV_ALIGN_TOP_RIGHT, -6, 6);

    /* Top Divider Line */
    static lv_point_precise_t top_line_pts[] = {{4, 22}, {196, 22}};
    lv_obj_t *top_line = lv_line_create(scr);
    lv_line_set_points(top_line, top_line_pts, 2);
    lv_obj_set_style_line_width(top_line, 1, 0);
    lv_obj_set_style_line_color(top_line, lv_color_black(), 0);

    /* Large Relative Humidity Readout (Primary Humidor Metric) */
    lv_obj_t *lbl_rh = lv_label_create(scr);
    lv_label_set_text_fmt(lbl_rh, "%.1f%%", (double)telem->ambient_rh_pct);
    lv_obj_align(lbl_rh, LV_ALIGN_TOP_MID, 0, 36);

    lv_obj_t *lbl_rh_desc = lv_label_create(scr);
    lv_label_set_text(lbl_rh_desc, "RELATIVE HUMIDITY");
    lv_obj_align(lbl_rh_desc, LV_ALIGN_TOP_MID, 0, 68);

    /* Mid Divider Line */
    static lv_point_precise_t mid_line_pts[] = {{4, 90}, {196, 90}};
    lv_obj_t *mid_line = lv_line_create(scr);
    lv_line_set_points(mid_line, mid_line_pts, 2);
    lv_obj_set_style_line_width(mid_line, 1, 0);
    lv_obj_set_style_line_color(mid_line, lv_color_black(), 0);

    /* Temperature Readout (Dual F & C) */
    lv_obj_t *lbl_temp_f = lv_label_create(scr);
    lv_label_set_text_fmt(lbl_temp_f, "%.1f°F", (double)telem->ambient_temp_f);
    lv_obj_align(lbl_temp_f, LV_ALIGN_TOP_LEFT, 10, 104);

    lv_obj_t *lbl_temp_c = lv_label_create(scr);
    lv_label_set_text_fmt(lbl_temp_c, "(%.1f°C)", (double)telem->ambient_temp_c);
    lv_obj_align(lbl_temp_c, LV_ALIGN_TOP_LEFT, 10, 126);

    /* Dew Point Readout */
    lv_obj_t *lbl_dew = lv_label_create(scr);
    lv_label_set_text_fmt(lbl_dew, "Dew: %.1f°F", (double)telem->dew_point_f);
    lv_obj_align(lbl_dew, LV_ALIGN_TOP_RIGHT, -10, 104);

    lv_obj_t *lbl_dew_c = lv_label_create(scr);
    lv_label_set_text_fmt(lbl_dew_c, "(%.1f°C)", (double)telem->dew_point_c);
    lv_obj_align(lbl_dew_c, LV_ALIGN_TOP_RIGHT, -10, 126);

    /* Bottom Divider Line */
    static lv_point_precise_t btm_line_pts[] = {{4, 154}, {196, 154}};
    lv_obj_t *btm_line = lv_line_create(scr);
    lv_line_set_points(btm_line, btm_line_pts, 2);
    lv_obj_set_style_line_width(btm_line, 1, 0);
    lv_obj_set_style_line_color(btm_line, lv_color_black(), 0);

    /* Status Footer: Humidor Status / Settled Status */
    lv_obj_t *lbl_footer = lv_label_create(scr);
    if (telem->is_thermally_settled) {
        lv_label_set_text_fmt(lbl_footer, "SETTLED | CYCLE #%lu", (unsigned long)cycle_count);
    } else {
        lv_label_set_text_fmt(lbl_footer, "SETTLING (%u%%) | #%lu", telem->trinode_confidence_pct, (unsigned long)cycle_count);
    }
    lv_obj_align(lbl_footer, LV_ALIGN_BOTTOM_MID, 0, -8);

    lv_screen_load(scr);
    h1os_lvgl_unlock();
}
