/**
 * @file sensor_reader.h
 * @brief Environmental & System Telemetry Acquisition Engine
 * 
 * Hardware Target:
 *  - SHTC3 I2C Temperature & Relative Humidity Sensor
 *  - Battery Voltage ADC
 *  - Wi-Fi RSSI Monitor
 * 
 * @attribution
 * - Humidyne Labs / Humiditron (2026)
 * 
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Environmental Telemetry Snapshot
 */
typedef struct {
    float   temp_k;         /*!< Temperature in Kelvin (Native Canonical) */
    float   temp_c;         /*!< Temperature in Celsius                   */
    float   temp_f;         /*!< Temperature in Fahrenheit                */
    float   rh_pct;         /*!< Relative Humidity percentage (0-100%)    */
    uint8_t battery_pct;    /*!< Battery State of Charge (0-100%)         */
    uint32_t battery_mv;    /*!< Battery Voltage in millivolts            */
    int     rssi_dbm;       /*!< Wi-Fi Signal Strength in dBm             */
    bool    valid;          /*!< True if sensor CRC passed                */
} app_telemetry_data_t;

/**
 * @brief Initialize Sensor Subsystem
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sensor_reader_init(void);

/**
 * @brief Read Complete Telemetry Snapshot
 * 
 * @param[out] out_data Destination struct
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sensor_reader_sample(app_telemetry_data_t *out_data);

#ifdef __cplusplus
}
#endif
