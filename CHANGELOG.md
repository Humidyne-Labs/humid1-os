# Changelog

All notable architectural, documentation, and firmware changes to **humid1-os** are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.2.0-dev] - 2026-11-02

### 🛠️ Semantic Alignment with `H1OS v2.4.9-dev` & Layer 4 Skeleton Overhaul
* **Declarative Lifecycle & Parallel Dual-Core Concurrency (`APP-AUD-01`, `APP-AUD-02`, `APP-AUD-03`)** (`firmware/main/main.c`):
  * Replaced pre-`v2.4.5` `h1os_config_t` / `H1OS_CONFIG_DEFAULT()` with `h1os_lifecycle_t` and `H1OS_LIFECYCLE_DEFAULT_CONFIG()`.
  * Updated lifecycle callback signatures to `app_on_boot(void *user_data)`, `app_on_wake(const h1os_wake_context_t *ctx, void *user_data)`, `app_on_sync(const h1os_wake_context_t *ctx, void *user_data)`, `app_on_pre_sleep(uint32_t sleep_sec, void *user_data)`, `app_on_shutdown(void *user_data)`, and `app_on_error(esp_err_t err, const char *context, void *user_data)`.
  * Moved Wi-Fi STA burst, SNTP RTC sync, and ThingsBoard MQTTS telemetry/RPC synchronization into `app_on_sync()` on Core 0 so network I/O executes in parallel with Core 1's LVGL layout and SSD1681 e-Paper display flush, dispatching inbound UI mutations to Core 1 via `h1os_ui_post()`.
* **Pure Layer 4 State, Flow Validation & Telemetry Mapping (`APP-AUD-04`, `APP-AUD-05`)** (`firmware/main/app_state.c`, `app_config.h`, `app_types.h`):
  * Removed direct Layer 3 `#include "h1os/orch_state.h"` and `orch_state_get()` mutations; wired runtime Flow A/B configuration exclusively through `h1os_set_flow_periods(flow_a_sec, flow_b_sec)` and `h1os_set_rh_alert_bounds()`.
  * Aligned Flow A (`60..14400 s`) and Flow B (`300..86400 s`) boundaries and 6-point prescaler/remainder validation with H1OS `MANDATE-02`.
  * Corrected all `h1os_telemetry_t` member bindings (`ambient_temp_k`, `dew_point_k`, `raw_shtc3_temp_c`, `vapor_pressure_hpa`, `sat_vapor_pressure_hpa`, `absolute_humidity_g`, `autonomy.autonomy_days`, `autonomy.avg_draw_ma`, `battery_mv`, `battery_percent`, `is_usb_powered`, `trinode_confidence_pct`, `is_thermally_settled`).
* **1-BPP Typography, Keypad Group Binding & QR Onboarding (`APP-AUD-06`, `APP-AUD-08`)** (`firmware/main/screens/*.c`, `main.c`):
  * Replaced non-existent font symbols (`h1os_font_ui_10..16`, `h1os_font_hero_48`) with H1OS 1-BPP bilevel fonts (`H1OS_FONT_14`, `H1OS_FONT_24`, `H1OS_FONT_32`), strictly enforcing the `>= 14px` floor and `<= 188.0 px` viewport width across all 28 screen string templates.
  * Bound `h1os_lvgl_get_indev()` to an `lv_group_t` on `lv_screen_active()` and implemented `app_keypad_event_cb()` for `BOOT` (`GPIO 0`) and `POWER` (`GPIO 18`) short-tap and 2-second hold actions, including `h1os_lvgl_request_full_refresh()` on polarity changes and `h1os_shutdown_for_storage()`.
  * Implemented `123×123 px` integer-scaled `lv_qrcode` rendering on `screen_onboarding.c`.
* **Time, Audio, Build System & 8 MB Dual-OTA Alignment (`APP-AUD-07`, `APP-AUD-09`, `APP-AUD-10`)** (`net_wifi_ble.c`, `app_ota.c`, `firmware/`):
  * Updated `net_wifi_ble.c` to use `<h1os/h1os_time.h>` (`h1os_time_is_valid()`, `h1os_time_sync_system_to_rtc()`, `h1os_time_sync_rtc_to_system()`).
  * Updated acoustic feedback to use `h1os_audio_play_tone()`, `h1os_audio_play_chime()`, and `h1os_set_volume()` gated by `is_interactive_wake && sound_enabled`.
  * Aligned `firmware/` with `ESP-IDF >= v6.1.0`, `ESP32-S3-PICO-1-N8R8` (8 MB QIO Flash, 8 MB Octal PSRAM), `https://github.com/Humidyne-Labs/H1OS.git`, `CMakePresets.json` (`dev` and `prod`), and the canonical 8 MB Dual-OTA `partitions.csv` with `epd_frame`.

### 🛡️ Agent Governance, Anti-Drift Guardrails & Documentation Suite (`APP-AUD-11`, `APP-AUD-12`)
* Overhauled `AGENTS.md`, `.agents/rules/*.md`, and `.agents/skills/*/SKILL.md` with 5 Hard Anti-Drift Guardrails locking autonomous agents to Layer 4 application development, enforcing the 5-step Spec-First workflow, and resolving all broken rule/skill links.
* Updated `Architectural_Outline.md`, `docs/h1os_reference.md`, and `docs/architecture/APP_01..08.md` (`MANDATE-01..06` marked `RESOLVED` in `H1OS v2.4.9-dev`), removed redundant `_docs/` monoliths, and added `tools/agent/` verification scripts (`sanitize.py`, `check_string_width.py`, `optimize_images.py`, `optimize_pdf.py`).

---

## [0.1.0-dev] - 2026-10-28

### Added
* Initial architectural bootstrap (`APP_01`–`APP_08`) and Layer 4 skeleton directory layout.
