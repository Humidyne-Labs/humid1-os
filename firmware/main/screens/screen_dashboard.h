/**
 * @file screen_dashboard.h
 * @brief Screen 1: Main Climate & Connection Dashboard (200x200 1-Bit Monochrome LVGL v9).
 *
 * Displays connection status, nickname/UID, AWAKE/SLEEP mode indicator, RH%,
 * Ambient Temp (converted from Kelvin to F/C/K), Dew Point, and Battery SOC%/mV (APP_04).
 */

#ifndef HUMID1_SCREEN_DASHBOARD_H
#define HUMID1_SCREEN_DASHBOARD_H

#include "lvgl.h"
#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Build the static LVGL v9 widget tree for Screen 1 (Main Dashboard).
 *
 * Must be called once during on_boot() while the LVGL lock is held.
 *
 * @param[in] root Parent LVGL screen object (200x200).
 */
void screen_dashboard_create(lv_obj_t *root);

/**
 * @brief Populate Screen 1 labels with the latest telemetry (in Kelvin -> F/C/K),
 *        connection state, alarm banner, and AWAKE/SLEEP mode badge.
 *
 * Must be called with h1os_lvgl_lock() held.
 *
 * @param[in] ctx Pointer to the master application context.
 */
void screen_dashboard_update(const app_context_t *ctx);

/**
 * @brief Return the root container object of Screen 1 for visibility switching.
 *
 * @return Pointer to the Screen 1 LVGL container.
 */
lv_obj_t *screen_dashboard_get_container(void);

#ifdef __cplusplus
}
#endif

#endif /* HUMID1_SCREEN_DASHBOARD_H */
