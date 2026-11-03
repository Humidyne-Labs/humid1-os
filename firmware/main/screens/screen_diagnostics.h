/**
 * @file screen_diagnostics.h
 * @brief Screen 2: Psychrometric & Tri-Node Hardware Diagnostics (200x200 1-Bit e-Paper).
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#ifndef SCREEN_DIAGNOSTICS_H
#define SCREEN_DIAGNOSTICS_H

#include "lvgl.h"
#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void screen_diagnostics_render(lv_obj_t *parent, const app_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_DIAGNOSTICS_H */
