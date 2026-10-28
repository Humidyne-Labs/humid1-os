/**
 * @file screen_system.c
 * @brief Clean-state LVGL v9 1-bit monochrome implementation of Screen 3 (System, Alarms & Actions).
 */

#include "screen_system.h"
#include <stdio.h>
#include "app_ota.h"
#include "app_state.h"
#include "h1os/h1os.h"

static lv_obj_t *s_cont_sys    = NULL;
static lv_obj_t *s_lbl_title   = NULL;
static lv_obj_t *s_lbl_alarm   = NULL;
static lv_obj_t *s_lbl_fw_info = NULL;
static lv_obj_t *s_lbl_act[4]  = {NULL, NULL, NULL, NULL};

static const char *ACTION_LABELS[APP_SYS_ACTION_COUNT] = {
    "INVERT DISPLAY THEME",
    "CLEAR WIFI SETTINGS",
    "FULL FACTORY RESET",
    "POWER OFF (STORAGE)"
};

static lv_obj_t *create_sys_label(lv_obj_t *parent, int32_t x, int32_t y, const char *text)
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

void screen_system_create(lv_obj_t *root)
{
    if (root == NULL) {
        return;
    }

    s_cont_sys = lv_obj_create(root);
    lv_obj_remove_style_all(s_cont_sys);
    lv_obj_set_size(s_cont_sys, (int32_t)APP_UI_SCREEN_WIDTH_PX, (int32_t)APP_UI_SCREEN_HEIGHT_PX);
    lv_obj_set_style_bg_color(s_cont_sys, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(s_cont_sys, LV_OPA_COVER, 0);
    lv_obj_add_flag(s_cont_sys, LV_OBJ_FLAG_HIDDEN);

    s_lbl_title   = create_sys_label(s_cont_sys, 6, 4, "[3/3] SYSTEM & ALARMS");
    s_lbl_alarm   = create_sys_label(s_cont_sys, 6, 28, "ALARM: NONE (NOMINAL)");
    s_lbl_fw_info = create_sys_label(s_cont_sys, 6, 62, "FW: " APP_FW_VERSION " (ota_0)");

    for (uint8_t i = 0U; i < (uint8_t)APP_SYS_ACTION_COUNT; ++i) {
        s_lbl_act[i] = create_sys_label(s_cont_sys, 6, 102 + ((int32_t)i * 22), ACTION_LABELS[i]);
    }
}

void screen_system_update(const app_context_t *ctx)
{
    if (ctx == NULL || s_cont_sys == NULL) {
        return;
    }

    char buf[64] = {0};
    const app_rtc_cache_t *rtc = app_state_get_rtc_cache();

    snprintf(buf, sizeof(buf), "[3/3] SYS %s", ctx->is_interactive ? "[ AWAKE ]" : "ZZZ SLEEP");
    lv_label_set_text(s_lbl_title, buf);

    if (rtc != NULL && rtc->active_alarm.active) {
        snprintf(buf, sizeof(buf), "ALM: %s", rtc->active_alarm.type);
    } else {
        snprintf(buf, sizeof(buf), "ALARM: NONE (NOMINAL)");
    }
    lv_label_set_text(s_lbl_alarm, buf);

    char slot_label[16] = {0};
    (void)app_ota_get_running_partition_label(slot_label, sizeof(slot_label));
    snprintf(buf, sizeof(buf), "FW:%s (%s) E:%ld",
             APP_FW_VERSION, slot_label,
             (long)((rtc != NULL) ? rtc->last_err_code : 0));
    lv_label_set_text(s_lbl_fw_info, buf);

    for (uint8_t i = 0U; i < (uint8_t)APP_SYS_ACTION_COUNT; ++i) {
        snprintf(buf, sizeof(buf), "%c %s",
                 (i == ctx->selected_action_idx) ? '>' : ' ',
                 ACTION_LABELS[i]);
        lv_label_set_text(s_lbl_act[i], buf);
    }
}

lv_obj_t *screen_system_get_container(void)
{
    return s_cont_sys;
}
