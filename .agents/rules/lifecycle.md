# Master Rule: Humid1-OS Application Lifecycle & H1OS Middleware Integration

## 1. Primary Directive
`h1os_start()` is the **MANDATORY single entry point** and lifecycle orchestrator for application firmware in Humid1-OS.

Direct low-level peripheral manipulation, raw register accesses, manual reset reason parsing (`esp_reset_reason()`), raw GPIO wake mask configuration, or bypassing the H1OS public API (`h1os/h1os.h`, `h1os/h1os_lvgl.h`, `h1os/h1os_time.h`) in `app_main()` is **PROHIBITED**.

All application initialization, telemetry queries, display updates, and sleep transitions MUST be driven through the declarative `h1os_lifecycle_t` event framework and H1OS public contracts.

---

## 2. Architecture Boundary Hierarchy

```
+-------------------------------------------------------------------------------+
| APPLICATION & CLOUD ECOSYSTEM TIER (humid1-os)                                |
| - firmware/main/: Humidor monitoring, ThingsBoard MQTTS/HTTPS client          |
| - screens/: LVGL v9 screens (Hero dashboard, BLE QR code, Claiming, Inverted) |
| - cloud/: Device claiming, dynamic attributes, remote RPC, dual-slot OTA      |
| - network/: BLE GATT provisioning, Wi-Fi fast-connect, SNTP time sync         |
+-------------------------------------------------------------------------------+
                                       |
                                       v  (Clean Public API: h1os.h, h1os_lvgl.h)
+-------------------------------------------------------------------------------+
| OS & MIDDLEWARE TIER (H1OS v2.4.4-dev via idf_component.yml)                  |
| - Layer 4: Application Lifecycle Engine & LVGL v9 Task pinned to Core 1       |
| - Layer 3: System Orchestrator (Flow A / Flow B arbiter, RTC Slow Mem, Math)  |
| - Layer 2: Peripheral Subsystems (SSD1681 EPD, SHTC3, PCF85063A RTC, ES8311)  |
| - Layer 1: Hardware Abstraction Layer (HAL IO, SPI DMA, I2C, Power Latch)    |
| - ULP-RISC-V Coprocessor Environmental Monitor                                |
+-------------------------------------------------------------------------------+
```

### Hierarchy & Boundary Invariants:
1. **Unidirectional Control:** Humid1-OS calls H1OS public APIs; application code never hacks or manipulates low-level hardware registers directly.
2. **OS API Feature Mandate Protocol:** If Humid1-OS requires a middleware capability not yet exposed by H1OS, document it formally in [`docs/architecture/APP_07_os_api_mandates.md`](../../docs/architecture/APP_07_os_api_mandates.md) so it can be resolved upstream in H1OS.
3. **Dual Execution Flows in H1OS:**
   - **Flow A:** Headless sensor sample burst in fast RTC wake stub (<10 ms at 80 MHz, zero PCB self-heating).
   - **Flow B:** Interactive user session (FreeRTOS SMP, LVGL v9 on Core 1, Wi-Fi/ThingsBoard telemetry upload, sleep transition).

---

## 3. Application Lifecycle Sequence

1. **Cold Boot / Provisioning Check:**
   - On initial power-up or unprovisioned state, start BLE GATT provisioning service.
   - Generate hardware TRNG Proof of Possession (PoP) PIN.
   - Render LVGL QR code (`lv_qrcode_create`) with device name and PoP PIN on e-Paper.
   - Once Wi-Fi credentials are confirmed, tear down BLE radio completely.
2. **SNTP Time Synchronization:**
   - Connect to external SNTP server (`pool.ntp.org`) to acquire authoritative epoch time.
   - Synchronize NXP PCF85063A hardware RTC to ensure TLS/SSL cert validation succeeds.
3. **ThingsBoard Custom Cloud Connection & Claiming:**
   - Connect to self-hosted ThingsBoard at [Humid1.com](https://www.humid1.com/) via MQTTS/TLS.
   - If unclaimed: generate 6-character claiming token, publish to `v1/devices/me/claim`, and render token on screen.
   - If claimed: publish telemetry (`temp_k`, `humidity_pct`, `dew_point_k`, `vbat_mv`, `battery_pct`), receive dynamic shared attributes, check OTA channel.
4. **Display Flush & Fast Sleep Transition:**
   - Update LVGL hero screen with current readings (converting Kelvin to °F/°C).
   - Flush e-Paper frame via `h1os_sleep()` or pre-sleep hooks.
   - Enter deep sleep immediately to preserve battery voltage (`VBATT`).
