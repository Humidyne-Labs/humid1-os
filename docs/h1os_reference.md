# H1OS Middleware Reference & Hardware Invariants

`HUMID1-OS` builds directly upon **H1OS v2.4.4-dev** as its underlying embedded operating system and peripheral middleware framework.

## 1. Upstream Middleware Repositories & Resources
- **Upstream Repository:** [https://github.com/Humidyne-Labs/H1OS](https://github.com/Humidyne-Labs/H1OS)
- **Local Reference Tree:** `../H1OS` (or cloned locally alongside `humid1-os`)
- **Primary Component:** `components/h1os`

## 2. Hardware Encapsulation & Pure App Boundary
Because `humid1-os` is a pure application and cloud ecosystem repository, all low-level hardware orchestration is delegated to H1OS:
- **Display Driver & Waveforms:** Solomon Systech SSD1681 1.54" 200×200 1-bit monochrome e-Paper panel, 6-cycle anti-ghosting full refresh, and `epd_frame` deep sleep frame caching.
- **Environmental Sensing:** Sensirion SHTC3 I2C (`0x70`) with zero-heat steady-state thermal sampling and August-Roche-Magnus psychrometric equations.
- **Real-Time Clock:** NXP PCF85063A I2C (`0x51`) calendar RTC with 32.768 kHz quartz oscillator.
- **Acoustic Subsystem:** Everest Semi ES8311 I2S codec + NS4168 Class-D amplifier.
- **Power Architecture:** Battery LDO power hold latch (`GPIO 17`), battery voltage divider ADC (`GPIO 4`), and physical button wake handling (`GPIO 18`).

For hardware pinouts, circuit schematics, PCB Rev B guides, and silicon errata, refer directly to the authoritative H1OS documentation:
- [H1OS Master Engineering Specification](https://github.com/Humidyne-Labs/H1OS/blob/main/AGENTS.md)
- [H1OS Hardware Pinout Map](https://github.com/Humidyne-Labs/H1OS/blob/main/docs/pinout.md)
- [Hardware Errata & Rev B Guide](https://github.com/Humidyne-Labs/H1OS/blob/main/ERRATA.md)

---

## 3. Upstream Component Packaging Defect & Fix Plan (Font Generator Script)

### Issue Description
When consuming H1OS as an external Git component via ESP-IDF Component Manager (`firmware/main/idf_component.yml` with `path: components/h1os`), CMake configuration initially succeeds, but compilation halts during Ninja build:
```text
ninja: error: '<project_dir>/firmware/tools/generate_1bpp_fonts.py', 
needed by 'esp-idf/h1os/generated_fonts/h1os_font_montserrat_14_1bpp.c', 
missing and no known rule to make it
```

### Root Cause Analysis
In `H1OS`, `components/h1os/CMakeLists.txt` resolves font synthesis tools using relative paths that assume a monorepo workspace layout:
```cmake
set(FONT_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/../../tools/fonts")
set(FONT_GEN_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/../../tools/generate_1bpp_fonts.py")
```
When downstream projects pull H1OS via `idf_component.yml`, the ESP-IDF Component Manager copies **only** the `components/h1os` directory into `managed_components/h1os`. The repository-root `tools/` directory is not included. Consequently, `../../tools` resolves into the downstream project's own `firmware/` directory (`firmware/tools`), where the script does not exist by default.

### Downstream Workaround (Applied)
In `humid1-os`, the typography generator and vendored font assets have been temporarily vendored directly into [`firmware/tools/`](../firmware/tools/) so builds succeed deterministically.

### Required Upstream Fix in `H1OS`
To make the `h1os` component 100% self-contained for external consumers and Component Registry publishing:
1. Move or vendor `generate_1bpp_fonts.py` and `tools/fonts/` into the component directory:
   - `components/h1os/tools/generate_1bpp_fonts.py`
   - `components/h1os/tools/fonts/`
2. Update `components/h1os/CMakeLists.txt` to reference the self-contained component path:
   ```cmake
   set(FONT_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/tools/fonts")
   set(FONT_GEN_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/tools/generate_1bpp_fonts.py")
   ```
3. Update internal H1OS example apps to remove any redundant references to root `tools/fonts`.

