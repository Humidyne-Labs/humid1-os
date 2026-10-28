/**
 * @file screen_dashboard.c
 * @brief Clean-state LVGL v9 1-bit monochrome implementation of Screen 1 (Main Dashboard).
 */

#include "screen_dashboard.h"
#include <stdio.h>
#include "h1os/h1os.h"

static lv_obj_t *s_cont_dash   = NULL;
static lv_obj_t *s_lbl_header  = NULL;
static lv_obj_t *s_lbl_mode    = NULL;
static lv_obj_t *s_lbl_alarm   = NULL;
static lv_obj_t *s_lbl_rh      = NULL;
static lv_obj_t *s_lbl_temp    = NULL;
static lv_obj_t *s_lbl_dew     = NULL;
static lv_obj_t *s_lbl_footer  = NULL;

static lv_obj_t *create_1bpp_label(lv_obj_t *parent, const lv_font_t *font,
                                   int32_t x, int32_t y, const char *initial_text)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_style_text_color(lbl, lv_color_black(), 0);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(lbl, (int32_t)APP_UI_SAFE_TEXT_WIDTH_PX);
    lv_obj_set_pos(lbl, x, y);
    lv_label_set_text(lbl, initial_text);
    return lbl;
}

void screen_dashboard_create(lv_obj_t *root)
{
    if (root == NULL) {
        return;
    }

    s_cont_dash = lv_obj_create(root);
    lv_obj_remove_style_all(s_cont_dash);
    lv_obj_set_size(s_cont_dash, (int32_t)APP_UI_SCREEN_WIDTH_PX, (int32_t)APP_UI_SCREEN_HEIGHT_PX);
    lv_obj_set_style_bg_color(s_cont_dash, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(s_cont_dash, LV_OPA_COVER, 0);

    s_lbl_header = create_1bpp_label(s_cont_dash, h1os_font_ui_12, 6, 4, "HUMID1 READY");
    s_lbl_mode   = create_1bpp_label(s_cont_dash, h1os_font_ui_12, 134, 4, "[ AWAKE ]");
    s_lbl_alarm  = create_1bpp_label(s_cont_dash, h1os_font_ui_12, 6, 26, "STATUS: NOMINAL");
    s_lbl_rh     = create_1bpp_label(s_cont_dash, h1os_font_hero_48, 6, 52, "--.-%");
    s_lbl_temp   = create_1bpp_label(s_cont_dash, h1os_font_ui_16, 6, 122, "TEMP: --.- F");
    s_lbl_dew    = create_1bpp_label(s_cont_dash, h1os_font_ui_14, 6, 148, "DEW:  --.- F");
    s_lbl_footer = create_1bpp_label(s_cont_dash, h1os_font_ui_12, 6, 178, "BAT: --% (----mV)");
}

void screen_dashboard_update(const app_context_t *ctx)
{
    if (ctx == NULL || s_cont_dash == NULL) {
        return;
    }

    char buf[48] = {0};
    const char *id_str = (ctx->attrs.device_nickname[0] != '\0')
                             ? ctx->attrs.device_nickname
                             : ctx->attrs.device_uid;

    snprintf(buf, sizeof(buf), "%s %.11s",
             ctx->wifi_connected ? "OK" : "OFF", id_str);
    lv_label_set_text(s_lbl_header, buf);

    lv_label_set_text(s_lbl_mode, ctx->is_interactive ? "[ AWAKE ]" : "ZZZ SLEEP");

    const app_rtc_cache_t *rtc = (const app_rtc_cache_t *)0;
    (void)rtc;
    lv_label_set_text(s_lbl_alarm, "STATUS: NOMINAL");

    snprintf(buf, sizeof(buf), "%.1f%%", (double)ctx->telemetry.rh_pct);
    lv_label_set_text(s_lbl_rh, buf);

    const float disp_temp = app_kelvin_to_display(ctx->telemetry.temp_k, ctx->attrs.temp_unit);
    const float disp_dew  = app_kelvin_to_display(ctx->telemetry.dew_point_k, ctx->attrs.temp_unit);
    const char  unit_ch   = app_temp_unit_char(ctx->attrs.temp_unit);

    snprintf(buf, sizeof(buf), "TEMP: %.1f %c", (double)disp_temp, unit_ch);
    lv_label_set_text(s_lbl_temp, buf);

    snprintf(buf, sizeof(buf), "DEW:  %.1f %c", (double)disp_dew, unit_ch);
    lv_label_set_text(s_lbl_dew, buf);

    if (ctx->telemetry.vbus_present) {
        snprintf(buf, sizeof(buf), "USB CHG (%u mV)", (unsigned int)ctx->telemetry.vbat_mv);
    } else {
        snprintf(buf, sizeof(buf), "BAT: %u%% (%u mV)",
                 (unsigned int)ctx->telemetry.battery_pct,
                 (unsigned int)ctx->telemetry.vbat_mv);
    }
    lv_label_set_text(s_lbl_footer, buf);
}

lv_obj_t *screen_dashboard_get_container(void)
{
    return s_cont_dash;
}
