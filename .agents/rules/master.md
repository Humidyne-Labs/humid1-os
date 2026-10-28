# Master Agent Execution Rules: H1OS & ESP32-S3 Firmware Development

## 1. Core Directives & Mandates

1. **Zero Guesswork Policy:** NEVER guess or extrapolate function prototypes, struct members, enums, peripheral pins, or configuration flags.
   - Use compiler-backed semantic code intelligence (`get_definition`, `find_references`, `get_type_info`, `get_callers`, `get_callees`) for exact symbol inspection.
   - Cross-reference authoritative headers (`h1os.h`, `hal_types.h`, `sub_*.h`, `orch_*.h`) before authoring calls.
2. **Four-Layer Unidirectional Boundary Hierarchy:**
   - **Layer 1: HAL (`hal_*`)**: Stateless primitives manipulating registers and buses. Zero application state, zero math.
   - **Layer 2: Peripheral Subsystems (`sub_*`)**: Encapsulated device drivers (`sub_display`, `sub_sensor`, `sub_rtc`, `sub_power`, `sub_audio`, `sub_nvs`, `sub_sdcard`).
   - **Layer 3: Orchestrator & ULP (`orch_*`)**: Flow arbiter (Flow A vs. Flow B), persistent RTC state (`orch_state`), thermodynamic invariant math (`orch_math`), and the ULP-RISC-V coprocessor monitor.
   - **Layer 4: Application Lifecycle (`h1os_*`)**: Declarative lifecycle (`h1os_start`, `h1os_sleep`), Core 1 LVGL v9 UI engine (`h1os_lvgl.h`), time sync (`h1os_time.h`).
   - *Invariants:* Upper layers call lower layers; lower layers **NEVER** call up or cross-talk.
3. **Strict Error Checking:** Every ESP-IDF, FreeRTOS, and H1OS API call returning `esp_err_t` MUST be checked via `ESP_ERROR_CHECK()` or handled deterministically.
4. **Hardware-First Mandate ("Use the Hardware"):**
   - **Dual-Core Affinity:** Core 0 handles system, radio, and headless Flow A bursts; Core 1 is dedicated to the LVGL v9 presentation task.
   - **Power Latching:** `BAT_CTRL` (GPIO 17) is asserted in `h1os_boot_hook` within 25 ms and held via RTC pad hold across deep sleep.
   - **Zero-Heat Sensing:** Never poll the SHTC3 during active CPU operation. SHTC3 sampling is restricted to headless Flow A sample bursts (<10 ms at 80 MHz) or the ULP coprocessor.
   - **Never Busy-Wait:** Do not spin in `while()` loops during idle states; block on FreeRTOS notifications, semaphores, or hardware interrupts.

---

## 2. Constraint-Driven UI Development (LVGL 9 Bilevel Specification)

All graphics code MUST comply with `.agents/rules/lvgl.md`:
* **Geometry & Usable Boundary:** 200 x 200 pixels, 1:1 aspect ratio, 188 DPI. Maximum text width is $\le 188.0\text{ px}$.
* **Color Depth & Buffer:** 1-bit Monochrome (`LV_COLOR_FORMAT_I1`), pure black/white, 5,000-byte draw buffer.
* **100% Opacity Mandate:** Opacity must strictly be `LV_OPA_COVER`. Alpha blending, dithering, and partial opacity are prohibited.
* **Spatial Budget:** Header ($Y = 0..20\text{ px}$ with 1 px bottom border), Body ($Y = 22..148\text{ px}$), Footer ($Y = 150..200\text{ px}$).
* **Typography:** Minimum font height is **14 pixels**. Small fonts (<14 px) bleed and are forbidden.
* **Thread Safety:** Wrap all LVGL object mutations in `h1os_lvgl_lock()` / `h1os_lvgl_unlock()`.

---

## 3. Subagent Spawning & Parallel Delegation

Proactively spawn subagents using `invoke_subagent` whenever:
* **Research & Deep Lookups:** Multi-file codebase sweeps, register maps, or datasheet conversions (delegate to `research` subagent).
* **Parallel Build & Analysis:** Independent tasks running concurrently (e.g. build verification running alongside documentation synthesis).
* **Autonomous Continuation:** Do NOT poll subagents in loops; wait for reactive background system messages.

---

## 4. Compiler Diagnostics & Two-Strike Resolution Protocol

* **Verification Mandate:** Never declare success without compilation verification via the canonical profile command (`C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1`).
* **Two-Strike Protocol:**
  1. If compilation fails, isolate the exact diagnostic line from compiler output.
  2. Propose exactly one targeted syntactic or signature fix.
  3. If compilation fails a second time, **STOP IMMEDIATELY**. Do not guess alternative signatures or cast types to force compilation. State the diagnostic verbatim and ask the user for confirmation.

---

## 5. Naming & Code Aesthetics

* **The Gump Naming Rule:** Name identifiers literally by what they physically do:
  `[Layer/Module Prefix] _ [Physical Target/Peripheral] _ [Literal Action]`
  *(e.g., `orch_flow_sample_sensors()`, `hal_io_release_power_latch()`, `sub_display_clear_to_white()`)*.
* **Visual Block Alignment ("Pretty Code"):** Format struct definitions, constants, and function arguments into clean columnar blocks for rapid vertical scanning.
