# Doxygen Documentation & Source Code Annotation Standards

This rule governs source code documentation across all application headers (`firmware/main/*.h`), screens, and cloud/network management source files in **Humid1-OS**.

## Documenting for Tools & Humans ("For Us and Them")

High-quality code documentation serves two equal masters:
1. **The Human Engineers:** Rapid comprehension of preconditions, hardware physics, and API contracts.
2. **The Automation Tooling:** Flawless automated generation of developer manuals via Doxygen into `docs/`.

---

## 1. Comment Block Standards

Use Javadoc-style comment blocks:

```c
/**
 * @brief Brief one-line summary of function purpose.
 *
 * @details Extended technical explanation detailing state machines,
 *          timings, or network constraints.
 *
 * @param[in]  config    Pointer to valid configuration struct. Must not be NULL.
 * @param[out] out_value Calculated telemetry value in engineering units.
 *
 * @return
 *   - ESP_OK: Operation completed successfully.
 *   - ESP_ERR_INVALID_ARG: Null pointer or parameter out of bounds.
 *   - ESP_ERR_TIMEOUT: Bus transaction timed out.
 *
 * @note Thread-safe. Acquires LVGL or network mutex.
 * @warning Must NOT be invoked from ISR context.
 */
esp_err_t app_screen_dashboard_update(const app_telemetry_t *data);
```

---

## 2. Mandatory Doxygen Tags for Functions

Every public or cross-module function prototype must include:
- `@brief`: Concise, single-sentence purpose.
- `@param[in]`, `@param[out]`, `@param[in,out]`: Direction and description of each argument.
- `@return`: Exhaustive list of return values and error conditions.
- `@note` (when applicable): Concurrency, task affinity, or FreeRTOS context.
- `@warning` (when applicable): Flash write wear, radio stack constraints, or sleep implications.

---

## 3. Struct & Enum Annotations

Use trailing `/**< ... */` comments for members and enumerators:

```c
/**
 * @brief Humid1 Application Configuration Struct.
 */
typedef struct {
    uint32_t sleep_interval_sec; /**< Wake interval between telemetry uploads. */
    char     temp_unit;          /**< Display temperature unit: 'F' or 'C'. */
    bool     sound_enabled;      /**< Audible chime enablement flag. */
    bool     claimed;            /**< Device claiming status on Humid1 cloud. */
} app_config_t;

/**
 * @brief Humid1 System Operational State.
 */
typedef enum {
    APP_STATE_PROVISIONING = 0, /**< BLE GATT Wi-Fi provisioning active with QR code. */
    APP_STATE_CLAIMING,         /**< Claim token displayed on e-Paper waiting for cloud binding. */
    APP_STATE_NORMAL_TELEMETRY, /**< Connected to ThingsBoard, syncing attributes & telemetry. */
    APP_STATE_OTA_UPDATING,     /**< Dual-slot OTA image transfer and flash write active. */
    APP_STATE_ALARM_ACTIVE      /**< Inverted high-contrast screen with server alarm chime. */
} app_state_t;
```

---

## 4. Header File Anatomy

All header files must begin with a standardized file header:

```c
/**
 * @file app_config.h
 * @brief Humid1-OS Application Configuration and Cloud Constants.
 * @copyright Copyright (c) 2026 Humidyne Labs / Humiditron
 * @license SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
```
