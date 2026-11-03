# APP_04: 200×200 1-Bit UI Screens, Typography & 2-Button Navigation

> **Module Scope:** `firmware/main/screens/screen_*.c`, `firmware/main/main.c`
> **Display Target:** Waveshare 1.54" V2 (`SSD1681`, `200×200 px`, 188 DPI, Non-Touch, 1-Bit Monochrome)

---

## 1. H1OS 1-BPP Typography & Viewport Mandates

1. **Strictly Allowed Fonts (`h1os_fonts.h`):**
   * Only H1OS build-time generated 1-bit bilevel fonts are permitted: `H1OS_FONT_14`, `H1OS_FONT_16`, `H1OS_FONT_18`, `H1OS_FONT_20`, `H1OS_FONT_24`, and `H1OS_FONT_32`.
   * **Hard Floor (`>= 14px`):** Sub-14px fonts (`10pt`, `12pt`) and 4-bpp anti-aliased LVGL built-in fonts are strictly prohibited due to stroke erosion on 1-bit e-Paper.
2. **Safe Viewport Geometry (`188×188 px` inside `6 px` Bezel Margin):**
   * All text labels and containers must sit within `x = 6..194` (`width <= 188.0 px`) and `y = 0..198`.
   * Every `snprintf()` and static label string in `firmware/main/screens/*.c` must pass `python tools/agent/check_string_width.py --audit-screens`.
3. **Zero Animations & Solid 1-Bit Contrast:**
   * All dividers and borders use `width >= 2 px` and `LV_OPA_COVER` (`255`). Translucent opacities, drop shadows, and animations are forbidden.
   * Switching between `APP_THEME_LIGHT` and `APP_THEME_DARK` must invoke `h1os_lvgl_request_full_refresh()` to scrub residual charge from the electrophoretic capsules.

---

## 2. Screen Inventory & Layout Specifications

| Screen ID | File | Primary Elements & Font Hierarchy |
| :--- | :--- | :--- |
| `SCREEN_DASHBOARD` | `screen_dashboard.c` | Header (`Nickname` + `SoC%`, `H1OS_FONT_14`), Hero RH (`%.1f%% RH`, `H1OS_FONT_32`), Settled Temp (`%.1f°C/F` or `%.1f K`, `H1OS_FONT_24`), Dew Point & Vapor Pressure (`H1OS_FONT_14`), Status/Alert Banner (`H1OS_FONT_14`), Footer (`Est: %.0fd \| Sync: %lum`, `H1OS_FONT_14`). |
| `SCREEN_DIAGNOSTICS` | `screen_diagnostics.c` | Inverted Header (`DIAGNOSTICS [2/3]`), Absolute Humidity (`g/m³`), Actual vs Saturation Vapor Pressure (`VP: %.1f/%.1f hPa`), Tri-Node Temps (`SHT`, `Die`, `EPD`, `ΔT`), Battery (`mV` & `mA`), Flow A/B intervals, Wake & Sample counters (`H1OS_FONT_14`). |
| `SCREEN_SYSTEM` | `screen_system.c` | Inverted Header (`SYSTEM MENU [3/3]`), Firmware Version (`FW: v0.2.0-dev`), 4 selectable 2px-bordered rows (`1. THEME`, `2. AUDIO`, `3. RESET CLOUD PAIR`, `4. STORAGE POWER OFF`) (`H1OS_FONT_14`). |
| `SCREEN_ONBOARDING` | `screen_onboarding.c` | Forced Light polarity, Title (`SCAN TO SETUP WIFI` / `SCAN TO CLAIM UNIT`), centered `123×123 px` `lv_qrcode`, Claim Key (`Key: H1-XXXXXXXX`), live `RH | Temp` preview (`H1OS_FONT_14`). |

---

## 3. 2-Button Hardware Keypad Navigation Matrix (`app_keypad_event_cb`)

H1OS maps `GPIO 0` (`BOOT`) and `GPIO 18` (`POWER`) into the LVGL keypad `lv_indev_t` (`h1os_lvgl_get_indev()`). Every button event calls `h1os_reset_session_timer()`:

| Active Screen | `BOOT` (`GPIO 0`) Short Tap (`LV_EVENT_KEY`: `LV_KEY_RIGHT`) | `POWER` (`GPIO 18`) Short Tap (`LV_EVENT_SHORT_CLICKED`) | `POWER` (`GPIO 18`) >= 2.0s Hold (`LV_EVENT_LONG_PRESSED`) |
| :--- | :--- | :--- | :--- |
| **Dashboard (`1/3`)** | Advance to **Diagnostics (`2/3`)** | Cycle Temp Unit (`°C` $ightarrow$ `°F` $ightarrow$ `K`) | Safe Storage Power-Off (`h1os_shutdown_for_storage()`) |
| **Diagnostics (`2/3`)**| Advance to **System Menu (`3/3`)** | Trigger Full Waveform Scrub (`h1os_lvgl_request_full_refresh()`) | Safe Storage Power-Off (`h1os_shutdown_for_storage()`) |
| **System Menu (`3/3`)**| Return to **Dashboard (`1/3`)** | Cycle Highlighted Row (`1` $ightarrow$ `2` $ightarrow$ `3` $ightarrow$ `4`) | Execute Highlighted Action (Theme / Mute / Reset / Power Off) |
| **Onboarding (`QR`)** | Toggle Live **Dashboard** Preview | — | Safe Storage Power-Off (`h1os_shutdown_for_storage()`) |
