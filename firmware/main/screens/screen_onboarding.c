/**
 * @file screen_onboarding.c
 * @brief Screen 4: 1-Bit QR Code Onboarding & Device Claiming View.
 *
 * Uses integer-scaled 123x123 px QR code (3x3 px per module for Version 6 = 41x41 modules)
 * with mandatory 4-module white quiet zone for high-reliability optical camera scanning.
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#include "screen_onboarding.h"
#include <stdio.h>
#include <string.h>
#include "esp_mac.h"
#include "h1os/h1os_lvgl.h"

void screen_onboarding_render(lv_obj_t *parent, const app_context_t *ctx)
{
    if (parent == NULL || ctx == NULL) {
        return;
    }

    lv_obj_clean(parent);

    /* Onboarding always renders high-contrast Black-on-White for optical QR scanners */
    const lv_color_t bg_col = lv_color_hex(0xFFFFFF);
    const lv_color_t fg_col = lv_color_hex(0x000000);

    lv_obj_set_style_bg_color(parent, bg_col, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_remove_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    /* Top Title (y=4) */
    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_title, H1OS_FONT_14, 0);
    lv_obj_set_style_text_color(lbl_title, fg_col, 0);
    lv_label_set_text_static(lbl_title, !ctx->cfg.provisioned ? "SCAN TO SETUP WIFI" : "SCAN TO CLAIM UNIT");
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 4);

    /* Format provisioning / claiming URI */
    uint8_t mac[6] = {0};
    (void)esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char qr_uri[APP_QR_PAYLOAD_MAX_LEN];
    snprintf(qr_uri, sizeof(qr_uri),
             APP_CLAIM_BASE_URL "?d=H1-%02X%02X%02X&k=%s",
             mac[3], mac[4], mac[5], ctx->cfg.claim_secret);

#if LV_USE_QRCODE
    lv_obj_t *qr = lv_qrcode_create(parent);
    lv_qrcode_set_size(qr, APP_QR_TARGET_SIZE_PX);
    lv_qrcode_set_dark_color(qr, fg_col);
    lv_qrcode_set_light_color(qr, bg_col);
    lv_qrcode_update(qr, qr_uri, (uint32_t)strlen(qr_uri));
    lv_obj_align(qr, LV_ALIGN_TOP_MID, 0, 26);
#else
    lv_obj_t *qr_box = lv_obj_create(parent);
    lv_obj_remove_style_all(qr_box);
    lv_obj_set_size(qr_box, APP_QR_TARGET_SIZE_PX, APP_QR_TARGET_SIZE_PX);
    lv_obj_align(qr_box, LV_ALIGN_TOP_MID, 0, 26);
    lv_obj_set_style_border_width(qr_box, 2, 0);
    lv_obj_set_style_border_color(qr_box, fg_col, 0);
    lv_obj_set_style_border_opa(qr_box, LV_OPA_COVER, 0);

    lv_obj_t *lbl_qr = lv_label_create(qr_box);
    lv_obj_set_style_text_font(lbl_qr, H1OS_FONT_14, 0);
    lv_obj_set_style_text_color(lbl_qr, fg_col, 0);
    lv_label_set_text_static(lbl_qr, "BLE SETUP");
    lv_obj_center(lbl_qr);
#endif

    /* Bottom Claim Key & Temp/RH preview (y=156, y=176) */
    char key_buf[40];
    snprintf(key_buf, sizeof(key_buf), "Key: %s", ctx->cfg.claim_secret);
    lv_obj_t *lbl_key = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_key, H1OS_FONT_14, 0);
    lv_obj_set_style_text_color(lbl_key, fg_col, 0);
    lv_label_set_text(lbl_key, key_buf);
    lv_obj_align(lbl_key, LV_ALIGN_TOP_MID, 0, 155);

    char env_buf[40];
    snprintf(env_buf, sizeof(env_buf), "%.1f%% RH | %.1f°C",
             (double)ctx->telemetry.ambient_rh_pct, (double)ctx->telemetry.ambient_temp_c);
    lv_obj_t *lbl_env = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_env, H1OS_FONT_14, 0);
    lv_obj_set_style_text_color(lbl_env, fg_col, 0);
    lv_label_set_text(lbl_env, env_buf);
    lv_obj_align(lbl_env, LV_ALIGN_BOTTOM_MID, 0, -4);
}
