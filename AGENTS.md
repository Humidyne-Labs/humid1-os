# Humid1-OS Master Agent Specification & Anti-Drift Guardrails

> **Application Version:** `v0.2.0-dev` | **OS Middleware Component:** `H1OS >= v2.4.9-dev`
> **Target Platform:** Waveshare ESP32-S3 ePaper 1.54" V2 (`ESP32-S3-PICO-1-N8R8`, ESP-IDF `>= v6.1.0`)

---

## 1. Prime Directive & Strict Anti-Drift Guardrails (READ FIRST)

You are developing **humid1-os**, a **Layer 4 User Application** built on the **H1OS** operating system component (`components/h1os`). To prevent scope drift, API hallucination, or unrequested refactoring, you MUST obey these **5 Hard Guardrails** at all times:

1. **GUARDRAIL-01 — Layer 4 Application Scope Lock (Never Modify H1OS Internals):**
   * Your working scope is strictly limited to `firmware/main/`, `docs/`, and `tools/` in this repository.
   * **NEVER** modify files inside `managed_components/h1os` or reach into the upstream `H1OS` repository unless explicitly instructed by the user.
   * **NEVER** `#include` H1OS Layer 1 (`hal_*.h`), Layer 2 (`sub_*.h`), or Layer 3 (`orch_*.h`) internal headers in application code, and **NEVER** call `orch_state_get()` directly. Interact with H1OS exclusively through `<h1os/h1os.h>`, `<h1os/h1os_lvgl.h>`, `<h1os/h1os_time.h>`, and `h1os_fonts.h` (documented in [`docs/h1os_reference.md`](docs/h1os_reference.md)).
2. **GUARDRAIL-02 — "Documentation is Law" (Spec-First Workflow):**
   * Every firmware module in `firmware/main/` is governed by an authoritative specification in `docs/architecture/APP_01..08.md` (see Section 2 below).
   * **NEVER** invent ad-hoc MQTT topics, RPC method names, NVS keys (`> 15` chars), flow intervals (`< 60s` Flow A or `< 300s` Flow B), or UI font symbols outside the `APP_01`–`APP_08` specifications.
   * If a task requires changing an architectural contract, you **MUST** update the corresponding `APP_0X_*.md` specification **before** modifying `.h`/`.c` files.
3. **GUARDRAIL-03 — Dual-Core Concurrency & Zero-Heat / Zero-Heap Invariants:**
   * **Core 0 (`on_boot`, `on_wake`, `on_sync`):** `app_on_boot()` and `app_on_wake()` prepare state and initial widgets prior to Core 1 startup. `app_on_sync()` runs on Core 0 **in parallel** with Core 1's LVGL display flush. Any UI mutation triggered from `app_on_sync()` (or MQTT/BLE callbacks) **MUST** be dispatched to Core 1 via `h1os_ui_post(fn, arg, timeout_ms)`.
   * **Zero-Heat Sensing:** Never poll I2C sensors from Layer 4; read environmental telemetry exclusively via `h1os_get_telemetry()` and `h1os_get_telemetry_history()`.
   * **Unattended Silence:** Never trigger audio (`h1os_audio_play_tone` / `chime`) when `ctx->wake_state == H1OS_WAKE_STATE_RTC_FLOW_B`.
   * **Zero-Heap Steady State:** Use static `.bss` buffers (`s_app_ctx`, `s_mqtt_tx_buf`) and `snprintf()`; never allocate dynamic heap buffers on recurring Flow B wakes.
4. **GUARDRAIL-04 — 200×200 1-Bit e-Paper & Typography Discipline:**
   * Use **only** `H1OS_FONT_14`, `H1OS_FONT_16`, `H1OS_FONT_18`, `H1OS_FONT_20`, `H1OS_FONT_24`, and `H1OS_FONT_32` (`>= 14px` floor).
   * Keep all text labels within the `188.0 px` safe width (`6 px` left/right margins) and verify every change with `python tools/agent/check_string_width.py --audit-screens`.
   * Always call `h1os_lvgl_request_full_refresh()` when switching between `APP_THEME_LIGHT` and `APP_THEME_DARK`.
5. **GUARDRAIL-05 — Two-Strike Compiler Rule & Task Focus Gate:**
   * Modify **only** the files required for the user's current request. Do not refactor unrelated screens or modules.
   * If `idf.py -B build_dev --preset dev build` fails, isolate the exact compiler diagnostic and apply one targeted fix. If the build fails a second time on the same issue, **STOP IMMEDIATELY**, report the exact error output, and ask the user for confirmation.

---

## 2. Mandatory 5-Step Execution Workflow & Spec Mapping

For every engineering task, execute these 5 steps in order:

1. **Step 1 (Consult Authoritative Spec):** Read [`docs/h1os_reference.md`](docs/h1os_reference.md) and the mapped `APP_0X` specification below:
   * `main.c`, `app_state.c`, `app_types.h`, `app_config.h` $ightarrow$ [`APP_01_system_architecture.md`](docs/architecture/APP_01_system_architecture.md)
   * `cloud_tb.c`, `cloud_tb.h` $ightarrow$ [`APP_02_cloud_and_telemetry.md`](docs/architecture/APP_02_cloud_and_telemetry.md)
   * `net_wifi_ble.c`, `screen_onboarding.c` $ightarrow$ [`APP_03_onboarding_and_claiming.md`](docs/architecture/APP_03_onboarding_and_claiming.md)
   * `screens/screen_*.c` $ightarrow$ [`APP_04_ui_and_screens.md`](docs/architecture/APP_04_ui_and_screens.md)
   * Acoustic chimes & chirps in `main.c` $ightarrow$ [`APP_05_audio_feedback.md`](docs/architecture/APP_05_audio_feedback.md)
   * `app_ota.c`, `firmware/partitions.csv` $ightarrow$ [`APP_06_ota_management.md`](docs/architecture/APP_06_ota_management.md)
   * H1OS cross-repo contracts $ightarrow$ [`APP_07_os_api_mandates.md`](docs/architecture/APP_07_os_api_mandates.md)
2. **Step 2 (Inspect Symbols via LSP/Headers):** Verify exact types and signatures in `firmware/main/*.h` and `docs/h1os_reference.md`.
3. **Step 3 (Implement Scoped Change):** Keep `.h` declarations and `.c` definitions synchronized with `-Wall -Wextra -Werror -Wstrict-prototypes` compliance.
4. **Step 4 (Run Automated Pre-Flight Gates):**
   * `python tools/agent/sanitize.py --check` (0 lexicon or identity violations)
   * `python tools/agent/check_string_width.py --audit-screens` (0 viewport overflows)
   * `cd firmware && idf.py -B build_dev --preset dev build` (0 compiler warnings/errors)
5. **Step 5 (Update Ledger):** Record significant behavioral or architectural changes in `CHANGELOG.md`.

---

## 3. Active Rules & Skills Index

### 3.1 Always-Active Rules (`.agents/rules/`)
* [`master.md`](.agents/rules/master.md) — Core Layer 4 scope lock, zero-guesswork policy, and Two-Strike build rule.
* [`arch.md`](.agents/rules/arch.md) — Layer 4 boundary enforcement and Dual-Core `on_sync` / `h1os_ui_post` concurrency.
* [`lifecycle.md`](.agents/rules/lifecycle.md) — `h1os_lifecycle_t` callbacks, Flow A/B slicing invariants, and sleep barrier rules.
* [`lvgl.md`](.agents/rules/lvgl.md) — 200×200 1-bit e-Paper layout, `H1OS_FONT_14..32` floor (`>= 14px`), and `<= 188px` string width rules.
* [`coding.md`](.agents/rules/coding.md) — Embedded C99/C11 static memory discipline, `-Werror` compliance, and zero-heap steady state.
* [`naming.md`](.agents/rules/naming.md) — Module prefixes (`app_`, `net_`, `cloud_`, `screen_`) and NVS `<= 15`-char key naming.
* [`lexicon.md`](.agents/rules/lexicon.md) — Mandatory IEEE/embedded engineering vocabulary and prohibited slang table.
* [`science.md`](.agents/rules/science.md) — Psychrometric unit conversions (`H1OS_CELSIUS_TO_*`) and compound alarm hysteresis.
* [`docs.md`](.agents/rules/docs.md) & [`gfm.md`](.agents/rules/gfm.md) — GitHub Flavored Markdown & Mermaid diagram formatting standards.
* [`doxygen.md`](.agents/rules/doxygen.md) — Header and function Doxygen comment standards.
* [`shrink.md`](.agents/rules/shrink.md) — Lossless PDF metadata and progressive JPEG EXIF quantization rules.
* [`main-merge.md`](.agents/rules/main-merge.md) — Pre-flight verification and release checklist.

### 3.2 On-Demand Skills (`.agents/skills/`)
* [`h1os-build`](.agents/skills/h1os-build/SKILL.md) — Building `firmware/` with `idf.py -B build_dev --preset dev build` and `build_prod`.
* [`lvgl-ui`](.agents/skills/lvgl-ui/SKILL.md) — Authoring and auditing 200×200 1-bit LVGL screens and `123×123 px` QR codes.
* [`tb-cloud-protocol`](.agents/skills/tb-cloud-protocol/SKILL.md) — ThingsBoard CE MQTTS telemetry, shared attributes, RPCs, claiming, and Dual-OTA.
* [`esp-idf-codegen`](.agents/skills/esp-idf-codegen/SKILL.md) — ESP-IDF v6.1 Wi-Fi, NimBLE, NVS, and OTA coding patterns.
* [`h1os-firmware-standards`](.agents/skills/h1os-firmware-standards/SKILL.md) — Static allocation, error handling, and Layer 4 H1OS contract patterns.
* [`clangd-lsp-mcp`](.agents/skills/clangd-lsp-mcp/SKILL.md) & [`mcp-tool-harness`](.agents/skills/mcp-tool-harness/SKILL.md) — Semantic C/C++ code intelligence workflows.
* [`gfm`](.agents/skills/gfm/SKILL.md) — Authoring valid GFM tables, alerts, and Mermaid diagrams.
* [`shrink`](.agents/skills/shrink/SKILL.md) & [`main-merge-protocol`](.agents/skills/main-merge-protocol/SKILL.md) — Pre-commit asset optimization and merge gates.
