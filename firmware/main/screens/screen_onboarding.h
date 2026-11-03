/**
 * @file screen_onboarding.h
 * @brief Screen 4: Out-of-box QR Code & BLE/ThingsBoard Claiming Screen (200x200 1-Bit e-Paper).
 *
 * Copyright (c) 2026 Humidyne Labs
 * SPDX-License-Identifier: MIT
 */

#ifndef SCREEN_ONBOARDING_H
#define SCREEN_ONBOARDING_H

#include "lvgl.h"
#include "app_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void screen_onboarding_render(lv_obj_t *parent, const app_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_ONBOARDING_H */
