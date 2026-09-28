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

#ifdef __cplusplus
}
#endif

