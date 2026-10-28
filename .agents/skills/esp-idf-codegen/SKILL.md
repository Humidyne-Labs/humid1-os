---
name: esp-idf-codegen
description: Conservative, verified C/C++ firmware generation and patching for ESP-IDF projects. Use whenever writing, editing, refactoring, or reviewing ESP32 / ESP-IDF C source code, drivers, or header files.
---

# ESP-IDF Code Generation Protocol

## 1. Operating Rules & Blast Radius
- **Single File Only:** Modify or generate code for strictly one file per request. Never chain edits across multiple files.
- **No Monolithic Overwrites:** Provide exact snippets, diffs, or standalone functions. Do not rewrite unmodified setup code or replace whole files unless explicitly told to initialize a blank file.
- **No Autopilot / Auto-Commit:** Generate the code and stop. Do not attempt to automatically stage, commit, or trigger secondary build loops without confirmation.

## 2. API Grounding & Zero Hallucination
- **ESP-IDF Registry Check:** Only use standard, existing ESP-IDF APIs and drivers.
- **No Invented APIs:** If an API function or macro name is uncertain, do not guess standard-sounding names (e.g., `esp_sleep_enable_ulp_gpio_wakeup()`). Query the local ESP-IDF docs/MCP server or inspect project header files first.
- **Diagnostics First:** Assume a `clangd` language server is active. Ensure all referenced types, macros, and function declarations have concrete header `#include` statements present in the file.

## 3. Hardware & Silicon Constraints
- **Pin & Domain Verification:**
  - Verify pin capabilities before configuring (e.g., ESP32-S3 RTC GPIO is strictly GPIO 0–21).
  - Explicitly identify power domain dependencies (e.g., keeping `ESP_PD_DOMAIN_RTC_PERIPH` enabled during deep sleep if RTC IO interrupt detectors or pull-ups/pull-downs are active).
- **FreeRTOS Rules:**
  - Never call blocking APIs or non-ISR FreeRTOS primitives from interrupt service routines.
  - Never spin in infinite `while(1)` loops inside FreeRTOS tasks without explicit `vTaskDelay()` or event waits.

## 4. Build Error "Two-Strike" Policy
If the build tool or compiler reports an error following an edit:
1. Isolate the exact compiler diagnostic from the terminal or `compile_commands.json`.
2. Propose exactly one targeted syntactic or signature fix.
3. If the compilation still fails, **STOP IMMEDIATELY**. Do not guess alternative signatures, suppress warnings, or cast types to force compilation. State the diagnostic verbatim and ask the user for confirmation.