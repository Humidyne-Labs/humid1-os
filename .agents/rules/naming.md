# Naming Conventions & Literal Engineering Identifiers

This rule governs the naming of functions, variables, macros, and types across **H1OS**.

## The Core Philosophy: "The Gump Naming Rule"

> *"Well, what's it do? Well, call it that."*

In embedded systems and hardware firmware, **literal clarity strictly trumps cleverness or abstract flexibility**. Function and type names must plainly describe what they physically do or hold at runtime.

---

## 1. The Literal Construction Pattern

Construct function identifiers using a clear, predictable hierarchy:

```text
[Layer/Module Prefix] _ [Physical Target/Peripheral] _ [Literal Action]
```

### Examples:
- ✅ `orch_flow_sample_sensors()` — Clearly samples physical sensors in the flow arbiter.
- ✅ `hal_io_release_power_latch()` — Literally releases the hardware power latch pin.
- ✅ `sub_display_clear_to_white()` — Literally drives the electrophoretic display to pure white.
- ✅ `sub_power_enter_deep_sleep()` — Literally configures sleep pads and enters deep sleep.
- ❌ `orch_flow_execute_sub_sprint()` — Fails the literal test ("sprint" is athletic slang, not a physical action).
- ❌ `sub_display_neutralize()` — Vague, non-literal metaphor.
- ❌ `process_tick()` / `do_work()` — Overly generic dumping grounds.

---

## 2. Bucket is to Array, as Function is to Runtime Operation

- **Data Structures:** Names must reflect their physical memory structure and payload:
  - `ring_buffer` for circular FIFO ring buffers.
  - `sample_table` for sequential lookup arrays.
  - `persisted_state` for RTC Slow Memory retention blocks.
- **Functions:** If a function performs two coordinated operations in sequence, either:
  1. Break it into 2 separate helper functions (preferred per the single-responsibility rule), or
  2. If an atomic sequence is required, name the composite outcome literally (e.g., `orch_flow_sample_and_sleep()`).

---

## 3. Pragmatic Brevity & Permitted Shorthand

Literal names must remain concise. Do not write 60-character sentences. Use universally recognized embedded engineering shorthand:

| Permitted Shorthand | Literal Meaning | Prohibited Slang / Buzzwords |
| :--- | :--- | :--- |
| `init` / `deinit` | Initialize / De-initialize | `setup_magic`, `kickoff`, `prime` |
| `cfg` | Configuration struct | `options`, `settings_bucket` |
| `buf` / `len` | Buffer / Length | `stash`, `chunker` |
| `cb` | Callback function pointer | `listener`, `hooker` |
| `ms` / `us` / `sec` | Milliseconds / Microseconds / Seconds | `ticks` (unless raw RTOS timer ticks) |
| `pwr` / `batt` / `vbat` | Power / Battery voltage | `juice`, `fuel`, `life` |
| `sample` | Physical sensor measurement | `sprint`, `sortie`, `hit`, `sniff` |
| `err` / `ret` | Error code / Return status | `result_code`, `flag_status` |

---

## 4. The 3:00 AM Comprehension Test

If a firmware engineer looks at a function declaration in a header and has to open the `.c` file just to know which peripheral, GPIO rail, or memory domain it touches, **the name has failed**. Rename it literally.
