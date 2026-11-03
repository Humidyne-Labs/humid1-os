---
name: h1os-build
description: Build, clean, and verify humid1-os firmware on ESP-IDF v6.1+ using CMakePresets (dev and prod).
---

# Skill: humid1-os Firmware Build & Pre-Flight Verification

## 1. Pre-Flight Verification Commands (Run from Repository Root)
```bash
python tools/agent/sanitize.py --check
python tools/agent/check_string_width.py --audit-screens
```

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
