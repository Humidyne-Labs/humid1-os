---
trigger: always_on
---

# Rule: Layer 4 Architecture & Dual-Core Concurrency

1. **Unidirectional Layer 4 Boundary:**
   - Application modules (`main.c`, `app_state.c`, `net_wifi_ble.c`, `cloud_tb.c`, `app_ota.c`, `screens/screen_*.c`) call into H1OS public APIs (`h1os_*`).
   - Direct access to `orch_state_get()` or raw I2C/SPI/GPIO peripherals is strictly forbidden in `firmware/main/`.
2. **Dual-Core Flow B Parallel Execution (`MANDATE-04`):**
   - `app_on_boot()` and `app_on_wake()` run on Core 0 before Core 1 LVGL task startup; initialize state and build initial screen widgets here.
   - `app_on_sync()` runs on Core 0 **in parallel** with Core 1's LVGL rendering and SSD1681 display flush.
   - **Thread-Safe UI Mutation Rule:** Never call `lv_*` functions directly from `app_on_sync()`, MQTT callbacks, or BLE callbacks on Core 0. Always post UI mutations to Core 1 via `h1os_ui_post(fn, arg, timeout_ms)`.
3. **Async Sleep Barrier Hold:**
   - If `app_on_sync()` spawns or awaits an asynchronous operation (such as an OTA update), call `h1os_sync_hold()` before returning and `h1os_sync_notify_complete()` upon completion.
