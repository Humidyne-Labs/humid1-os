# APP_06: Over-The-Air (OTA) Firmware Deployment

## 1. Overview
Field-deployed humidor monitors require zero-touch maintenance. `HUMID1-OS` integrates dual-slot OTA firmware updating via **Humid1's self-hosted ThingsBoard instance ([Humid1.com](https://www.humid1.com/))**.

---

## 2. Dual-Slot Flash Partition Layout
The custom 8MB flash partition table (`firmware/partitions.csv`) reserves two identical application slots to guarantee safe failover and rollback protection:

```csv
# Name,     Type, SubType, Offset, Size,     Flags
nvs,        data, nvs,           , 0x14000,
otadata,    data, ota,           , 0x2000,
phy_init,   data, phy,           , 0x1000,
ota_0,      app,  ota_0,         , 0x300000,
ota_1,      app,  ota_1,         , 0x300000,
storage,    data, spiffs,        , 0x1DC000,
epd_frame,  data, undefined,     , 0x4000,
```

- Each application partition (`ota_0` and `ota_1`) provides **3.0 MB (`0x300000`)**, leaving abundant headroom for application logic, network stacks, and graphics assets.
- `otadata` stores the active boot slot state and rollback markers.

---

## 3. Update Workflow & Verification Guard
1. **Version Check:** During the wake cycle attribute sync, ThingsBoard announces whether a target firmware version (e.g. `title`, `version`, `url`) differs from the local `APP_VERSION_STR`.
2. **Download Stream:** If an update is pending, the device enters OTA mode, displays `screen_ota` with an active progress bar, and streams the binary chunk-by-chunk directly into the passive OTA slot.
3. **Integrity & Checksum:** The binary is verified with SHA-256 and ESP-IDF image header validation before marking the slot bootable.
4. **Diagnostic Rollback Guard:** On first boot of a new image, the firmware executes a self-diagnostic run loop. If the application crashes or fails to connect, the bootloader automatically rolls back to the previous stable slot.
