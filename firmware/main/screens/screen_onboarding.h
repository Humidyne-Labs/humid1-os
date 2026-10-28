/**
 * @file screen_onboarding.h
 * @brief Modal Lifecycle Screens: BLE Provisioning (QR + PIN + UID), Customer Claiming
 *        (PIN + UID + 180s Countdown), and OTA Firmware Update Progress (APP_03, APP_04, APP_06).
 */

#ifndef HUMID1_SCREEN_ONBOARDING_H
#define HUMID1_SCREEN_ONBOARDING_H

#include <stdint.h>
#include "lvgl.h"
#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Build the static LVGL v9 container for lifecycle modal screens
 *        (Provisioning, Claiming, and OTA Progress).
 *
 * @param[in] root Parent LVGL screen object (200x200).
 */
void screen_onboarding_create(lv_obj_t *root);

/**
 * @brief Render the BLE Wi-Fi Provisioning screen showing simultaneous QR code,
 *        8-digit TRNG Proof-of-Possession PIN, and HUMID1_<MAC> Device ID (APP_03).
 *
 * @param[in] ctx Pointer to the master application context.
 */
void screen_onboarding_show_ble_prov(const app_context_t *ctx);

/**
 * @brief Render the ThingsBoard Customer Claiming screen showing the 6-character
 *        TRNG Claim PIN, HUMID1_<MAC> Device Name, and 180s countdown timer (APP_02).
 *
 * @param[in] ctx Pointer to the master application context.
 */
void screen_onboarding_show_claiming(const app_context_t *ctx);

/**
 * @brief Render the Dual-Slot OTA Firmware Update progress screen (APP_06).
 *
 * @param[in] ctx          Pointer to the master application context.
 * @param[in] progress_pct Download completion percentage (0..100%).
 * @param[in] status_text  Short status string ("DOWNLOADING", "VERIFYING", "REBOOTING").
 */
void screen_onboarding_show_ota(const app_context_t *ctx, uint8_t progress_pct, const char *status_text);

/**
 * @brief Return the root container object of the modal onboarding screen.
 *
 * @return Pointer to the modal LVGL container.
 */
lv_obj_t *screen_onboarding_get_container(void);

#ifdef __cplusplus
}
#endif

#endif /* HUMID1_SCREEN_ONBOARDING_H */
