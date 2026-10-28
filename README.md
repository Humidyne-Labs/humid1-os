# Humid1-OS — Precision Humidor Telemetry Instrument Firmware

* **Target MCU:** ESP32-S3-WROOM-1-N16R8 (16 MB Quad Flash, 8 MB Octal PSRAM)
* **Operating System Middleware:** [`H1OS` v2.4.4+](https://github.com/voidired/H1OS.git)
* **Cloud Platform:** ThingsBoard Community Edition (`mqtts://humid1.com:8883`)
* **Display Engine:** LVGL v9 1-Bit Monochrome ($200 \times 200$ Waveshare 1.54" SSD1681 e-Paper)
* **Governance Standard:** `APP-STD-001` / `H1OS-STD-001` ("Documentation is Law")

---

## 1. Overview

**Humid1-OS** is the Layer 4 embedded application firmware for the **Humid1** battery-powered environmental telemetry instrument. Built on the **H1OS** operating system middleware, it implements:

1. **Encrypted First-Boot BLE Wi-Fi Provisioning (`APP_03`):** Uses `protocomm_ble` Security 1 with a simultaneous on-screen QR code, 8-digit hardware TRNG Proof-of-Possession PIN, and deterministic `HUMID1_<MAC>` device identifier. Once Wi-Fi connects, the BLE stack is completely deinitialized.
2. **Two-Phase ThingsBoard Registration & Persistent Claiming (`APP_02`):** Automatically provisions its device entity via `/provision/request` to obtain a permanent access token (`tb_token`), then guides the customer through `v1/devices/me/claim` with an on-screen 6-character Claim PIN and 180-second countdown timer.
3. **Server-Driven Kelvin Telemetry & Dynamic Wake Cadence (`APP_01`, `APP_02`):** Publishes server-specified telemetry in **Kelvin ($\text{K}$)** (`rh`, `temp`, `battery`, `rssi`) and synchronizes dynamic **Flow A** (headless sensor sample) and **Flow B** (cloud sync + e-Paper refresh) wake intervals from ThingsBoard Shared Attributes.
4. **Three-Screen Two-Button User Interface (`APP_04`):** Features an explicit `[ AWAKE ]` vs. `ZZZ SLEEP` mode badge, a hardcoded 30-second interactive timeout, and three user screens (**Main Dashboard**, **Diagnostics & Tri-Node Metrology**, and **System States, Alarms & Actions**) plus modal screens for BLE Provisioning, Claiming, and Dual-Slot OTA updates (`APP_06`).

---

## 2. Repository Structure

| Path | Description |
| :--- | :--- |
| [`Architectural_Outline.md`](Architectural_Outline.md) | Master architectural blueprint, system lifecycle diagram, and specification index. |
| [`AGENTS.md`](AGENTS.md) | Mandatory workflow, coding standards, and audit rules for AI coding agents. |
| [`CHANGELOG.md`](CHANGELOG.md) | Semantic Versioning 2.0.0 release and modification log. |
| [`docs/h1os_reference.md`](docs/h1os_reference.md) | Complete Layer 1–4 H1OS middleware API and hardware pinout reference. |
| [`docs/architecture/APP_01..08`](docs/architecture/) | Authoritative subsystem specifications (`APP_01` through `APP_08`). |
| [`docs/audits/APP_AUDIT_01_infrastructure_and_api_review.md`](docs/audits/APP_AUDIT_01_infrastructure_and_api_review.md) | Full-stack audit of ThingsBoard Rule Chains, Web Dashboard, and H1OS API mandates. |
| [`firmware/`](firmware/) | Clean-state ESP-IDF v5.4.1 project with modular C/H headers and skeleton implementations. |
| [`.agents/`](.agents/) | Agent rules (`.agents/rules/*.md`) and domain skills (`.agents/skills/*/SKILL.md`). |

---

## 3. Quickstart & Build Instructions

### 3.1 Prerequisites
- **ESP-IDF v5.4.1** installed and exported in your shell environment.
- Python 3.10+ with `Pillow` installed for 1-BPP LVGL font generation (`generate_1bpp_fonts.py`).

### 3.2 Building & Flashing
```bash
cd firmware
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

### 3.3 Running Automated Pre-Flight Verification Gates
Before committing any changes, run the repository verification suite from the root directory:
```bash
python3 firmware/tools/agent/sanitize.py
python3 firmware/tools/agent/check_string_width.py
```
