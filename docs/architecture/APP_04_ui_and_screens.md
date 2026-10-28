# APP_04: LVGL v9 UI Architecture & Screen Specifications

## 1. Display Hardware & Rendering Engine
- **Panel Target:** Solomon Systech SSD1681 1.54" Monochrome 200×200 pixel e-Paper display (188 DPI, non-touch).
- **Color Depth:** 1-bit Bilevel (`LV_COLOR_FORMAT_I1`). Pure black and pure white.
- **Opacity Mandate:** `LV_OPA_COVER` (100% opaque fills). No alpha blending or gray dithering.
- **Core Affinity:** Core 1 presentation task managed by H1OS (`h1os_lvgl.h`).
- **Deep Sleep Frame Cache:** Retained across sleep via the dedicated flash partition `epd_frame` (`0x4000`) and the bi-stable physical ink capsules.
- **Asset Storage:** Graphic icons, bitmaps, and font binaries reside in the `storage` partition (`0x1DC000`, SPIFFS).

---

## 2. Complete Screen Catalog

### A. The Hero Environmental Screen (`screen_dashboard`)
The hero screen is the flagship visual display of the humidor monitor:

```
+-----------------------------------+
| HUMID1-OS           [⚡ USB / 92%] |  <- Y=0..20: Header
+-----------------------------------+
|                                   |
|              68.4%                |  <- Y=32..68: Hero RH %
|        RELATIVE HUMIDITY          |
|                                   |
+-----------------------------------+
| 70.7°F (21.5°C)   Dew: 59.4°F     |  <- Y=94..140: Temp & Dew Point
+-----------------------------------+
| SETTLED (98%)       CYCLE #214    |  <- Y=154..200: Footer Status
+-----------------------------------+
```

- **Hero Metric (Relative Humidity):** Formatted with 32 px bold typography in center body.
- **Temperature Representation:** Canonical base value is **Kelvin** (`temp_k`). The display converts to **°F** or **°C** dynamically based on the ThingsBoard server attribute:
  $$T_{{}^\circ\text{F}} = (T_{\text{K}} - 273.15) \times \frac{9}{5} + 32$$
  $$T_{{}^\circ\text{C}} = T_{\text{K}} - 273.15$$
- **Dew Point:** Psychrometrically derived August-Roche-Magnus dew point displayed in the chosen unit.
- **Status Footer:** Shows thermal settling plateau state (`SETTLED` or `SETTLING %`), along with monotonic cycle counter and nickname if configured.

---

### B. High-Visibility Inverted Alert Screen (`screen_inversion`)
When ThingsBoard triggers an active alarm (`is_alarm_active == true`):
- **Visual Style:** Inverted color scheme (Solid black background `LV_COLOR_BLACK`, stark white foreground text).
- **Immediate Contrast:** Highly visible from across a room to alert the collector immediately.
- **Alert Details:**
  - Header: `⚠ CRITICAL ALERT`
  - Body: Displays the out-of-bounds metric (e.g. `RH: 56.2%` with target `65-72%`).
  - Action guidance: `Check Humidification Element`.

---

### C. Rapid Provisioning Screen (`screen_provisioning`)
- **LVGL QR Code:** Generated dynamically using `lv_qrcode_create` (112×112 px) containing the encrypted BLE pairing descriptor.
- **Monospaced Identifiers:**
  - Service Name: `PROV_HUMID1_A3F8`
  - PoP Key: `KEY: 7K9X2M4B`
- **User Instructions:** Scanned directly by the Humid1 companion mobile app for rapid Wi-Fi onboarding.

---

### D. Device Claiming Screen (`screen_claiming`)
- **Header:** `DEVICE CLAIMING`
- **Hero Claim Key:** 6-character token in large 24 px font (e.g., `B7X4K2`).
- **Device Identifiers:** Displays `Device: HUMID1_A3F8` and optional `Alias: My Cigar Box`.
- **Countdown Timer:** Displays active expiration countdown (e.g., `Expires in: 2m 45s`).
- **Footer:** `Enter PIN on Humid1.com`.

---

### E. Automatic OTA Update Screen (`screen_ota`)
- **Header:** `FIRMWARE UPDATE`
- **Version Transition:** Shows current version and incoming version (e.g., `v2.0.0 -> v2.1.0`).
- **Progress Bar:** High-contrast 1-bit linear gauge indicating transfer percentage (0..100%).
- **Warning Text:** `Do Not Power Off During Update`.

---

## 3. Anti-Ghosting & Waveform Management
Monochrome e-Paper displays exhibit charge accumulation after repetitive partial refreshes:
1. **Differential Partial Updates:** Standard steady-state wake cycles execute fast partial updates (<300 ms).
2. **Automated 6-Cycle Full Refresh:** Governed by H1OS waveform policies, a complete 6-cycle full refresh is executed automatically:
   - On initial cold boot.
   - When transitioning into or out of the inverted alert screen.
   - Periodically every $N$ partial cycles to reset DC bias on the physical ink microcapsules.
