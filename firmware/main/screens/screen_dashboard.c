/**
 * @file screen_dashboard.c
 * @brief Screen 1: Primary Environmental Dashboard (H1OS 1-BPP Typography & Polarity Compliant).
 *
 * Layout (200x200 px, 6px safe margin -> 188px inner width):
 *   - Header (y=0..24): Nickname (left) + Battery SoC% (right)
 *   - Divider (y=25): 2px solid horizontal rule
 *   - Hero RH (y=30..64): Relative Humidity % (H1OS_FONT_32)
 *   - Primary Temp (y=68..96): Settled Ambient Temp in C/F/K (H1OS_FONT_24)
 *   - Psychrometric Row (y=102..138): Dew Point & Vapor Pressure (H1OS_FONT_14)
 *   - Alert / Settling Banner (y=144..170): Inverted 1-bit pill when alarm active
 *   - Footer (y=174..198): Autonomy Days & Flow B interval
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#include "screen_dashboard.h"
#include <stdio.h>
#include "app_state.h"
#include "h1os/h1os_lvgl.h"

void screen_dashboard_render(lv_obj_t *parent, const app_context_t *ctx)
{
    if (parent == NULL || ctx == NULL) {
        return;
    }

    lv_obj_clean(parent);

    const bool dark = (ctx->cfg.theme == APP_THEME_DARK);
    const lv_color_t bg_col = dark ? lv_color_hex(0x000000) : lv_color_hex(0xFFFFFF);
    const lv_color_t fg_col = dark ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x000000);

    lv_obj_set_style_bg_color(parent, bg_col, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_remove_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    char buf[48];
    const app_telemetry_snapshot_t *t = &ctx->telemetry;

    /* 1. Top Header Bar: Nickname (Left) + Battery SoC (Right) */
    lv_obj_t *lbl_nick = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_nick, H1OS_FONT_14, 0);
    lv_obj_set_style_text_color(lbl_nick, fg_col, 0);
    lv_label_set_long_mode(lbl_nick, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(lbl_nick, 128);
    lv_label_set_text(lbl_nick, ctx->cfg.nickname);
    lv_obj_set_pos(lbl_nick, 6, 5);

    lv_obj_t *lbl_bat = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_bat, H1OS_FONT_14, 0);
    lv_obj_set_style_text_color(lbl_bat, fg_col, 0);
    snprintf(buf, sizeof(buf), "%s%u%%", t->vbus_present ? "+" : "", (unsigned)t->battery_soc_pct);
    lv_label_set_text(lbl_bat, buf);
    lv_obj_align(lbl_bat, LV_ALIGN_TOP_RIGHT, -6, 5);

    /* 2px Header Rule */
    lv_obj_t *hdr_line = lv_obj_create(parent);
    lv_obj_remove_style_all(hdr_line);
    lv_obj_set_size(hdr_line, 188, 2);
    lv_obj_set_pos(hdr_line, 6, 24);
    lv_obj_set_style_bg_color(hdr_line, fg_col, 0);
    lv_obj_set_style_bg_opa(hdr_line, LV_OPA_COVER, 0);

    /* 2. Hero Metric: Relative Humidity (H1OS_FONT_32) */
    lv_obj_t *lbl_rh = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_rh, H1OS_FONT_32, 0);
    lv_obj_set_style_text_color(lbl_rh, fg_col, 0);
    snprintf(buf, sizeof(buf), "%.1f%% RH", (double)t->ambient_rh_pct);
    lv_label_set_text(lbl_rh, buf);
    lv_obj_align(lbl_rh, LV_ALIGN_TOP_MID, 0, 30);

    /* 3. Primary Settled Ambient Temperature (H1OS_FONT_24) */
    const char *t_unit = "C";
    float disp_temp = app_state_convert_temp(t->ambient_temp_c, ctx->cfg.temp_unit, &t_unit);
    lv_obj_t *lbl_temp = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_temp, H1OS_FONT_24, 0);
    lv_obj_set_style_text_color(lbl_temp, fg_col, 0);
    if (ctx->cfg.temp_unit == APP_TEMP_UNIT_KELVIN) {
        snprintf(buf, sizeof(buf), "%.1f K", (double)disp_temp);
    } else {
        snprintf(buf, sizeof(buf), "%.1f°%s", (double)disp_temp, t_unit);
    }
    lv_label_set_text(lbl_temp, buf);
    lv_obj_align(lbl_temp, LV_ALIGN_TOP_MID, 0, 68);

    /* 4. Secondary Psychrometric Strip: Dew Point & Vapor Pressure (H1OS_FONT_14) */
    float disp_dew = app_state_convert_temp(t->dew_point_c, ctx->cfg.temp_unit, &t_unit);
    lv_obj_t *lbl_dew = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_dew, H1OS_FONT_14, 0);
    lv_obj_set_style_text_color(lbl_dew, fg_col, 0);
    snprintf(buf, sizeof(buf), "Dew: %.1f %s", (double)disp_dew, t_unit);
    lv_label_set_text(lbl_dew, buf);
    lv_obj_set_pos(lbl_dew, 6, 102);

    const char *vp_unit = "hPa";
    float disp_vp = app_state_convert_vp(t->vapor_pressure_hpa, ctx->cfg.vp_unit, &vp_unit);
    lv_obj_t *lbl_vp = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_vp, H1OS_FONT_14, 0);
    lv_obj_set_style_text_color(lbl_vp, fg_col, 0);
    snprintf(buf, sizeof(buf), "VP: %.1f %s", (double)disp_vp, vp_unit);
    lv_label_set_text(lbl_vp, buf);
    lv_obj_set_pos(lbl_vp, 6, 121);

    /* 5. Status / Compound Alarm Banner (y=143..169) */
    lv_obj_t *banner = lv_obj_create(parent);
    lv_obj_remove_style_all(banner);
    lv_obj_set_size(banner, 188, 24);
    lv_obj_set_pos(banner, 6, 143);
    lv_obj_set_style_border_width(banner, 2, 0);
    lv_obj_set_style_border_color(banner, fg_col, 0);
    lv_obj_set_style_border_opa(banner, LV_OPA_COVER, 0);

    lv_obj_t *lbl_status = lv_label_create(banner);
    lv_obj_set_style_text_font(lbl_status, H1OS_FONT_14, 0);

    if (t->compound_alarm_active) {
        lv_obj_set_style_bg_color(banner, fg_col, 0);
        lv_obj_set_style_bg_opa(banner, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(lbl_status, bg_col, 0);
        lv_label_set_text_static(lbl_status, "! ALERT: LIMIT BREACH !");
    } else {
        lv_obj_set_style_bg_color(banner, bg_col, 0);
        lv_obj_set_style_bg_opa(banner, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(lbl_status, fg_col, 0);
        if (t->thermally_settled) {
            snprintf(buf, sizeof(buf), "SETTLED (%u%% CONF)", (unsigned)t->confidence_pct);
        } else {
            snprintf(buf, sizeof(buf), "EQUILIBRATING (%u%%)", (unsigned)t->confidence_pct);
        }
        lv_label_set_text(lbl_status, buf);
    }
    lv_obj_center(lbl_status);

    /* 6. Bottom Footer: Autonomy Days & Flow B Interval */
    lv_obj_t *ftr_line = lv_obj_create(parent);
    lv_obj_remove_style_all(ftr_line);
    lv_obj_set_size(ftr_line, 188, 2);
    lv_obj_set_pos(ftr_line, 6, 173);
    lv_obj_set_style_bg_color(ftr_line, fg_col, 0);
    lv_obj_set_style_bg_opa(ftr_line, LV_OPA_COVER, 0);

    lv_obj_t *lbl_ftr = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_ftr, H1OS_FONT_14, 0);
    lv_obj_set_style_text_color(lbl_ftr, fg_col, 0);
    if (t->autonomy_days > 0.0f) {
        snprintf(buf, sizeof(buf), "Est: %.0fd | Sync: %lum",
                 (double)t->autonomy_days, (unsigned long)(ctx->cfg.flow_b_sec / 60U));
    } else {
        snprintf(buf, sizeof(buf), "%umV | Sync: %lum",
                 (unsigned)t->battery_mv, (unsigned long)(ctx->cfg.flow_b_sec / 60U));
    }
    lv_label_set_text(lbl_ftr, buf);
    lv_obj_align(lbl_ftr, LV_ALIGN_BOTTOM_MID, 0, -4);
}
