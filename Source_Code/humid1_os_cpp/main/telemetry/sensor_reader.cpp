/**
 * @file sensor_reader.cpp
 * @brief Environmental & System Telemetry Acquisition Engine Implementation
 * 
 * @attribution
 * - Humidyne Labs / Humiditron (2026)
 * 
 * SPDX-License-Identifier: MIT
 */

#include "sensor_reader.h"
#include <string.h>
#include "esp_log.h"
#include "bsp/bsp.h"

static const char *TAG = "sensor_reader";

esp_err_t sensor_reader_init(void)
{
    // Sensors are already initialized by BSP during bsp_board_init_with_config
    ESP_LOGI(TAG, "Sensor acquisition engine ready");
    return ESP_OK;
}

esp_err_t sensor_reader_sample(app_telemetry_data_t *out_data)
{
    if (!out_data) return ESP_ERR_INVALID_ARG;
    memset(out_data, 0, sizeof(app_telemetry_data_t));

    // 1. Read SHTC3 Sensor
    bsp_shtc3_data_t raw_shtc3 = {};
    esp_err_t ret = bsp_shtc3_read(&raw_shtc3);
    if (ret == ESP_OK && raw_shtc3.valid) {
        out_data->temp_k = raw_shtc3.temperature_k;
        out_data->rh_pct = raw_shtc3.humidity_percent;
        out_data->valid  = true;
    } else {
        ESP_LOGW(TAG, "SHTC3 read failed (ret=%s, valid=%d), using fallback", esp_err_to_name(ret), raw_shtc3.valid);
        out_data->temp_k = 295.37f; // ~22.2 °C (72 °F)
        out_data->rh_pct = 50.0f;
        out_data->valid  = false;
    }

    // Convert Kelvin to Celsius and Fahrenheit
    out_data->temp_c = raw_shtc3.temperature_c; // copy new temp C
    out_data->temp_f = raw_shtc3.temperature_f; // copy new temp F

    // 2. Sample Battery Voltage & State of Charge
    uint32_t mv = 0;
    bsp_battery_get_voltage(&mv, NULL);
    out_data->battery_mv  = mv;
    out_data->battery_pct = bsp_battery_get_percentage();

    // 3. Sample Wi-Fi RSSI
    int rssi = -100;
    if (bsp_wifi_is_connected()) {
        bsp_wifi_get_rssi(&rssi);
    }
    out_data->rssi_dbm = rssi;

    ESP_LOGI(TAG, "Telemetry Sample: Temp=%.2f K (%.1f °F / %.1f °C), RH=%.1f %%, Batt=%u%% (%lu mV), RSSI=%d dBm",
             out_data->temp_k, out_data->temp_f, out_data->temp_c,
             out_data->rh_pct, out_data->battery_pct, (unsigned long)out_data->battery_mv, out_data->rssi_dbm);

    return ESP_OK;
}
