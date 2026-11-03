/**
 * @file screen_diagnostics.c
 * @brief Screen 2: Psychrometric Matrix, Tri-Node Array & Battery Diagnostics.
 *
 * All typography strictly uses H1OS_FONT_14 (>= 14px minimum floor) and <= 188px width.
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#include "screen_diagnostics.h"
#include <stdio.h>
#include "app_state.h"
#include "h1os/h1os_lvgl.h"

static void add_diag_row(lv_obj_t *parent, lv_coord_t y, const char *text, lv_color_t fg_col)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl, H1OS_FONT_14, 0);
    lv_obj_set_style_text_color(lbl, fg_col, 0);
    lv_label_set_text(lbl, text);
    lv_obj_set_pos(lbl, 6, y);
}

void screen_diagnostics_render(lv_obj_t *parent, const app_context_t *ctx)
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

    /* Inverted Title Bar (y=0..22) */
    lv_obj_t *hdr = lv_obj_create(parent);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, 200, 22);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_bg_color(hdr, fg_col, 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);

    lv_obj_t *lbl_title = lv_label_create(hdr);
    lv_obj_set_style_text_font(lbl_title, H1OS_FONT_14, 0);
    lv_obj_set_style_text_color(lbl_title, bg_col, 0);
    lv_label_set_text_static(lbl_title, "DIAGNOSTICS [2/3]");
    lv_obj_center(lbl_title);

    /* Row 1 (y=27): Absolute Humidity */
    snprintf(buf, sizeof(buf), "AH: %.2f g/m³", (double)t->absolute_humidity_gm3);
    add_diag_row(parent, 27, buf, fg_col);

    /* Row 2 (y=46): Vapor Pressure vs Saturation Vapor Pressure */
    const char *vp_unit = "hPa";
    float vp_val  = app_state_convert_vp(t->vapor_pressure_hpa, ctx->cfg.vp_unit, &vp_unit);
    float svp_val = app_state_convert_vp(t->sat_vapor_pressure_hpa, ctx->cfg.vp_unit, &vp_unit);
    snprintf(buf, sizeof(buf), "VP: %.1f/%.1f %s", (double)vp_val, (double)svp_val, vp_unit);
    add_diag_row(parent, 46, buf, fg_col);

    /* Row 3 (y=65): SHTC3 & MCU Die Temps */
    snprintf(buf, sizeof(buf), "SHT:%.1fC Die:%.1fC", (double)t->shtc3_raw_temp_c, (double)t->mcu_die_temp_c);
    add_diag_row(parent, 65, buf, fg_col);

    /* Row 4 (y=84): EPD Glass Temp & Tri-Node Delta */
    snprintf(buf, sizeof(buf), "EPD:%.1fC ΔT:%+.2fC", (double)t->epd_glass_temp_c, (double)t->trinode_delta_c);
    add_diag_row(parent, 84, buf, fg_col);

    /* Divider Line (y=104) */
    lv_obj_t *div = lv_obj_create(parent);
    lv_obj_remove_style_all(div);
    lv_obj_set_size(div, 188, 2);
    lv_obj_set_pos(div, 6, 104);
    lv_obj_set_style_bg_color(div, fg_col, 0);
    lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);

    /* Row 5 (y=110): Battery Voltage & Average Current */
    if (t->avg_current_ma > 0.0f) {
        snprintf(buf, sizeof(buf), "Bat:%umV %.2fmA", (unsigned)t->battery_mv, (double)t->avg_current_ma);
    } else {
        snprintf(buf, sizeof(buf), "Bat:%umV (%u%%)", (unsigned)t->battery_mv, (unsigned)t->battery_soc_pct);
    }
    add_diag_row(parent, 110, buf, fg_col);

    /* Row 6 (y=129): Flow A & Flow B Intervals */
    snprintf(buf, sizeof(buf), "Flow A:%lus B:%lus",
             (unsigned long)ctx->cfg.flow_a_sec, (unsigned long)ctx->cfg.flow_b_sec);
    add_diag_row(parent, 129, buf, fg_col);

    /* Row 7 (y=148): Wake & Sample Counters */
    snprintf(buf, sizeof(buf), "Wakes:%lu Smp:%lu",
             (unsigned long)t->wake_count, (unsigned long)t->sample_count);
    add_diag_row(parent, 148, buf, fg_col);

    /* Bottom Hint Bar (y=174) */
    lv_obj_t *ftr_div = lv_obj_create(parent);
    lv_obj_remove_style_all(ftr_div);
    lv_obj_set_size(ftr_div, 188, 2);
    lv_obj_set_pos(ftr_div, 6, 173);
    lv_obj_set_style_bg_color(ftr_div, fg_col, 0);
    lv_obj_set_style_bg_opa(ftr_div, LV_OPA_COVER, 0);

    add_diag_row(parent, 178, "BOOT:Next PWR:Scrub", fg_col);
}
