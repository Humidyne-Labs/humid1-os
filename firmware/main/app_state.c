/**
 * @file app_state.c
 * @brief Skeleton implementation of NVS persistence, RTC Slow SRAM cache, and dynamic Flow A/B sync.
 */

#include "app_state.h"
#include <stdio.h>
#include <string.h>
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "h1os/h1os.h"
#include "h1os/orch_state.h"

static const char *TAG = "app_state";

#define APP_RTC_CACHE_MAGIC (0x48314150UL) /* "H1AP" */

/**
 * @brief Persistent RTC Slow SRAM state retained across deep sleep (PIT-05-A).
 */
static RTC_DATA_ATTR app_rtc_cache_t s_rtc_cache;

app_rtc_cache_t *app_state_get_rtc_cache(void)
{
    return &s_rtc_cache;
}

esp_err_t app_state_init(app_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(ctx, 0, sizeof(app_context_t));

    /* 1. Derive deterministic hardware UID: HUMID1_<MAC_LAST_3_BYTES> */
    uint8_t mac[6] = {0};
    esp_err_t err = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_read_mac failed (0x%x), using placeholder UID", (unsigned int)err);
    }
    snprintf(ctx->attrs.device_uid, sizeof(ctx->attrs.device_uid),
             "%s%02X%02X%02X", APP_DEVICE_UID_PREFIX, mac[3], mac[4], mac[5]);

    /* 2. Initialize RTC Slow SRAM cache on cold power-on reset */
    if (s_rtc_cache.magic != APP_RTC_CACHE_MAGIC) {
        memset(&s_rtc_cache, 0, sizeof(app_rtc_cache_t));
        s_rtc_cache.magic = APP_RTC_CACHE_MAGIC;
        s_rtc_cache.thresholds.rh_low_crit        = 60.0f;
        s_rtc_cache.thresholds.rh_low_warn        = 62.0f;
        s_rtc_cache.thresholds.rh_high_warn       = 72.0f;
        s_rtc_cache.thresholds.rh_high_crit       = 75.0f;
        s_rtc_cache.thresholds.temp_low_crit_k    = 288.71f;
        s_rtc_cache.thresholds.temp_low_warn_k    = 290.93f;
        s_rtc_cache.thresholds.temp_high_warn_k   = 295.37f;
        s_rtc_cache.thresholds.temp_high_crit_k   = 297.59f;
        s_rtc_cache.thresholds.batt_low_crit_pct  = 10.0f;
        s_rtc_cache.thresholds.batt_low_warn_pct  = 20.0f;
        s_rtc_cache.thresholds.rh_hysteresis      = 1.0f;
        s_rtc_cache.thresholds.temp_hysteresis_k  = 0.3f;
        s_rtc_cache.thresholds.batt_hysteresis_pct = 2.0f;
    }

    /* 3. Set baseline defaults prior to NVS load */
    ctx->attrs.flow_a_period_sec     = APP_DEFAULT_FLOW_A_SEC;
    ctx->attrs.flow_b_period_sec     = APP_DEFAULT_FLOW_B_SEC;
    ctx->attrs.telemetry_schema_mask = (uint32_t)APP_TELE_MASK_DEFAULT;
    ctx->attrs.temp_unit             = APP_TEMP_UNIT_FAHRENHEIT;
    ctx->attrs.sound_enabled         = true;
    ctx->attrs.auto_update_enabled   = true;

    /* 4. Load persisted configuration from NVS namespace "app_cfg" */
    nvs_handle_t nvs = 0;
    err = nvs_open(APP_NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (err == ESP_OK) {
        uint8_t u8_val = 0;
        if (nvs_get_u8(nvs, APP_NVS_KEY_WIFI_PROV, &u8_val) == ESP_OK) {
            ctx->attrs.wifi_provisioned = (u8_val != 0U);
        }
        if (nvs_get_u8(nvs, APP_NVS_KEY_CLAIMED, &u8_val) == ESP_OK) {
            ctx->attrs.device_claimed = (u8_val != 0U);
        }
        if (nvs_get_u8(nvs, APP_NVS_KEY_TEMP_UNIT, &u8_val) == ESP_OK && u8_val <= 2U) {
            ctx->attrs.temp_unit = (app_temp_unit_t)u8_val;
        }
        if (nvs_get_u8(nvs, APP_NVS_KEY_THEME_DARK, &u8_val) == ESP_OK) {
            ctx->attrs.theme_dark = (u8_val != 0U);
        }
        if (nvs_get_u8(nvs, APP_NVS_KEY_SOUND_EN, &u8_val) == ESP_OK) {
            ctx->attrs.sound_enabled = (u8_val != 0U);
        }
        if (nvs_get_u8(nvs, APP_NVS_KEY_AUTO_OTA, &u8_val) == ESP_OK) {
            ctx->attrs.auto_update_enabled = (u8_val != 0U);
        }
        (void)nvs_get_u32(nvs, APP_NVS_KEY_FLOW_A_SEC, &ctx->attrs.flow_a_period_sec);
        (void)nvs_get_u32(nvs, APP_NVS_KEY_FLOW_B_SEC, &ctx->attrs.flow_b_period_sec);

        size_t len = sizeof(ctx->attrs.tb_access_token);
        (void)nvs_get_str(nvs, APP_NVS_KEY_TB_TOKEN, ctx->attrs.tb_access_token, &len);

        len = sizeof(ctx->attrs.device_nickname);
        (void)nvs_get_str(nvs, APP_NVS_KEY_NICKNAME, ctx->attrs.device_nickname, &len);

        nvs_close(nvs);
    } else {
        ESP_LOGI(TAG, "NVS namespace '%s' not yet created (first boot)", APP_NVS_NAMESPACE);
    }

    ESP_LOGI(TAG, "Initialized UID=%s prov=%u claimed=%u FlowA=%lus FlowB=%lus",
             ctx->attrs.device_uid,
             (unsigned int)ctx->attrs.wifi_provisioned,
             (unsigned int)ctx->attrs.device_claimed,
             (unsigned long)ctx->attrs.flow_a_period_sec,
             (unsigned long)ctx->attrs.flow_b_period_sec);
    return ESP_OK;
}

esp_err_t app_state_save_nvs(const app_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs = 0;
    esp_err_t err = nvs_open(APP_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        return err;
    }

    (void)nvs_set_u8(nvs, APP_NVS_KEY_WIFI_PROV, ctx->attrs.wifi_provisioned ? 1U : 0U);
    (void)nvs_set_u8(nvs, APP_NVS_KEY_CLAIMED, ctx->attrs.device_claimed ? 1U : 0U);
    (void)nvs_set_u8(nvs, APP_NVS_KEY_TEMP_UNIT, (uint8_t)ctx->attrs.temp_unit);
    (void)nvs_set_u8(nvs, APP_NVS_KEY_THEME_DARK, ctx->attrs.theme_dark ? 1U : 0U);
    (void)nvs_set_u8(nvs, APP_NVS_KEY_SOUND_EN, ctx->attrs.sound_enabled ? 1U : 0U);
    (void)nvs_set_u8(nvs, APP_NVS_KEY_AUTO_OTA, ctx->attrs.auto_update_enabled ? 1U : 0U);
    (void)nvs_set_u32(nvs, APP_NVS_KEY_FLOW_A_SEC, ctx->attrs.flow_a_period_sec);
    (void)nvs_set_u32(nvs, APP_NVS_KEY_FLOW_B_SEC, ctx->attrs.flow_b_period_sec);
    (void)nvs_set_str(nvs, APP_NVS_KEY_TB_TOKEN, ctx->attrs.tb_access_token);
    (void)nvs_set_str(nvs, APP_NVS_KEY_NICKNAME, ctx->attrs.device_nickname);

    /* Mandatory commit prior to handle close or sleep entry (PIT-05-D) */
    err = nvs_commit(nvs);
    nvs_close(nvs);
    return err;
}

esp_err_t app_state_apply_flow_periods(app_context_t *ctx, uint32_t flow_a_sec, uint32_t flow_b_sec)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (flow_a_sec < APP_MIN_FLOW_A_SEC || flow_a_sec > APP_MAX_FLOW_A_SEC ||
        flow_b_sec < APP_MIN_FLOW_B_SEC || flow_b_sec > APP_MAX_FLOW_B_SEC ||
        flow_b_sec < flow_a_sec) {
        return ESP_ERR_INVALID_ARG;
    }

    ctx->attrs.flow_a_period_sec = flow_a_sec;
    ctx->attrs.flow_b_period_sec = flow_b_sec;

    /* MANDATE-02 shim: Synchronize active H1OS orchestrator state immediately */
    h1os_rtc_state_t *orch = orch_state_get();
    if (orch != NULL) {
        orch->sample_interval_sec = (uint16_t)flow_a_sec;
        orch->flow_b_period_sec   = flow_b_sec;
    }

    return app_state_save_nvs(ctx);
}

esp_err_t app_state_refresh_telemetry(app_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    h1os_telemetry_t raw = {0};
    h1os_get_telemetry(&raw);

    ctx->telemetry.rh_pct           = raw.ambient_rh_pct;
    ctx->telemetry.temp_k           = app_celsius_to_kelvin(raw.ambient_temp_c);
    ctx->telemetry.dew_point_k      = app_celsius_to_kelvin(raw.dew_point_c);
    ctx->telemetry.shtc3_raw_k      = app_celsius_to_kelvin(raw.shtc3_raw_temp_c);
    ctx->telemetry.mcu_die_k        = app_celsius_to_kelvin(raw.mcu_die_temp_c);
    ctx->telemetry.epd_glass_k      = app_celsius_to_kelvin(raw.epd_glass_temp_c);
    ctx->telemetry.vapor_press_hpa  = raw.vapor_pressure_hpa;
    ctx->telemetry.sat_vp_hpa       = raw.sat_vapor_pressure_hpa;
    ctx->telemetry.abs_humidity_gm3 = raw.absolute_humidity_gm3;
    ctx->telemetry.autonomy_days    = raw.battery.autonomy_days;
    ctx->telemetry.avg_current_ma   = raw.battery.avg_current_ma;
    ctx->telemetry.vbat_mv          = raw.battery.voltage_mv;
    ctx->telemetry.battery_pct      = raw.battery.soc_pct;
    ctx->telemetry.confidence_pct   = raw.confidence_pct;
    ctx->telemetry.vbus_present     = raw.battery.vbus_present;
    ctx->telemetry.thermal_settled  = (raw.confidence_pct >= 80U);

    return ESP_OK;
}

esp_err_t app_state_reset_credentials(app_context_t *ctx, bool factory_reset)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ctx->attrs.wifi_provisioned = false;
    if (factory_reset) {
        ctx->attrs.device_claimed     = false;
        ctx->attrs.tb_access_token[0] = '\0';
        ctx->attrs.device_nickname[0] = '\0';
    }

    esp_err_t err = app_state_save_nvs(ctx);
    ESP_LOGW(TAG, "Credentials reset (factory_reset=%u, err=0x%x) — restarting",
             (unsigned int)factory_reset, (unsigned int)err);
    esp_restart();
    return err;
}
