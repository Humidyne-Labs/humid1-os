The pin multiplexing tree for the ESP32-S3 Touch ePaper board is organized below by hardware peripheral and signal routing.

**ESP32-S3 Pin Multiplexing Map**

| GPIO | Signal Name | Target Peripheral | Description / Function |
| --- | --- | --- | --- |
| **GPIO0** | `BOOT_KEY` | System / Bootloader | User BOOT switch |
| **GPIO1** | `BAT_ADC` | Power Management | Battery voltage monitoring via ADC1_CH0 |
| **GPIO2** | `PWR_KEY` | Power Management | Power hold / power switch control |
| **GPIO3** | `RTC_INT` | PCF8563 RTC | Real-Time Clock hardware interrupt |
| **GPIO4** | `TOUCH_RST` | Capacitive Touch | Touch controller hardware reset |
| **GPIO5** | `TOUCH_INT` | Capacitive Touch | Touch panel interrupt request |
| **GPIO6** | `I2C_SCL` | Shared I2C Bus | Clock line for Codec, SHTC3, RTC, and Touch |
| **GPIO7** | `I2C_SDA` | Shared I2C Bus | Data line for Codec, SHTC3, RTC, and Touch |
| **GPIO8** | `EPD_DC` | 1.54" e-Paper | Display Data/Command control pin |
| **GPIO9** | `EPD_RST` | 1.54" e-Paper | Display hardware reset |
| **GPIO10** | `EPD_CS` | 1.54" e-Paper | Display SPI Chip Select |
| **GPIO11** | `EPD_SCLK` | 1.54" e-Paper | Display SPI Clock |
| **GPIO12** | `EPD_MOSI` | 1.54" e-Paper | Display SPI MOSI (Data In) |
| **GPIO13** | `EPD_BUSY` | 1.54" e-Paper | Display busy status line |
| **GPIO14** | `I2S_MCLK` | ES8311 Codec | I2S Master Clock |
| **GPIO15** | `I2S_SCLK` | ES8311 Codec | I2S Bit Clock (BCLK) |
| **GPIO16** | `I2S_LRCK` | ES8311 Codec | I2S Frame Clock / Word Select (WS) |
| **GPIO17** | `I2S_ASOUT` | ES8311 Codec | Serial Audio Data (Codec DOUT → ESP32 DIN) |
| **GPIO18** | `I2S_DSIN` | ES8311 Codec | Serial Audio Data (ESP32 DOUT → Codec DIN) |
| **GPIO38** | `USER_LED` | Status Indicator | Onboard status LED |
| **GPIO39** | `SD_CLK` | MicroSD Card | SPI Clock |
| **GPIO40** | `SD_MISO` | MicroSD Card | SPI MISO (Data Out) |
| **GPIO41** | `SD_MOSI` | MicroSD Card | SPI MOSI (Data In) |
| **GPIO42** | `SD_CS` | MicroSD Card | SPI Chip Select |
| **GPIO47** | `PA_CTRL` | Audio Amp (NS4168/PA) | Amplifier gain and mode control |
| **GPIO48** | `PA_EN` | Audio Amp (NS4168/PA) | Power amplifier enable / shutdown control |

---

**Hardware Bus Architecture**

* **Shared System I2C (`GPIO6` / `GPIO7`):** Connects four onboard peripherals—ES8311 Audio Codec, SHTC3 Temperature/Humidity Sensor, PCF8563 RTC, and the Touch Panel controller.
* **Primary SPI (`GPIO10`–`GPIO12`):** Dedicated to the 1.54-inch e-Paper display driver along with discrete DC/RST/BUSY control pins (`GPIO8`, `GPIO9`, `GPIO13`).
* **Secondary SPI (`GPIO39`–`GPIO42`):** Dedicated bus reserved exclusively for TF/MicroSD card storage.
* **I2S Audio Pipeline (`GPIO14`–`GPIO18`):** Full-duplex I2S audio interface tied to the ES8311 codec, complemented by discrete power amp control lines (`GPIO47`, `GPIO48`).