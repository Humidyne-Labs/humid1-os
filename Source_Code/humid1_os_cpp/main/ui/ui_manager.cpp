/**
 * @file ui_manager.cpp
 * @brief LVGL 9 Display UI Manager Implementation
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


static const char *TAG = "ui_manager";

esp_err_t ui_manager_init(void)
{
    // LVGL is already initialized and started on Core 1 by bsp_init_mode / bsp_app_start
    ESP_LOGI(TAG, "UI Manager ready (LVGL managed by BSP)");
    return ESP_OK;
}


void ui_show_shutdown_screen(void)
{
    ESP_LOGI(TAG, "Rendering Space Cat Shutdown Screen (Persistent EPD)...");

    // Mount MMAP flash asset drive 'S:'
    bsp_assets_init("storage", 'S', 5, 0);

    // Use full OTP waveform refresh mode so the image persists sharply in zero-power state
    bsp_lvgl_set_first_flush_mode(true);

    bsp_lvgl_lock();

    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);

    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *img = lv_image_create(scr);
    lv_image_set_src(img, "S:space_cat.png");
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

    // Force LVGL to render immediately and flush to display
    lv_refr_now(NULL);

    bsp_lvgl_unlock();

    // Wait for e-Paper panel to complete full hardware refresh before power rail drops
    bsp_display_wait_busy(10000);
}

void ui_show_message(const char *title, const char *subtitle)
{
    bsp_lvgl_lock();

    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);

    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *card = lv_obj_create(scr);
    lv_obj_set_size(card, 184, 140);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(card, lv_color_white(), 0);
    lv_obj_set_style_border_color(card, lv_color_black(), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 6, 0);
    lv_obj_set_style_pad_all(card, 8, 0);

    lv_obj_t *lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, title ? title : "NOTIFICATION");
    lv_obj_set_style_text_color(lbl_title, lv_color_black(), 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 8);

    lv_obj_t *lbl_sub = lv_label_create(card);
    lv_label_set_text(lbl_sub, subtitle ? subtitle : "");
    lv_obj_set_style_text_color(lbl_sub, lv_color_black(), 0);
    lv_obj_align(lbl_sub, LV_ALIGN_CENTER, 0, 10);

    bsp_lvgl_unlock();
}

