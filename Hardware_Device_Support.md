# Humid1-OS - Hardware Support & Technical Reference
> **Official Hardware Repository / SDK Reference:** [Waveshare ESP32-S3-ePaper-1.54 GitHub Repository](https://github.com/waveshareteam/ESP32-S3-ePaper-1.54/tree/main)

<p align="center">
  <a href="https://www.waveshare.com/esp32-s3-epaper-1.54.htm?sku=32298">
    <img src="https://www.waveshare.com/img/devkit/ESP32-S3-ePaper-1.54/ESP32-S3-ePaper-1.54-details-intro.jpg" alt="ESP32-S3-32298" width="100%">
  </a>
</p>

---

This document serves as the comprehensive hardware, pinout, power management, and firmware development reference for `HUMID1-OS`.
---
## 1. Hardware Selection & Specifications
The hardware platform for the ecosystem is built around the **Waveshare ESP32-S3-ePaper-1.54** (SKU: 32298, V2 Revision), integrating processing, display, environmental sensing, and power management into a single cased module.
### Supplier, Cost & Revisions
* **Supplier:** Waveshare (SKU: 32298)
* **Cost:** Retails at $16.99 USD, with volume discounts down to $15.97 USD for orders of 4+.
* **Revisions:** The project targets the **V2 version** (phased in starting November 1, 2025). 
  * *V1 Version:* Equipped with ESP32-S3FH4R2, integrated 4MB Flash and 2MB PSRAM.
  * *V2 Version:* Equipped with ESP32-S3-PICO-1-N8R8, integrated 8MB Flash and 8MB PSRAM; features optimized whole-board power consumption in sleep mode.
* **Regulatory & Compliance Status:** Formal third-party regulatory safety certifications (such as FCC, CE, or UL) and official RoHS declarations are **not on file** for this specific hobbyist/developer module. Builders deploying hardware at scale should factor this into commercial plans, though it remains viable for open-source self-hosted maker projects.
* **Vendor Disclaimer:** `HUMID1-OS` is an independent open-source project and is not officially endorsed by, affiliated with, or sponsored by Waveshare. We are open to exploring sponsorship or collaboration opportunities with Waveshare.
### Microcontroller & Processing
* **SoC:** ESP32-S3-PICO-1-N8R8 System-on-Chip featuring an Xtensa 32-bit LX7 dual-core processor running at speeds up to 240 MHz.
* **Memory:** Integrated 8MB of Flash and 8MB of PSRAM in a stacked package.
* **Connectivity:** Supports 2.4 GHz Wi-Fi (802.11 b/g/n) and Bluetooth 5 (LE) with an onboard chip antenna.
### Display Panel
* **Type:** 1.54-inch reflective e-Paper display utilizing a 2-color (Black and White) ink configuration.
* **Resolution & Optics:** 200x200 pixels resolution with high contrast, a wide viewing angle exceeding 170 degrees, and zero backlight requirement.
* **Refresh Timings:** Full refresh takes approximately 2 seconds; partial refresh takes ~0.3 seconds.
### Onboard Sensors & Peripherals
* **Temperature & Humidity:** `SHTC3` sensor connected via a shared I2C bus (SDA: GPIO47, SCL: GPIO48) at address `0x70`.
* **Real-Time Clock:** `PCF85063` RTC chip located on the back side for time-keeping functions.
* **Audio & Storage:** `ES8311` audio codec with microphone/speaker headers, alongside a TF card slot supporting FAT32-formatted external storage.
---
## 2. Critical Power & Control Pinout
To properly manage deep sleep and hardware states on the ESP32-S3 board, firmware must configure the following critical GPIO pins:

| GPIO Pin | Role / Signal Name | Behavior & Configuration Notes |
| :--- | :--- | :--- |
| **GPIO17** | `vbat_power` | Battery power latch (active high; must be driven high at boot and frozen via `gpio_hold_en()` during deep sleep to prevent power cutoff when the physical button is released). |
| **GPIO6** | `epd_power` | Panel power pin (active low; driven low to power the panel during updates and high before sleep to cut panel idle draw while preserving the e-paper image). |
| **GPIO18** | `PWR Button` | Wakeup pin connected to the physical power button (configured as active-low with `INPUT_PULLUP`). |
| **GPIO4** | `BAT_ADC` | Battery voltage ADC pin, measuring `VBAT/2` through a 200K/200K resistor divider. |
| **GPIO47 / 48** | `I2C SDA / SCL` | Shared I2C bus connected to `SHTC3` (`0x70`), RTC, and audio codec. |

---
## 3. Power Performance, Battery Life & Charging
The module is powered by an included **400mAh (1.48Wh, 3.7V)** LiPo battery cell (form factor model `502535`).
### Runtime & Performance Estimates
* **Empirical Status:** Concrete field runtimes are currently pending empirical telemetry logging and long-term duty-cycle benchmarking.
* **Target Profile:** Firmware tuning aims to optimize deep-sleep intervals (e.g., periodic updates every 30 minutes or longer) to maximize longevity on the 400mAh cell. Real-world power draw will vary based on Wi-Fi transmission duration and display refresh frequency.
### Battery Charging Metrics
* Charging via the onboard management circuit takes approximately **30 minutes to reach 4.1V** and **44 minutes to fully charge**.
---
## 4. Enclosure Material Assessment & Humidor Safety
The protective enclosure housing for the unit is manufactured from **Polycarbonate (PC)**.
* **Physical Benefits:** Offers high impact resistance against drops, excellent dimensional stability under shifting ambient humidity, and a clean professional finish.
* **Risks & Mitigations:** PC is chemically vulnerable to polar organic solvents or concentrated aromatic oils over extended durations. *Mitigation:* Ensure safe, neutral cleaning protocols and isolate from direct chemical contact.
* **Cigar Safety Profile:** Solid, high-grade polycarbonate is chemically inert at standard humidor temperatures (16°C to 24°C). Because it is rigid without plasticizers like ortho-phthalates, it does not offgas VOCs or leach harmful chemicals, making it safe for placement inside a sealed humidor environment.
---
## 5. Development Frameworks & Troubleshooting
The board supports both **Arduino IDE** and **ESP-IDF** development frameworks.
### Arduino IDE Configuration Notice
When configuring the Arduino IDE for the V2 board revision, ensure tool parameters match the hardware profile (e.g., selecting `OPI PSRAM` instead of `QSPI PSRAM` to match the 8MB PSRAM configuration of the V2 PICO module).
### Common Troubleshooting Tips
* **Flashing Failures / Serial Connection Drops:** Press and hold the `BOOT` button while reconnecting power to force the module into serial download mode.
* **Blank Panel on Boot:** Verify that `GPIO6` is driven **LOW** (`output.turn_off: epd_power`) to enable power to the e-paper panel.

---

**Humiditron-2026**  
Licenses: [MIT](LICENSE) (Code) | [CC BY 4.0](LICENSE-ASSETS) (Media Assets)  
*Co-architected with a touch of C.A.D. (Companion-Assisted Design)*  