/**
 * @file screen_onboarding.c
 * @brief Clean-state LVGL v9 1-bit monochrome implementation of BLE Provisioning,
 *        Customer Claiming, and OTA Progress modal screens.
 */

#include "screen_onboarding.h"
#include <stdio.h>
#include "net_wifi_ble.h"
#include "h1os/h1os.h"

static lv_obj_t *s_cont_modal = NULL;
static lv_obj_t *s_lbl_header = NULL;
static lv_obj_t *s_lbl_line1  = NULL;
static lv_obj_t *s_lbl_line2  = NULL;
static lv_obj_t *s_lbl_line3  = NULL;
static lv_obj_t *s_lbl_footer = NULL;

static lv_obj_t *create_modal_label(lv_obj_t *parent, const lv_font_t *font,
                                    int32_t x, int32_t y, const char *text)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_style_text_color(lbl, lv_color_black(), 0);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(lbl, (int32_t)APP_UI_SAFE_TEXT_WIDTH_PX);
    lv_obj_set_pos(lbl, x, y);
    lv_label_set_text(lbl, text);
    return lbl;
}

void screen_onboarding_create(lv_obj_t *root)
{
    if (root == NULL) {
        return;
    }

    s_cont_modal = lv_obj_create(root);
    lv_obj_remove_style_all(s_cont_modal);
    lv_obj_set_size(s_cont_modal, (int32_t)APP_UI_SCREEN_WIDTH_PX, (int32_t)APP_UI_SCREEN_HEIGHT_PX);
    lv_obj_set_style_bg_color(s_cont_modal, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(s_cont_modal, LV_OPA_COVER, 0);
    lv_obj_add_flag(s_cont_modal, LV_OBJ_FLAG_HIDDEN);

    s_lbl_header = create_modal_label(s_cont_modal, h1os_font_ui_12, 6, 4, "WIFI SETUP [ AWAKE ]");
    s_lbl_line1  = create_modal_label(s_cont_modal, h1os_font_ui_12, 6, 40, "[ QR CODE PLACEHOLDER ]");
    s_lbl_line2  = create_modal_label(s_cont_modal, h1os_font_ui_12, 6, 134, "ID: HUMID1_------");
    s_lbl_line3  = create_modal_label(s_cont_modal, h1os_font_ui_16, 6, 156, "PIN: --------");
    s_lbl_footer = create_modal_label(s_cont_modal, h1os_font_ui_10, 6, 182, "SCAN QR OR ENTER PIN");
}

void screen_onboarding_show_ble_prov(const app_context_t *ctx)
{
    if (ctx == NULL || s_cont_modal == NULL) {
        return;
    }

    char qr_uri[128] = {0};
    (void)net_ble_prov_format_qr_uri(ctx, qr_uri, sizeof(qr_uri));

    char buf[48] = {0};
    lv_label_set_text(s_lbl_header, "WIFI SETUP [ AWAKE ]");
    lv_label_set_text(s_lbl_line1, "[ BLE QR: 100x100 PX ]");

    snprintf(buf, sizeof(buf), "ID: %s", ctx->attrs.device_uid);
    lv_label_set_text(s_lbl_line2, buf);

    snprintf(buf, sizeof(buf), "PIN: %s", ctx->prov_pop_pin);
    lv_label_set_text(s_lbl_line3, buf);

    lv_label_set_text(s_lbl_footer, "SCAN QR OR ENTER PIN");
}

void screen_onboarding_show_claiming(const app_context_t *ctx)
{
    if (ctx == NULL || s_cont_modal == NULL) {
        return;
    }

    char buf[48] = {0};
    lv_label_set_text(s_lbl_header, "CLAIM DEVICE [ AWAKE ]");
    lv_label_set_text(s_lbl_line1, "ENTER ON WEB DASHBOARD:");

    snprintf(buf, sizeof(buf), "ID: %s", ctx->attrs.device_uid);
    lv_label_set_text(s_lbl_line2, buf);

    snprintf(buf, sizeof(buf), "PIN: %s", ctx->claim_pin);
    lv_label_set_text(s_lbl_line3, buf);

    snprintf(buf, sizeof(buf), "TIME LEFT: %u SEC", (unsigned int)ctx->claim_rem_sec);
    lv_label_set_text(s_lbl_footer, buf);
}

void screen_onboarding_show_ota(const app_context_t *ctx, uint8_t progress_pct, const char *status_text)
{
    if (ctx == NULL || s_cont_modal == NULL) {
        return;
    }

    char buf[48] = {0};
    lv_label_set_text(s_lbl_header, "FIRMWARE UPDATE");

    snprintf(buf, sizeof(buf), "TARGET: v%s",
             (ctx->attrs.target_fw_version[0] != '\0') ? ctx->attrs.target_fw_version : APP_FW_VERSION);
    lv_label_set_text(s_lbl_line1, buf);

    snprintf(buf, sizeof(buf), "PROGRESS: %u%%", (unsigned int)progress_pct);
    lv_label_set_text(s_lbl_line2, buf);

    lv_label_set_text(s_lbl_line3, (status_text != NULL) ? status_text : "UPDATING...");
    lv_label_set_text(s_lbl_footer, "DO NOT REMOVE POWER");
}

lv_obj_t *screen_onboarding_get_container(void)
{
    return s_cont_modal;
}
