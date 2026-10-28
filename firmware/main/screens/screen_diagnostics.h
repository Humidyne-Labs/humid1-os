/**
 * @file screen_diagnostics.h
 * @brief Screen 2: Tri-Node Metrology, Psychrometrics & Network/Autonomy Diagnostics (APP_04).
 */

#ifndef HUMID1_SCREEN_DIAGNOSTICS_H
#define HUMID1_SCREEN_DIAGNOSTICS_H

#include "lvgl.h"
#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Build the static LVGL v9 widget tree for Screen 2 (Diagnostics).
 *
 * @param[in] root Parent LVGL screen object (200x200).
 */
void screen_diagnostics_create(lv_obj_t *root);

/**
 * @brief Update Screen 2 labels with Tri-Node temperatures, psychrometric vapor pressure,
 *        battery autonomy days, and dynamic Flow A/B wake intervals.
 *
 * @param[in] ctx Pointer to the master application context.
 */
void screen_diagnostics_update(const app_context_t *ctx);

/**
 * @brief Return the root container object of Screen 2 for visibility switching.
 *
 * @return Pointer to the Screen 2 LVGL container.
 */
lv_obj_t *screen_diagnostics_get_container(void);

#ifdef __cplusplus
}
#endif

#endif /* HUMID1_SCREEN_DIAGNOSTICS_H */
