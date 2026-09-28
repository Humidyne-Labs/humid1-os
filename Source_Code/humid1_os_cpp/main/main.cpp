/**
 * @file main.cpp
 * @brief ESP32-S3 ePaper BSP - Production Environmental Telemetry Node
 * 
 * Hardware Target: Waveshare ESP32-S3-Touch-ePaper-1.54 V2 (200x200 1-bit Mono EPD)
 * 
 * @copyright Copyright (c) 2026 Humidyne Labs / Humiditron
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_app_desc.h"
#include "lvgl.h"
#include "bsp/bsp.h"
#include "mmap_generate_storage.h"

static const char *TAG = "HUMID1_OS";

extern "C" void app_main(void)
{
    while(1); //test
}
