# HUMID1-OS - Architectural Specification & Blueprint
Welcome to the architectural roadmap for **HUMID1-OS**, an ultra-low-power, cloud-integrated IoT hydrometer built for precision cigar and tobacco humidor monitoring. 
---
## 1. System Overview & Core Workflow
`HUMID1-OS` is engineered for a "zero-touch-after-setup" lifecycle. The core operational flow follows these phases:
* **First Boot & Provisioning:** User enters Bluetooth provisioning mode via a physical button hold. The device announces itself to ThingsBoard for auto-discovery and is claimed via serial or a secure PIN code.
* **Asset Initialization:** If an SD card is present during provisioning, the device downloads an English audio sample catalog for voice alerts. Time syncs automatically over the network.
* **Steady-State Operation:** The device loops through deep sleep and active cycles, reading the SHTC3 sensor, updating the e-Paper display, reporting telemetry to ThingsBoard, and checking for scheduled OTA updates.
---
## 2. Hardware Specifications

| Component | Specification | Notes |
| :--- | :--- | :--- |
| **Development Kit** | ESP32-S3-ePaper-1.54 | Integrated low-power e-Paper display kit |
| **SoC** | ESP32-S3-PICO-1-N8R8 | Dual-core 32-bit LX7, clocked at 240 MHz |
| **Memory / Flash** | 8MB Flash / 8MB PSRAM | Requires custom partition table (Arduino/VS Code compatible) |
| **Display** | 1.54" e-Paper (No touch) | Static layout updated per wake cycle |
| **Sensor** | SHTC3 Temperature & Humidity | Factory-calibrated, I2C interface |
| **Power** | 400mAh Battery | USB-C charging (~44 min charge time); aggressive deep sleep required |
| **Storage** | MicroSD Card Slot | Optional storage for audio sample catalog |
| **User Input** | Single Tactile Button | Multi-click and long-press handling |

---
## 3. Firmware Architecture & Build System
* **Build Environment:** Compatible with Arduino IDE and Visual Studio Code (PlatformIO).
* **Partition Table:** Custom partition scheme required to optimize flash allocation for OTA redundancy, application code, and SPIFFS/LittleFS storage.
* **Build Flags:**
  * `--NOAUDIOSUPPORT`: Conditional compilation flag to omit audio catalog integration and SD card dependencies for lightweight builds.
* **Release Artifacts:** Compiled firmware binaries distributed alongside cryptographic checksums (MD5) for reliable flashing and verification.
---
## 4. User Interaction & Audio Feedback
### Button Control Matrix (Single Button)
* **Short Press / Multi-Click:** Reserved for local status checks or manual wake options (TBD based on multi-click logic).
* **3-Second Hold:** Triggers Bluetooth Low Energy (BLE) provisioning mode.
* **Flash Clear Action:** A specific button sequence or dashboard command triggers a factory reset, clearing stored credentials and provisioned data from flash (displayed on the e-Paper screen during execution).
### Audio Subsystem
* **Catalog Source:** Downloaded to the SD card during initial provisioning (English language only to conserve space).
* **Playback Events:** 
  * Booting chime
  * Pairing / Provisioning status
  * Network connected / disconnected
  * Low battery warning
  * Flash erased confirmation
---
## 5. Connectivity & Provisioning Architecture
* **Provisioning Method:** Bluetooth Low Energy (BLE) pairing.
* **Cloud Discovery (ThingsBoard):**
  * Device auto-discovers and registers within a ThingsBoard tenant/customer profile.
  * Claiming mechanism utilizes the device serial number or a secure random PIN code generated during first boot.
* **Data Persistence:** Wi-Fi credentials, device name, and cloud tokens are safely stored in non-volatile flash storage (NVS), with a mechanism to wipe data cleanly.
---
## 6. ThingsBoard Integration & Telemetry
### Telemetry Sent to Cloud
* **Identifiers:** SOC Series/ID, Serial Number, Firmware Version, MAC Address.
* **Sensor Data:** Temperature (C°/F°), Relative Humidity (%).
* **Device Health:** Battery percentage, Wi-Fi link status (On/Off), active SSID.
* **System Meta:** Current time, day of week, date, configured duty cycle interval.
### Dashboard Controls & Attributes
* **Temperature Unit Toggle:** Switch between Celsius and Fahrenheit.
* **Audio Playback Switch:** Enable or disable onboard audio chimes remotely.
* **Duty Cycle Configuration:** Adjustable sleep intervals (e.g., 5 min, 15 min, 30 min, 1 hour) to balance battery life and reporting granularity.
* **Device Naming:** Custom assignable friendly name (e.g., *“Bedroom Humidor #1”*).  
* **Theme Selection:** Selectable (Light/Dark) device theme.  
---
## 7. Display & UI Architecture (e-Paper)
* **Design Language:** Sci-fi minimalist aesthetic. The splash screen features the logo **humid1-os**, utilizing a compact "1" styled as a sleek hyphen/accent element.
* **Display States:**
  * **Boot / Splash:** Displays the `HUMID1-OS` branding.
  * **Provisioning Mode:** Explicitly states *"Provisioning..."* on screen.
  * **Flash Erase:** Explicitly states *"Erasing Flash..."* during factory resets.
  * **Standard Dashboard View (Static):**
    * Device Name (configurable via cloud)
    * Wi-Fi Status (On/Off & SSID)
    * Temperature & Relative Humidity
    * Time, Day of Week, and Date
    * Battery Percentage
    * MAC Address / Serial reference
* **Technical Note on Updates:** Because e-Paper displays refresh fully and consume power on updates, the UI relies on full-screen static refreshes tied to the configured deep-sleep duty cycle rather than continuous LVGL partial-screen streaming.
---
## 8. Power Management Strategy
* **Deep Sleep Optimization:** The ESP32-S3 remains in deep sleep for the vast majority of its lifecycle, waking up only according to the configured duty cycle timer or external hardware interrupts (button press).
* **Peripherals Control:** Power rails to the SHTC3 sensor, SD card reader, and audio amplifier are gated or idled aggressively when not in active measurement/transmission windows.
* **Battery Endurance:** Engineered around a 400mAh cell, aiming to maximize longevity between USB-C recharge cycles.

---

**Humiditron-2026**  
Licenses: [MIT](LICENSE) (Code) | [CC BY 4.0](LICENSE-ASSETS) (Media Assets)  
*Co-architected with a touch of C.A.D. (Companion-Assisted Design)*  