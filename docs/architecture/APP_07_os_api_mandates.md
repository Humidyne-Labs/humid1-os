# APP_07: OS API Feature Mandate Register

## 1. Governance & Purpose
`HUMID1-OS` maintains a strict architectural boundary: low-level hardware drivers and peripheral orchestration belong to **H1OS** (`Humidyne-Labs/H1OS`), while application business logic belongs to this repository.

When developing `humid1-os`, developers and agents must **NEVER** implement fragile hardware hacks, direct register manipulation, or duplicate peripheral drivers to bypass an OS limitation. Instead, any requirement that H1OS does not currently expose or handle cleanly must be formally entered into this register as an **OS API Feature Mandate**.

These mandates are prioritized and implemented directly upstream in `H1OS`.

---

## 2. Active Mandate Register

| Mandate ID | Status | Subsystem | Title / Summary | Upstream Target |
| :--- | :--- | :--- | :--- | :--- |
| **MANDATE-01** | **Open** | Typography / Build | Self-Contained Font Generator & Typography Packaging | `H1OS/components/h1os` |
| **MANDATE-02** | **Proposed** | Telemetry / API | Direct Kelvin & Multi-Unit Psychrometric Accessors | `H1OS/components/h1os/include/h1os/h1os.h` |
| **MANDATE-03** | **Proposed** | Sleep / RTC | Fast RTC Wi-Fi Context Persistence Helper | `H1OS/components/h1os/include/h1os/orch_state.h` |

---

## 3. Detailed Mandate Specifications

### MANDATE-01: Self-Contained Font Generator & Typography Packaging
- **Status:** Open (Workaround currently applied downstream in `firmware/tools/`)
- **Category:** Build System / Component Isolation
- **Symptom:** When downstream applications consume H1OS via the ESP-IDF Component Manager (`idf_component.yml` with `path: components/h1os`), CMake fails during Ninja compilation:
  ```text
  ninja: error: '<project_dir>/firmware/tools/generate_1bpp_fonts.py', 
  needed by 'esp-idf/h1os/generated_fonts/h1os_font_montserrat_14_1bpp.c', 
  missing and no known rule to make it
  ```
- **Root Cause:** `components/h1os/CMakeLists.txt` uses relative paths (`../../tools/`) expecting the monorepo root. Component Manager only extracts `components/h1os`, discarding root `tools/`.
- **Required Upstream Fix in `H1OS`:**
  1. Relocate `tools/generate_1bpp_fonts.py` to `components/h1os/tools/generate_1bpp_fonts.py`.
  2. Relocate `tools/fonts/` to `components/h1os/tools/fonts/`.
  3. Update `components/h1os/CMakeLists.txt` to reference:
     ```cmake
     set(FONT_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/tools/fonts")
     set(FONT_GEN_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/tools/generate_1bpp_fonts.py")
     ```

---

### MANDATE-02: Direct Kelvin & Multi-Unit Psychrometric Accessors
- **Status:** Proposed
- **Category:** Public API (`h1os/h1os.h`)
- **Requirement:** `HUMID1-OS` uses **Kelvin** as its canonical base scientific temperature unit for all internal logic and cloud telemetry schemas. While `h1os_telemetry_t` currently exposes `ambient_temp_c` and `ambient_temp_f`, exposing `ambient_temp_k` directly in `h1os_telemetry_t` (and `dew_point_k`) standardizes SI units across the middleware layer.
- **Proposed Upstream Change in `H1OS`:**
  Add `float ambient_temp_k;` and `float dew_point_k;` to `h1os_telemetry_t` in `components/h1os/include/h1os/h1os.h`, derived in `orch_math.c`.

---

### MANDATE-03: Fast RTC Wi-Fi Context Persistence Helper
- **Status:** Proposed
- **Category:** RTC Slow Memory Persistence (`orch_state`)
- **Requirement:** Fast wake-to-sleep telemetry cycles require connecting to the provisioned Wi-Fi access point in <400 ms. Storing BSSID, channel, and authentication state in RTC Slow SRAM across deep sleep avoids full spectrum scanning.
- **Proposed Upstream Change in `H1OS`:**
  Expose a dedicated section in `orch_state_t` (or an application user scratch area in RTC memory) to allow the application layer to persist fast Wi-Fi connection parameters without risk of memory collision across wake stub cycles.
