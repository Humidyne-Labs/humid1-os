# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.4.4-dev] - 2026-10-27

### 🚀 Added
* **Terraformed Application Firmware Architecture** (`firmware/`): Migrated repository to a pure application and cloud ecosystem tier built upon H1OS v2.4.4-dev (`components/h1os` via ESP-IDF Component Manager).
* **Authoritative Partition Table** (`firmware/partitions.csv`): Synchronized canonical H1OS 8MB dual-slot OTA table (`ota_0`, `ota_1`, `storage`, `epd_frame`, `nvs`, `otadata`).
* **Initial Humidor Presentation Layer** (`firmware/main/screens/screen_dashboard.c`): Implemented 1-bit monochrome 200x200 hero dashboard with dynamic Kelvin-to-Fahrenheit conversion.
* **Master Architectural Documentation Suite** (`docs/architecture/`): Authoritative blueprints `APP_01` through `APP_07` detailing system architecture, custom ThingsBoard cloud telemetry, BLE GATT QR code provisioning, bilevel UI, audio feedback, dual-slot rollback OTA, and the upstream OS API mandate register.
* **Senior Agent Engineering Suite** (`.agents/rules/`, `.agents/skills/`, `AGENTS.md`): Integrated Senior Architect Field Guide, 3-Finger MCP tool harness, subagent orchestration, and pre-merge governance.
* **Vendored Asset & Typography Tools** (`firmware/tools/`): Included 1-bit font generator, Montserrat font CSVs, and repository hygiene tools (`sanitize.py`, `optimize_images.py`, `optimize_pdf.py`).

### 🔄 Changed
* **Repository Layout**: Consolidated firmware build tree under `firmware/` while maintaining shared assets and documentation at repository root.
* **Asset Hygiene**: Quantized test bench and banner graphics with progressive JPEG encoding and `HUMIDITRON/HUMADYNE_LABS` EXIF attribution, shrinking asset footprint by 90.9%.

### 🗑️ Removed
* **Legacy Monolithic Codebase** (`Source_Code/`): Removed non-functional legacy BSP code after creating a full backup at `c:\Users\Matt\Documents\GitHub\humid1-os_legacy_source_backup` and preserving on branch `legacy-bsp-archive`.
