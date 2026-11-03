# APP_07: H1OS Layer 4 API Mandate Ledger & Verification Status

> **Purpose:** Tracks cross-repository API requirements between `humid1-os` (Layer 4 Application) and `H1OS` (`components/h1os`).
> **Current Status (`H1OS v2.4.9-dev`):** **100% RESOLVED.** All 6 original architectural mandates plus the 4 third-party audit additions (`H1OS-AUD-01..04`) are implemented and verified in `H1OS v2.4.9-dev`.

---

## 1. Resolved H1OS Architectural Mandates (`MANDATE-01` – `MANDATE-06`)

| Mandate ID | Title | H1OS Implementation (`v2.4.9-dev`) | Status |
| :--- | :--- | :--- | :--- |
| **MANDATE-01** | Dual-Slot OTA Partition Layout (`ota_0` + `ota_1` + `epd_frame`) | Implemented in `components/h1os/partitions.csv` and auto-linked via `project_include.cmake` (`ota_0` 3 MB, `ota_1` 3 MB, `storage` 1.86 MB, `epd_frame` 16 KB). | **RESOLVED** |
| **MANDATE-02** | Runtime Configurable Flow A & Flow B Intervals | Implemented via `h1os_set_flow_periods(flow_a_sec, flow_b_sec)` in `h1os.h` / `h1os_lifecycle.c` with 6-point silicon & prescaler validation. | **RESOLVED** |
| **MANDATE-03** | Base Celsius Derived Unit Macros & Telemetry Fields | Implemented via `H1OS_CELSIUS_TO_KELVIN()`, `H1OS_CELSIUS_TO_FAHRENHEIT()`, `ambient_temp_k`, `dew_point_k`, and `sat_vapor_pressure_hpa` in `h1os_telemetry_t`. | **RESOLVED** |
| **MANDATE-04** | Parallel Dual-Core `on_sync` Hook & Sleep Barrier | Implemented via `lifecycle.on_sync`, `h1os_sync_hold()`, `h1os_sync_notify_complete()`, `h1os_ui_post()`, and `h1os_wake_state_t` (`BOOT`, `INTERACTIVE_SESSION`, `RTC_FLOW_B`). | **RESOLVED** |
| **MANDATE-05** | Chronological RTC Ring-Buffer History Export | Implemented via `h1os_get_telemetry_history(out_samples, max_samples, out_count)` in `h1os.h`. | **RESOLVED** |
| **MANDATE-06** | Fatal Startup Error Hook & Battery Latch Protection | Implemented via `lifecycle.on_error(err, context, user_data)` and automatic `GPIO 17` (`BAT_CTRL`) release in `h1os_handle_fatal_startup_error()`. | **RESOLVED** |

---

## 2. Third-Party Audit Enhancements Added in `H1OS v2.4.9-dev`

| Audit ID | Enhancement | Public API / Header Location | Status |
| :--- | :--- | :--- | :--- |
| **H1OS-AUD-01** | Enforce `BARRIER_SYNC_DONE` wait inside `h1os_sleep()` for interactive/manual sleep transitions | `components/h1os/src/h1os_lifecycle.c` (`h1os_sleep`) | **RESOLVED** |
| **H1OS-AUD-02** | Add `union { deep_sleep_count; wake_count; }` and `ulp_dispatch_reason` to `h1os_wake_context_t` | `components/h1os/include/h1os/h1os.h` | **RESOLVED** |
| **H1OS-AUD-03** | Add `float sat_vapor_pressure_hpa` to `h1os_telemetry_t` | `components/h1os/include/h1os/h1os.h` | **RESOLVED** |
| **H1OS-AUD-04** | Add `h1os_lvgl_request_full_refresh()` inline helper for Layer 4 theme polarity switches | `components/h1os/include/h1os/h1os_lvgl.h` | **RESOLVED** |
