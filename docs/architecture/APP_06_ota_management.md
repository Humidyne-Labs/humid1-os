# APP_06: Dual-Slot Over-The-Air (OTA) Firmware Update Architecture

* **Document ID:** `APP-ARCH-006` (Rev 2.0)
* **Parent Blueprint:** [`Architectural_Outline.md`](../../Architectural_Outline.md)
* **Normative Scope:** Dual-Slot Flash Partitioning (`ota_0` / `ota_1`), Wake-Check FOTA Workflow, NVS & Claim Persistence Across Upgrades, Battery Safety Gates, and Rollback Verification

---

## 1. Flash Partition Topology (16 MB Quad/Octal Flash)

As defined in [`firmware/partitions.csv`](../../firmware/partitions.csv), the 16 MB SPI flash is partitioned with dual $4\,\text{MB}$ application slots (`ota_0` and `ota_1`) to guarantee atomic, fail-safe firmware upgrades:

| Partition Name | Type | SubType | Offset | Size | Purpose |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `nvs` | `data` | `nvs` | `0x009000` | `24 KB` | Persistent Wi-Fi credentials, `tb_token`, claim state, and user preferences. |
| `otadata` | `data` | `ota` | `0x00F000` | `8 KB` | ESP-IDF dual-slot boot selector and rollback state (`ESP_OTA_IMG_PENDING_VERIFY`). |
| `phy_init` | `data` | `phy` | `0x011000` | `4 KB` | RF calibration data. |
| `ota_0` | `app` | `ota_0` | `0x020000` | `4096 KB` | Primary application slot A. |
| `ota_1` | `app` | `ota_1` | `0x420000` | `4096 KB` | Secondary application slot B. |
| `coredump` | `data` | `coredump` | `0x820000` | `64 KB` | Crash backtrace telemetry storage. |

---

## 2. Wake-Check OTA Lifecycle ("On Wake: Check Update -> Update or Sleep")

Per product requirements, the firmware evaluates pending OTA updates during its periodic `Flow B` wake burst (or when triggered via the Web Dashboard `OtaUpdateCenter.tsx`):

```mermaid
graph TD;
    Wake["Flow B Wake / Interactive Sync"] --> FetchAttr["Fetch Shared Attributes (fw_title, fw_version, fw_checksum, auto_update_enabled, manual_ota_trigger)"];
    FetchAttr --> EvalTrigger{"(manual_ota_trigger == true) OR (auto_update_enabled == true AND fw_version > current)?"};
    EvalTrigger -->|No Update Pending| NormalSleep["Flush EPD Dashboard & Enter Deep Sleep (flow_b_period_sec)"];
    EvalTrigger -->|Update Pending| BattGate{"Battery SOC >= 25% OR USB VBUS Present?"};
    BattGate -->|No (Low Battery)| DeferOTA["Publish fw_state: 'DEFERRED_LOW_BATTERY' & Enter Deep Sleep"];
    BattGate -->|Yes| StartOTA["Transition to APP_STATE_OTA_UPDATE & Render screen_ota"];
    StartOTA --> Download["Stream Firmware Image to Inactive Slot (ota_0 / ota_1)"];
    Download --> Verify["Verify SHA-256 Checksum == fw_checksum & Image Magic 0xE9"];
    Verify -->|Checksum Mismatch| AbortOTA["Abort OTA, Publish fw_state: 'FAILED', Resume Normal Sleep"];
    Verify -->|Verified OK| ClearTrigger["Clear manual_ota_trigger, Set Boot Partition, Publish 'UPDATING'"];
    ClearTrigger --> Reboot["esp_restart() into New Firmware Slot"];
    Reboot --> PostBoot["New Firmware Boots -> Verify Sensors & MQTTS -> esp_ota_mark_app_valid_cancel_rollback()"];
```

### 2.1 Trigger Conditions

During every `Flow B` attribute synchronization (`v1/devices/me/attributes/response/+`), `app_ota_check_and_run()` inspects the ThingsBoard OTA shared attributes:
- `fw_title`: Must match `"humid1-os"`.
- `fw_version`: Target semantic version string (e.g., `"0.2.0"`).
- `fw_checksum` & `fw_checksum_algorithm`: Expected `SHA256` digest of the binary image.
- `auto_update_enabled` (`bool`) & `manual_ota_trigger` (`bool`): Update proceeds if `fw_version` differs from `APP_FW_VERSION` and either `auto_update_enabled == true` or `manual_ota_trigger == true`.

### 2.2 Battery & Brownout Protection Gate

Flashing 1–2 MB over Wi-Fi consumes $\sim 110\,\text{mA}$ for 15–35 seconds. To prevent mid-flash brownout:
- The update is permitted only if `telemetry.battery.vbus_present == true` (USB power connected) **OR** (`telemetry.battery.soc_pct >= 25` AND `telemetry.battery.voltage_mv >= 3650`).
- If the battery is below threshold, the firmware publishes `{"fw_state": "DEFERRED_LOW_BATTERY"}` to `v1/devices/me/telemetry` and proceeds directly to deep sleep.

---

## 3. NVS Credential Persistence & Server-Driven Compatibility Across OTA

A core architectural mandate from the product specification is:
> *"Updates should not require user to re-claim the device, or reset wifi creds, should adapt to new structs (Server drives internal structure)."*

1. **Zero NVS Wipe During OTA:** The `nvs` partition (`0x009000`) is completely untouched during `ota_0` $\leftrightarrow$ `ota_1` updates. Wi-Fi credentials (`nvs.net80211`), ThingsBoard device access token (`"tb_token"`), and customer claim status (`"claimed" = 1`) persist seamlessly across all OTA upgrades.
2. **Forward-Compatible NVS & Server Schema:**
   - Each NVS key in `"app_cfg"` is stored as an independent key-value pair (never as a raw C struct blob whose memory layout could shift across compiler versions). If a newer firmware introduces a new NVS key, `app_state_load_nvs()` initializes only the missing key with its default value while preserving all existing keys.
   - Because telemetry serialization and alarm evaluation are **server-driven** via JSON keys and `telemetry_schema_mask`, upgrading firmware never invalidates historical telemetry streams or dashboard widgets.

---

## 4. ThingsBoard FOTA Telemetry State Reporting & Rollback Guard

Throughout the update lifecycle, the firmware publishes standard ThingsBoard FOTA status telemetry to `v1/devices/me/telemetry`:

1. `"fw_state": "DOWNLOADING"` — When chunked MQTT (`v2/fw/request/{id}/chunk/{chunk}`) or HTTPS firmware streaming begins.
2. `"fw_state": "DOWNLOADED"` — When all bytes are written to the inactive OTA partition.
3. `"fw_state": "VERIFIED"` — After hardware-accelerated `mbedtls_sha256` matches `fw_checksum`.
4. `"fw_state": "UPDATING"` — Immediately prior to calling `esp_ota_set_boot_partition()` and `esp_restart()`.
5. `"fw_state": "UPDATED"` (plus updated `CLIENT_SCOPE` `"fw_version"` and `"current_fw_version"`) — On first boot of the new image, **after** `h1os_start()` initializes sensors and `cloud_tb_sync()` confirms MQTTS connectivity, at which point the firmware invokes:
   ```c
   esp_ota_mark_app_valid_cancel_rollback();
   ```
   If the new firmware crashes or fails to reach `esp_ota_mark_app_valid_cancel_rollback()` before the bootloader watchdog expires, the ESP-IDF bootloader automatically rolls back to the previous working slot (`ota_0` or `ota_1`).
