/**
 * @file app_state.c
 * @brief Pure Layer 4 implementation of NVS persistence, telemetry normalization, and alarm logic.
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#include "app_state.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_mac.h"

static const char *TAG = "app_state";

static void populate_factory_defaults(app_nvs_config_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->provisioned   = false;
    cfg->claimed       = false;
    strncpy(cfg->nickname, APP_DEFAULT_NICKNAME, sizeof(cfg->nickname) - 1);
    cfg->flow_a_sec    = APP_DEFAULT_FLOW_A_SEC;
    cfg->flow_b_sec    = APP_DEFAULT_FLOW_B_SEC;
    cfg->temp_unit     = APP_TEMP_UNIT_CELSIUS;
    cfg->vp_unit       = APP_VP_UNIT_HPA;
    cfg->theme         = APP_THEME_LIGHT;
    cfg->sound_enabled = true;
    cfg->volume_pct    = APP_AUDIO_DEFAULT_VOLUME_PCT;
    cfg->alert_mode         = APP_ALERT_MODE_EITHER_OR;
    cfg->batch_telemetry_en = false; /* Default: Wake -> Send -> Sleep */
    cfg->batch_flow_b_ratio = 4U;    /* Optional batch mode: push every 4th Flow B */
    cfg->temp_high_c   = APP_DEFAULT_TEMP_HIGH_C;
    cfg->temp_low_c    = APP_DEFAULT_TEMP_LOW_C;
    cfg->rh_high_pct   = APP_DEFAULT_RH_HIGH_PCT;
    cfg->rh_low_pct    = APP_DEFAULT_RH_LOW_PCT;

    uint8_t mac[6] = {0};
    (void)esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(cfg->claim_secret, sizeof(cfg->claim_secret),
             "H1-%02X%02X%02X%02X", mac[2], mac[3], mac[4], mac[5]);
}

bool app_state_validate_flow_periods(uint32_t flow_a_sec, uint32_t flow_b_sec)
{
    if (flow_a_sec < APP_MIN_FLOW_A_SEC || flow_a_sec > APP_MAX_FLOW_A_SEC) {
        return false;
    }
    if (flow_b_sec < APP_MIN_FLOW_B_SEC || flow_b_sec > APP_MAX_FLOW_B_SEC) {
        return false;
    }
    if (flow_a_sec > 255U && (flow_a_sec % 60U != 0U)) {
        return false;
    }
    if (flow_b_sec <= flow_a_sec) {
        return false;
    }
    uint32_t remainder = flow_b_sec % flow_a_sec;
    if (remainder > 0U && remainder < H1OS_FLOW_REMAINDER_MIN_SEC) {
        return false;
    }
    if (remainder > 255U && (remainder % 60U != 0U)) {
        return false;
    }
    uint32_t overflow = (flow_b_sec / flow_a_sec) + (remainder > 0U ? 1U : 0U);
    return (overflow >= 2U);
}

esp_err_t app_state_load_config(app_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    populate_factory_defaults(&ctx->cfg);

    uint8_t u8_val = 0;
    uint32_t u32_val = 0;
    int32_t i32_val = 0;
    size_t len = 0;

    if (h1os_nvs_get_u8(APP_NVS_KEY_PROVISIONED, &u8_val) == ESP_OK) {
        ctx->cfg.provisioned = (u8_val != 0);
    }
    if (h1os_nvs_get_u8(APP_NVS_KEY_CLAIMED, &u8_val) == ESP_OK) {
        ctx->cfg.claimed = (u8_val != 0);
    }

    len = sizeof(ctx->cfg.tb_token);
    (void)h1os_nvs_get_str(APP_NVS_KEY_TB_TOKEN, ctx->cfg.tb_token, &len);

    len = sizeof(ctx->cfg.claim_secret);
    (void)h1os_nvs_get_str(APP_NVS_KEY_CLAIM_SECRET, ctx->cfg.claim_secret, &len);

    len = sizeof(ctx->cfg.nickname);
    (void)h1os_nvs_get_str(APP_NVS_KEY_NICKNAME, ctx->cfg.nickname, &len);

    if (h1os_nvs_get_u32(APP_NVS_KEY_FLOW_A_SEC, &u32_val) == ESP_OK) {
        ctx->cfg.flow_a_sec = u32_val;
    }
    if (h1os_nvs_get_u32(APP_NVS_KEY_FLOW_B_SEC, &u32_val) == ESP_OK) {
        ctx->cfg.flow_b_sec = u32_val;
    }
    if (!app_state_validate_flow_periods(ctx->cfg.flow_a_sec, ctx->cfg.flow_b_sec)) {
        ESP_LOGW(TAG, "Stored flow intervals (%lu s / %lu s) violate H1OS invariants; restoring defaults (%u s / %u s)",
                 (unsigned long)ctx->cfg.flow_a_sec, (unsigned long)ctx->cfg.flow_b_sec,
                 APP_DEFAULT_FLOW_A_SEC, APP_DEFAULT_FLOW_B_SEC);
        ctx->cfg.flow_a_sec = APP_DEFAULT_FLOW_A_SEC;
        ctx->cfg.flow_b_sec = APP_DEFAULT_FLOW_B_SEC;
    }

    if (h1os_nvs_get_u8(APP_NVS_KEY_UNIT_TEMP, &u8_val) == ESP_OK && u8_val <= APP_TEMP_UNIT_KELVIN) {
        ctx->cfg.temp_unit = (app_temp_unit_t)u8_val;
    }
    if (h1os_nvs_get_u8(APP_NVS_KEY_UNIT_VP, &u8_val) == ESP_OK && u8_val <= APP_VP_UNIT_KPA) {
        ctx->cfg.vp_unit = (app_vp_unit_t)u8_val;
    }
    if (h1os_nvs_get_u8(APP_NVS_KEY_THEME, &u8_val) == ESP_OK && u8_val <= APP_THEME_DARK) {
        ctx->cfg.theme = (app_theme_t)u8_val;
    }
    if (h1os_nvs_get_u8(APP_NVS_KEY_SOUND_EN, &u8_val) == ESP_OK) {
        ctx->cfg.sound_enabled = (u8_val != 0);
    }
    if (h1os_nvs_get_u8(APP_NVS_KEY_VOLUME_PCT, &u8_val) == ESP_OK && u8_val <= 100U) {
        ctx->cfg.volume_pct = u8_val;
    }
    if (h1os_nvs_get_u8(APP_NVS_KEY_ALERT_MODE, &u8_val) == ESP_OK && u8_val <= APP_ALERT_MODE_BOTH_REQUIRED) {
        ctx->cfg.alert_mode = (app_alert_mode_t)u8_val;
    }
    if (h1os_nvs_get_u8(APP_NVS_KEY_BATCH_EN, &u8_val) == ESP_OK) {
        ctx->cfg.batch_telemetry_en = (u8_val != 0);
    }
    if (h1os_nvs_get_u8(APP_NVS_KEY_BATCH_RATIO, &u8_val) == ESP_OK && u8_val >= 1U && u8_val <= 16U) {
        ctx->cfg.batch_flow_b_ratio = u8_val;
    }

    /* Centi-unit fixed-point storage for thresholds in NVS */
    if (h1os_nvs_get_i32(APP_NVS_KEY_TH_TEMP_HI, &i32_val) == ESP_OK) {
        ctx->cfg.temp_high_c = (float)i32_val / 100.0f;
    }
    if (h1os_nvs_get_i32(APP_NVS_KEY_TH_TEMP_LO, &i32_val) == ESP_OK) {
        ctx->cfg.temp_low_c = (float)i32_val / 100.0f;
    }
    if (h1os_nvs_get_i32(APP_NVS_KEY_TH_RH_HI, &i32_val) == ESP_OK) {
        ctx->cfg.rh_high_pct = (float)i32_val / 100.0f;
    }
    if (h1os_nvs_get_i32(APP_NVS_KEY_TH_RH_LO, &i32_val) == ESP_OK) {
        ctx->cfg.rh_low_pct = (float)i32_val / 100.0f;
    }

    /* Synchronize RH alert bounds with H1OS Layer 4 API */
    (void)h1os_set_rh_alert_bounds(ctx->cfg.rh_low_pct, ctx->cfg.rh_high_pct);

    ESP_LOGI(TAG, "Loaded NVS config: nick='%s', prov=%d, claimed=%d, FlowA=%lus, FlowB=%lus",
             ctx->cfg.nickname, ctx->cfg.provisioned, ctx->cfg.claimed,
             (unsigned long)ctx->cfg.flow_a_sec, (unsigned long)ctx->cfg.flow_b_sec);
    return ESP_OK;
}

esp_err_t app_state_save_config(const app_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    (void)h1os_nvs_set_u8(APP_NVS_KEY_PROVISIONED, ctx->cfg.provisioned ? 1U : 0U);
    (void)h1os_nvs_set_u8(APP_NVS_KEY_CLAIMED, ctx->cfg.claimed ? 1U : 0U);
    (void)h1os_nvs_set_str(APP_NVS_KEY_TB_TOKEN, ctx->cfg.tb_token);
    (void)h1os_nvs_set_str(APP_NVS_KEY_CLAIM_SECRET, ctx->cfg.claim_secret);
    (void)h1os_nvs_set_str(APP_NVS_KEY_NICKNAME, ctx->cfg.nickname);
    (void)h1os_nvs_set_u32(APP_NVS_KEY_FLOW_A_SEC, ctx->cfg.flow_a_sec);
    (void)h1os_nvs_set_u32(APP_NVS_KEY_FLOW_B_SEC, ctx->cfg.flow_b_sec);
    (void)h1os_nvs_set_u8(APP_NVS_KEY_UNIT_TEMP, (uint8_t)ctx->cfg.temp_unit);
    (void)h1os_nvs_set_u8(APP_NVS_KEY_UNIT_VP, (uint8_t)ctx->cfg.vp_unit);
    (void)h1os_nvs_set_u8(APP_NVS_KEY_THEME, (uint8_t)ctx->cfg.theme);
    (void)h1os_nvs_set_u8(APP_NVS_KEY_SOUND_EN, ctx->cfg.sound_enabled ? 1U : 0U);
    (void)h1os_nvs_set_u8(APP_NVS_KEY_VOLUME_PCT, ctx->cfg.volume_pct);
    (void)h1os_nvs_set_u8(APP_NVS_KEY_ALERT_MODE, (uint8_t)ctx->cfg.alert_mode);
    (void)h1os_nvs_set_u8(APP_NVS_KEY_BATCH_EN, ctx->cfg.batch_telemetry_en ? 1U : 0U);
    (void)h1os_nvs_set_u8(APP_NVS_KEY_BATCH_RATIO, ctx->cfg.batch_flow_b_ratio);
    (void)h1os_nvs_set_i32(APP_NVS_KEY_TH_TEMP_HI, (int32_t)(ctx->cfg.temp_high_c * 100.0f));
    (void)h1os_nvs_set_i32(APP_NVS_KEY_TH_TEMP_LO, (int32_t)(ctx->cfg.temp_low_c * 100.0f));
    (void)h1os_nvs_set_i32(APP_NVS_KEY_TH_RH_HI, (int32_t)(ctx->cfg.rh_high_pct * 100.0f));
    (void)h1os_nvs_set_i32(APP_NVS_KEY_TH_RH_LO, (int32_t)(ctx->cfg.rh_low_pct * 100.0f));

    return ESP_OK;
}

esp_err_t app_state_apply_flow_periods(app_context_t *ctx, uint32_t flow_a_sec, uint32_t flow_b_sec)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!app_state_validate_flow_periods(flow_a_sec, flow_b_sec)) {
        ESP_LOGE(TAG, "Rejected invalid flow intervals: Flow A=%lu s, Flow B=%lu s",
                 (unsigned long)flow_a_sec, (unsigned long)flow_b_sec);
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = h1os_set_flow_periods(flow_a_sec, flow_b_sec);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "h1os_set_flow_periods(%lu, %lu) failed: %s",
                 (unsigned long)flow_a_sec, (unsigned long)flow_b_sec, esp_err_to_name(err));
        return err;
    }

    ctx->cfg.flow_a_sec = flow_a_sec;
    ctx->cfg.flow_b_sec = flow_b_sec;
    ctx->telemetry.flow_a_period_sec = flow_a_sec;
    ctx->telemetry.flow_b_period_sec = flow_b_sec;
    return app_state_save_config(ctx);
}

esp_err_t app_state_refresh_telemetry(app_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    h1os_telemetry_t raw = {0};
    esp_err_t err = h1os_get_telemetry(&raw);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "h1os_get_telemetry returned %s; retaining last valid snapshot", esp_err_to_name(err));
    }

    app_telemetry_snapshot_t *t = &ctx->telemetry;
    t->ambient_temp_c        = raw.ambient_temp_c;
    t->ambient_temp_f        = raw.ambient_temp_f;
    t->ambient_temp_k        = raw.ambient_temp_k;
    t->ambient_rh_pct        = raw.ambient_rh_pct;
    t->dew_point_c           = raw.dew_point_c;
    t->dew_point_f           = raw.dew_point_f;
    t->dew_point_k           = raw.dew_point_k;
    t->vapor_pressure_hpa    = raw.vapor_pressure_hpa;
    t->sat_vapor_pressure_hpa= raw.sat_vapor_pressure_hpa;
    t->absolute_humidity_gm3 = raw.absolute_humidity_g;

    t->shtc3_raw_temp_c      = raw.raw_shtc3_temp_c;
    t->mcu_die_temp_c        = raw.mcu_die_temp_c;
    t->epd_glass_temp_c      = raw.epd_glass_temp_c;
    t->trinode_temp_c        = raw.trinode_temp_c;
    t->trinode_delta_c       = raw.trinode_delta_c;
    t->window_low_c          = raw.window_low_c;
    t->window_high_c         = raw.window_high_c;
    t->confidence_pct        = raw.trinode_confidence_pct;
    t->thermally_settled     = raw.is_thermally_settled;
    t->valid_trend           = raw.valid_trend;

    t->autonomy_days         = raw.autonomy.autonomy_days;
    t->avg_current_ma        = raw.autonomy.avg_draw_ma;
    t->battery_mv            = raw.battery_mv;
    t->battery_soc_pct       = raw.battery_percent;
    t->vbus_present          = raw.is_usb_powered;
    t->battery_state         = raw.battery_state;

    t->sample_count          = raw.sample_count;
    t->boot_count            = raw.boot_count;
    t->wake_count            = raw.wake_count;
    t->flow_b_count          = raw.flow_b_count;
    t->button_wake_count     = raw.button_wake_count;
    t->flow_a_period_sec     = raw.flow_a_period_sec;
    t->flow_b_period_sec     = raw.flow_b_period_sec;

    return ESP_OK;
}

bool app_state_evaluate_alarms(app_context_t *ctx)
{
    if (ctx == NULL) {
        return false;
    }

    app_telemetry_snapshot_t *t = &ctx->telemetry;
    const app_nvs_config_t *c   = &ctx->cfg;

    if (t->alarm_temp_high) {
        t->alarm_temp_high = (t->ambient_temp_c > (c->temp_high_c - APP_ALARM_HYSTERESIS_TEMP_C));
    } else {
        t->alarm_temp_high = (t->ambient_temp_c > c->temp_high_c);
    }

    if (t->alarm_temp_low) {
        t->alarm_temp_low = (t->ambient_temp_c < (c->temp_low_c + APP_ALARM_HYSTERESIS_TEMP_C));
    } else {
        t->alarm_temp_low = (t->ambient_temp_c < c->temp_low_c);
    }

    if (t->alarm_rh_high) {
        t->alarm_rh_high = (t->ambient_rh_pct > (c->rh_high_pct - APP_ALARM_HYSTERESIS_RH_PCT));
    } else {
        t->alarm_rh_high = (t->ambient_rh_pct > c->rh_high_pct);
    }

    if (t->alarm_rh_low) {
        t->alarm_rh_low = (t->ambient_rh_pct < (c->rh_low_pct + APP_ALARM_HYSTERESIS_RH_PCT));
    } else {
        t->alarm_rh_low = (t->ambient_rh_pct < c->rh_low_pct);
    }

    bool any_temp = (t->alarm_temp_high || t->alarm_temp_low);
    bool any_rh   = (t->alarm_rh_high   || t->alarm_rh_low);

    switch (c->alert_mode) {
        case APP_ALERT_MODE_ANY_TEMP:
            t->compound_alarm_active = any_temp;
            break;
        case APP_ALERT_MODE_ANY_RH:
            t->compound_alarm_active = any_rh;
            break;
        case APP_ALERT_MODE_BOTH_REQUIRED:
            t->compound_alarm_active = (any_temp && any_rh);
            break;
        case APP_ALERT_MODE_EITHER_OR:
        default:
            t->compound_alarm_active = (any_temp || any_rh);
            break;
    }

    return t->compound_alarm_active;
}

float app_state_convert_temp(float temp_c, app_temp_unit_t unit, const char **out_unit_str)
{
    switch (unit) {
        case APP_TEMP_UNIT_FAHRENHEIT:
            if (out_unit_str) *out_unit_str = "F";
            return H1OS_CELSIUS_TO_FAHRENHEIT(temp_c);
        case APP_TEMP_UNIT_KELVIN:
            if (out_unit_str) *out_unit_str = "K";
            return H1OS_CELSIUS_TO_KELVIN(temp_c);
        case APP_TEMP_UNIT_CELSIUS:
        default:
            if (out_unit_str) *out_unit_str = "C";
            return temp_c;
    }
}

float app_state_convert_vp(float vp_hpa, app_vp_unit_t unit, const char **out_unit_str)
{
    if (unit == APP_VP_UNIT_KPA) {
        if (out_unit_str) *out_unit_str = "kPa";
        return vp_hpa * 0.1f;
    }
    if (out_unit_str) *out_unit_str = "hPa";
    return vp_hpa;
}
