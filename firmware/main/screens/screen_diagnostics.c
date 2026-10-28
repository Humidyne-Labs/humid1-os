/**
 * @file screen_diagnostics.c
 * @brief Clean-state LVGL v9 1-bit monochrome implementation of Screen 2 (Diagnostics).
 */

#include "screen_diagnostics.h"
#include <stdio.h>
#include "h1os/h1os.h"

static lv_obj_t *s_cont_diag  = NULL;
static lv_obj_t *s_lbl_title  = NULL;
static lv_obj_t *s_lbl_trinode = NULL;
static lv_obj_t *s_lbl_psych  = NULL;
static lv_obj_t *s_lbl_power  = NULL;
static lv_obj_t *s_lbl_net    = NULL;

static lv_obj_t *create_diag_label(lv_obj_t *parent, int32_t x, int32_t y, const char *text)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl, h1os_font_ui_12, 0);
    lv_obj_set_style_text_color(lbl, lv_color_black(), 0);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(lbl, (int32_t)APP_UI_SAFE_TEXT_WIDTH_PX);
    lv_obj_set_pos(lbl, x, y);
    lv_label_set_text(lbl, text);
    return lbl;
}

void screen_diagnostics_create(lv_obj_t *root)
{
    if (root == NULL) {
        return;
    }

    s_cont_diag = lv_obj_create(root);
    lv_obj_remove_style_all(s_cont_diag);
    lv_obj_set_size(s_cont_diag, (int32_t)APP_UI_SCREEN_WIDTH_PX, (int32_t)APP_UI_SCREEN_HEIGHT_PX);
    lv_obj_set_style_bg_color(s_cont_diag, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(s_cont_diag, LV_OPA_COVER, 0);
    lv_obj_add_flag(s_cont_diag, LV_OBJ_FLAG_HIDDEN);

    s_lbl_title   = create_diag_label(s_cont_diag, 6, 4, "[2/3] DIAGNOSTICS [AWAKE]");
    s_lbl_trinode = create_diag_label(s_cont_diag, 6, 32, "TRI-NODE: SHTC3 / MCU / EPD");
    s_lbl_psych   = create_diag_label(s_cont_diag, 6, 76, "VP: --.- hPa | AH: --.- g/m3");
    s_lbl_power   = create_diag_label(s_cont_diag, 6, 120, "PWR: ----mV | --.-d left");
    s_lbl_net     = create_diag_label(s_cont_diag, 6, 164, "FLOW A: 60s | FLOW B: 900s");
}

void screen_diagnostics_update(const app_context_t *ctx)
{
    if (ctx == NULL || s_cont_diag == NULL) {
        return;
    }

    char buf[64] = {0};

    snprintf(buf, sizeof(buf), "[2/3] DIAG %s", ctx->is_interactive ? "[ AWAKE ]" : "ZZZ SLEEP");
    lv_label_set_text(s_lbl_title, buf);

    snprintf(buf, sizeof(buf), "S:%.1fK M:%.1fK C:%u%%",
             (double)ctx->telemetry.shtc3_raw_k,
             (double)ctx->telemetry.mcu_die_k,
             (unsigned int)ctx->telemetry.confidence_pct);
    lv_label_set_text(s_lbl_trinode, buf);

    snprintf(buf, sizeof(buf), "VP:%.1fhPa AH:%.1fg/m3",
             (double)ctx->telemetry.vapor_press_hpa,
             (double)ctx->telemetry.abs_humidity_gm3);
    lv_label_set_text(s_lbl_psych, buf);

    snprintf(buf, sizeof(buf), "%umV %.2fmA ~%.0fd",
             (unsigned int)ctx->telemetry.vbat_mv,
             (double)ctx->telemetry.avg_current_ma,
             (double)ctx->telemetry.autonomy_days);
    lv_label_set_text(s_lbl_power, buf);

    snprintf(buf, sizeof(buf), "A:%lus B:%lus RSSI:%ddBm",
             (unsigned long)ctx->attrs.flow_a_period_sec,
             (unsigned long)ctx->attrs.flow_b_period_sec,
             (int)ctx->telemetry.rssi_dbm);
    lv_label_set_text(s_lbl_net, buf);
}

lv_obj_t *screen_diagnostics_get_container(void)
{
    return s_cont_diag;
}
