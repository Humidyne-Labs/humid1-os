# Humid1-OS (`v0.2.0-dev`) — Smart Humidor & Precision Environmental Appliance Firmware

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v6.1-blue.svg)](https://idf.espressif.com/)
[![H1OS](https://img.shields.io/badge/H1OS-%3E%3Dv2.4.9--dev-brightgreen.svg)](https://github.com/Humidyne-Labs/H1OS)
[![Target](https://img.shields.io/badge/Hardware-Waveshare%20ESP32--S3--ePaper--1.54%20V2-green.svg)](https://www.waveshare.com)
[![License](https://img.shields.io/badge/License-MIT-purple.svg)](LICENSE)

**Humid1-OS** is the Layer 4 user application firmware for the **Humidyne Labs** smart humidor and precision environmental monitoring appliance. It runs on the **Waveshare ESP32-S3 ePaper 1.54" V2** (`ESP32-S3-PICO-1-N8R8`, 8 MB QIO Flash, 8 MB Octal PSRAM, Non-Touch SSD1681 200×200 1-Bit e-Paper) and consumes [**H1OS (`>= v2.4.9-dev`)**](https://github.com/Humidyne-Labs/H1OS) as an external ESP-IDF middleware component.

---

## 1. Key Architectural Capabilities

* **Declarative Dual-Core H1OS Lifecycle (`MANDATE-04`):**
  * **Flow A (Headless Sensor Burst):** `~18.5 ms` @ 80 MHz in H1OS `RTC_IRAM_ATTR` wake stub (`orch_wake_stub.c`), sampling SHTC3 (`0x70`) and MCU die TSENS with zero PCB thermal self-heating.
  * **Flow B (Parallel Display & Cloud Sync):** Core 1 renders the 200×200 1-bit LVGL v9 interface and drives the SSD1681 e-Paper display while Core 0 executes `app_on_sync()` (Wi-Fi STA burst, SNTP PCF85063A RTC sync, ThingsBoard CE MQTTS telemetry publish, shared attribute sync, and server RPC dispatch) in parallel.
* **4 Monochrome 1-Bit LVGL v9 Screens (`H1OS_FONT_14..32`):**
  * **Screen 1 (`screen_dashboard.c`):** Hero relative humidity (`H1OS_FONT_32`), settled temperature (`°C`/`°F`/`K`, `H1OS_FONT_24`), dew point, vapor pressure (`hPa`/`kPa`), compound alarm pill, and empirical battery autonomy projection.
  * **Screen 2 (`screen_diagnostics.c`):** Absolute humidity (`g/m³`), actual vs. saturation vapor pressure, Tri-Node temperature verification (`SHTC3`, `MCU Die`, `EPD Glass`, `ΔT`), battery voltage/current, and wake/sample counters.
  * **Screen 3 (`screen_system.c`):** Interactive 2-button system menu for Light/Dark polarity toggling (`h1os_lvgl_request_full_refresh()`), audio mute, cloud pairing reset, and clean-white safe storage power-off (`h1os_shutdown_for_storage()`).
  * **Screen 4 (`screen_onboarding.c`):** Out-of-box `123×123 px` integer-scaled Version 6 QR code and BLE GATT / ThingsBoard claiming screen.
* **Canonical 8 MB Dual-Slot OTA Layout (`firmware/partitions.csv`):**
  * Dual `3.0 MB` application slots (`ota_0`, `ota_1`), `1.86 MB` FATFS `storage`, and `16 KB` `epd_frame` persistent shadow framebuffer with `>= 25%` SoC (`3650 mV`) battery safety gating and automatic rollback protection.

---

## 2. Authoritative Documentation & Agent Governance

* **[Master Architectural Outline](Architectural_Outline.md)**
* **[H1OS (`v2.4.9-dev`) Public API Reference](docs/h1os_reference.md)**
* **[Master Agent Specification & Anti-Drift Guardrails](AGENTS.md)**
* **Modular Architecture Suite (`docs/architecture/`):**
  * [`APP_01_system_architecture.md`](docs/architecture/APP_01_system_architecture.md) — System layering, Dual-Core lifecycle, Flow A/B invariants & NVS schema
  * [`APP_02_cloud_and_telemetry.md`](docs/architecture/APP_02_cloud_and_telemetry.md) — ThingsBoard CE MQTTS telemetry, attributes & RPC dictionary
  * [`APP_03_onboarding_and_claiming.md`](docs/architecture/APP_03_onboarding_and_claiming.md) — BLE provisioning, `123×123 px` QR code & device claiming
  * [`APP_04_ui_and_screens.md`](docs/architecture/APP_04_ui_and_screens.md) — 200×200 1-bit UI screens, typography (`>= 14px`) & 2-button navigation
  * [`APP_05_audio_feedback.md`](docs/architecture/APP_05_audio_feedback.md) — Layer 4 acoustic tones/chimes & unattended Flow B silence mandate
  * [`APP_06_ota_management.md`](docs/architecture/APP_06_ota_management.md) — 8 MB Dual-Slot OTA state machine & battery safety gate
  * [`APP_07_os_api_mandates.md`](docs/architecture/APP_07_os_api_mandates.md) — H1OS cross-repository API mandate verification ledger (`100% RESOLVED`)
  * [`APP_08_audit_and_doc_governance.md`](docs/architecture/APP_08_audit_and_doc_governance.md) — Pre-flight verification gates & IEEE/embedded lexicon rules

---

## 3. Build & Pre-Flight Verification (`ESP-IDF >= v6.1.0`)

```bash
# 1. Run pre-flight lexicon & 1-BPP viewport string width gates (from repo root)
python tools/agent/sanitize.py --check
python tools/agent/check_string_width.py --audit-screens

# 2. Build development or production firmware (inside firmware/)
cd firmware
idf.py -B build_dev --preset dev build
idf.py -B build_prod --preset prod build
```
