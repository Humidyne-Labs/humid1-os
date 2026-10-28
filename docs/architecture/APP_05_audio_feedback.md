# APP_05: Acoustic Feedback, Attribute Gating & Power Isolation

* **Document ID:** `APP-ARCH-005` (Rev 2.0)
* **Parent Blueprint:** [`Architectural_Outline.md`](../../Architectural_Outline.md)
* **Normative Scope:** ES8311 + NS4168 Audio Pipeline Usage, Dual Attribute/Hardware Gates (`sound_enabled` & `has_sd_card`), and Unattended Deep-Sleep Silence Invariant

---

## 1. Acoustic Feedback Architecture

The Humid1 hardware integrates an **ES8311 low-power mono audio codec** (I2C address `0x18`, I2S digital audio bus) paired with an **NS4168 Class-D power amplifier** gated by active-low `PA_EN` (`GPIO 46`, via inverter). H1OS encapsulates the entire I2S DMA streaming and amplifier power-rail sequence behind a single non-blocking API:

```c
esp_err_t h1os_audio_play_Async(const char *filepath);
```

- **Storage Medium:** Uncompressed 16-bit PCM `.wav` assets reside on the FATFS microSD card mounted by H1OS at `/sdcard/`.
- **Non-Blocking Execution:** `h1os_audio_play_Async()` spawns a dedicated background FreeRTOS worker inside H1OS that powers up `PA_EN`, streams the `.wav` file over I2S DMA, and automatically shuts down the NS4168 amplifier and ES8311 DAC upon completion. Application UI and network threads must **never** busy-wait on audio playback.

---

## 2. Mandatory Three-Condition Playback Gate

Every acoustic event in `humid1-os` must pass a deterministic three-condition gate before invoking `h1os_audio_play_Async()`:

```c
bool app_audio_can_play(const app_state_t *state, bool is_rtc_wake) {
    if (is_rtc_wake) {
        return false; // Rule 1: Unattended periodic Flow B wakes are strictly silent
    }
    if (!state->attrs.sound_enabled) {
        return false; // Rule 2: Server/NVS sound_enabled attribute must be true
    }
    if (!h1os_sdcard_is_mounted()) {
        return false; // Rule 3: MicroSD card must be physically present and mounted
    }
    return true;
}
```

1. **Condition 1 — Interactive Session Only (`!is_rtc_wake`):** Unattended periodic `Flow B` wakes occur every 15 minutes day and night. Enabling the Class-D amplifier during unattended wakes would disturb users and waste $\sim 45\text{--}120\,\text{mA}$ of battery current. Audio cues are strictly restricted to interactive sessions (`AWAKE`) and explicit user onboarding transitions.
2. **Condition 2 — User Mute Preference (`sound_enabled`):** Synchronized from the ThingsBoard Shared Attribute `sound_enabled` and cached in NVS key `"sound_en"`.
3. **Condition 3 — Media Availability (`h1os_sdcard_is_mounted()`):** Verified at boot and reported to ThingsBoard via the Client Attribute `"has_sd_card"` and `"audio_synced"`.

---

## 3. Canonical Audio Asset Map (`/sdcard/`)

| Event Trigger | Asset Path | Duration | Description |
| :--- | :--- | :--- | :--- |
| **Button Tap (`BOOT` / `POWER`)** | `/sdcard/ui_tap.wav` | $\le 60\,\text{ms}$ | Crisp tactile confirmation click on screen navigation. |
| **Action Confirm (2s Hold)** | `/sdcard/ui_confirm.wav` | $\le 180\,\text{ms}$ | Dual-tone chime confirming theme toggle or sync request. |
| **Onboarding / Claim Complete** | `/sdcard/claim_ok.wav` | $\le 450\,\text{ms}$ | Ascending triad played when Wi-Fi + customer claim succeed. |
| **Server Warning Alarm** | `/sdcard/alarm_warn.wav` | $\le 600\,\text{ms}$ | Played once upon interactive wake if `alarm_state.severity == "WARNING"`. |
| **Server Critical Alarm** | `/sdcard/alarm_crit.wav` | $\le 900\,\text{ms}$ | Played upon interactive wake if `alarm_state.severity == "CRITICAL"`. |
| **Error / Auth Failure** | `/sdcard/err_buzz.wav` | $\le 300\,\text{ms}$ | Low double-tone played on BLE Wi-Fi authentication failure. |
