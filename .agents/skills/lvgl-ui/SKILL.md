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

## 3. H1OS Font Metric CSV Tables (`metrics/*.csv`)

IBM Plex Sans is a **proportional (variable-width)** font with 4-bit fractional advance widths (`adv_w_fp4`, exact LVGL v9 step `(adv_w_fp4 + kern_fp4 + 8) >> 4`), while IBM Plex Mono & Terminus provide fixed tabular widths. Never use multiple spaces for proportional horizontal alignment—use separate column labels or tabular mono strikes.

Per-strike manifests (`char_widths_<symbol>.csv`, `kern_pairs_<symbol>.csv`) and `humid1_epd_vault.json.gz` are supplied by the H1OS dependency (`tools/fonts/metrics/`) or `epd-font` (`dist/h1os_distro/metrics/`).

---

## 4. Layout Verification (`embedded-ops` MCP & `epd-typography` Skill)

### A. Single String / Two-Column Budget (`embedded-ops/epd_string_budget`)
```text
embedded-ops/epd_string_budget(
  strike="humid1_font_plex_sans_normal_14",
  text="Power: CHARGED 4.00V",
  workspace_dir="C:/Users/Matt/Documents/GitHub/humid1-os"
)
```

### B. Automated Screen File Audit (`embedded-ops/epd_audit_screens`)
Scans `firmware/main/screens/*.c`, resolves widget fonts and worst-case `printf` expansions, and asserts `<= 188 px`:
```text
embedded-ops/epd_audit_screens(workspace_dir="C:/Users/Matt/Documents/GitHub/humid1-os")
```

### C. Regenerate Font Assets (`epd-font` v1.0.23)
Font regeneration and CSV extraction happen solely upstream in `epd-font` (`scripts\h1os-build-distro\02_build_fonts.bat` + `07_package_distro.bat`), then sync into `H1OS/components/h1os/tools/fonts/`.

### D. Query Metrics via DuckDB SQL
Query glyph or kerning metrics using DuckDB MCP:
```sql
SELECT char, width_px FROM read_csv_auto('firmware/managed_components/humid1__h1os/tools/fonts/metrics/char_widths_humid1_font_plex_sans_normal_14.csv') WHERE unicode_dec < 128 ORDER BY width_px DESC LIMIT 8;
```
