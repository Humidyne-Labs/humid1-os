/**
 * @file screen_dashboard.h
 * @brief Humidor Environmental Telemetry Dashboard Screen Header
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "h1os/h1os.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Render the primary humidor dashboard screen.
 *
 * @param[in] telem       Current validated environmental telemetry from H1OS.
 * @param[in] cycle_count Monotonic wake/render cycle counter.
 */
void screen_dashboard_render(const h1os_telemetry_t *telem, uint32_t cycle_count);

#ifdef __cplusplus
}
#endif
