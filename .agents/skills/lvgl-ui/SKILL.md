---
name: lvgl-ui
description: Authoritative design guidelines, bilevel e-paper constraints, layout grids, font CSV paths, kerning metrics, and string width validation tools for LVGL v9 on SSD1681.
---

# LVGL v9 Bilevel UI Design & E-Paper Font Validation

This skill defines the canonical constraints, design patterns, font metric tables, and automated validation tools for developing LVGL v9 user interfaces on the **Waveshare ESP32-S3 ePaper 1.54" V2** (SSD1681) panel.

---

## 1. Hardware & Silicon Display Constraints

- **Resolution:** 200 x 200 pixels (1:1 square aspect ratio).
- **Usable Margin:** $X = 6\text{ px}$ to $194\text{ px}$ ($\le 188.0\text{ px}$ maximum string width).
- **Color Depth:** 1-bit Monochrome (`LV_COLOR_FORMAT_I1`), pure black and pure white.
- **Draw Buffer:** Exactly 5,000 bytes ($200 \times 200 \div 8\text{ bits}$).
- **Opacity Mandate:** Strict 100% opacity (`LV_OPA_COVER`). No alpha blending or partial opacities.
- **Thread Safety:** The LVGL presentation task is pinned to **Core 1** at FreeRTOS priority 4. Wrap all LVGL operations in `h1os_lvgl_lock()` / `h1os_lvgl_unlock()`.

---

## 2. Spatial Budget & Vertical Layout Allocation (200 px Total)

```
Y = 0 px   +-------------------------------------------------------+
           | 1. HEADER REGION (Height: 20 px, Y = 0..20)           |
           | Title, status flag, or category (14pt Bold Centered)  |
Y = 20 px  |-------------------------------------------------------| (1 px Solid Border)
           |                                                       |
           | 2. MAIN BODY CONTENT REGION (Height: 128 px)          |
           | Available: Y = 22 to 148 px                           |
           | Telemetry values, psychrometric dew point, gauge arc, |
           | QR provisioning canvas, or multi-field sensor rows.   |
           |                                                       |
Y = 148 px |-------------------------------------------------------|
           | 3. FOOTER / ACTION REGION (Height: 50 px)             |
           | Y = 150 px: Primary Status / Value                    |
           | Y = 170 px: Secondary Physical Button Action Hint    |
Y = 200 px +-------------------------------------------------------+
```

---

## 3. Font Metric CSV Tables (`tools/fonts/`)

Montserrat is a **proportional (variable-width)** font with 4-bit fractional advance widths and class-based kerning. Never use multiple spaces for horizontal alignment—use separate column labels or single-space padding.

### Character Advance Widths & Bounding Boxes
- `tools/fonts/char_widths_montserrat_14.csv`
- `tools/fonts/char_widths_montserrat_16.csv`
- `tools/fonts/char_widths_montserrat_18.csv`
- `tools/fonts/char_widths_montserrat_20.csv`
- `tools/fonts/char_widths_montserrat_24.csv`
- `tools/fonts/char_widths_montserrat_32.csv`

### Class Kerning Pair Adjustments (3,400+ pairs per font)
- `tools/fonts/kern_pairs_montserrat_14.csv`
- `tools/fonts/kern_pairs_montserrat_16.csv`
- `tools/fonts/kern_pairs_montserrat_18.csv`
- `tools/fonts/kern_pairs_montserrat_20.csv`
- `tools/fonts/kern_pairs_montserrat_24.csv`
- `tools/fonts/kern_pairs_montserrat_32.csv`

---

## 4. Layout Verification & Tool Execution

### A. Check Single String Width (CLI)
Calculates exact sub-pixel string width $\sum(\text{adv\_w} + \text{kern})$ with per-character breakdown:
```powershell
# Evaluate string against 188px limit in Montserrat-14 (default):
python3 firmware/tools/agent/check_string_width.py "Power: CHARGED 4.00V"

# Evaluate in a different font size (e.g. 20pt hero text):
python3 firmware/tools/agent/check_string_width.py --size 20 "72.5% RH"
```

### B. Automated Screen File Audit (`--audit-screens`)
Scans all C screen files in `firmware/main/screens/`, extracts string formatting templates, simulates worst-case parameter expansion, and asserts that no string exceeds $188.0\text{ px}$:
```powershell
python3 firmware/tools/agent/check_string_width.py --audit-screens
```

### C. Re-Extract Font Metrics & Kerning CSVs
Re-generates all 12 CSV tables from generated LVGL 1bpp C font files:
```powershell
python3 firmware/tools/agent/extract_font_metrics.py
```

### D. Query Metrics via DuckDB SQL
Query glyph or kerning metrics using DuckDB MCP:
```sql
-- Widest ASCII characters in Montserrat-14:
SELECT char, width_px FROM 'tools/fonts/char_widths_montserrat_14.csv' WHERE unicode_dec < 128 ORDER BY width_px DESC LIMIT 8;

-- Negative kerning adjustments (tighter pairs):
SELECT left_char, right_char, kern_px FROM 'tools/fonts/kern_pairs_montserrat_14.csv' WHERE kern_px < 0 ORDER BY kern_px ASC LIMIT 10;
```
