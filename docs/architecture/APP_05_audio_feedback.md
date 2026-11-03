# APP_05: Acoustic Feedback & Unattended Silence Mandate

> **Module Scope:** `firmware/main/main.c`, `firmware/main/app_config.h`
> **Hardware Subsystem:** Everest Semi `ES8311` I2S Codec (`0x18`) + `NS4168` Class-D Amplifier (`PA_EN = GPIO 42` active LOW, `PA_CTRL = GPIO 46` active HIGH)

---

## 1. Layer 4 H1OS Audio API Contract

`humid1-os` uses the public Layer 4 H1OS acoustic synthesis and PCM APIs declared in `<h1os/h1os.h>`:
* `esp_err_t h1os_audio_play_tone(uint32_t freq_hz, uint32_t duration_ms);` — Asynchronous pure-tone synthesis queued to `h1os_audio_worker`.
* `esp_err_t h1os_audio_play_chime(h1os_chime_type_t chime_type);` — Built-in multi-note acoustic patterns (`H1OS_CHIME_BOOT`, `H1OS_CHIME_SLEEP`, `H1OS_CHIME_SHUTDOWN`, `H1OS_CHIME_ALARM`).
* `esp_err_t h1os_audio_play_raw(const int16_t *ptr, size_t len, size_t *bytes_written);` — Raw 16-bit signed mono 16 kHz PCM playback.
* `esp_err_t h1os_set_volume(uint8_t volume_pct);` — Sets perceptual DAC volume (`0..100%`; `0%` mutes and places amplifier in standby).

---

## 2. Unattended Wake Silence Invariant (`TST_005` / `TST_006` Thermal & Power Guard)

* **Rule:** Audio initialization and playback are **strictly prohibited** during unattended periodic wakes (`ctx->wake_state == H1OS_WAKE_STATE_RTC_FLOW_B`).
* **Hardware Rationale:** During unattended RTC Flow B wakes, H1OS intentionally skips `sub_audio_init()` so the `ES8311` codec remains in its `< 1.0 µA` deep sleep state (`Reg 0x0D = 0xFC`), preventing both the `+4.4°C` codec thermal hotspot (`TST_005`) and unnecessary Class-D amplifier current draw.
* **Application Gate:** Every call to `h1os_audio_play_tone()` or `h1os_audio_play_chime()` in `main.c` MUST be guarded by:
  ```c
  if (app->is_interactive_wake && app->cfg.sound_enabled) { ... }
  ```

---

## 3. Acoustic Event Mapping

| User / System Event | H1OS Layer 4 Call | Parameters |
| :--- | :--- | :--- |
| **Cold Boot Power-On** | Automatic via `lifecycle.presentation.chime_mask` | `H1OS_CHIME_BOOT` |
| **Tactile Button Short Tap (`BOOT` / `POWER`)** | `h1os_audio_play_tone(1046, 50)` | `1046 Hz` (C6), `50 ms` |
| **System Menu Long-Press Confirm** | `h1os_audio_play_tone(1318, 80)` | `1318 Hz` (E6), `80 ms` |
| **Interactive Wake with Active Threshold Breach** | `h1os_audio_play_chime(H1OS_CHIME_ALARM)` | Built-in alarm pattern |
| **Orderly Power-Down / Storage Shutdown** | Automatic via `lifecycle.presentation.chime_mask` | `H1OS_CHIME_SHUTDOWN` |
