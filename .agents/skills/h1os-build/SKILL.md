---
name: h1os-build
description: Build, clean, and verify humid1-os firmware on ESP-IDF v6.1+ using CMakePresets (dev and prod).
---

# Skill: humid1-os Firmware Build & Pre-Flight Verification

## 1. Pre-Flight Verification Commands
```bash
python firmware/tools/agent/sanitize.py --check
```
Also verify UI label pixel budgets via `embedded-ops/epd_audit_screens(workspace_dir="C:/Users/Matt/Documents/GitHub/humid1-os")` (`<= 188 px`).

## 2. ESP-IDF v6.1 Build Commands (Run inside `firmware/`)
* **Development Profile (`build_dev`):**
  ```powershell
  cd firmware
  idf.py -B build_dev --preset dev build
  ```
* **Production Profile (`build_prod`):**
  ```powershell
  cd firmware
  idf.py -B build_prod --preset prod build
  ```
