/**
 * @file main.c
 * @brief humid1-os Layer 4 Entry Point & Declarative H1OS v2.4.9-dev Lifecycle Orchestration.
 *
 * Implements the parallel Dual-Core Flow B architecture:
 *   - Core 0: app_on_boot() / app_on_wake() state preparation, followed by parallel
 *             app_on_sync() Wi-Fi/SNTP/MQTTS telemetry burst while Core 1 flushes LVGL.
 *   - Core 1: Dedicated LVGL v9 1-bit rendering & SSD1681 display task, receiving
 *             cross-core UI mutations exclusively via h1os_ui_post().
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "h1os/h1os.h"
#include "h1os/h1os_lvgl.h"

#include "app_config.h"
#include "app_types.h"
#include "app_state.h"
#include "net_wifi_ble.h"
#include "cloud_tb.h"
#include "app_ota.h"

#include "screens/screen_dashboard.h"
#include "screens/screen_diagnostics.h"
#include "screens/screen_system.h"
#include "screens/screen_onboarding.h"

#include "esp_timer.h"

static const char *TAG = "humid1_main";

/* Master static application context in .bss (zero heap allocation) */
static app_context_t s_app_ctx;
static esp_timer_handle_t s_onboarding_keepalive_timer = NULL;

/**
 * @brief Dedicated Initial Setup keep-alive callback.
 *
 * Prevents unintentional 30-second H1OS inactivity sleep from interrupting BLE/SoftAP
 * credential transfer or ThingsBoard claiming for up to APP_ONBOARDING_HOLD_TIMEOUT_SEC (300s).
 */
static void app_onboarding_keepalive_cb(void *arg)
{
    app_context_t *app = (app_context_t *)arg;
    if (app == NULL || !app->in_onboarding_mode) {
        return;
    }

    if (app->cfg.provisioned && app->cfg.claimed) {
        app->in_onboarding_mode = false;
        if (s_onboarding_keepalive_timer != NULL) {
            (void)esp_timer_stop(s_onboarding_keepalive_timer);
        }
        h1os_sync_notify_complete();
        return;
    }

    app->onboarding_elapsed_sec += APP_ONBOARDING_KEEPALIVE_SEC;
    if (app->onboarding_elapsed_sec < APP_ONBOARDING_HOLD_TIMEOUT_SEC) {
        h1os_reset_session_timer();
        ESP_LOGI(TAG, "[Setup Mode] Holding session awake (%lu / %u s)...",
                 (unsigned long)app->onboarding_elapsed_sec, APP_ONBOARDING_HOLD_TIMEOUT_SEC);
    } else {
        ESP_LOGW(TAG, "[Setup Mode] Max setup hold (%u s) elapsed; releasing sleep barrier.",
                 APP_ONBOARDING_HOLD_TIMEOUT_SEC);
        app->in_onboarding_mode = false;
        if (s_onboarding_keepalive_timer != NULL) {
            (void)esp_timer_stop(s_onboarding_keepalive_timer);
        }
        h1os_sync_notify_complete();
    }
}

static void app_enter_dedicated_onboarding_mode(app_context_t *app)
{
    if (app == NULL || !app->is_interactive_wake) {
        return;
    }
    app->in_onboarding_mode     = true;
    app->onboarding_elapsed_sec = 0U;
    h1os_sync_hold();
    h1os_reset_session_timer();

    if (s_onboarding_keepalive_timer == NULL) {
        const esp_timer_create_args_t t_args = {
            .callback = &app_onboarding_keepalive_cb,
            .arg      = app,
            .name     = "onboard_hold",
        };
        (void)esp_timer_create(&t_args, &s_onboarding_keepalive_timer);
    }
    if (s_onboarding_keepalive_timer != NULL) {
        (void)esp_timer_stop(s_onboarding_keepalive_timer);
        (void)esp_timer_start_periodic(s_onboarding_keepalive_timer,
                                       (uint64_t)APP_ONBOARDING_KEEPALIVE_SEC * 1000000ULL);
    }
    ESP_LOGI(TAG, "Entered dedicated Initial Setup mode (inactivity sleep inhibited for up to %u s)",
             APP_ONBOARDING_HOLD_TIMEOUT_SEC);
}


static void app_render_active_screen(app_context_t *app)
{
    if (app == NULL || app->root_screen == NULL) {
        return;
    }

    switch (app->active_screen) {
        case SCREEN_ONBOARDING:
            screen_onboarding_render(app->root_screen, app);
            break;
        case SCREEN_DIAGNOSTICS:
            screen_diagnostics_render(app->root_screen, app);
            break;
        case SCREEN_SYSTEM:
            screen_system_render(app->root_screen, app);
            break;
        case SCREEN_DASHBOARD:
        default:
            screen_dashboard_render(app->root_screen, app);
            break;
    }
}

static void app_play_tactile_chirp(const app_context_t *app)
{
    if (app != NULL && app->is_interactive_wake && app->cfg.sound_enabled) {
        (void)h1os_audio_play_tone(APP_AUDIO_BUTTON_CHIRP_HZ, APP_AUDIO_BUTTON_CHIRP_MS);
    }
}

/**
 * @brief LVGL keypad event dispatcher executed on Core 1 inside the LVGL context.
 *
 * Maps H1OS hardware keypad events:
 *   - BOOT (GPIO 0) short tap   -> LV_EVENT_KEY (LV_KEY_RIGHT): Cycle Screen
 *   - POWER (GPIO 18) short tap -> LV_EVENT_SHORT_CLICKED: Screen Context Action
 *   - POWER (GPIO 18) 2s hold   -> LV_EVENT_LONG_PRESSED: Execute System Action / Shutdown
 */
static void app_keypad_event_cb(lv_event_t *e)
{
    app_context_t *app = (app_context_t *)lv_event_get_user_data(e);
    if (app == NULL) {
        return;
    }

    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if (key == LV_KEY_RIGHT || key == LV_KEY_NEXT) {
            h1os_reset_session_timer();
            app_play_tactile_chirp(app);

            if (!app->cfg.provisioned || !app->cfg.claimed) {
                /* Toggle between Onboarding QR and live Dashboard preview */
                app->active_screen = (app->active_screen == SCREEN_ONBOARDING)
                                         ? SCREEN_DASHBOARD
                                         : SCREEN_ONBOARDING;
            } else {
                if (app->active_screen == SCREEN_DASHBOARD) {
                    app->active_screen = SCREEN_DIAGNOSTICS;
                } else if (app->active_screen == SCREEN_DIAGNOSTICS) {
                    app->active_screen = SCREEN_SYSTEM;
                } else {
                    app->active_screen = SCREEN_DASHBOARD;
                }
            }
            app_render_active_screen(app);
        }
    } else if (code == LV_EVENT_SHORT_CLICKED) {
        h1os_reset_session_timer();
        app_play_tactile_chirp(app);

        if (app->active_screen == SCREEN_DASHBOARD) {
            /* Cycle temperature display unit (C -> F -> K -> C) */
            app->cfg.temp_unit = (app_temp_unit_t)((app->cfg.temp_unit + 1U) % 3U);
            (void)app_state_save_config(app);
            app_render_active_screen(app);
        } else if (app->active_screen == SCREEN_DIAGNOSTICS) {
            /* Request full anti-ghosting waveform scrub and refresh */
            h1os_lvgl_request_full_refresh();
            (void)app_state_refresh_telemetry(app);
            app_render_active_screen(app);
        } else if (app->active_screen == SCREEN_SYSTEM) {
            /* Advance highlighted menu row */
            app->system_menu_index = (uint8_t)((app->system_menu_index + 1U) % APP_SYSTEM_MENU_ITEM_COUNT);
            app_render_active_screen(app);
        }
    } else if (code == LV_EVENT_LONG_PRESSED) {
        h1os_reset_session_timer();

        if (app->active_screen == SCREEN_SYSTEM) {
            if (app->cfg.sound_enabled) {
                (void)h1os_audio_play_tone(APP_AUDIO_CONFIRM_TONE_HZ, APP_AUDIO_CONFIRM_TONE_MS);
            }
            switch (app->system_menu_index) {
                case 0: /* Invert Display Theme (Light <-> Dark) with full waveform scrub */
                    app->cfg.theme = (app->cfg.theme == APP_THEME_LIGHT) ? APP_THEME_DARK : APP_THEME_LIGHT;
                    (void)app_state_save_config(app);
                    h1os_lvgl_request_full_refresh();
                    app_render_active_screen(app);
                    break;
                case 1: /* Toggle Audio Mute */
                    app->cfg.sound_enabled = !app->cfg.sound_enabled;
                    (void)h1os_set_volume(app->cfg.sound_enabled ? app->cfg.volume_pct : 0U);
                    (void)app_state_save_config(app);
                    app_render_active_screen(app);
                    break;
                case 2: /* Reset Cloud Claiming / Provisioning */
                    app->cfg.claimed       = false;
                    app->cfg.provisioned   = false;
                    app->cfg.tb_token[0]   = '\0';
                    (void)app_state_save_config(app);
                    app->active_screen = SCREEN_ONBOARDING;
                    h1os_lvgl_request_full_refresh();
                    app_render_active_screen(app);
                    break;
                case 3: /* Safe Storage Shutdown (Full Clean White Scrub + Power Cut) */
                default:
                    h1os_shutdown_for_storage();
                    break;
            }
        } else {
            /* Long press on Dashboard/Diagnostics triggers orderly storage shutdown */
            h1os_shutdown_for_storage();
        }
    }
}

static void app_bind_screen_and_keypad(app_context_t *app)
{
    app->root_screen = lv_screen_active();
    if (app->root_screen == NULL) {
        return;
    }

    if (app->input_group == NULL) {
        app->input_group = lv_group_create();
    }
    lv_indev_t *indev = h1os_lvgl_get_indev();
    if (indev != NULL && app->input_group != NULL) {
        lv_indev_set_group(indev, app->input_group);
        lv_group_remove_all_objs(app->input_group);
        lv_group_add_obj(app->input_group, app->root_screen);
        lv_group_focus_obj(app->root_screen);
    }

    lv_obj_add_event_cb(app->root_screen, app_keypad_event_cb, LV_EVENT_ALL, app);
}

/**
 * @brief Asynchronous UI mutation callback posted from Core 0 on_sync() to Core 1 LVGL task.
 */
static void app_ui_sync_update_cb(void *arg)
{
    app_context_t *app = (app_context_t *)arg;
    if (app != NULL && app->sync_ui_dirty) {
        app->sync_ui_dirty = false;
        app_render_active_screen(app);
    }
}

/* ============================================================================
 * Declarative H1OS Lifecycle Callbacks
 * ========================================================================== */

static void app_on_boot(void *user_data)
{
    app_context_t *app = (app_context_t *)user_data;
    ESP_LOGI(TAG, "[on_boot] Cold boot initialization (App %s on H1OS %s)...",
             APP_FW_VERSION, h1os_get_version());

    app->wake_state          = H1OS_WAKE_STATE_BOOT;
    app->is_interactive_wake = true;

    (void)app_ota_mark_running_slot_valid();
    (void)app_state_load_config(app);
    (void)net_wifi_ble_init(app);
    (void)app_state_refresh_telemetry(app);
    (void)app_state_evaluate_alarms(app);

    /* Apply persisted Flow A and Flow B intervals to H1OS RTC state */
    (void)h1os_set_flow_periods(app->cfg.flow_a_sec, app->cfg.flow_b_sec);

    app->active_screen = (!app->cfg.provisioned || !app->cfg.claimed)
                             ? SCREEN_ONBOARDING
                             : SCREEN_DASHBOARD;

    app_bind_screen_and_keypad(app);
    app_render_active_screen(app);
}

static void app_on_wake(const h1os_wake_context_t *ctx, void *user_data)
{
    app_context_t *app = (app_context_t *)user_data;
    if (ctx == NULL || app == NULL) {
        return;
    }

    app->wake_state          = ctx->wake_state;
    app->is_interactive_wake = (ctx->wake_state == H1OS_WAKE_STATE_INTERACTIVE_SESSION);

    ESP_LOGI(TAG, "[on_wake] Wake state=%d (wakes=%lu, flow_b=%lu, btn=%u)",
             (int)ctx->wake_state,
             (unsigned long)ctx->wake_count,
             (unsigned long)ctx->flow_b_count,
             (unsigned)ctx->wake_button);

    (void)app_state_load_config(app);
    (void)net_wifi_ble_init(app);
    (void)app_state_refresh_telemetry(app);
    bool alarm_triggered = app_state_evaluate_alarms(app);

    if (app->is_interactive_wake && app->cfg.sound_enabled) {
        if (alarm_triggered) {
            (void)h1os_audio_play_chime(H1OS_CHIME_ALARM);
        } else {
            (void)h1os_audio_play_tone(APP_AUDIO_BUTTON_CHIRP_HZ, APP_AUDIO_BUTTON_CHIRP_MS);
        }
    }

    if (!app->cfg.provisioned || !app->cfg.claimed) {
        app->active_screen = SCREEN_ONBOARDING;
    } else {
        /* Always present primary Dashboard on unattended RTC Flow B wakes */
        app->active_screen = SCREEN_DASHBOARD;
    }

    app_bind_screen_and_keypad(app);
    app_render_active_screen(app);
}

static void app_on_sync(const h1os_wake_context_t *ctx, void *user_data)
{
    app_context_t *app = (app_context_t *)user_data;
    if (ctx == NULL || app == NULL) {
        return;
    }

    ESP_LOGI(TAG, "[on_sync] Core 0 parallel synchronization started (state=%d)...", (int)ctx->wake_state);

    /* Unprovisioned or Unclaimed interactive session: enter dedicated Initial Setup mode */
    if (!app->cfg.provisioned || !app->cfg.claimed) {
        if (ctx->wake_state != H1OS_WAKE_STATE_RTC_FLOW_B) {
            app_enter_dedicated_onboarding_mode(app);
            if (!app->cfg.provisioned) {
                (void)net_ble_provisioning_start(app);
            }
        }
        return;
    }

    /* Provisioned & Claimed:
     * - Primary Mode (!batch_telemetry_en): Wake -> Send -> Sleep on every Flow B wake.
     * - Optional Batch Mode (batch_telemetry_en): Defer Wi-Fi burst to every Nth Flow B wake
     *   unless an environmental threshold alarm is active or user pressed a button. */
    if (ctx->wake_state == H1OS_WAKE_STATE_RTC_FLOW_B &&
        app->cfg.batch_telemetry_en &&
        !app->telemetry.compound_alarm_active &&
        app->cfg.batch_flow_b_ratio > 1U &&
        (ctx->flow_b_count % app->cfg.batch_flow_b_ratio) != 0U) {
        ESP_LOGI(TAG, "[Batch Mode] Deferring Wi-Fi burst on Flow B #%lu (ratio=1/%u); returning to sleep once display settles.",
                 (unsigned long)ctx->flow_b_count, (unsigned)app->cfg.batch_flow_b_ratio);
        return;
    }

    if (net_wifi_connect_burst(app, APP_WIFI_CONNECT_TIMEOUT_MS) == ESP_OK) {
        (void)net_sntp_sync_rtc_if_needed(app);
        (void)cloud_tb_sync_burst(app);
        (void)net_wifi_disconnect_and_stop(app);
    }

    /* Post any inbound cloud attribute/RPC UI updates safely to Core 1 via h1os_ui_post() */
    if (app->sync_ui_dirty) {
        (void)h1os_ui_post(app_ui_sync_update_cb, app, 100U);
    }
}

static void app_on_pre_sleep(uint32_t sleep_sec, void *user_data)
{
    app_context_t *app = (app_context_t *)user_data;
    ESP_LOGI(TAG, "[on_pre_sleep] Quiescing Layer 4 network services before %lu s sleep...",
             (unsigned long)sleep_sec);

    if (s_onboarding_keepalive_timer != NULL) {
        (void)esp_timer_stop(s_onboarding_keepalive_timer);
    }
    if (app != NULL) {
        app->in_onboarding_mode = false;
        if (!app->cfg.provisioned) {
            (void)net_ble_provisioning_stop();
        }
        (void)net_wifi_disconnect_and_stop(app);
    }
}

static void app_on_shutdown(void *user_data)
{
    app_context_t *app = (app_context_t *)user_data;
    ESP_LOGW(TAG, "[on_shutdown] Saving application state and unmounting storage prior to power cut...");

    if (app != NULL) {
        (void)net_wifi_disconnect_and_stop(app);
        (void)app_state_save_config(app);
    }
    if (h1os_sdcard_is_mounted()) {
        (void)h1os_sdcard_unmount();
    }
}

static void app_on_error(esp_err_t err, const char *context, void *user_data)
{
    (void)user_data;
    ESP_LOGE(TAG, "[on_error] Fatal startup/lifecycle fault in '%s': %s (0x%x)",
             context ? context : "unknown", esp_err_to_name(err), (unsigned)err);
}

void app_main(void)
{
    memset(&s_app_ctx, 0, sizeof(s_app_ctx));

    h1os_lifecycle_t lifecycle = H1OS_LIFECYCLE_DEFAULT_CONFIG();
    lifecycle.on_boot      = app_on_boot;
    lifecycle.on_wake      = app_on_wake;
    lifecycle.on_sync      = app_on_sync;
    lifecycle.on_pre_sleep = app_on_pre_sleep;
    lifecycle.on_shutdown  = app_on_shutdown;
    lifecycle.on_error     = app_on_error;
    lifecycle.user_data    = &s_app_ctx;

    lifecycle.session.inactivity_timeout_sec = APP_INTERACTIVE_TIMEOUT_SEC;
    lifecycle.session.flow_a_period_sec      = APP_DEFAULT_FLOW_A_SEC;
    lifecycle.session.flow_b_period_sec      = APP_DEFAULT_FLOW_B_SEC;
    lifecycle.session.sync_wait_ms           = APP_SYNC_WAIT_GATE_MS;
    lifecycle.session.sync_timeout_ms        = APP_SYNC_BARRIER_TIMEOUT_MS;

    lifecycle.presentation.volume_pct = APP_AUDIO_DEFAULT_VOLUME_PCT;
    lifecycle.presentation.chime_mask = H1OS_CHIME_MASK_ALL;

    ESP_ERROR_CHECK(h1os_start(&lifecycle));
}
