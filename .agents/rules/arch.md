# Application Architecture and Boundary Rules

This rule defines the core architectural invariants for **`humid1-os`** as a pure application and cloud ecosystem layer built on top of **H1OS v2.4.4-dev**.

## 1. Separation of Responsibilities

1. **Underlying OS / Middleware Tier (`H1OS`)**:
   - Manages all low-level hardware interactions (SSD1681 1-bit e-Paper display, Sensirion SHTC3 sensor, NXP PCF85063A RTC, ES8311/NS4168 audio codec, SDMMC, power latching).
   - Manages ULP-RISC-V coprocessor execution, headless Flow A sample bursts, and zero-heat psychrometric telemetry math.
   - Applications must NEVER bypass H1OS to directly manipulate display, sensor, or power registers.

2. **Application Tier (`humid1-os`)**:
   - Consumes the clean public H1OS API (`h1os/h1os.h`, `h1os/h1os_lvgl.h`, `h1os/h1os_time.h`).
   - Implements humidor application logic, LVGL v9 screens, BLE GATT provisioning, and MQTT telemetry reporting.
   - Communicates with **Humid1's self-hosted ThingsBoard instance on our custom cloud ([Humid1.com](https://www.humid1.com/))**.

## 2. Declarative Application Lifecycle

- The application executes in the awake FreeRTOS domain following standard H1OS lifecycle patterns (`h1os_start`, `h1os_get_telemetry`, render, `h1os_sleep`).
- Sensor sampling is requested through `h1os_get_telemetry(&telem)`, maintaining zero-heat sensing invariants.
- Power transitions must call `h1os_sleep(&sleep_cfg)` rather than manual low-level sleep calls, ensuring power latching and display RAM state remain preserved.

## 3. UI Invariants (LVGL v9 1-bit Monochrome)

- Visual assets and layouts must be designed strictly for the 200×200 1-bit monochrome SSD1681 display (`LV_COLOR_FORMAT_I1`).
- All screen background fills must be opaque (`bg_opa: COVER`) using binary black/white color palettes.
- Animations and continuous redraw loops are prohibited to avoid excessive e-Paper refresh wear and energy drain.
- Display updates are rendered during scheduled wake intervals or triggered user interactions.
