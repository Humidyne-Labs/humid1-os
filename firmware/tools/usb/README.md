# ⚡ H1 // USB Recovery Terminal & Port Cycler

A lightweight, zero-dependency Windows diagnostic GUI and hardware port-cycling utility engineered for embedded development. Designed specifically for low-power architectures (ESP32-S3 USB-JTAG/Serial, ULP coprocessors, and custom silicon) to reliably recover soft-bricked states, resolve USB descriptor enumeration failures (Code 43), and monitor boot telemetry without cable pulls.

---

## 🛠 Problem Overview

During firmware development involving low-power deep sleep cycles, ULP coprocessor handoffs, or clock domain switches, on-chip USB engines often enter invalid states. Windows subsequently drops the interface into:

```text
Unknown USB Device (Device Descriptor Request Failed) [Code 43]

```

At this stage:

* **PnP DevNode restarts (`pnputil /restart-device`) fail**, because the on-chip PHY remains stuck and cannot complete the software-initiated re-handshake.
* **Physical replugging works**, but interrupts automated testing, degrades hardware connectors, and slows down iterative debugging.

This tool bridges that gap by directly sending **`IOCTL_USB_HUB_CYCLE_PORT`** commands to the upstream USB host controller, forcing a hardware-level Single-Ended Zero (SE0) bus reset across the physical D+/D- lines while staying pinned to your workspace as an interactive terminal.

---

## ✨ Features

* **Hardware USB Bus Reset:** Bypasses software DevNode locks and triggers physical USB bus resets via raw Win32 IOCTLs (equivalent to UsbTreeView port cycling).
* **Embedded Serial Monitor:** Integrated non-blocking serial reader with dynamic COM port auto-detection, configurable baud rates (up to 921,600), and ANSI CSI color stripping.
* **Handle Protection Pipeline:** Automatically and cleanly drops active serial COM handles prior to issuing bus resets to avoid Windows kernel deadlocks.
* **Node Purge & Rescan:** Cleans out faulted or phantom device nodes and commands the Windows Plug-and-Play manager to rebuild the device tree.
* **One-Click Diagnostic Export:** Rapidly export complete console buffers with timestamps to `.log` or `.txt` files for crash backtrace analysis and register inspection.
* **Zero-Dependency GUI:** Built on native Windows Forms and C# P/Invoke within PowerShell; requires no Python runtime, pip wheels, or compiled binary installations.

---

## 🚀 Quick Start

### Prerequisites

* Windows 10 / 11 (x64)
* PowerShell 5.1+ or PowerShell Core (Admin privileges required for Win32 USB IOCTLs)

### Launching the Utility

1. Save `term_usb_toolbox.ps1` to your local workspace.
2. Launch directly from an elevated terminal or shortcut:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\term_usb_toolbox.ps1"

```

> **Note:** The script will automatically prompt for UAC elevation if launched from an unprivileged terminal.

---

## 🕹 Interface & Controls

| Control | Description |
| --- | --- |
| `[ OPEN MONITOR ]` | Connects to the selected COM port and streams bootloader/runtime serial telemetry. |
| `[ EXPORT LOG ]` | Opens a file dialog pre-filled with a timestamped filename (`esp32_boot_log_*.log`) to export the text buffer. |
| `>> [1] POWER CYCLE PORT (REPLUG)` | Sends an upstream hub port cycle command to simulate a physical cable disconnect/reconnect. |
| `>> [2] RESTART DRIVER (DISABLE/ENABLE)` | Toggles the PnP device state (`Disable-PnpDevice` followed by `Enable-PnpDevice`). |
| `>> [3] UNINSTALL FAULTED DEVICE (CODE 43)` | Uninstalls un-enumerated/broken DevNodes via `pnputil /remove-device`. |
| `>> [4] SCAN FOR HARDWARE CHANGES` | Triggers a PnP subsystem scan (`pnputil /scan-devices`) to re-enumerate hardware. |
| `ALWAYS ON TOP` | Pins the terminal console above IDEs, debuggers, and schematics. |

---

## 📋 Recommended Recovery Sequence

When hitting an enumeration lockup after a failed boot or sleep-transition cycle:

1. **Observe Terminal Header:** The status bar will switch to:
```text
PORTS: [NONE] // FAULTED NODES: 1 (RESET REQUIRED)

```


2. **Execute Hardware Reset:** Click **`>> [1] POWER CYCLE PORT (REPLUG)`**.
* The tool automatically closes any active COM handles.
* Upstream hub hardware drops power/SE0 lines momentarily.


3. **Verify Re-Enumeration:** Windows will re-detect the silicon and assign an address. The header returns to:
```text
SYS_ACTIVE // PORTS: [COMx] // FAULTS: 0

```


4. **Re-Open Monitor:** Click **`[ OPEN MONITOR ]`** to resume streaming device telemetry.

---

## 🔍 Post-Crash Backtrace Decoding

If the device panics before or during a recovery event, export the log using **`[ EXPORT LOG ]`** and feed the instruction pointers to your cross-compiler toolchain:

```bash
# Xtensa Main Core (ESP32-S3):
xtensa-esp32s3-elf-addr2line -pfia -e build/target.elf 0x420014a6 0x40378b20

# RISC-V ULP Coprocessor:
riscv32-esp-elf-addr2line -pfia -e build/esp-idf/ulp/ulp_main.elf 0x00000028

```

---

## 🔒 Security & Architecture Notes

* **Driver Safety:** No third-party filter drivers (such as WinUSB / LibUSB-Win32) are injected. All operations use native Windows PnP mechanisms and Microsoft host-controller IOCTL interfaces (`IOCTL_USB_HUB_CYCLE_PORT` = `0x220444`).
* **Handle Concurrency:** Windows prevents bus resets when an open file handle exists on a virtual serial device (`\\.\COMx`). The tool enforces handle closure before firing IOCTL routines to prevent system thread hangs.
