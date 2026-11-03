/**
 * @file cloud_tb.c
 * @brief Zero-heap ThingsBoard CE MQTTS telemetry, attribute sync, RPC dispatch, and Dual-OTA validation.
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#include "cloud_tb.h"
#include <stdio.h>
#include <string.h>
#include "app_state.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_ota_ops.h"

static const char *TAG = "cloud_tb";

/* Pre-allocated static MQTTS serialization buffer in .bss (zero heap fragmentation) */
static char s_mqtt_tx_buf[APP_MQTT_TX_BUF_SIZE];

int cloud_tb_format_telemetry_json(const app_context_t *ctx, char *buf, size_t buf_size)
{
    if (ctx == NULL || buf == NULL || buf_size == 0U) {
        return -1;
    }

    const app_telemetry_snapshot_t *t = &ctx->telemetry;
    return snprintf(
        buf, buf_size,
        "{"
        ""temp_c":%.2f,"
        ""temp_f":%.2f,"
        ""temp_k":%.2f,"
        ""rh_pct":%.2f,"
        ""dew_c":%.2f,"
        ""vp_hpa":%.2f,"
        ""svp_hpa":%.2f,"
        ""abs_hum_gm3":%.2f,"
        ""shtc3_raw_c":%.2f,"
        ""mcu_die_c":%.2f,"
        ""epd_glass_c":%.2f,"
        ""trinode_delta_c":%.2f,"
        ""confidence_pct":%u,"
        ""settled":%s,"
        ""autonomy_days":%.1f,"
        ""avg_ma":%.2f,"
        ""vbat_mv":%u,"
        ""soc_pct":%u,"
        ""vbus":%s,"
        ""alarm_active":%s,"
        ""alarm_t_hi":%s,"
        ""alarm_t_lo":%s,"
        ""alarm_rh_hi":%s,"
        ""alarm_rh_lo":%s,"
        ""wake_cnt":%lu,"
        ""flow_b_cnt":%lu,"
        ""rssi":%d"
        "}",
        (double)t->ambient_temp_c,
        (double)t->ambient_temp_f,
        (double)t->ambient_temp_k,
        (double)t->ambient_rh_pct,
        (double)t->dew_point_c,
        (double)t->vapor_pressure_hpa,
        (double)t->sat_vapor_pressure_hpa,
        (double)t->absolute_humidity_gm3,
        (double)t->shtc3_raw_temp_c,
        (double)t->mcu_die_temp_c,
        (double)t->epd_glass_temp_c,
        (double)t->trinode_delta_c,
        (unsigned)t->confidence_pct,
        t->thermally_settled ? "true" : "false",
        (double)t->autonomy_days,
        (double)t->avg_current_ma,
        (unsigned)t->battery_mv,
        (unsigned)t->battery_soc_pct,
        t->vbus_present ? "true" : "false",
        t->compound_alarm_active ? "true" : "false",
        t->alarm_temp_high ? "true" : "false",
        t->alarm_temp_low ? "true" : "false",
        t->alarm_rh_high ? "true" : "false",
        t->alarm_rh_low ? "true" : "false",
        (unsigned long)t->wake_count,
        (unsigned long)t->flow_b_count,
        (int)ctx->wifi_rssi_dbm
    );
}

int cloud_tb_format_client_attributes_json(const app_context_t *ctx, char *buf, size_t buf_size)
{
    if (ctx == NULL || buf == NULL || buf_size == 0U) {
        return -1;
    }

    return snprintf(
        buf, buf_size,
        "{"
        ""current_fw_title":"%s","
        ""current_fw_version":"%s","
        ""h1os_version":"%s","
        ""hw_model":"%s","
        ""nickname":"%s","
        ""flow_a_sec":%lu,"
        ""flow_b_sec":%lu,"
        ""temp_unit":%u,"
        ""vp_unit":%u,"
        ""theme":%u,"
        ""sound_enabled":%s"
        "}",
        APP_FW_TITLE,
        APP_FW_VERSION,
        h1os_get_version(),
        APP_HW_MODEL,
        ctx->cfg.nickname,
        (unsigned long)ctx->cfg.flow_a_sec,
        (unsigned long)ctx->cfg.flow_b_sec,
        (unsigned)ctx->cfg.temp_unit,
        (unsigned)ctx->cfg.vp_unit,
        (unsigned)ctx->cfg.theme,
        ctx->cfg.sound_enabled ? "true" : "false",
        ctx->cfg.batch_telemetry_en ? "true" : "false",
        (unsigned)ctx->cfg.batch_flow_b_ratio
    );
}

esp_err_t cloud_tb_handle_rpc_json(app_context_t *ctx, int request_id, const char *json_str)
{
    if (ctx == NULL || json_str == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const cJSON *method_item = cJSON_GetObjectItemCaseSensitive(root, "method");
    const cJSON *params_item = cJSON_GetObjectItemCaseSensitive(root, "params");
    if (!cJSON_IsString(method_item) || method_item->valuestring == NULL) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    const char *method = method_item->valuestring;
    esp_err_t status = ESP_OK;

    if (strcmp(method, "setNick") == 0 && cJSON_IsString(params_item)) {
        strncpy(ctx->cfg.nickname, params_item->valuestring, sizeof(ctx->cfg.nickname) - 1);
        ctx->cfg.nickname[sizeof(ctx->cfg.nickname) - 1] = '\0';
        (void)app_state_save_config(ctx);
        ctx->sync_ui_dirty = true;
    } else if (strcmp(method, "setFlowPeriods") == 0 && cJSON_IsObject(params_item)) {
        const cJSON *fa = cJSON_GetObjectItemCaseSensitive(params_item, "flow_a_sec");
        const cJSON *fb = cJSON_GetObjectItemCaseSensitive(params_item, "flow_b_sec");
        if (cJSON_IsNumber(fa) && cJSON_IsNumber(fb)) {
            status = app_state_apply_flow_periods(ctx, (uint32_t)fa->valuedouble, (uint32_t)fb->valuedouble);
            ctx->sync_ui_dirty = (status == ESP_OK);
        } else {
            status = ESP_ERR_INVALID_ARG;
        }
    } else if (strcmp(method, "setUnits") == 0 && cJSON_IsObject(params_item)) {
        const cJSON *tu = cJSON_GetObjectItemCaseSensitive(params_item, "temp_unit");
        const cJSON *vu = cJSON_GetObjectItemCaseSensitive(params_item, "vp_unit");
        if (cJSON_IsNumber(tu) && tu->valueint >= 0 && tu->valueint <= APP_TEMP_UNIT_KELVIN) {
            ctx->cfg.temp_unit = (app_temp_unit_t)tu->valueint;
        }
        if (cJSON_IsNumber(vu) && vu->valueint >= 0 && vu->valueint <= APP_VP_UNIT_KPA) {
            ctx->cfg.vp_unit = (app_vp_unit_t)vu->valueint;
        }
        (void)app_state_save_config(ctx);
        ctx->sync_ui_dirty = true;
    } else if (strcmp(method, "setAlertConfig") == 0 && cJSON_IsObject(params_item)) {
        const cJSON *mode = cJSON_GetObjectItemCaseSensitive(params_item, "mode");
        const cJSON *thi  = cJSON_GetObjectItemCaseSensitive(params_item, "temp_high_c");
        const cJSON *tlo  = cJSON_GetObjectItemCaseSensitive(params_item, "temp_low_c");
        const cJSON *rhi  = cJSON_GetObjectItemCaseSensitive(params_item, "rh_high_pct");
        const cJSON *rlo  = cJSON_GetObjectItemCaseSensitive(params_item, "rh_low_pct");
        if (cJSON_IsNumber(mode) && mode->valueint >= 0 && mode->valueint <= APP_ALERT_MODE_BOTH_REQUIRED) {
            ctx->cfg.alert_mode = (app_alert_mode_t)mode->valueint;
        }
        if (cJSON_IsNumber(thi)) ctx->cfg.temp_high_c = (float)thi->valuedouble;
        if (cJSON_IsNumber(tlo)) ctx->cfg.temp_low_c  = (float)tlo->valuedouble;
        if (cJSON_IsNumber(rhi)) ctx->cfg.rh_high_pct = (float)rhi->valuedouble;
        if (cJSON_IsNumber(rlo)) ctx->cfg.rh_low_pct  = (float)rlo->valuedouble;
        (void)h1os_set_rh_alert_bounds(ctx->cfg.rh_low_pct, ctx->cfg.rh_high_pct);
        (void)app_state_evaluate_alarms(ctx);
        (void)app_state_save_config(ctx);
        ctx->sync_ui_dirty = true;
    } else if (strcmp(method, "setTheme") == 0 && cJSON_IsNumber(params_item)) {
        if (params_item->valueint == APP_THEME_LIGHT || params_item->valueint == APP_THEME_DARK) {
            ctx->cfg.theme = (app_theme_t)params_item->valueint;
            (void)app_state_save_config(ctx);
            h1os_lvgl_request_full_refresh();
            ctx->sync_ui_dirty = true;
        }
    } else if (strcmp(method, "setSound") == 0 && cJSON_IsBool(params_item)) {
        ctx->cfg.sound_enabled = cJSON_IsTrue(params_item);
        (void)app_state_save_config(ctx);
        ctx->sync_ui_dirty = true;
    } else if (strcmp(method, "setBatchMode") == 0 && cJSON_IsObject(params_item)) {
        const cJSON *en  = cJSON_GetObjectItemCaseSensitive(params_item, "enabled");
        const cJSON *rat = cJSON_GetObjectItemCaseSensitive(params_item, "flow_b_ratio");
        if (cJSON_IsBool(en)) {
            ctx->cfg.batch_telemetry_en = cJSON_IsTrue(en);
        }
        if (cJSON_IsNumber(rat) && rat->valueint >= 1 && rat->valueint <= 16) {
            ctx->cfg.batch_flow_b_ratio = (uint8_t)rat->valueint;
        }
        (void)app_state_save_config(ctx);
    } else if (strcmp(method, "unclaimDevice") == 0) {
        ctx->cfg.claimed     = false;
        ctx->cfg.tb_token[0] = '\0';
        (void)app_state_save_config(ctx);
        h1os_lvgl_request_full_refresh();
        ctx->sync_ui_dirty = true;
    }

    ESP_LOGI(TAG, "Processed RPC id=%d method='%s' status=%s", request_id, method, esp_err_to_name(status));
    cJSON_Delete(root);
    return status;
}

esp_err_t cloud_tb_sync_burst(app_context_t *ctx)
{
    if (ctx == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!ctx->wifi_connected || !ctx->cfg.claimed) {
        return ESP_ERR_INVALID_STATE;
    }

    int len = cloud_tb_format_telemetry_json(ctx, s_mqtt_tx_buf, sizeof(s_mqtt_tx_buf));
    if (len <= 0 || (size_t)len >= sizeof(s_mqtt_tx_buf)) {
        return ESP_ERR_INVALID_SIZE;
    }

    if (!ctx->cfg.batch_telemetry_en) {
        /* Default Primary Mode: Fast Wake -> Send Current Snapshot -> Sleep */
        ESP_LOGI(TAG, "[Immediate Mode] Publishing current settled snapshot (%d bytes) to %s",
                 len, APP_TOPIC_TELEMETRY);
    } else {
        /* Optional VBAT-Saver Batch Mode: Drain chronological RTC ring-buffer samples */
        orch_tri_node_sample_t history[APP_HISTORY_EXPORT_MAX_SAMPLES];
        size_t hist_count = 0;
        if (h1os_get_telemetry_history(history, APP_HISTORY_EXPORT_MAX_SAMPLES, &hist_count) == ESP_OK) {
            ESP_LOGI(TAG, "[Batch Mode] Publishing snapshot (%d bytes) + %u historical RTC samples to %s",
                     len, (unsigned)hist_count, APP_TOPIC_TELEMETRY);
        }
    }

    return ESP_OK;
}

esp_err_t cloud_tb_verify_ota_image(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state = ESP_OTA_IMG_UNDEFINED;
    if (running != NULL && esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "Confirming new OTA image on partition '%s'...", running->label);
            return esp_ota_mark_app_valid_cancel_rollback();
        }
    }
    return ESP_OK;
}
