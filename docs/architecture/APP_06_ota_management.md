# APP_06: 8 MB Dual-Slot OTA Architecture & Rollback Safety

> **Module Scope:** `firmware/main/app_ota.h`, `firmware/main/app_ota.c`, `firmware/partitions.csv`
> **Target Flash:** `ESP32-S3-PICO-1-N8R8` (**8 MB QIO Flash**)

---

## 1. Canonical 8 MB Dual-Slot Partition Table (`firmware/partitions.csv`)

`humid1-os` uses the canonical H1OS 8 MB partition layout (`components/h1os/project_include.cmake` and `firmware/partitions.csv`):

| Name | Type | SubType | Offset | Size | Purpose |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `nvs` | `data` | `nvs` | `0x009000` | `24 KB` (`0x6000`) | Persistent Wi-Fi, ThingsBoard token, user settings, and autonomy runtime counters. |
| `otadata` | `data` | `ota` | `0x00F000` | `8 KB` (`0x2000`) | Dual-slot boot selector & `ESP_OTA_IMG_PENDING_VERIFY` state. |
| `phy_init` | `data` | `phy` | `0x011000` | `4 KB` (`0x1000`) | RF calibration data. |
| `ota_0` | `app` | `ota_0` | `0x020000` | `3.0 MB` (`0x300000`) | Primary OTA application slot A. |
| `ota_1` | `app` | `ota_1` | `0x320000` | `3.0 MB` (`0x300000`) | Secondary OTA application slot B. |
| `storage` | `data` | `fat` | `0x620000` | `1.86 MB` (`0x1DC000`) | Internal FATFS flash storage. |
| `epd_frame` | `data` | `0x40` | `0x7FC000` | `16 KB` (`0x4000`) | H1OS persistent 1-bit e-Paper shadow framebuffer (`orch_display_sync`). |
| `coredump` | `data` | `coredump`| `0x800000` | `0 KB` | Crash diagnostics marker. |

---

## 2. OTA Safety Gates & Sleep Barrier Hold

1. **Battery Safety Gate (`app_ota_is_update_ready()`):**
   * An OTA download is permitted ONLY if `vbus_present == true` OR (`battery_soc_pct >= 25%` AND `battery_mv >= 3650 mV`).
2. **Deep-Sleep Barrier Hold (`h1os_sync_hold()`):**
   * Before streaming OTA chunks inside `app_on_sync()`, `app_ota_execute_update()` calls `h1os_sync_hold()` so H1OS does not enter deep sleep while flash writes are in progress.
3. **Anti-Brick Rollback Verification (`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y`):**
   * On the first boot into a newly flashed OTA slot (`ESP_OTA_IMG_PENDING_VERIFY`), `app_on_boot()` invokes `app_ota_mark_running_slot_valid()` (`esp_ota_mark_app_valid_cancel_rollback()`) once core initialization succeeds.
