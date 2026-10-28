# Master Rule: LVGL 9 Bilevel UI Design & E-Paper Constraints Specification

## 1. Hardware & Silicon Display Constraints

All graphical user interfaces on the platform target the Waveshare ESP32-S3 ePaper 1.54" V2 panel. All UI development MUST strictly obey the physical constraints of the display controller:

* **Controller & Driver:** Solomon Systech SSD1681 active matrix electrophoretic display driver.
* **Resolution:** 200 x 200 pixels (1:1 square aspect ratio).
* **Pixel Density (DPI):** 188 DPI (1 physical pixel ≈ 0.135 mm).
* **Color Depth:** 1-bit Monochrome (`LV_COLOR_FORMAT_I1`). Pure black (`lv_color_black()`) and pure white (`lv_color_white()`).
* **Frame Draw Buffer:** Exactly 5,000 bytes ($200 \times 200 \div 8\text{ bits}$).
* **Non-Touch Interface:** No touch controller exists on the board. All user interaction is driven exclusively via physical tactile buttons (BOOT on GPIO 0, POWER on GPIO 18).

---

## 2. Bilevel Visual Styling Constraints

Because electrophoretic microcapsules physically migrate under electrostatic voltage, standard graphical anti-aliasing and translucency do not apply:

1. **Strict 100% Opacity (No Alpha Blending):**
   - Opacity MUST always be set to `LV_OPA_COVER` (100%).
   - Partial opacities (`LV_OPA_50`, `LV_OPA_TRANSP`, alpha blending) are **STRICTLY PROHIBITED**. They produce corrupt pseudo-random dither noise on 1-bit buffers.
2. **Sharp Borders & Geometry:**
   - Use 1 px solid black borders (`lv_color_black()`, `width = 1`).
   - Prefer sharp corners (`radius = 0`) or subtle corners (`radius = 2..4 px`).
3. **Contrast Hierarchy:**
   - Body backgrounds are pure white (`lv_color_white()`).
   - Emphasized telemetry cards or headers may invert to a solid black box (`lv_color_black()`) with white text (`lv_color_white()`).

---

## 3. Spatial Budget & Vertical Layout Allocation (200 px Total)

Every pixel row is accounted for in the canonical 3-tier vertical grid:

```
Y = 0 px   +-------------------------------------------------------+
           | 1. HEADER REGION (Height: 20 px, Y = 0..20)           |
           | Title, status flag, or category (14pt Bold Centered)  |
Y = 20 px  |-------------------------------------------------------| (1 px Solid Border)
           |                                                       |
           | 2. MAIN BODY CONTENT REGION (Height: 128 px)          |
           | Available: Y = 22 to 148 px                           |
           |                                                       |
           | Telemetry values, psychrometric dew point, gauge arc, |
           | QR provisioning canvas, or multi-field sensor rows.   |
           |                                                       |
Y = 148 px |-------------------------------------------------------|
           | 3. FOOTER / ACTION REGION (Height: 50 px)             |
           | Y = 150 px: Primary Status / Value                    |
           | Y = 170 px: Secondary Physical Button Action Hint    |
Y = 200 px +-------------------------------------------------------+
```

* **Header:** Maintain a 1 px solid bottom border (`LV_BORDER_SIDE_BOTTOM`).
* **Clearance:** Maintain a minimum 4 px vertical clearance between stacked labels to prevent ink bleeding.
* **Padding:** Provide at least 4 px horizontal padding from left/right screen edges (`X = 4..196 px`).

---

## 4. Typography & Font Constraints

1. **Minimum Readable Font Size Mandate (≥ 14 px Height):**
   - **Minimum font height is 14 pixels** (e.g. `LV_FONT_MONTSERRAT_14` or pre-rendered 1bpp fonts).
   - Fonts smaller than 14 px (e.g. 8pt, 10pt, 12pt) are **STRICTLY FORBIDDEN**. At 188 DPI, glyph strokes narrower than 2 physical pixels blur into illegibility during differential partial updates.
2. **Emphasis Hierarchy:**
   - Large Telemetry Metric: 24 px to 32 px height.
   - Header & Subtitle: 14 px height bold / uppercase.
   - Action & Button Hints: 14 px regular height.

---

## 5. QR Code Integer Scaling Constraints

When rendering Wi-Fi provisioning or pairing QR codes onto 1-bit e-Paper:

* **Whole-Factor Integer Scaling Mandate:**
  $$\text{Canvas Dimension} = (\text{Base Modules} + 8\text{ Quiet Zone}) \times \text{Scale Factor}$$
* **Minimum Module Width:** The scale factor MUST be an exact integer $\ge 3$ pixels per module. (Sub-pixel scaling or float scaling causes black/white module edge collision).
* **Standard JSON Provisioning Dimensioning:**
  - Version 4/5 QR code (41 modules including quiet zone):
    - Scale 3x: **$123 \times 123\text{ px}$ canvas** (Fits centered in Y = 22..148 body region).
    - Scale 4x: **$164 \times 164\text{ px}$ canvas**.

---

## 6. E-Paper Refresh Dynamics & Settling Invariants

1. **Differential Partial Refresh:** Used during active Flow B user sessions for sub-300ms flicker-free blits.
2. **Periodic Full OTP Refresh:** Executed during cold boot or every $N$ partial cycles to fully clear residual electrophoretic ghosting.
3. **Mandatory 250 ms Microcapsule Settling Delay:**
   - Before deep sleeping or cutting EPD power (`EPD_3V3_EN`), the driver MUST wait for the SSD1681 `BUSY` signal to release and enforce a mandatory 250 ms settling delay.
   - Cutting power while pigment microcapsules are migrating permanently damages panel contrast.

---

## 7. Thread Safety & Dual-Core Affinity

* **Core Affinity:** The LVGL presentation task is pinned strictly to **Core 1** at FreeRTOS priority 4. Core 0 handles sensor acquisition, system events, and wireless stacks.
* **Mutex Guard Mandate:** All UI creation, label modification, screen cleans, and theme changes MUST be wrapped in the thread-safe mutex boundary:
  ```c
  h1os_lvgl_lock();
  // Modify LVGL objects
  h1os_lvgl_unlock();
  ```

---

## 8. Canonical Screen Construction Pattern

```c
#include "h1os/h1os_lvgl.h"
#include "lvgl.h"

void ui_render_telemetry_screen(const char *title, float temp_c, float rh_pct, float dew_point_c)
{
    h1os_lvgl_lock();

    lv_obj_t *scr = lv_screen_active();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);

    // 1. Header (200x20 px)
    lv_obj_t *header = lv_obj_create(scr);
    lv_obj_set_size(header, 200, 20);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_white(), 0);
    lv_obj_set_style_border_color(header, lv_color_black(), 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_all(header, 1, 0);

    lv_obj_t *lbl_title = lv_label_create(header);
    lv_label_set_text(lbl_title, title ? title : "TELEMETRY");
    lv_obj_set_style_text_color(lbl_title, lv_color_black(), 0);
    lv_obj_align(lbl_title, LV_ALIGN_CENTER, 0, 0);

    // 2. Main Body Content (Y = 22..148 px)
    lv_obj_t *lbl_temp = lv_label_create(scr);
    lv_label_set_text_fmt(lbl_temp, "%.1f C", temp_c);
    lv_obj_set_style_text_color(lbl_temp, lv_color_black(), 0);
    lv_obj_align(lbl_temp, LV_ALIGN_TOP_MID, 0, 36);

    lv_obj_t *lbl_rh = lv_label_create(scr);
    lv_label_set_text_fmt(lbl_rh, "%.1f %% RH  |  Td: %.1f C", rh_pct, dew_point_c);
    lv_obj_set_style_text_color(lbl_rh, lv_color_black(), 0);
    lv_obj_align(lbl_rh, LV_ALIGN_TOP_MID, 0, 80);

    // 3. Footer Action Hint (Y = 150..200 px)
    lv_obj_t *lbl_hint = lv_label_create(scr);
    lv_label_set_text(lbl_hint, "BOOT: Cycle  |  HOLD: Sleep");
    lv_obj_set_style_text_color(lbl_hint, lv_color_black(), 0);
    lv_obj_align(lbl_hint, LV_ALIGN_TOP_MID, 0, 168);

    h1os_lvgl_unlock();
}
```


---

## 9. Font Metric Tables & String Width Verification Tools

To prevent horizontal text clipping and overflow on the 200x200 display (maximum allowable width $\le 188.0\text{ px}$):

### Font Metric CSV Tables (`tools/fonts/`)
- **Character Advance Widths & Bounding Boxes:**
  - `tools/fonts/char_widths_montserrat_14.csv`
  - `tools/fonts/char_widths_montserrat_16.csv`
  - `tools/fonts/char_widths_montserrat_18.csv`
  - `tools/fonts/char_widths_montserrat_20.csv`
  - `tools/fonts/char_widths_montserrat_24.csv`
  - `tools/fonts/char_widths_montserrat_32.csv`
- **Class Kerning Pairs (3,400+ pairs per font):**
  - `tools/fonts/kern_pairs_montserrat_14.csv`
  - `tools/fonts/kern_pairs_montserrat_16.csv`
  - `tools/fonts/kern_pairs_montserrat_18.csv`
  - `tools/fonts/kern_pairs_montserrat_20.csv`
  - `tools/fonts/kern_pairs_montserrat_24.csv`
  - `tools/fonts/kern_pairs_montserrat_32.csv`

### Verification Tools
* **Single String Width Calculation (CLI):**
  ```powershell
  python tools/agent/check_string_width.py --size 14 "Power: CHARGED 4.00V"
  ```
* **Full Screen Template Automated Audit:**
  ```powershell
  python tools/agent/check_string_width.py --audit-screens
  ```
* **Font Metric Extractor:**
  ```powershell
  python tools/agent/extract_font_metrics.py
  ```
* **DuckDB SQL Query:**
  ```sql
  SELECT char, width_px FROM 'tools/fonts/char_widths_montserrat_14.csv' ORDER BY width_px DESC LIMIT 10;
  ```
