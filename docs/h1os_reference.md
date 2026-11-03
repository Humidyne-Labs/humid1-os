# H1OS (`v2.4.9-dev`) Public Layer 4 API & Hardware Reference

> **Purpose:** Authoritative local reference for `humid1-os` developers and autonomous coding agents.
> **Rule:** Never guess or fabricate H1OS function signatures, struct fields, or font macros. Every Layer 4 interaction with H1OS MUST use the exact declarations documented below from `<h1os/h1os.h>`, `<h1os/h1os_lvgl.h>`, `<h1os/h1os_time.h>`, and `h1os_fonts.h`.

---

## 1. Hardware Platform & Fixed Pinout (`ESP32-S3-PICO-1-N8R8`)

* **MCU & Memory:** Espressif `ESP32-S3-PICO-1-N8R8` (Dual-Core Xtensa LX7 @ up to 240 MHz, **8 MB Quad SPI Flash**, **8 MB Octal PSRAM**, **8 KB RTC Slow SRAM**, **8 KB RTC Fast SRAM**).
* **Display (Non-Touch):** 1.54" 200×200 1-bit monochrome e-Paper (`SSD1681`, 188 DPI).
  * `EPD_PWR` (Active LOW): `GPIO 6` | `EPD_BUSY`: `GPIO 8` | `EPD_RST`: `GPIO 9` | `EPD_DC`: `GPIO 10` | `EPD_CS`: `GPIO 11` | `EPD_SCLK`: `GPIO 12` | `EPD_MOSI`: `GPIO 13`.
* **Shared I2C Bus (`I2C_NUM_0` @ 400 kHz):** `SDA = GPIO 47`, `SCL = GPIO 48`.
  * Sensirion `SHTC3` (`0x70`) — sampled exclusively in Layer 3 `orch_wake_stub.c` during headless Flow A bursts (`~18.5 ms` @ 80 MHz) to prevent PCB thermal contamination.
  * NXP `PCF85063A` Calendar RTC (`0x51`) — countdown timer & alarm output routed to `RTC_INT` (`GPIO 5`).
  * Everest Semi `ES8311` Audio Codec (`0x18`) — kept in `< 1.0 µA` deep standby across unattended wakes.
* **Audio I2S & Amplifier:** `MCLK = GPIO 14`, `BCLK = GPIO 15`, `WS = GPIO 45`, `DOUT = GPIO 38`, `PA_EN = GPIO 42` (Active LOW, held LOW in sleep per Errata E4), `PA_CTRL = GPIO 46` (Active HIGH NS4168 Class-D enable).
* **Tactile Buttons & Power Latch:**
  * `BOOT` Button: `GPIO 0` (Active LOW, RTC IO).
  * `POWER` Button: `GPIO 18` (Active LOW, RTC IO).
  * `BAT_CTRL` Power Latch: `GPIO 17` (Asserted HIGH within 25 ms by `h1os_boot_hook`).
  * `VBAT_ADC`: `GPIO 4` (`ADC1_CHANNEL_3`, 200kΩ/200kΩ divider).
* **1-Bit SDMMC Slot:** `CLK = GPIO 39`, `CMD = GPIO 40`, `D0 = GPIO 41` (mounted at `/sdcard`).

---

## 2. Lifecycle & Flow Configuration (`<h1os/h1os.h>`)

### 2.1 Flow Period Constants & Silicon Invariants (`MANDATE-02`)
```c
#define H1OS_FLOW_A_MIN_SEC               60U     /* 1 min minimum Flow A period */
#define H1OS_FLOW_A_MAX_SEC               14400U  /* 4 hr maximum Flow A period */
#define H1OS_FLOW_B_MIN_SEC               300U    /* 5 min minimum Flow B period */
#define H1OS_FLOW_B_MAX_SEC               86400U  /* 24 hr maximum Flow B period */
#define H1OS_FLOW_REMAINDER_MIN_SEC       60U     /* Minimum non-zero remainder slice */
#define H1OS_DEFAULT_FLOW_A_PERIOD_SEC    180U    /* 3 minutes */
#define H1OS_DEFAULT_FLOW_B_PERIOD_SEC    900U    /* 15 minutes */
#define H1OS_DEFAULT_INACTIVITY_TIMEOUT_SEC 30U   /* 30 seconds interactive timeout */

#define H1OS_CELSIUS_TO_KELVIN(c)         ((float)(c) + 273.15f)
#define H1OS_CELSIUS_TO_FAHRENHEIT(c)     (((float)(c) * 1.8f) + 32.0f)
```
**6 Mandatory Flow Slicing Invariants enforced by `h1os_start()` and `h1os_set_flow_periods()`:**
1. `60 <= flow_a_sec <= 14400`
2. `300 <= flow_b_sec <= 86400`
3. If `flow_a_sec > 255`, `flow_a_sec % 60 == 0` (PCF85063A 1/60 Hz prescaler alignment).
4. `flow_b_sec > flow_a_sec`
5. `remainder = flow_b_sec % flow_a_sec` must be `0` or `>= 60`.
6. If `remainder > 255`, `remainder % 60 == 0`, and total slices `overflow >= 2`.

### 2.2 Wake Context & Lifecycle Structs
```c
typedef enum {
    H1OS_WAKE_STATE_BOOT                = 0, /* Cold boot / power-on reset cycle */
    H1OS_WAKE_STATE_INTERACTIVE_SESSION = 1, /* User tactile button wake (30s session) */
    H1OS_WAKE_STATE_RTC_FLOW_B          = 2  /* Unattended periodic RTC Flow B refresh & sync */
} h1os_wake_state_t;

typedef struct {
    esp_reset_reason_t reset_reason;
    uint32_t wakeup_causes;
    h1os_wake_state_t wake_state;
    uint32_t boot_count;
    union {
        uint32_t deep_sleep_count;
        uint32_t wake_count;
    };
    uint32_t flow_b_count;
    uint32_t button_wake_count;
    bool woke_from_button;
    uint8_t wake_button;         /* 0=None, 1=BOOT (GPIO 0), 2=POWER (GPIO 18) */
    uint8_t ulp_dispatch_reason; /* h1os_ulp_reason_t */
    uint32_t sleep_duration_sec;
    void *user_data;
} h1os_wake_context_t;

typedef void (*h1os_boot_cb_t)(void *user_data);
typedef void (*h1os_wake_cb_t)(const h1os_wake_context_t *ctx, void *user_data);
typedef void (*h1os_sync_cb_t)(const h1os_wake_context_t *ctx, void *user_data);
typedef void (*h1os_pre_sleep_cb_t)(uint32_t sleep_sec, void *user_data);
typedef void (*h1os_shutdown_cb_t)(void *user_data);
typedef void (*h1os_error_cb_t)(esp_err_t err, const char *context, void *user_data);

typedef struct {
    h1os_boot_cb_t on_boot;
    h1os_wake_cb_t on_wake;
    h1os_sync_cb_t on_sync;
    h1os_pre_sleep_cb_t on_pre_sleep;
    h1os_shutdown_cb_t on_shutdown;
    h1os_error_cb_t on_error;
    struct {
        h1os_splash_cb_t on_splash;
        h1os_chime_cb_t on_chime;
        uint8_t chime_mask;
        uint8_t volume_pct;
    } presentation;
    struct {
        uint32_t inactivity_timeout_sec;
        sys_clk_mode_t clk_mode;
        uint32_t flow_a_period_sec;
        uint32_t flow_b_period_sec;
        uint32_t sync_wait_ms;
        uint32_t sync_timeout_ms;
    } session;
    void *user_data;
} h1os_lifecycle_t;
```

### 2.3 Core Lifecycle, Sync Barrier & Power Functions
* `esp_err_t h1os_start(const h1os_lifecycle_t *config);`
* `esp_err_t h1os_set_flow_periods(uint32_t flow_a_sec, uint32_t flow_b_sec);`
* `void h1os_sync_hold(void);` — Hold the deep-sleep barrier open for an async task started inside `on_sync()`.
* `void h1os_sync_notify_complete(void);` — Signal `BARRIER_SYNC_DONE` and release pre-flush sync gate.
* `void h1os_reset_session_timer(void);` — Reset the 30s interactive inactivity timer on button input.
* `esp_err_t h1os_sleep(uint32_t sleep_sec);` — Enter coordinated deep sleep.
* `void h1os_shutdown(void);` — Orderly power off preserving current e-Paper image.
* `void h1os_shutdown_for_storage(void);` — Orderly power off after full clean-white e-Paper scrub.

---

## 3. Telemetry & History Snapshot (`h1os_telemetry_t`)

```c
typedef struct {
    float ambient_temp_c;
    float ambient_temp_f;
    float ambient_temp_k;
    float ambient_rh_pct;
    float dew_point_c;
    float dew_point_f;
    float dew_point_k;
    float vapor_pressure_hpa;
    float sat_vapor_pressure_hpa;
    float absolute_humidity_g;

    float window_low_c;
    float window_high_c;
    float window_avg_shtc3_c;
    float window_avg_die_c;
    float temp_historical_c;
    float rh_historical_pct;

    float raw_shtc3_temp_c;
    float mcu_die_temp_c;
    float epd_glass_temp_c;
    float trinode_temp_c;
    float trinode_delta_c;
    uint8_t trinode_confidence_pct;

    bool is_thermally_settled;
    uint16_t valid_trend;
    uint32_t sample_count;
    uint32_t boot_count;
    uint32_t wake_count;
    uint32_t flow_b_count;
    uint32_t button_wake_count;
    uint8_t ulp_dispatch_reason;
    uint32_t flow_a_period_sec;
    uint32_t flow_b_period_sec;
    uint16_t battery_mv;
    uint16_t boot_vbat_mv;
    uint8_t battery_percent;
    bool is_usb_powered;
    h1os_battery_state_t battery_state; /* H1OS_BATTERY_DISCHARGING, CHARGING, CHARGED */
    h1os_autonomy_metrics_t autonomy;   /* avg_draw_ma, autonomy_days, mah_spent_lap */
    uint32_t sensor_fail_count;
    uint8_t last_sensor_error;
    bool is_rh_alert_active;
} h1os_telemetry_t;

esp_err_t h1os_get_telemetry(h1os_telemetry_t *out_telemetry);
esp_err_t h1os_get_telemetry_history(orch_tri_node_sample_t *out_samples, size_t max_samples, size_t *out_count);
esp_err_t h1os_set_rh_alert_bounds(float min_rh_pct, float max_rh_pct);
```

---

## 4. LVGL v9 Graphics, Actor Queue & 1-BPP Typography (`<h1os/h1os_lvgl.h>`)

* `esp_err_t h1os_ui_post(h1os_ui_fn_t fn, void *arg, uint32_t timeout_ms);` — **Mandatory** way for Core 0 (`on_sync`, MQTT/BLE callbacks) to mutate LVGL widgets on Core 1.
* `void h1os_lvgl_request_full_refresh(void);` / `void h1os_lvgl_set_first_flush_mode(bool full_refresh);` — Request a full OTP flashing waveform scrub on the next flush.
* `lv_indev_t *h1os_lvgl_get_indev(void);` — Retrieve the hardware 2-button keypad `lv_indev_t *`.
* **1-BPP Bilevel Fonts (`h1os_fonts.h`, minimum 14px floor):**
  * `H1OS_FONT_14` (`&h1os_font_montserrat_14_1bpp`) — Default UI body & diagnostic labels.
  * `H1OS_FONT_16` (`&h1os_font_montserrat_16_1bpp`)
  * `H1OS_FONT_18` (`&h1os_font_montserrat_18_1bpp`)
  * `H1OS_FONT_20` (`&h1os_font_montserrat_20_1bpp`)
  * `H1OS_FONT_24` (`&h1os_font_montserrat_24_1bpp`) — Primary ambient temperature display.
  * `H1OS_FONT_32` (`&h1os_font_montserrat_32_1bpp`) — Hero relative humidity display.

---

## 5. Audio, Time, NVS & Storage APIs

* **Audio (`<h1os/h1os.h>`):**
  * `esp_err_t h1os_audio_play_tone(uint32_t freq_hz, uint32_t duration_ms);`
  * `esp_err_t h1os_audio_play_chime(h1os_chime_type_t chime_type);` (`H1OS_CHIME_BOOT`, `H1OS_CHIME_SLEEP`, `H1OS_CHIME_SHUTDOWN`, `H1OS_CHIME_ALARM`)
  * `esp_err_t h1os_audio_play_raw(const int16_t *ptr, size_t len, size_t *bytes_written);`
  * `esp_err_t h1os_set_volume(uint8_t volume_pct);`
* **Time & RTC (`<h1os/h1os_time.h>`):**
  * `bool h1os_time_is_valid(void);`
  * `esp_err_t h1os_time_sync_system_to_rtc(void);`
  * `esp_err_t h1os_time_sync_rtc_to_system(void);`
  * `esp_err_t h1os_time_set_timezone(const char *tz_posix);`
  * `esp_err_t h1os_time_get_formatted(char *buf, size_t max_len);`
* **NVS (`<h1os/h1os.h>`):**
  * `h1os_nvs_get_u8` / `h1os_nvs_set_u8`, `h1os_nvs_get_u16` / `h1os_nvs_set_u16`, `h1os_nvs_get_u32` / `h1os_nvs_set_u32`, `h1os_nvs_get_i32` / `h1os_nvs_set_i32`, `h1os_nvs_get_str` / `h1os_nvs_set_str`, `h1os_nvs_get_blob` / `h1os_nvs_set_blob`, `h1os_nvs_erase_key`, `h1os_nvs_reset`.
