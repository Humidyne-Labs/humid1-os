/**
 * @file screen_system.h
 * @brief Screen 3: System States, Active Server Alarms, Error Codes & Interactive Actions (APP_04).
 *
 * Allows the user to inspect active ThingsBoard alarms, partition slot, error codes,
 * and execute actions (Invert Theme, Clear Wi-Fi, Full Factory Reset, Power Off).
 */

#ifndef HUMID1_SCREEN_SYSTEM_H
#define HUMID1_SCREEN_SYSTEM_H

#include "lvgl.h"
#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Selectable action items on Screen 3 cycled via POWER (GPIO 18) short tap
 *        and executed via POWER (GPIO 18) 2s-3s hold.
 */
typedef enum {
    APP_SYS_ACTION_INVERT_THEME = 0, /**< Toggle Light / Dark e-Paper theme */
    APP_SYS_ACTION_CLEAR_WIFI   = 1, /**< Clear Wi-Fi credentials and reboot to BLE setup */
    APP_SYS_ACTION_FACTORY_RST  = 2, /**< Clear Wi-Fi + Claim token and reboot */
    APP_SYS_ACTION_POWER_OFF    = 3, /**< Enter hardware storage shutdown (h1os_power_off) */
    APP_SYS_ACTION_COUNT        = 4
} app_sys_action_t;

/**
 * @brief Build the static LVGL v9 widget tree for Screen 3 (System, Alarms & Actions).
 *
 * @param[in] root Parent LVGL screen object (200x200).
 */
void screen_system_create(lv_obj_t *root);

/**
 * @brief Update Screen 3 labels with active alarm details, firmware slot, error code,
 *        and highlighted action item.
 *
 * @param[in] ctx Pointer to the master application context.
 */
void screen_system_update(const app_context_t *ctx);

/**
 * @brief Return the root container object of Screen 3 for visibility switching.
 *
 * @return Pointer to the Screen 3 LVGL container.
 */
lv_obj_t *screen_system_get_container(void);

#ifdef __cplusplus
}
#endif

#endif /* HUMID1_SCREEN_SYSTEM_H */
