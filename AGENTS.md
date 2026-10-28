# Humid1-OS Master Engineering Specification & Agent Invariants

This document constitutes the authoritative, always-active engineering and behavioral specification for autonomous agents and firmware developers working on **`humid1-os`** on ESP32-S3.

---

## 1. Core Directives & Architectural Invariants

1. **Zero Guesswork Policy (Code & Silicon):**
   - NEVER guess or extrapolate function signatures, struct members, enums, peripheral pins, or configuration options.
   - Use compiler-backed semantic code intelligence (`get_definition`, `find_references`, `get_type_info`) and authoritative headers (`h1os/h1os.h`, `h1os/h1os_lvgl.h`, `h1os/h1os_time.h`).
   - If a signature or struct member is unknown, inspect the header file directly.

2. **Empirical Ground Truth & No Made-Up Numbers ("Sherlock Ohming vs. Fact"):**
   - **Never Fabricate Metrics:** Operational figures (wake latency, deep sleep current, battery discharge rates, RF connection times) MUST be explicitly cited from authoritative documentation or verified on physical test instruments. Unverified guesses must never be stated as factual quantities.
   - **Hypothesis vs. Fact Boundary ("Sherlock Ohming"):** Observed circuit or network behavior may support a deduction (e.g., fast RTC cache hit, AP beacon timing, power latch behavior), but without instrumented verification or explicit logs, it remains an *unverified hypothesis*, never an established fact.

3. **Pure Application Repository Boundary & OS API Feature Mandate Protocol:**
   - **Underlying Middleware (H1OS v2.4.4-dev):** Low-level hardware drivers (SSD1681 1-bit e-Paper display, Sensirion SHTC3 I2C sensor, NXP PCF85063A RTC, ES8311 audio codec, power latching), ULP coprocessor execution, and zero-heat psychrometric math are encapsulated by H1OS. Application code must NEVER bypass H1OS to manipulate hardware registers directly.
   - **Application Tier (`humid1-os`):** Consumes the clean public H1OS API (`h1os.h`, `h1os_lvgl.h`, `h1os_time.h`) to implement humidor environmental monitoring, LVGL v9 screens, BLE GATT provisioning with QR code, audio feedback, and ThingsBoard cloud communication.
   - **OS API Feature Mandate:** If the application requires a capability that H1OS does not currently expose or handle cleanly, do NOT write hacky hardware workarounds in the application layer. Instead, document it in [`docs/architecture/APP_07_os_api_mandates.md`](docs/architecture/APP_07_os_api_mandates.md) as an authoritative **OS API Feature Mandate** so it can be resolved upstream in H1OS.

4. **Independent Self-Hosted Custom Cloud Infrastructure:**
   - Device telemetry, device claiming, dynamic shared attributes, remote RPC commands, and OTA updates communicate with **Humid1's self-hosted ThingsBoard Community Edition instance running on our custom cloud ([Humid1.com](https://www.humid1.com/))**.
   - The platform is completely independent of third-party SaaS cloud dependencies.
   - All server communication uses TLS/SSL (MQTTS/HTTPS), requiring accurate epoch time synchronized via external SNTP on boot.

5. **Authoritative Partition Table Preservation:**
   - The partition table is tightly bound to low-level H1OS operation. The 8MB dual-slot OTA table (`firmware/partitions.csv`) must preserve the exact byte offsets and sizes:
     - `nvs`: `0x14000`
     - `otadata`: `0x2000`
     - `phy_init`: `0x1000`
     - `ota_0` & `ota_1`: `0x300000` (3.0 MB each)
     - `storage` (SPIFFS for audio/graphics assets): `0x1DC000`
     - `epd_frame` (dedicated e-Paper frame cache across sleep): `0x4000`

6. **Compiler Diagnostics & Two-Strike Resolution Protocol:**
   - Always verify compilation with the canonical ESP-IDF profile (`C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1`).
   - If a build fails, isolate the exact diagnostic line and propose one targeted fix. If it fails a second time, **STOP IMMEDIATELY**, state the diagnostic verbatim, and request confirmation.

7. **Never Busy-Wait (Two-Domain Synchronization Law):**
   - **Awake FreeRTOS Domain:** Never spawn dedicated polling tasks or `vTaskDelay()` spin-wait loops to monitor timeouts, task exits, or hardware lines. Use `esp_timer` for timeouts/periodic operations, FreeRTOS event groups / task notifications / semaphores for synchronization, and event-driven callbacks.
   - **Fast Wake / Immediate Sleep Invariant:** When waking from deep sleep, the application must connect, publish telemetry, sync attributes, update the display, and return to deep sleep as quickly as possible. Prolonged wake intervals deplete battery voltage (`VBATT`).

---

## 2. Engineering Nomenclature & Technical Lexicon Policy

Strictly adhere to professional **Embedded Systems Hardware and Software Engineering** standards. Informal jargon, military slang, pop-culture metaphors, and ideological commentary are strictly prohibited across all committed code, documentation, and commit messages.

### Required Engineering Vocabulary Mapping
| Prohibited Jargon / Slang | Required Engineering Term | Technical Context |
| :--- | :--- | :--- |
| **stand-down / stand down** | **sleep transition / power down / suspend** | Transitioning the SoC or peripherals into deep sleep. |
| **sentinel / sentry / bouncer** | **monitor / supervisor / watchdog** | Coprocessor, task, or interrupt handler overseeing events. |
| **tripwire** | **threshold / limit trigger / brownout detector** | Voltage, thermal, or timer boundary triggering a state transition. |
| **sprint / sortie / mission** | **sample / sample burst / measurement pass** | Sensor acquisition pulse before thermal conduction. |
| **recon / reconnaissance** | **probing / polling / scanning / telemetry query** | Checking bus lines, I2C discovery, or sensor interrogation. |
| **payload** | **data buffer / telemetry frame / sample packet** | Sensor reading struct, ring buffer element, or network transmission. |
| **scaffold purge / purge** | **removal / erase / clear / refactoring / pruning** | Deleting obsolete code, wiping configuration, or clearing caches. |
| **neutralize** | **clear / discharge / de-energize / reset** | Clearing display RAM, resetting DC bias, or de-asserting GPIO rails. |
| **kill / terminate (non-POSIX)**| **halt / disable / abort / power cut** | Graceful shutdown or gate de-assertion. |
| **fallback / fall back** | **default / recovery mode / safe state** | Fault handling logic. |
| **doctrine / dogma** | **standard / architecture / principle / specification**| Non-technical, cultish, or ideological framing. |
| **bones / muscle / vanity** | **contracts / subsystems / presentation layer** | Anatomical slang / informal construction metaphors. |

### The Tone & Professionalism Clause
1. **Eliminate Hostile / Adversarial Phrasing:** Prohibit violent idioms (*"nuke"*, *"blow away"*, *"destroy"*, *"wipe out"*, *"strangle"*). Use standard engineering terms (*"reset"*, *"erase"*, *"clear"*, *"abort"*).
2. **No Manifesto-Style Prose:** Maintain a neutral, matter-of-fact, dispassionate tone focused purely on empirical measurement, circuit schematics, system architecture, and API contracts.
3. **No Pop-Culture or Whimsical Lore:** Prohibit superheroes, movie quotes, religious/theological terms, and fantasy metaphors (*"magic"*, *"pixie dust"*, *"dragons"*). Note: *daemon* is permitted strictly in POSIX service context.

---

## 3. LVGL v9 Bilevel UI & E-Paper Constraints

All UI code targets the Waveshare ESP32-S3 ePaper 1.54" V2 panel (SSD1681 driver):
1. **Geometry:** 200×200 pixels (1:1 square aspect ratio), 188 DPI.
2. **Color Depth:** 1-bit Monochrome (`LV_COLOR_FORMAT_I1`), pure black and pure white. Draw buffer: exactly 5,000 bytes.
3. **100% Opacity Mandate:** Background fills must always be `LV_OPA_COVER` (100%). Alpha blending, partial opacity, and pseudo-random dithering are prohibited.
4. **Typography:** Minimum font height is **14 pixels** (e.g. `H1OS_FONT_14`). Fonts <14 px blur into illegibility during differential updates.
5. **Zero Continuous Redraw:** Animations, continuous progress spinners, and rapid redraw loops are strictly prohibited to prevent battery exhaustion and display ghosting.
6. **QR Code Generation:** Use the built-in LVGL QR code engine (`lv_qrcode_create`) to render compact, camera-scannable QR codes for rapid BLE provisioning.
7. **Thread Safety:** Wrap all UI mutations in `h1os_lvgl_lock()` / `h1os_lvgl_unlock()`.

---

## 4. Application State & Operational Workflow Rules

1. **Turnkey Universal Firmware:**
   - The repository produces a single, turnkey binary that can be flashed to any compatible board.
   - First boot enters BLE GATT provisioning. The device generates a random Proof of Possession (PoP) PIN via hardware TRNG (`esp_random()`), displays the QR code, device name, and PoP key.
   - Once Wi-Fi is verified, the BLE radio stack is completely shut down to conserve RAM and power.

2. **Network Time Synchronization (SNTP):**
   - ThingsBoard requires TLS v1.3 / SSL validation, which mandates a valid epoch time.
   - Because ThingsBoard does not provide an NTP server command, the device connects to an external SNTP server on boot/re-connect, updating system time and the NXP PCF85063A hardware RTC.
   - Periodic SNTP synchronization occurs every 24 hours; in between, the hardware RTC is the source of truth.

3. **Device Claiming:**
   - Unclaimed devices generate a random 6-character claiming token via hardware TRNG, publish it to `v1/devices/me/claim`, and display it alongside the device name and nickname on the e-Paper screen.
   - The user inputs the token on [Humid1.com](https://www.humid1.com/) to bind ownership.

4. **Telemetry & Dynamic Attributes:**
   - Base temperature unit in internal logic is **Kelvin**; display presents **°F** or **°C** as dictated by ThingsBoard shared attributes (`temp_unit`).
   - ThingsBoard pushes dynamic shared attributes (`sleep_interval_sec`, `temp_unit`, `sound_enabled`, threshold limits). The device acknowledges and mirrors active parameters.

5. **Server-Managed Alarms:**
   - Environmental alerting is managed by ThingsBoard rule chains (push notifications, SMS, email).
   - The device does NOT alert users autonomously; instead, it subscribes to the ThingsBoard alarms channel/attributes to trigger local visual changes (e.g., high-visibility inverted display card) or audible chimes when an active alarm is flagged by the server.

6. **Automatic Dual-Slot OTA & Rollback:**
   - OTA updates are delivered automatically via the ThingsBoard OTA channel.
   - Firmware images are written directly to the passive partition (`ota_0` or `ota_1`), verified with SHA-256, and marked bootable.
   - Rollback protection is active: if a new image fails to complete its initial diagnostic cycle and cloud handshake, the bootloader automatically reverts to the previous working slot.

7. **Storage Partition (`storage` SPIFFS):**
   - Media assets, English audio sample catalogs, and UI graphics are stored in the dedicated `storage` SPIFFS partition (`0x1DC000`).

---

## 5. Documentation Governance ("Documentation is Law")

1. **Synchronous Updates:** When APIs, state machines, communication schemas, or UI layouts change, corresponding architectural documentation in [`docs/architecture/`](docs/architecture/) MUST be updated in the exact same commit.
2. **Living Architecture Index:**
   - [`APP_01_system_architecture.md`](docs/architecture/APP_01_system_architecture.md): Lifecycle, state machine, duty cycles.
   - [`APP_02_cloud_and_telemetry.md`](docs/architecture/APP_02_cloud_and_telemetry.md): ThingsBoard custom cloud, telemetry schemas, dynamic attributes, claiming.
   - [`APP_03_ble_provisioning.md`](docs/architecture/APP_03_ble_provisioning.md): BLE GATT protocol, QR code generator, PoP key.
   - [`APP_04_ui_and_screens.md`](docs/architecture/APP_04_ui_and_screens.md): Hero screen, 1-bit monochrome layouts, inverted alert cards.
   - [`APP_05_audio_feedback.md`](docs/architecture/APP_05_audio_feedback.md): Audio chime events, sample catalog in SPIFFS, quiet hours.
   - [`APP_06_ota_management.md`](docs/architecture/APP_06_ota_management.md): Automatic ThingsBoard OTA, dual-slot partitioning, rollback guard.
   - [`APP_07_os_api_mandates.md`](docs/architecture/APP_07_os_api_mandates.md): Formal register of upstream H1OS API requests and deficiencies.
   - [`docs/h1os_reference.md`](docs/h1os_reference.md): Upstream H1OS reference, pinouts, and errata.

---

## 6. Senior Architect's Field Guide for Future Junior Agents & Maintainers

Welcome to the `humid1-os` source tree. This application layer exists to bring commercial-grade humidor climate telemetry, custom cloud claiming, and seamless user interaction to the ESP32-S3 hardware platform powered by H1OS. Before modifying any file, internalize these six core engineering principles:

1. **Respect the Two Execution Worlds (FreeRTOS Awake vs. Bare-Metal RTC Sleep):**
   - When the system is **awake in Flow B**, let hardware interrupts, `esp_timer`, and FreeRTOS event loops do the waiting. Never write a `while (busy) { vTaskDelay(10); }` loop or spawn dedicated polling tasks to wait for network responses, display refreshes, or user buttons. Use event-driven callbacks and static semaphores so both Xtensa cores can enter low-power idle states cleanly.
   - **Fast Wake / Immediate Sleep Invariant:** When waking from deep sleep, the application must connect via Wi-Fi fast-connect, publish telemetry, sync dynamic attributes, update the e-Paper display, and return to deep sleep as quickly as possible. Prolonged wake intervals deplete battery voltage (`VBATT`).
2. **Never Fight Physics with Software Fudge Factors:**
   - The Sensirion SHTC3 sensor is factory-calibrated. Trust the readings and compute true psychrometric invariants (water vapor pressure $e$, dew point $T_{\text{dew}}$, absolute humidity $d_v$) using pure August-Roche-Magnus equations. Never introduce arbitrary fudge factors or empirical scaling offsets. Base internal temperature representation is strictly **Kelvin**; convert dynamically for display presentation only.
3. **Strict Application-Middleware Boundary (The H1OS API Mandate):**
   - `humid1-os` is a pure application and cloud ecosystem repository. Low-level peripheral control, register bit-twiddling, and power latch manipulation belong strictly to H1OS middleware. Never bypass H1OS to touch hardware registers directly. If an OS capability is missing, log an authoritative **OS API Feature Mandate** in [`docs/architecture/APP_07_os_api_mandates.md`](docs/architecture/APP_07_os_api_mandates.md).
4. **Guard Flash Endurance & Static SRAM Headroom:**
   - Flash write endurance is finite. Throttle non-volatile storage commits and avoid thrashing SPIFFS. Allocate tasks and long-lived memory statically where practical (`StaticTask_t`, `StaticSemaphore_t`), and tear down BLE and Wi-Fi radio stacks cleanly the moment network operations finish to return RAM and power to the platform.
5. **Measure Twice, Cut Once (Craftsmanship & Verification):**
   - Read the target header, trace callers via semantic code intelligence (`clangd`), and check the hardware errata before authoring code. Enforce checked return codes on all ESP-IDF and H1OS calls. Adhere to **The Gump Naming Rule** (`[Module]_[Target]_[Action]`) and visual columnar alignment for clean, readable code.
6. **Never Bloat Git History (Asset Hygiene & The `/shrink` Directive):**
   - Git is an immutable distributed ledger; checking in uncompressed bitmaps, multi-megabyte digital camera photos, or redundant asset dumps penalizes every engineer who clones the repo forever. Always run `/shrink` (or `python firmware/tools/agent/optimize_images.py` and `python firmware/tools/agent/optimize_pdf.py`) prior to committing. Ensure all image EXIF and PDF metadata is stamped `HUMIDITRON/HUMADYNE_LABS`.

---

## 7. MCP Tool Harness: Scope, Architecture & Strategic Planning

The 3-Finger MCP tool suite equips the agent with compiler-grade AST intelligence, binary memory profiling, hardware telemetry SQL, and formal mathematical solvers. Using the right tool eliminates guesswork, prevents token burn, and guarantees physical correctness.

### 1. Semantic Code Intelligence (`clangd`)
* **Available Tools:** `find_symbol`, `get_definition`, `find_references`, `get_type_info`, `get_callers`, `get_callees`, `list_file_symbols`.
* **Purpose & Application:** AST-accurate C/C++ semantic navigation backed by LLVM 18 and `compile_commands.json`. Resolves active `#ifdef` configurations, struct layouts, type hierarchies, and full incoming/outgoing call graphs.
* **Why Preferred Over `grep`:**
  - `grep` is syntactic and blind: it returns false positives in comments, string literals, inactive `#if 0` blocks, and similarly named variables across unrelated files.
  - `clangd` is semantic and exact: `get_definition` jumps straight to the true compiler definition; `find_references` lists exact call sites; `get_type_info` provides precise struct packing and byte offsets.

### 2. Embedded Diagnostics & Memory Budgets (`embedded-ops`)
* **Available Tools:** `murdock_memory_budget`, `ba_serial_capture_and_decode`, `clangd_touch_reload`.
* **Purpose & Application:** Binary inspection and runtime verification.
  - `murdock_memory_budget`: Evaluates compiled `.elf` using `xtensa-esp32s3-elf-size` and `nm` to return structured JSON byte counts for DRAM, DIRAM, RTC Slow, and Flash partition headroom.
  - `ba_serial_capture_and_decode`: Captures serial boot sequences and automatically runs `xtensa-esp32s3-elf-addr2line` to decode panic backtraces to `file:line (function)`.
  - `clangd_touch_reload`: Busts clangd AST cache after CMake reconfigures without restarting sessions.

### 3. Telemetry & Tabular Analytics (`duckdb`)
* **Available Tools:** `query`.
* **Purpose & Application:** High-performance in-memory SQL analytics directly against disk CSV and Parquet files without database imports or file-locking conflicts.
* **Why Preferred Over Ad-Hoc Scripts:** Allows instant multi-file aggregation (e.g., font metric audits, telemetry queries) in a single turn without writing throwaway Python parsing scripts.

### 4. Mathematical Grounding & Formal Verification (`science`)
* **Available Tools:** `derive_fixed_point_polynomial`, `solve_z3_assertions`, `fit_thermal_rc`, `eval_symbolic_math`.
* **Purpose & Application:** Formal verification of state machines, polynomial approximation for fixed-point algorithms, and empirical curve fitting.

### 5. Documentation & Technical Reference (`local-index` & `espressif-documentation`)
* **Available Tools:** `search_code_advanced`, `find_files`, `get_file_summary`, `search_espressif_sources`.
* **Purpose & Application:** Precision search across repository architecture docs, H1OS middleware references, and Espressif Technical Reference Manuals.
* **Tool Quirk:** `local-index/search_code_advanced` strictly requires argument `pattern: str` (do not pass `query`).

### Subagent Spawning & Parallel Delegation
Proactively spawn subagents using `invoke_subagent` whenever:
* **Research & Deep Lookups:** Multi-file codebase sweeps, register maps, or datasheet conversions (delegate to `research` subagent).
* **Parallel Build & Analysis:** Independent tasks running concurrently (e.g. build verification running alongside documentation synthesis).
* **Autonomous Continuation:** Do NOT poll subagents in loops; wait for reactive background system messages.

---

## 8. Release Governance & Pre-Merge Checklist

Direct commits to `main` are strictly prohibited. Before merging any branch into `main`:
1. **Identity & Name Sanitization Audit:** Run `python firmware/tools/agent/sanitize.py --check` (or `--fix`) to verify zero personal identities, workstation usernames, or local filesystem paths leak into committed repository files or release archives.
2. **Asset Optimization & Metadata Invariant (`/shrink` or `/quant`):** Execute the repository asset shrink pipeline (`firmware/tools/agent/optimize_images.py` and `firmware/tools/agent/optimize_pdf.py`) prior to staging or committing changes. All photographic evidence must be stamped with all-caps EXIF attribution (`HUMIDITRON/HUMADYNE_LABS`) and progressive JPEG quantization. PDFs are strictly preserved in their original compressed format with lossless metadata harmonization.
3. **Zero-Warning Build:** Verify compilation passes with 0 warnings/errors via `idf.py -C firmware build`.
4. **Synchronize Semantic Version Strings:** Update `firmware/main/app_config.h`, `firmware/main/idf_component.yml`, `README.md`, and `CHANGELOG.md`.
5. **Synchronous Architecture Documentation:** Verify all architectural changes are fully documented in `docs/architecture/` (`APP_01` through `APP_07`).
6. **Touch Changelog:** Record changes in `CHANGELOG.md` under Keep a Changelog categories (`Added`, `Changed`, `Fixed & Hardened`, `Removed`).
7. **Tag Release:** Push annotated tag `vX.Y.Z` to remote.

---

## 9. Engineering Attribution & Contributors

* **Lead Hardware & Systems Architect:** Humiditron (Humidyne Labs)
* **Principal Firmware Audit, `-O2` Wake Stub Hardening & Event-Driven Architecture Refactor (`v2.4.4-dev`):** Gemini (Autonomous Embedded Systems Architect & Principal Auditor)
* **Core Firmware Development, Metrology & Simulation Engineering (`v2.0` – `v2.4.3-dev`):** Antigravity Gemini 3.8 (Junior Embedded Systems Agent)

