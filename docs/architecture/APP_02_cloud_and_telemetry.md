# APP_02: Cloud Connectivity, Zero-Heap Telemetry & RPC Protocol

> **Module Scope:** `firmware/main/cloud_tb.h`, `firmware/main/cloud_tb.c`, `firmware/main/net_wifi_ble.c`
> **Broker Endpoint:** `mqtts://humid1.com:8883` (ThingsBoard CE MQTTS over TLS 1.2/1.3)

---

## 1. Zero-Heap Steady-State Serialization Mandate

During unattended `H1OS_WAKE_STATE_RTC_FLOW_B` wakes, `cloud_tb_sync_burst()` executes on Core 0 inside `app_on_sync()` while Core 1 flushes the SSD1681 e-Paper display. To guarantee zero heap fragmentation over multi-month deployments:

* Outbound telemetry (`v1/devices/me/telemetry`) and client attributes (`v1/devices/me/attributes`) are formatted directly into a pre-allocated static `.bss` buffer (`static char s_mqtt_tx_buf[1024]`) using bounded `snprintf()`.
* Up to `APP_HISTORY_EXPORT_MAX_SAMPLES` (`8`) chronological Tri-Node samples are retrieved from `RTC_SLOW_MEM` via `h1os_get_telemetry_history()` without exposing Layer 3 internal structures.

---

## 2. Outbound Telemetry Schema (`v1/devices/me/telemetry`)

| JSON Key | Type | Unit | Source Field in `h1os_telemetry_t` |
| :--- | :--- | :--- | :--- |
| `temp_c` | `float` | `°C` | `raw.ambient_temp_c` |
| `temp_f` | `float` | `°F` | `raw.ambient_temp_f` |
| `temp_k` | `float` | `K` | `raw.ambient_temp_k` |
| `rh_pct` | `float` | `%RH` | `raw.ambient_rh_pct` |
| `dew_c` | `float` | `°C` | `raw.dew_point_c` |
| `vp_hpa` | `float` | `hPa` | `raw.vapor_pressure_hpa` |
| `svp_hpa` | `float` | `hPa` | `raw.sat_vapor_pressure_hpa` |
| `abs_hum_gm3` | `float` | `g/m³` | `raw.absolute_humidity_g` |
| `shtc3_raw_c` | `float` | `°C` | `raw.raw_shtc3_temp_c` |
| `mcu_die_c` | `float` | `°C` | `raw.mcu_die_temp_c` |
| `epd_glass_c` | `float` | `°C` | `raw.epd_glass_temp_c` |
| `trinode_delta_c`| `float` | `°C` | `raw.trinode_delta_c` |
| `confidence_pct` | `uint8` | `%` | `raw.trinode_confidence_pct` |
| `settled` | `bool` | — | `raw.is_thermally_settled` |
| `autonomy_days` | `float` | `days` | `raw.autonomy.autonomy_days` |
| `avg_ma` | `float` | `mA` | `raw.autonomy.avg_draw_ma` |
| `vbat_mv` | `uint16` | `mV` | `raw.battery_mv` |
| `soc_pct` | `uint8` | `%` | `raw.battery_percent` |
| `vbus` | `bool` | — | `raw.is_usb_powered` |
| `alarm_active` | `bool` | — | Compound alarm evaluation (`app_state_evaluate_alarms`) |
| `wake_cnt` | `uint32` | cycles | `raw.wake_count` |
| `flow_b_cnt` | `uint32` | cycles | `raw.flow_b_count` |

---

## 3. Server-Side RPC Command Dictionary (`v1/devices/me/rpc/request/+`)

All RPC mutations processed on Core 0 set `ctx->sync_ui_dirty = true` and post `app_ui_sync_update_cb` to Core 1 via `h1os_ui_post()`:

| RPC Method | `params` Schema | Action & Layer 4 Side Effects |
| :--- | :--- | :--- |
| `setNick` | `"Main Humidor"` (`string`) | Truncates to 23 chars, saves to NVS (`nick_str`), posts UI update. |
| `setFlowPeriods` | `{"flow_a_sec":180,"flow_b_sec":900}` | Validates 6 `MANDATE-02` invariants, calls `h1os_set_flow_periods()`, saves NVS. |
| `setUnits` | `{"temp_unit":0,"vp_unit":0}` | Updates `temp_unit` (`0=C, 1=F, 2=K`) and `vp_unit` (`0=hPa, 1=kPa`), saves NVS. |
| `setAlertConfig` | `{"mode":2,"temp_high_c":23.0,...}` | Updates limits, calls `h1os_set_rh_alert_bounds()`, re-evaluates alarms, saves NVS. |
| `setTheme` | `0` (`LIGHT`) or `1` (`DARK`) | Updates theme, calls `h1os_lvgl_request_full_refresh()`, posts UI update. |
| `setSound` | `true` or `false` (`bool`) | Updates `sound_enabled`, calls `h1os_set_volume()`, saves NVS. |
| `unclaimDevice` | `{}` | Clears `claimed` and `tb_token`, switches to `SCREEN_ONBOARDING` with full refresh. |


---

## 4. Telemetry Transmission Modes: Immediate (`Wake-Send-Sleep`) vs. Optional Batch Push

1. **Primary Mode (`batch_telemetry_en = false`, Default):**
   * Every unattended `H1OS_WAKE_STATE_RTC_FLOW_B` wake executes a fast **Wake $\rightarrow$ Send Settled Snapshot $\rightarrow$ Sleep** cycle on Core 0 in parallel with Core 1's e-Paper refresh (`sync_wait_ms = 0U`).
2. **Optional VBAT-Extending Batch Mode (`batch_telemetry_en = true`, `batch_flow_b_ratio = 2..16`):**
   * Configured via NVS (`batch_en`, `batch_rat`) or the `setBatchMode` RPC (`{"enabled":true,"flow_b_ratio":4}`).
   * On intermediate Flow B wakes where `(ctx->flow_b_count % batch_flow_b_ratio) != 0` and no threshold alarm is active, Core 0 skips Wi-Fi radio bring-up entirely while Core 1 still updates the local e-Paper screen.
   * On every $N$-th Flow B wake (or immediately upon any threshold alarm breach), Core 0 connects to `mqtts://humid1.com:8883` and drains the chronological RTC Slow SRAM ring buffer via `h1os_get_telemetry_history()`.
