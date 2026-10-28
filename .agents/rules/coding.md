# Embedded C/C++ Coding Standards, Hardware Utilization & Code Aesthetics

This rule governs the implementation quality, hardware-first design principles, memory safety, and visual aesthetics of C/C++ firmware in **H1OS**.

---

## 1. Hardware-First Mandate ("Use the Hardware")

This is embedded silicon code. Actively exploit all on-chip hardware accelerators, coprocessors, and peripheral engines:

1. **Leverage FreeRTOS SMP:** Use dual-core affinity (Core 0 for system/radio, Core 1 for UI/sensors), queues, semaphores, and event groups rather than bespoke synchronization.
2. **Never Busy-Wait (Event-Driven Awake vs. Bare-Metal Sleep Split):**
   - **Awake FreeRTOS Layers:** Polling with `vTaskDelay()` spin loops for hardware `BUSY` pins, button states, inactivity timeouts, or worker task exits is prohibited. Use `esp_timer` (`esp_timer_start_once` / `esp_timer_start_periodic` / `esp_timer_restart`) for timeouts and periodic state machines, GPIO edge interrupts (`gpio_isr_handler_add`) + hardware glitch filters (`gpio_new_pin_glitch_filter`) + `xSemaphoreTake` / `ulTaskNotifyTake` for hardware lines, and static binary semaphores (`xSemaphoreCreateBinaryStatic`) for task teardown joins.
   - **Bare-Metal Wake Stub (`orch_wake_stub.c`) & ULP (`ulp/main.c`):** Never call FreeRTOS or `esp_timer` while SPI Flash is powered down. Use bounded `ccount`/register loops guarded with `__attribute__((noinline))`, loop register clobbers (`asm volatile("" : "+r"(i))`), and `rtc_reg_barrier()` (`memw` / `fence`).
3. **Coprocessor & DMA Utilization:** Delegate ultra-low-power interrupt evaluation to the ULP-RISC-V coprocessor and bulk display transfers to SPI DMA channels.
4. **Platform & Hardware Reality Checks:** Always cross-reference proposed features against actual board schematics and Espressif documentation. If an operation is physically impossible (e.g. attempting RTC I2C transactions on digital-only GPIOs), state it as an unalterable hardware fact.

---

## 2. Memory Safety & Pointer Hygiene

1. **Zero Dangling Pointers:**
   - Always initialize pointers to `NULL`.
   - Immediately set pointers to `NULL` after calling `free()` or releasing memory.
2. **Parameter Validation:** Validate pointer arguments at all public and subsystem boundaries (`if (ptr == NULL) return ESP_ERR_INVALID_ARG;`).
3. **Explicit Error Handling:** Check return codes on all ESP-IDF and FreeRTOS calls. Propagate or log errors deterministically.

---

## 3. Single-Responsibility Functions

- If a function performs more than one distinct task, **split it into 2 or 3 smaller, well-named helper functions**.
- Modularity, testability, and code readability strictly take priority over negligible function-call overhead.
- Keep function scope focused and self-documenting.

---

## 4. Visual Block Alignment ("Pretty Code")

Code must be formatted into clean, columnar visual blocks that allow rapid vertical scanning:

### Example: Columnar Struct Definitions
```c
typedef struct {
    uint32_t  flow_a_period_sec;      /**< Period between headless sensor sample bursts. */
    uint32_t  flow_b_period_sec;      /**< Period between unattended display flushes. */
    uint32_t  inactivity_timeout_sec; /**< Interactive session idle sleep timeout. */
    bool      enable_audio;           /**< Audio codec and amplifier power gate. */
    bool      enable_sdcard;          /**< MicroSD FATFS mount flag. */
} h1os_session_config_t;
```

### Example: Columnar Assignments & Macros
```c
#define HAL_PIN_BAT_CTRL      17
#define HAL_PIN_EPD_3V3_EN     6
#define HAL_PIN_STATUS_LED     3
#define HAL_PIN_BAT_ADC        4
#define HAL_PIN_RTC_INT        5

static esp_err_t s_init_peripherals(void)
{
    esp_err_t ret      = ESP_OK;
    uint32_t  attempts = 0;
    bool      bus_busy = false;
    /* ... */
}
```

---

## 5. LVGL Presentation Layer Discipline & OSHW Reuse

1. **Presentation Isolation:** LVGL v9 belongs strictly to Layer 4. Never embed raw physical bus transactions (I2C/SPI writes) inside LVGL widget event callbacks.
2. **Minimal Footprint:** Favor 1-bit packed binary assets, static buffers where practical, and minimal RAM overhead.
3. **Open Source Reusability:** Design reusable, cleanly decoupled widgets and drivers to benefit the wider Open Source Hardware (OSHW) and embedded community.
