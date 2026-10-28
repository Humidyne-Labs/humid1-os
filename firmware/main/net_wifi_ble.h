/**
 * @file net_wifi_ble.h
 * @brief BLE GATT Security 1 Wi-Fi Provisioning, Fast RTC Wi-Fi Burst, and SNTP RTC Synchronization.
 *
 * Implements the radio lifecycle and BLE teardown contract specified in APP_02 and APP_03.
 */

#ifndef HUMID1_NET_WIFI_BLE_H
#define HUMID1_NET_WIFI_BLE_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generate an 8-digit Proof-of-Possession PIN via hardware TRNG and start
 *        encrypted BLE GATT Wi-Fi provisioning (WIFI_PROV_SECURITY_1).
 *
 * Once Wi-Fi connects, the BLE stack is completely deinitialized and freed (APP_03).
 *
 * @param[in,out] ctx Pointer to the master application context.
 * @return ESP_OK on successful start, or an esp_err_t code.
 */
esp_err_t net_ble_prov_start(app_context_t *ctx);

/**
 * @brief Format the ESP-IDF v1 provisioning JSON string for on-screen QR code rendering.
 *
 * Example output: {"ver":"v1","name":"HUMID1_A1B2C3","pop":"84920173","transport":"ble"}
 *
 * @param[in]  ctx     Pointer to the master application context.
 * @param[out] out_buf Destination character buffer.
 * @param[in]  buf_len Capacity of out_buf in bytes.
 * @return ESP_OK on success.
 */
esp_err_t net_ble_prov_format_qr_uri(const app_context_t *ctx, char *out_buf, size_t buf_len);

/**
 * @brief Execute a bounded Wi-Fi STA connection burst using cached RTC BSSID/channel when available.
 *
 * @param[in,out] ctx        Pointer to the master application context.
 * @param[in]     timeout_ms Maximum wait time in milliseconds (bounded per APP-PIT-08).
 * @return ESP_OK when IPv4 address is acquired, ESP_ERR_TIMEOUT on timeout.
 */
esp_err_t net_wifi_connect_burst(app_context_t *ctx, uint32_t timeout_ms);

/**
 * @brief cleanly disconnect and power down the Wi-Fi radio prior to deep sleep (APP-PIT-02).
 *
 * @param[in,out] ctx Pointer to the master application context.
 * @return ESP_OK on success.
 */
esp_err_t net_wifi_stop_burst(app_context_t *ctx);

/**
 * @brief Verify that the PCF85063A hardware RTC holds a valid epoch (>= 2026), or perform
 *        a bounded SNTP synchronization ("pool.ntp.org", "time.cloudflare.com") and update
 *        the hardware RTC via h1os_set_time() (APP-PIT-03).
 *
 * @param[in,out] ctx        Pointer to the master application context.
 * @param[in]     timeout_ms Maximum wait time in milliseconds.
 * @return ESP_OK when wall-clock time is valid.
 */
esp_err_t net_sntp_ensure_valid_time(app_context_t *ctx, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* HUMID1_NET_WIFI_BLE_H */
