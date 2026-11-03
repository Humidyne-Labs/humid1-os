/**
 * @file screen_system.c
 * @brief Screen 3: Interactive 2-Button System Controls & Safe Storage Shutdown.
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#include "screen_system.h"
#include <stdio.h>
#include "h1os/h1os_lvgl.h"

void screen_system_render(lv_obj_t *parent, const app_context_t *ctx)
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

    /* Inverted Header Bar */
    lv_obj_t *hdr = lv_obj_create(parent);
    lv_obj_remove_style_all(hdr);
    lv_obj_set_size(hdr, 200, 22);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_bg_color(hdr, fg_col, 0);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);

    lv_obj_t *lbl_title = lv_label_create(hdr);
    lv_obj_set_style_text_font(lbl_title, H1OS_FONT_14, 0);
    lv_obj_set_style_text_color(lbl_title, bg_col, 0);
    lv_label_set_text_static(lbl_title, "SYSTEM MENU [3/3]");
    lv_obj_center(lbl_title);

    /* Version Info Row (y=26) */
    char ver_buf[48];
    snprintf(ver_buf, sizeof(ver_buf), "FW: v%s", APP_FW_VERSION);
    lv_obj_t *lbl_ver = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_ver, H1OS_FONT_14, 0);
    lv_obj_set_style_text_color(lbl_ver, fg_col, 0);
    lv_label_set_text(lbl_ver, ver_buf);
    lv_obj_set_pos(lbl_ver, 6, 26);

    /* 4 Selectable Action Rows (y=48..164) */
    char item0[32];
    char item1[32];
    snprintf(item0, sizeof(item0), "1. THEME: %s", dark ? "DARK" : "LIGHT");
    snprintf(item1, sizeof(item1), "2. AUDIO: %s", ctx->cfg.sound_enabled ? "ON" : "MUTE");
    const char *items[APP_SYSTEM_MENU_ITEM_COUNT] = {
        item0,
        item1,
        "3. RESET CLOUD PAIR",
        "4. STORAGE POWER OFF"
    };

    for (uint8_t i = 0; i < APP_SYSTEM_MENU_ITEM_COUNT; i++) {
        bool selected = (ctx->system_menu_index == i);
        lv_obj_t *row = lv_obj_create(parent);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, 188, 26);
        lv_obj_set_pos(row, 6, (lv_coord_t)(48 + (i * 30)));
        lv_obj_set_style_border_width(row, 2, 0);
        lv_obj_set_style_border_color(row, fg_col, 0);
        lv_obj_set_style_border_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(row, selected ? fg_col : bg_col, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);

        lv_obj_t *lbl = lv_label_create(row);
        lv_obj_set_style_text_font(lbl, H1OS_FONT_14, 0);
        lv_obj_set_style_text_color(lbl, selected ? bg_col : fg_col, 0);
        lv_label_set_text(lbl, items[i]);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 6, 0);
    }

    /* Footer Hint (y=176) */
    lv_obj_t *lbl_hint = lv_label_create(parent);
    lv_obj_set_style_text_font(lbl_hint, H1OS_FONT_14, 0);
    lv_obj_set_style_text_color(lbl_hint, fg_col, 0);
    lv_label_set_text_static(lbl_hint, "PWR:Sel HOLD:Exec");
    lv_obj_align(lbl_hint, LV_ALIGN_BOTTOM_MID, 0, -4);
}
