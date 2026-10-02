/**
 * @file ui_manager.h
 * @brief LVGL 9 Display UI Manager & Screen Transitions
 * 
 * Hardware Target:
 *  - Waveshare ESP32-S3 ePaper 1.54" V2 (200x200 1-bit Monochrome EPD)
 * 
 * Screens:
 *  1. Provisioning Screen: Header + 116x116 QR Code + PoP PIN & Device Name.
 *  2. Claiming Screen: Header + 6-Char Dashboard Claiming PIN + Status Hint.
 *  3. Dashboard Screen: Clock, RH%, Temp (F/C/K), Battery %, RSSI, Card Inversion.
 *  4. OTA Update Screen: Header + Progress Bar / Percentage + Status Hint.
 *  5. System Message Screen: Status notifications & Factory Reset alert.
 * 
 * @attribution
 * - Humidyne Labs / Humiditron (2026)
 * 
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "lvgl.h"
#include "telemetry/sensor_reader.h"
#include "app_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LVGL 9 Subsystem & Display Port
 */
esp_err_t ui_manager_init(void);

/**
 * @brief Display BLE Wi-Fi Provisioning Greeting Screen (QR Code + PIN)
 */
void ui_show_provisioning_screen(const char *service_name, const char *pop_pin);

/**
 * @brief Display ThingsBoard Device Claiming Screen
 */
void ui_show_claiming_screen(const char *device_name, const char *claim_token, uint32_t remaining_sec);

/**
 * @brief Display Active Environmental Telemetry Dashboard Screen
 */
void ui_show_dashboard_screen(const app_telemetry_data_t *telemetry, app_temp_unit_t unit, bool alarm_active);

/**
 * @brief Display OTA Firmware Update Progress Screen
 * 
 * @param progress_pct Download/flash percentage (0 to 100)
 * @param msg Status text (e.g. "Downloading...", "Flashing...", "Success")
 */
void ui_show_ota_screen(int progress_pct, const char *msg);

/**
 * @brief Display Space Cat Shutdown Splash Screen on Power Off
 */
void ui_show_shutdown_screen(void);

/**
 * @brief Display Full-Screen Status / Notification Message
 */
void ui_show_message(const char *title, const char *subtitle);

/**
 * @brief Create an Inverted Text Label (Black background container, white text)
 * 
 * @param parent Parent LVGL object (or NULL for active screen)
 * @param text Text to display
 * @param width Container width in pixels (or LV_SIZE_CONTENT)
 * @param radius Corner radius in pixels
 * @return lv_obj_t* Pointer to container object
 */
lv_obj_t *ui_create_inverted_label(lv_obj_t *parent, const char *text, int32_t width, int32_t radius);

/**
 * @brief Create an Inverted Card Container (Black background, rounded corners)
 * 
 * @param parent Parent LVGL object (or NULL for active screen)
 * @param width Card width in pixels
 * @param height Card height in pixels
 * @param radius Corner radius in pixels
 * @return lv_obj_t* Pointer to card container object
 */
lv_obj_t *ui_create_inverted_card(lv_obj_t *parent, int32_t width, int32_t height, int32_t radius);

/**
 * @brief Display Inversion UI Demo Screen (Component-Based Inversion test)
 */
void ui_show_inversion_demo_screen(void);

/**
 * @brief Get shared inverted background style instance (Black BG)
 */
lv_style_t *ui_style_get_inverted_bg(void);

/**
 * @brief Get shared inverted text style instance (White Text)
 */
lv_style_t *ui_style_get_inverted_text(void);

/**
 * @brief Apply standard inverted LVGL style to an object or label
 */
void ui_apply_inverted_style(lv_obj_t *obj);

/**
 * @brief Set full screen background inversion (Black vs White)
 */
void ui_set_screen_inverted(lv_obj_t *scr, bool inverted);

/**
 * @brief Display Inversion UI Demo Screen (Standard LVGL Style/Theme Inversion test)
 */
void ui_show_inversion_demo_normal_screen(void);

/**
 * @brief Attach state-based inversion styles (LV_STATE_DEFAULT = Normal, LV_STATE_CHECKED = Inverted)
 */
void ui_apply_state_inversion_styles(lv_obj_t *obj);

/**
 * @brief Toggle LV_STATE_CHECKED state on an object to flip inversion
 */
void ui_toggle_widget_inversion(lv_obj_t *obj);

/**
 * @brief Direct local style inversion (sets bg_opa=LV_OPA_COVER, bg_color, and text_color)
 */
void ui_invert_label_direct(lv_obj_t *obj, bool inverted);

/**
 * @brief Display Inversion UI Demo Screen (State-Based LV_STATE_CHECKED test)
 */
void ui_show_inversion_demo_state_screen(void);

#ifdef __cplusplus
}
#endif





