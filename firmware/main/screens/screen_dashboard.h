/**
 * @file screen_dashboard.h
 * @brief Screen 1: Primary environmental dashboard (200x200 1-Bit e-Paper).
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#ifndef SCREEN_DASHBOARD_H
#define SCREEN_DASHBOARD_H

#include "lvgl.h"
#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void screen_dashboard_render(lv_obj_t *parent, const app_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_DASHBOARD_H */
