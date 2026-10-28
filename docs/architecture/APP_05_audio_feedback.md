# APP_05: Audio Subsystem & Acoustic Event Chimes

## 1. Acoustic Hardware Pipeline
The Waveshare ESP32-S3 ePaper 1.54" V2 board integrates an Everest Semi **ES8311 I2S low-power codec** paired with an **NS4168 Class-D mono power amplifier** driving an onboard miniature speaker.

All low-level I2S DMA streaming, ES8311 CSM ready status polling (Reg `0xFC`), soft-mute zero-cross ramps, and power gating are managed by the H1OS audio worker (`sub_audio.h` / `h1os_audio_worker`).

---

## 2. Audio Sample Storage & SPIFFS Catalog
1. **Flash Partition:** Audio clips and acoustic tone tables are stored in the dedicated `storage` SPIFFS partition (`0x1DC000`, 1904 KB).
2. **Audio Formats:** Supported formats include raw 16-bit signed mono PCM clips (at 16 kHz or 24 kHz) and frequency tone tables.
3. **Optional External SDMMC:** If an external FAT32 MicroSD card is present, extended voice alert catalogs (e.g. spoken alerts) can be streamed via `sub_sdcard`.

---

## 3. Application Sound Events & Trigger Matrix

| Event Name | Acoustic Pattern / Tone | Trigger Condition |
| :--- | :--- | :--- |
| **`CHIME_BOOT`** | Ascending three-tone chime (523 Hz -> 659 Hz -> 784 Hz) | Cold boot or resume from manual power cycle. |
| **`CHIME_NOTIFY`** | Short crisp dual-tone chirp (880 Hz -> 1046 Hz) | Wi-Fi associated, BLE pairing success, or device claimed. |
| **`CHIME_ALARM`** | Rapid warble tone (880 Hz <-> 1175 Hz, 3 cycles) | ThingsBoard active alarm notification or claim timeout. |
| **`CHIME_SHUTDOWN`** | Descending three-tone chime (784 Hz -> 659 Hz -> 523 Hz) | Clean physical power button shutdown. |

---

## 4. Power & Quiet-Hours Rules
1. **Zero Quiescent Draw:** The NS4168 amplifier power rail is held de-asserted when no audio is playing to eliminate battery leakage.
2. **Dynamic Server Switch (`sound_enabled`):**
   - The master audio toggle is dictated by ThingsBoard shared attributes (`sound_enabled: true/false`).
   - When set to `false`, all notification and alarm chimes are muted to avoid nocturnal disturbance in residential humidor environments.
