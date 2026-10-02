/**
 * @file screen_inversion.cpp
 * @brief LVGL 9 Inverted UI Components, Dynamic Threshold Sweep & Demo Screens for EPD
 * 
 * Hardware Target:
 *  - Waveshare ESP32-S3 ePaper 1.54" V2 (200x200 1-bit Mono EPD)
 * 
 * Implements 4 distinct inversion paradigms + dynamic runtime I1 luminance threshold sweep:
 *  1. Custom Component Bounding Box: Wraps text/elements in black container boxes.
 *  2. Standard LVGL Style/Theme Inversion: Applies full-screen black/white dark mode styles.
 *  3. State-Based Inversion (LV_STATE_CHECKED): Toggles normal vs. inverted via state.
 *  4. Direct Local Style Modification: Sets bg_opa=LV_OPA_COVER + bg_color/text_color directly.
 *  5. Dynamic Runtime Threshold Sweep: Mutates g_i1_lum_threshold (0 to 255 in steps of 32).
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

static const char *TAG = "screen_inversion";

// Reusable LVGL style instances
static lv_style_t s_style_inv_bg;
static lv_style_t s_style_inv_text;
static lv_style_t s_style_state_normal;
static lv_style_t s_style_state_inverted;
static bool s_styles_initialized = false;

static void init_inversion_styles_if_needed(void)
{
    if (s_styles_initialized) return;

    // Background style (Black background, 100% opacity)
    lv_style_init(&s_style_inv_bg);
    lv_style_set_bg_color(&s_style_inv_bg, lv_color_black());
    lv_style_set_bg_opa(&s_style_inv_bg, LV_OPA_COVER);

    // Text style (White text color)
    lv_style_init(&s_style_inv_text);
    lv_style_set_text_color(&s_style_inv_text, lv_color_white());

    // State-based Normal Style (White BG, Black Text)
    lv_style_init(&s_style_state_normal);
    lv_style_set_bg_opa(&s_style_state_normal, LV_OPA_COVER);
    lv_style_set_bg_color(&s_style_state_normal, lv_color_white());
    lv_style_set_text_color(&s_style_state_normal, lv_color_black());
    lv_style_set_pad_all(&s_style_state_normal, 6);
    lv_style_set_radius(&s_style_state_normal, 4);

    // State-based Inverted Style (Black BG, White Text for LV_STATE_CHECKED)
    lv_style_init(&s_style_state_inverted);
    lv_style_set_bg_opa(&s_style_state_inverted, LV_OPA_COVER);
    lv_style_set_bg_color(&s_style_state_inverted, lv_color_black());
    lv_style_set_text_color(&s_style_state_inverted, lv_color_white());

    s_styles_initialized = true;
}

/* =========================================================================
 * 1. Component / Box Bounding Inversion Helpers
 * ========================================================================= */

lv_obj_t *ui_create_inverted_label(lv_obj_t *parent, const char *text, int32_t width, int32_t radius)
{
    const int32_t MAX_DISP_W = 200;
    const int32_t MAX_DISP_H = 200;
    const int32_t pad_h = 10;
    const int32_t pad_v = 5;

    lv_obj_t *box = lv_obj_create(parent ? parent : lv_screen_active());
    if (!box) return NULL;
    lv_obj_remove_style_all(box);
    lv_obj_set_style_bg_color(box, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(box, radius, 0);

    lv_obj_set_style_pad_left(box, pad_h, 0);
    lv_obj_set_style_pad_right(box, pad_h, 0);
    lv_obj_set_style_pad_top(box, pad_v, 0);
    lv_obj_set_style_pad_bottom(box, pad_v, 0);

    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *label = lv_label_create(box);
    if (!label) {
        lv_obj_delete(box);
        return NULL;
    }
    lv_obj_set_style_text_color(label, lv_color_white(), 0);

    #if defined(LV_FONT_MONTSERRAT_14_BOLD)
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14_bold, 0);
    #endif

    if (text) {
        lv_label_set_text(label, text);
    }

    if (LV_COORD_IS_PX(width) && width > (pad_h * 2)) {
        int32_t clamped_w = (width > MAX_DISP_W) ? MAX_DISP_W : width;
        int32_t content_w = clamped_w - (pad_h * 2);
        lv_obj_set_width(label, content_w);
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(box, clamped_w);
    }

    lv_obj_update_layout(label);
    int32_t label_w = lv_obj_get_width(label);
    int32_t label_h = lv_obj_get_height(label);

    int32_t box_w = (!LV_COORD_IS_PX(width) || width <= 0) ? (label_w + (pad_h * 2)) : lv_obj_get_width(box);
    if (box_w > MAX_DISP_W) box_w = MAX_DISP_W;

    int32_t box_h = label_h + (pad_v * 2);
    if (box_h > MAX_DISP_H) box_h = MAX_DISP_H;

    lv_obj_set_width(box, box_w);
    lv_obj_set_height(box, box_h);

    return box;
}

lv_obj_t *ui_create_inverted_card(lv_obj_t *parent, int32_t width, int32_t height, int32_t radius)
{
    lv_obj_t *card = lv_obj_create(parent ? parent : lv_screen_active());
    if (!card) return NULL;

    lv_obj_remove_style_all(card);
    lv_obj_set_style_bg_color(card, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, radius, 0);
    lv_obj_set_style_pad_all(card, 6, 0);

    if (LV_COORD_IS_PX(width) && width > 0) {
        lv_obj_set_width(card, (width > 200) ? 200 : width);
    }
    if (LV_COORD_IS_PX(height) && height > 0) {
        lv_obj_set_height(card, (height > 200) ? 200 : height);
    }

    return card;
}

void ui_show_inversion_demo_screen(void)
{
    ESP_LOGI(TAG, "Rendering Component-Based Inversion Demo Screen...");

    bsp_lvgl_lock();

    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);

    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *banner = ui_create_inverted_label(scr, "INVERTED DISPLAY MODE", 186, 0);
    if (banner) lv_obj_align(banner, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *card = ui_create_inverted_card(scr, 180, 86, 8);
    if (card) {
        lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 44);

        lv_obj_t *lbl_title = lv_label_create(card);
        lv_label_set_text(lbl_title, "DARK MODE CARD");
        lv_obj_set_style_text_color(lbl_title, lv_color_white(), 0);
        lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 4);

        lv_obj_t *lbl_val = lv_label_create(card);
        lv_label_set_text(lbl_val, "68.5 %RH | 72.4 F");
        lv_obj_set_style_text_color(lbl_val, lv_color_white(), 0);
        lv_obj_align(lbl_val, LV_ALIGN_CENTER, 0, 8);
    }

    lv_obj_t *badge = ui_create_inverted_label(scr, "EPD INVERSION DEMO", LV_SIZE_CONTENT, 12);
    if (badge) lv_obj_align(badge, LV_ALIGN_TOP_MID, 0, 142);

    bsp_lvgl_unlock();
}

/* =========================================================================
 * 2. Standard LVGL Style / Theme Inversion ("Normal" Way)
 * ========================================================================= */

lv_style_t *ui_style_get_inverted_bg(void)
{
    init_inversion_styles_if_needed();
    return &s_style_inv_bg;
}

lv_style_t *ui_style_get_inverted_text(void)
{
    init_inversion_styles_if_needed();
    return &s_style_inv_text;
}

void ui_apply_inverted_style(lv_obj_t *obj)
{
    if (!obj) return;
    init_inversion_styles_if_needed();
    lv_obj_add_style(obj, &s_style_inv_bg, 0);

    if (lv_obj_check_type(obj, &lv_label_class)) {
        lv_obj_add_style(obj, &s_style_inv_text, 0);
    }
}

void ui_set_screen_inverted(lv_obj_t *scr, bool inverted)
{
    if (!scr) scr = lv_screen_active();
    init_inversion_styles_if_needed();

    if (inverted) {
        lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    }
}

void ui_show_inversion_demo_normal_screen(void)
{
    ESP_LOGI(TAG, "Rendering Standard LVGL Style Inversion Demo Screen...");

    bsp_lvgl_lock();

    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);

    ui_set_screen_inverted(scr, true);

    lv_obj_t *lbl_header = lv_label_create(scr);
    lv_label_set_text(lbl_header, "LVGL NORMAL INVERSION");
    lv_obj_add_style(lbl_header, ui_style_get_inverted_text(), 0);
    lv_obj_align(lbl_header, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *card = lv_obj_create(scr);
    lv_obj_set_size(card, 180, 96);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 36);
    lv_obj_add_style(card, ui_style_get_inverted_bg(), 0);
    lv_obj_set_style_border_color(card, lv_color_white(), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 8, 0);

    lv_obj_t *lbl_card_title = lv_label_create(card);
    lv_label_set_text(lbl_card_title, "STANDARD LVGL DARK MODE");
    lv_obj_add_style(lbl_card_title, ui_style_get_inverted_text(), 0);
    lv_obj_align(lbl_card_title, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *lbl_val = lv_label_create(card);
    lv_label_set_text(lbl_val, "Humidity: 45.2 %\nTemp: 23.1 C");
    lv_obj_add_style(lbl_val, ui_style_get_inverted_text(), 0);
    lv_obj_align(lbl_val, LV_ALIGN_CENTER, 0, 10);

    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_set_size(btn, 140, 32);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 146);
    lv_obj_set_style_bg_color(btn, lv_color_white(), 0);
    lv_obj_set_style_radius(btn, 16, 0);

    lv_obj_t *lbl_btn = lv_label_create(btn);
    lv_label_set_text(lbl_btn, "TOGGLE MODE");
    lv_obj_set_style_text_color(lbl_btn, lv_color_black(), 0);
    lv_obj_center(lbl_btn);

    bsp_lvgl_unlock();
}

/* =========================================================================
 * 3. State-Based Inversion (LV_STATE_CHECKED) & Direct Method Helpers
 * ========================================================================= */

void ui_apply_state_inversion_styles(lv_obj_t *obj)
{
    if (!obj) return;
    init_inversion_styles_if_needed();
    lv_obj_add_style(obj, &s_style_state_normal, LV_STATE_DEFAULT);
    lv_obj_add_style(obj, &s_style_state_inverted, LV_STATE_CHECKED);
}

void ui_toggle_widget_inversion(lv_obj_t *obj)
{
    if (!obj) return;
    if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
        lv_obj_remove_state(obj, LV_STATE_CHECKED);
    } else {
        lv_obj_add_state(obj, LV_STATE_CHECKED);
    }
}

void ui_invert_label_direct(lv_obj_t *obj, bool inverted)
{
    if (!obj) return;
    if (inverted) {
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(obj, lv_color_black(), 0);
        lv_obj_set_style_text_color(obj, lv_color_white(), 0);
    } else {
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(obj, lv_color_white(), 0);
        lv_obj_set_style_text_color(obj, lv_color_black(), 0);
    }
}

void ui_show_inversion_demo_state_screen(void)
{
    ESP_LOGI(TAG, "Rendering State-Based Inversion Demo Screen...");

    bsp_lvgl_lock();

    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);

    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);

    lv_obj_t *lbl_norm = lv_label_create(scr);
    lv_label_set_text(lbl_norm, "DEFAULT STATE (NORMAL)");
    ui_apply_state_inversion_styles(lbl_norm);
    lv_obj_align(lbl_norm, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t *lbl_inv = lv_label_create(scr);
    lv_label_set_text(lbl_inv, "CHECKED STATE (INVERTED)");
    ui_apply_state_inversion_styles(lbl_inv);
    lv_obj_add_state(lbl_inv, LV_STATE_CHECKED);
    lv_obj_align(lbl_inv, LV_ALIGN_TOP_MID, 0, 65);

    lv_obj_t *lbl_direct = lv_label_create(scr);
    lv_label_set_text(lbl_direct, "DIRECT LOCAL INVERSION");
    ui_invert_label_direct(lbl_direct, true);
    lv_obj_set_style_pad_all(lbl_direct, 6, 0);
    lv_obj_set_style_radius(lbl_direct, 6, 0);
    lv_obj_align(lbl_direct, LV_ALIGN_TOP_MID, 0, 115);

    bsp_lvgl_unlock();
}

