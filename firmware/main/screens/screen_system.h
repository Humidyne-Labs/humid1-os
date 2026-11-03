/**
 * @file screen_system.h
 * @brief Screen 3: System Controls & Action Menu (200x200 1-Bit e-Paper).
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#ifndef SCREEN_SYSTEM_H
#define SCREEN_SYSTEM_H

#include "lvgl.h"
#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_SYSTEM_MENU_ITEM_COUNT  4U

void screen_system_render(lv_obj_t *parent, const app_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_SYSTEM_H */
