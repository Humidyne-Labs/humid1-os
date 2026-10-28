# AGENTS.md — Humid1-OS Autonomous Engineering Governance

* **Target Repository:** `humid1-os` (Layer 4 Production Firmware on H1OS Middleware)
* **Hardware Target:** ESP32-S3-WROOM-1-N16R8 (`16 MB` Flash, `8 MB` Octal PSRAM, Waveshare $1.54\text{"}$ $200 \times 200$ 1-Bit e-Paper)
* **Cloud Authority:** ThingsBoard Community Edition (`mqtts://humid1.com:8883`)
* **Governing Standard:** `APP-STD-001` ([`docs/architecture/APP_08_audit_and_doc_governance.md`](docs/architecture/APP_08_audit_and_doc_governance.md))

---

## 1. Prime Directive: "Documentation is Law"

Before modifying any C/H source file in `firmware/main/`, you **must** read:
1. [`Architectural_Outline.md`](Architectural_Outline.md) — Master blueprint and lifecycle flow.
2. [`docs/architecture/APP_01_system_architecture.md`](docs/architecture/APP_01_system_architecture.md) through [`APP_08_audit_and_doc_governance.md`](docs/architecture/APP_08_audit_and_doc_governance.md).
3. [`docs/h1os_reference.md`](docs/h1os_reference.md) — Public Layer 4 `<h1os/h1os.h>` API reference.

**Never implement an undocumented state, MQTT topic, JSON key, NVS key, or UI layout.** If a feature requires changing the architecture, update the corresponding `docs/architecture/APP_0X_*.md` file first. If an operation requires modifying the underlying `H1OS` middleware, log a formal entry in [`docs/architecture/APP_07_os_api_mandates.md`](docs/architecture/APP_07_os_api_mandates.md).

---

## 2. Core Architectural Invariants for Agents

1. **Server-Driven Telemetry in Kelvin (`APP_02`):**
   - All temperature values sent to ThingsBoard (`"temp"`, `"dew_point"`) and all alarm thresholds (`alarm_thresholds.temp*`) use **Kelvin ($\text{K}$)** as the canonical base unit ($T_{\text{K}} = T_{^\circ\text{C}} + 273.15$).
   - Local e-Paper screen rendering converts Kelvin to the user's selected `temp_unit` (`F`, `C`, or `K`) using `app_kelvin_to_display()`.
2. **Two-Phase ThingsBoard Onboarding (`APP_02`):**
   - **Phase 1:** Auto-provision via `/provision/request` (`username = "provision"`) to obtain `credentialsValue` and store it in NVS `"tb_token"`.
   - **Phase 2:** Publish 6-character TRNG `secretKey` + `durationMs: 180000` to `v1/devices/me/claim` and display `screen_claiming` (PIN + `HUMID1_<MAC>` + countdown timer). Once claimed, ownership is persistent across reboots and OTA updates.
3. **Encrypted First-Boot BLE Provisioning & Total Teardown (`APP_03`):**
   - Use `protocomm_ble` Security 1 (`WIFI_PROV_SECURITY_1`) with simultaneous on-screen QR code, 8-digit TRNG PoP PIN, and `HUMID1_<MAC>` device ID.
   - Immediately deinitialize and free the entire BLE stack (`WIFI_PROV_EVENT_HANDLER_FREE_BTDM` / `wifi_prov_mgr_deinit()`) once Wi-Fi connects.
4. **Dynamic Flow A/B Wake Cadence & 30s Interactive Timeout (`APP_01`):**
   - Synchronize `flow_a_period_sec` (headless sensor sample) and `sleep_interval_sec` (`Flow B` cloud sync + e-Paper refresh) from ThingsBoard Shared Attributes via `app_state_apply_flow_periods()`.
   - Keep the interactive button session timeout hardcoded at `APP_INACTIVITY_TIMEOUT_SEC = 30UL`.
5. **1-Bit Monochrome UI & Mode Indicator (`APP_04`):**
   - Enforce the $188\,\text{px}$ horizontal text width limit (`APP_UI_SAFE_TEXT_WIDTH_PX = 188U`), zero animations (`LV_LABEL_LONG_CLIP`), and always display `[ AWAKE ]` during interactive sessions and `ZZZ SLEEP` prior to entering deep sleep.

---

## 3. Active Agent Rules & Domain Skills

### 3.1 Always-On Rules (`.agents/rules/`)
- [`agent_behavior.md`](.agents/rules/agent_behavior.md) — Analytical rigor, empirical verification, and honest tradeoffs.
- [`code-formatting.md`](.agents/rules/code-formatting.md) — Explicit `<stdint.h>` types, `f` float suffixes, guard clauses, Doxygen comments, and Gump naming.
- [`debug-discipline.md`](.agents/rules/debug-discipline.md) — Root-cause isolation prior to patching.
- [`gfm.md`](.agents/rules/gfm.md) — GitHub Flavored Markdown, Mermaid (`graph TD;`), and MathJax (`$$`) syntax rules.
- [`git-commit-policy.md`](.agents/rules/git-commit-policy.md) — Explicit approval before creating git commits.
- [`image-optimization.md`](.agents/rules/image-optimization.md) & [`pdf-optimization.md`](.agents/rules/pdf-optimization.md) — Media optimization prior to reading visual assets.
- [`lexicon.md`](.agents/rules/lexicon.md) — Professional technical vocabulary enforcement.
- [`main-merge.md`](.agents/rules/main-merge.md) — Pre-merge audit and documentation verification gates.
- [`security-mandates.md`](.agents/rules/security-mandates.md) — Hardware TRNG, BLE Security 1, MQTTS TLS bundle verification, and OTA SHA-256 checks.

### 3.2 Domain Skills (`.agents/skills/`)
- [`h1os-build`](.agents/skills/h1os-build/SKILL.md) — Building, flashing, and sizing `firmware/` with ESP-IDF v5.4.1.
- [`h1os-firmware-standards`](.agents/skills/h1os-firmware-standards/SKILL.md) — Embedded C safety, static memory, and numerical precision rules.
- [`lvgl-ui`](.agents/skills/lvgl-ui/SKILL.md) — $200 \times 200$ 1-bit monochrome e-Paper layout and $188\,\text{px}$ width verification.
- [`tb-cloud-protocol`](.agents/skills/tb-cloud-protocol/SKILL.md) — ThingsBoard MQTTS auto-provisioning, claiming, Kelvin telemetry, shared attributes, and dual-slot OTA.
- [`clangd-lsp-mcp`](.agents/skills/clangd-lsp-mcp/SKILL.md) — AST navigation and compile-commands inspection in `firmware/build/`.
- [`context-First`](.agents/skills/context-First/SKILL.md) — Context-driven architecture research before editing code.
- [`git-commit`](.agents/skills/git-commit/SKILL.md) & [`shrink`](.agents/skills/shrink/SKILL.md) — Commit formatting and documentation compaction.

---

## 4. Mandatory Pre-Flight Verification Commands

Before declaring any task complete, execute from the repository root:

```bash
python3 firmware/tools/agent/sanitize.py
python3 firmware/tools/agent/check_string_width.py
```
