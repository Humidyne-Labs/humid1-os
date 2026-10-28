/**
 * @file app_ota.h
 * @brief Dual-slot Over-The-Air (OTA) firmware update manager and boot rollback verification.
 *
 * Implements the wake-check OTA lifecycle and NVS credential preservation specified in APP_06.
 */

#ifndef HUMID1_APP_OTA_H
#define HUMID1_APP_OTA_H

#include <stdbool.h>
#include "esp_err.h"
#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Confirm the currently running OTA partition image is valid after boot self-test
 *        (cancels automatic bootloader rollback to previous slot).
 *
 * @return ESP_OK on success.
 */
esp_err_t app_ota_mark_running_slot_valid(void);

/**
 * @brief Retrieve the active boot partition label ("ota_0" or "ota_1") for UI diagnostics.
 *
 * @param[out] out_label Buffer to receive partition name (at least 16 bytes).
 * @param[in]  buf_len   Capacity of out_label in bytes.
 * @return ESP_OK on success.
 */
esp_err_t app_ota_get_running_partition_label(char *out_label, size_t buf_len);

/**
 * @brief Evaluate shared attributes after a Flow B sync to determine whether a firmware
 *        update is pending and permitted by the battery safety gate (APP_06 Section 2).
 *
 * @param[in] ctx Pointer to the master application context.
 * @return true if an OTA update should be executed prior to sleeping, false otherwise.
 */
bool app_ota_is_update_ready(const app_context_t *ctx);

/**
 * @brief Execute dual-slot firmware download, SHA-256 verification, boot slot switch,
 *        and system restart while preserving NVS Wi-Fi and claim state.
 *
 * @param[in,out] ctx Pointer to the master application context.
 * @return ESP_OK prior to restart, or an esp_err_t code if aborted.
 */
esp_err_t app_ota_execute_update(app_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* HUMID1_APP_OTA_H */
