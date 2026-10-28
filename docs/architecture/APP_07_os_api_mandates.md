# APP_07: H1OS Operating System API Mandates & Middleware Boundary Register

* **Document ID:** `APP-ARCH-007` (Rev 2.0)
* **Parent Blueprint:** [`Architectural_Outline.md`](../../Architectural_Outline.md)
* **Normative Scope:** Formal Register of Required H1OS Middleware Fixes, Build Packaging Corrections, and Public Layer 4 API Extensions

---

## 1. Governance Rule for OS Boundary Modifications

The `humid1-os` repository is a pure **Layer 4 application** consuming the `h1os` middleware component (`https://github.com/voidired/H1OS.git`). Application code in `firmware/main/` must never bypass `h1os.h` to mutate internal HAL registers, SPI/I2C bus locks, or ULP wakeup states directly unless documented as a temporary shimmed workaround below.

Whenever an application requirement exposes a bug, packaging defect, or missing API in `h1os`, it is logged in this register with a formal `MANDATE-XX` identifier and exact C/CMake remediation specification for the `H1OS` repository.

---

## 2. Active H1OS Middleware Mandates

### `MANDATE-01`: Standalone CMake Font Generator Path & Missing TTF Assets (`S0` Build Blocker)

- **Status:** `OPEN — VENDORED WORKAROUND ACTIVE IN HUMID1-OS`
- **Affected H1OS File:** `CMakeLists.txt` (lines 48–49 in `H1OS` v2.4.4-dev)
- **Root Cause:** `H1OS/CMakeLists.txt` locates the 1-BPP font generator using `${CMAKE_CURRENT_LIST_DIR}/../../tools/generate_1bpp_fonts.py` and `${CMAKE_CURRENT_LIST_DIR}/../../tools/fonts`. When `h1os` is pulled into `firmware/managed_components/h1os` via `idf_component.yml`, `../..` resolves to `firmware/`, requiring `firmware/tools/generate_1bpp_fonts.py` and `firmware/tools/fonts/*.ttf` to exist in the application repository. Furthermore, `firmware/tools/fonts/` in the initial archive contained only `.gitkeep` without the required `.ttf`/`.woff` files (`Montserrat-Bold.ttf`, `Montserrat-Medium.ttf`, `DejaVuSans.ttf`, `FontAwesome5-Solid+Brands+Regular.woff`).
- **Required Fix in H1OS Repository:**
  1. Bundle `tools/generate_1bpp_fonts.py` and `tools/fonts/*` directly inside the `H1OS` component directory (`${CMAKE_CURRENT_LIST_DIR}/tools/...`), OR pre-compile `h1os_fonts_1bpp.c` as a committed C source file inside `H1OS/src/`.
- **Application-Side Workaround Implemented in `humid1-os`:**
  - Updated `firmware/tools/generate_1bpp_fonts.py` so that if the TrueType font files are absent from `firmware/tools/fonts/`, it automatically synthesizes a valid, self-contained fallback `h1os_fonts_1bpp.c` delegating `h1os_font_*` symbols to LVGL built-in monochrome fonts (`lv_font_montserrat_12`..`48`), preventing build failures while preserving binary compatibility once the TTF assets are dropped into `firmware/tools/fonts/`.

---

### `MANDATE-02`: Public Runtime Setter for Dynamic Flow A & Flow B Wake Intervals (`S1` Functional Gap)

- **Status:** `OPEN — SHIM IMPLEMENTED IN app_state.c`
- **Affected H1OS Files:** `include/h1os/h1os.h`, `src/h1os_lifecycle.c`
- **Root Cause:** `h1os_start()` copies `config->session.flow_a_period_sec` and `config->session.flow_b_period_sec` into `orch_state_get()->sample_interval_sec` and `orch_state_get()->flow_b_period_sec` during early boot **before** `on_wake()` executes. When the application fetches updated `sleep_interval_sec` (`Flow B`) or `flow_a_period_sec` (`Flow A`) from ThingsBoard Shared Attributes inside `on_wake()`, `h1os.h` provides no public setter function to update the active orchestrator sleep periods prior to `h1os_sleep(st->flow_b_period_sec)` at the end of `on_wake()`.
- **Required Fix in H1OS Repository (`include/h1os/h1os.h` & `src/h1os_lifecycle.c`):**
  ```c
  /**
   * @brief Dynamically update Flow A (headless sample) and Flow B (full wake) intervals at runtime.
   *
   * Updates both the active RTC orchestrator state and the ULP/RTC wakeup ratio so the next
   * h1os_sleep() call immediately arms the PCF85063A alarm with the new cadence.
   *
   * @param flow_a_sec Headless sensor sampling interval in seconds (15..3600).
   * @param flow_b_sec Full wake & cloud telemetry interval in seconds (60..86400).
   * @return ESP_OK on success, ESP_ERR_INVALID_ARG if bounds are violated.
   */
  esp_err_t h1os_set_flow_periods(uint32_t flow_a_sec, uint32_t flow_b_sec);
  ```
- **Application-Side Workaround Implemented in `humid1-os`:**
  - `app_state_apply_flow_periods()` in `firmware/main/app_state.c` includes `<h1os/orch_state.h>` to synchronize `orch_state_get()->sample_interval_sec` and `orch_state_get()->flow_b_period_sec` immediately when ThingsBoard attributes change, ensuring zero-latency adherence to dashboard wake settings.

---

### `MANDATE-03`: Native Kelvin Psychrometric Fields & Configurable Battery Hysteresis (`S2` Metrology Alignment)

- **Status:** `PROPOSED FOR H1OS v2.5.0`
- **Affected H1OS Files:** `include/h1os/h1os.h`, `src/h1os_lifecycle.c`, `src/sub_power.c`
- **Rationale:** All ThingsBoard telemetry (`temp`), alarm evaluations (`Alarms` Rule Chain), and notification pipelines (`Compose and Send Email`, `POST FCM Alerts`) standardize on **Kelvin ($\text{K}$)** as the base temperature unit. While `humid1-os` provides exact inline conversions (`T_K = T_C + 273.15f`), exposing `ambient_temp_k` and `dew_point_k` directly in `h1os_telemetry_t` (alongside configurable battery SOC hysteresis deadband in `sub_power.c`) eliminates redundant conversions across Layer 4 modules.

---

### `MANDATE-04`: Unattended RTC Wake (`Flow B`) Network Burst Hook Semantics (`S1` Lifecycle Contract)

- **Status:** `DOCUMENTED & ENFORCED IN APP_01`
- **Affected H1OS File:** `src/h1os_lifecycle.c` (lines 430–455)
- **Observation:** In `h1os_launch_interactive_session()`, when `is_rtc_wake == true` (unattended periodic Flow B wake), H1OS acquires `h1os_lvgl_lock(1000)`, calls `s_app_lifecycle.on_wake(&ctx, user_data)`, releases `h1os_lvgl_lock()`, starts the LVGL task, drains the display buffer once, and immediately calls `h1os_sleep(st->flow_b_period_sec)`.
- **Required H1OS Enhancement:**
  1. Pass `bool is_rtc_wake` (or `h1os_wake_reason_t wake_reason`) inside `h1os_ui_context_t` so `on_wake()` does not need to inspect `orch_state_get()->last_wake_reason` to distinguish an unattended RTC `Flow B` wake from an interactive button `EXT1` wake.
  2. Optionally provide a dedicated `on_cloud_sync(bool is_rtc_wake, void *user_data)` hook invoked *before* `h1os_lvgl_lock(1000)` so network I/O is decoupled from the LVGL mutex acquisition window.

---

### `MANDATE-05`: First-Boot Unconfigured RTC Grace Handling (`S2` Diagnostic Cleanliness)

- **Status:** `DOCUMENTED`
- **Affected H1OS File:** `src/sub_rtc.c`
- **Observation:** On factory-fresh hardware prior to initial Wi-Fi provisioning and SNTP synchronization, the PCF85063A OS (Oscillator Stop) integrity bit is asserted (`year < 2025`), causing `h1os_rtc_is_time_valid()` to return `false` and log a warning on boot.
- **Resolution:** This is expected physical behavior on unprovisioned units; `humid1-os` suppresses cloud TLS connections until `net_sntp_sync_rtc()` succeeds and sets the PCF85063A register via `h1os_set_time()`, clearing the OS flag permanently.
