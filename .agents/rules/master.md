---
trigger: always_on
---

# Rule: Master Layer 4 Application Invariants & Anti-Drift Lock

1. **Strict Layer 4 Scope Lock:**
   - This repository (`humid1-os`) is a **Layer 4 User Application** built on the `h1os` (`>= v2.4.9-dev`) middleware component.
   - **NEVER** modify `managed_components/h1os` or upstream `H1OS` files during application development.
   - **NEVER** include Layer 1 (`hal_*.h`), Layer 2 (`sub_*.h`), or Layer 3 (`orch_*.h`) headers directly in `firmware/main/`. Use only `<h1os/h1os.h>`, `<h1os/h1os_lvgl.h>`, `<h1os/h1os_time.h>`, and `h1os_fonts.h`.
2. **Spec-First Execution ("Documentation is Law"):**
   - Always consult `docs/h1os_reference.md` and `docs/architecture/APP_01..08.md` before modifying code in `firmware/main/`.
   - Never hallucinate H1OS functions, struct fields, or font names.
3. **Two-Strike Compiler Resolution Protocol:**
   - Build inside `firmware/` using `idf.py -B build_dev --preset dev build`.
   - If compilation fails twice on the same diagnostic, **STOP IMMEDIATELY**, output the verbatim compiler error, and ask the user for direction.
