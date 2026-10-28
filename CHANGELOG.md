# Changelog

All notable architectural and code changes to the **Humid1-OS** application firmware are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html).

---

## [0.1.0-dev] - 2026-10-27

### Added
- **Master Architecture Suite (`docs/architecture/APP_01..08` & `Architectural_Outline.md`):**
  - Specified two-phase ThingsBoard onboarding (`/provision/request` auto-provisioning followed by persistent `v1/devices/me/claim` customer claiming).
  - Established the server-driven telemetry contract using **Kelvin ($\text{K}$)** as the canonical temperature base unit (`rh`, `temp`, `battery`, `rssi`, plus `telemetry_schema_mask` extensions).
  - Defined dynamic server-controlled wake intervals for **Flow A** (`flow_a_period_sec`) and **Flow B** (`sleep_interval_sec`), paired with a hardcoded 30-second interactive timeout.
  - Designed the 3-screen 2-button user interface (`screen_dashboard`, `screen_diagnostics`, `screen_system`) and 3 lifecycle screens (`screen_provisioning`, `screen_claiming`, `screen_ota`) with a mandatory `AWAKE` / `SLEEP` mode indicator.
  - Added [`APP_08_audit_and_doc_governance.md`](docs/architecture/APP_08_audit_and_doc_governance.md) defining the `APP-STD-001` 8-domain audit framework and change documentation rules.
- **Full-Stack Infrastructure Audit (`docs/audits/APP_AUDIT_01_infrastructure_and_api_review.md`):**
  - Documented 7 findings and exact patches across ThingsBoard Rule Chains, Web Dashboard TypeScript code, and H1OS v2.4.4-dev middleware.
- **Clean-State Modular Firmware Skeleton (`firmware/main/`):**
  - Created `app_config.h`, `app_types.h`, `app_state.h/.c`, `net_wifi_ble.h/.c`, `cloud_tb.h/.c`, `app_ota.h/.c`, `main.c`, and all LVGL screen modules (`screens/screen_*.h/.c`).
  - Added `firmware/sdkconfig.defaults` and automated agent verification tools (`firmware/tools/agent/sanitize.py`, `check_string_width.py`, `optimize_images.py`, `optimize_pdf.py`).
  - Hardened `firmware/tools/generate_1bpp_fonts.py` with automatic built-in LVGL font synthesis when external `.ttf` files are absent (`MANDATE-01`).
- **Agent Rules & Skills (`.agents/` & `AGENTS.md`):**
  - Updated all rules and skills for `humid1-os` application development and added the `tb-cloud-protocol` skill.
