/**
 * @file main.c
 * @brief Humid1-OS Layer 4 Application Entrypoint & H1OS Lifecycle Orchestration.
 *
 * Connects the H1OS declarative lifecycle callbacks (on_boot, on_wake, on_pre_sleep,
 * on_shutdown) to the modular application subsystems:
 *   - app_state:      NVS persistence, RTC fast cache, dynamic Flow A/B sync
 *   - net_wifi_ble:   BLE Security 1 provisioning, fast Wi-Fi burst, SNTP RTC sync
 *   - cloud_tb:       ThingsBoard auto-provisioning, claiming, Kelvin telemetry, alarms
 *   - app_ota:        Dual-slot FOTA on wake check and rollback cancellation
 *   - screens/*:      3 user screens (Dashboard, Diagnostics, System) + 3 modal screens
 */

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_log.h"
#include "h1os/h1os.h"
#include "h1os/orch_state.h"

#include "app_config.h"
#include "app_ota.h"
#include "app_state.h"
#include "app_types.h"
#include "cloud_tb.h"
#include "net_wifi_ble.h"
#include "screens/screen_dashboard.h"
#include "screens/screen_diagnostics.h"
#include "screens/screen_onboarding.h"
#include "screens/screen_system.h"

static const char *TAG = "humid1_main";

/**
 * @brief Master statically allocated application context (Zero runtime heap per JPL Rule 3).
 */
static app_context_t s_app_ctx;

/**
 * @brief Switch active LVGL screen container visibility while LVGL lock is held.
 */
static void app_ui_show_screen(app_screen_id_t screen_id)
{
    s_app_ctx.active_screen = screen_id;

    lv_obj_t *c_dash  = screen_dashboard_get_container();
    lv_obj_t *c_diag  = screen_diagnostics_get_container();
    lv_obj_t *c_sys   = screen_system_get_container();
    lv_obj_t *c_modal = screen_onboarding_get_container();

    if (c_dash != NULL)  { lv_obj_add_flag(c_dash, LV_OBJ_FLAG_HIDDEN); }
    if (c_diag != NULL)  { lv_obj_add_flag(c_diag, LV_OBJ_FLAG_HIDDEN); }
    if (c_sys != NULL)   { lv_obj_add_flag(c_sys, LV_OBJ_FLAG_HIDDEN); }
    if (c_modal != NULL) { lv_obj_add_flag(c_modal, LV_OBJ_FLAG_HIDDEN); }

    switch (screen_id) {
        case APP_SCREEN_DASHBOARD:
            if (c_dash != NULL) { lv_obj_remove_flag(c_dash, LV_OBJ_FLAG_HIDDEN); }
            screen_dashboard_update(&s_app_ctx);
            break;
        case APP_SCREEN_DIAGNOSTICS:
            if (c_diag != NULL) { lv_obj_remove_flag(c_diag, LV_OBJ_FLAG_HIDDEN); }
            screen_diagnostics_update(&s_app_ctx);
            break;
        case APP_SCREEN_SYSTEM:
            if (c_sys != NULL) { lv_obj_remove_flag(c_sys, LV_OBJ_FLAG_HIDDEN); }
            screen_system_update(&s_app_ctx);
            break;
        case APP_SCREEN_MODAL_PROV:
            if (c_modal != NULL) { lv_obj_remove_flag(c_modal, LV_OBJ_FLAG_HIDDEN); }
            screen_onboarding_show_ble_prov(&s_app_ctx);
            break;
        case APP_SCREEN_MODAL_CLAIM:
            if (c_modal != NULL) { lv_obj_remove_flag(c_modal, LV_OBJ_FLAG_HIDDEN); }
            screen_onboarding_show_claiming(&s_app_ctx);
            break;
        case APP_SCREEN_MODAL_OTA:
            if (c_modal != NULL) { lv_obj_remove_flag(c_modal, LV_OBJ_FLAG_HIDDEN); }
            screen_onboarding_show_ota(&s_app_ctx, 0U, "CHECKING UPDATE...");
            break;
        default:
            if (c_dash != NULL) { lv_obj_remove_flag(c_dash, LV_OBJ_FLAG_HIDDEN); }
            screen_dashboard_update(&s_app_ctx);
            break;
    }
}

/**
 * @brief H1OS Cold-Boot Callback: Create all static LVGL screen trees once.
 */
static void app_on_boot(const h1os_ui_context_t *ui_ctx, void *user_data)
{
    (void)user_data;
    if (ui_ctx == NULL || ui_ctx->root_screen == NULL) {
        return;
    }

    ESP_LOGI(TAG, "on_boot: Constructing static LVGL v9 1-bit screen hierarchy");
    screen_dashboard_create(ui_ctx->root_screen);
    screen_diagnostics_create(ui_ctx->root_screen);
    screen_system_create(ui_ctx->root_screen);
    screen_onboarding_create(ui_ctx->root_screen);
}

/**
 * @brief H1OS Wake Callback: Executed on Cold Boot, RTC Flow B Wake, or Button EXT1 Wake.
 *
 * Note: H1OS holds h1os_lvgl_lock() around on_wake() prior to starting the LVGL task.
 */
static void app_on_wake(const h1os_ui_context_t *ui_ctx, void *user_data)
{
    (void)ui_ctx;
    (void)user_data;

    /* Determine whether this is an unattended RTC Flow B wake or an interactive button wake */
    const h1os_rtc_state_t *orch = orch_state_get();
    const bool is_rtc_wake = (orch != NULL && orch->last_wake_reason == H1OS_WAKE_RTC);
    s_app_ctx.is_interactive = !is_rtc_wake;

    /* Refresh latest sensor and battery metrology in Kelvin */
    (void)app_state_refresh_telemetry(&s_app_ctx);

    /* Confirm running OTA slot validity after successful sensor init */
    (void)app_ota_mark_running_slot_valid();

    /* Step 1: First-Boot BLE Wi-Fi Provisioning Gate (APP_03) */
    if (!s_app_ctx.attrs.wifi_provisioned) {
        s_app_ctx.fsm_state = APP_STATE_BLE_PROV;
        (void)net_ble_prov_start(&s_app_ctx);
        app_ui_show_screen(APP_SCREEN_MODAL_PROV);
        return;
    }

    /* Step 2: Wi-Fi Fast Burst + SNTP Wall-Clock Gate (APP_02) */
    if (net_wifi_connect_burst(&s_app_ctx, APP_NET_CONNECT_TIMEOUT_MS) == ESP_OK) {
        (void)net_sntp_ensure_valid_time(&s_app_ctx, APP_NET_CONNECT_TIMEOUT_MS);

        /* Step 3: Phase 1 ThingsBoard Auto-Provisioning if tb_access_token is empty */
        if (s_app_ctx.attrs.tb_access_token[0] == '\0') {
            s_app_ctx.fsm_state = APP_STATE_TB_AUTO_PROV;
            (void)cloud_tb_auto_provision(&s_app_ctx, APP_NET_CONNECT_TIMEOUT_MS);
        }

        /* Step 4: Phase 2 Customer Claiming if not yet claimed */
        if (!s_app_ctx.attrs.device_claimed) {
            s_app_ctx.fsm_state = APP_STATE_TB_CLAIMING;
            (void)cloud_tb_begin_claiming(&s_app_ctx);
            app_ui_show_screen(APP_SCREEN_MODAL_CLAIM);
            return;
        }

        /* Step 5: Normal Operation — Publish Kelvin Telemetry & Sync Shared Attributes */
        (void)cloud_tb_sync_burst(&s_app_ctx, APP_NET_CONNECT_TIMEOUT_MS);

        /* Step 6: Check for Pending Dual-Slot OTA Update on Wake (APP_06) */
        if (app_ota_is_update_ready(&s_app_ctx)) {
            s_app_ctx.fsm_state = APP_STATE_OTA_UPDATE;
            app_ui_show_screen(APP_SCREEN_MODAL_OTA);
            (void)app_ota_execute_update(&s_app_ctx);
            return;
        }
    }

    /* Step 7: Render Main Dashboard (with AWAKE or SLEEP mode pill) */
    s_app_ctx.fsm_state = is_rtc_wake ? APP_STATE_FLOW_B_SYNC : APP_STATE_INTERACTIVE;
    app_ui_show_screen(APP_SCREEN_DASHBOARD);
}

/**
 * @brief H1OS Pre-Sleep Callback: Ensure SLEEP badge is rendered and all radios are off.
 */
static void app_on_pre_sleep(const h1os_ui_context_t *ui_ctx, void *user_data)
{
    (void)ui_ctx;
    (void)user_data;

    ESP_LOGI(TAG, "on_pre_sleep: Transitioning UI badge to SLEEP and stopping radios");
    s_app_ctx.is_interactive = false;

    if (h1os_lvgl_lock(200)) {
        app_ui_show_screen(APP_SCREEN_DASHBOARD);
        h1os_lvgl_unlock();
    }

    (void)net_wifi_stop_burst(&s_app_ctx);
}

/**
 * @brief H1OS Shutdown Callback: Invoked prior to hardware power-latch cutoff.
 */
static void app_on_shutdown(const h1os_ui_context_t *ui_ctx, void *user_data)
{
    (void)ui_ctx;
    (void)user_data;

    ESP_LOGI(TAG, "on_shutdown: Flushing NVS state and stopping radios");
    (void)net_wifi_stop_burst(&s_app_ctx);
    (void)app_state_save_nvs(&s_app_ctx);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting %s v%s on H1OS middleware", APP_FW_TITLE, APP_FW_VERSION);

    /* Load persisted NVS configuration and initialize RTC cache prior to H1OS start */
    (void)app_state_init(&s_app_ctx);

    h1os_config_t os_cfg = H1OS_CONFIG_DEFAULT();
    os_cfg.session.flow_a_period_sec      = s_app_ctx.attrs.flow_a_period_sec;
    os_cfg.session.flow_b_period_sec      = s_app_ctx.attrs.flow_b_period_sec;
    os_cfg.session.inactivity_timeout_sec = APP_INACTIVITY_TIMEOUT_SEC;

    os_cfg.callbacks.on_boot      = app_on_boot;
    os_cfg.callbacks.on_wake      = app_on_wake;
    os_cfg.callbacks.on_pre_sleep = app_on_pre_sleep;
    os_cfg.callbacks.on_shutdown  = app_on_shutdown;
    os_cfg.callbacks.user_data    = &s_app_ctx;

    ESP_ERROR_CHECK(h1os_start(&os_cfg));
}
