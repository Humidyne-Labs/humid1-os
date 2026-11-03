# APP_08: Documentation Governance, Engineering Lexicon & Pre-Flight Gates

> **Purpose:** Enforces strict documentation alignment, professional IEEE/embedded terminology, and automated pre-flight verification across `humid1-os`.

---

## 1. Mandatory Pre-Flight Verification Gates

Before committing any code or documentation change in `humid1-os`, developers and autonomous agents MUST execute and pass:

1. **Identity & Engineering Lexicon Gate:**
   ```bash
   python tools/agent/sanitize.py --check
   ```
   Must report `[PASS] Zero identity leaks or lexicon violations detected.`
2. **1-BPP e-Paper String Width Gate (`<= 188.0 px`):**
   ```bash
   python tools/agent/check_string_width.py --audit-screens
   ```
   Must report `0 FAIL` across all `firmware/main/screens/*.c` templates.
3. **ESP-IDF v6.1 Strict Compiler Gate (`-Wall -Wextra -Werror`):**
   ```bash
   cd firmware
   idf.py -B build_dev --preset dev build
   ```

---

## 2. Engineering Lexicon Standard

Informal metaphors, military slang, and rhythmic colloquialisms are prohibited in source code, comments, commit messages, and documentation. Always use standard IEEE/embedded terminology (`interval` or `period` instead of banned terms; `data buffer` or `telemetry frame`; `sample burst`; `monitor`; `threshold`; `default` or `recovery mode`; `clear` or `discharge`).
