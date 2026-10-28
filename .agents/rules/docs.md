# Documentation Governance, Historical Memory & Architecture Index

This rule governs documentation integrity, living memory tracking, and open-source attribution in **Humid1-OS**.

---

## 1. Documentation is Law

> *"Documentation is memory and history reflective of progress and hard lessons learned. We document the weeds so we can find flowers next time."*

1. **Mandatory Synchronous Updates:**
   - Documentation is never an afterthought; it is committed alongside code changes in the exact same commit.
   - When APIs, telemetry schemas, dynamic attributes, state machines, or screen layouts change, the corresponding documentation in `docs/architecture/` MUST be updated simultaneously.
2. **Documenting the Weeds (Failure Analysis & Errata):**
   - Record design dead-ends, server quirks, discarded UI patterns, or radio stack behaviors in `docs/architecture/APP_07_os_api_mandates.md` and `docs/h1os_reference.md`.
   - Never erase historical technical rationale—distill it into authoritative digests so future maintainers avoid repeating discarded experiments.

---

## 2. Open Source Hardware (OSHW) & Attribution

1. **Public Standards & Clean Boundaries:**
   - Ensure clear modular boundaries between upstream H1OS middleware and the humid1-os application and cloud tier.
   - Maintain clear hardware reference documentation (`docs/h1os_reference.md`), pinouts, and errata.
2. **Strict Attribution & Licensing:**
   - Always credit original authors, upstream algorithms, font designers, and driver creators.
   - Maintain appropriate SPDX license tags (`SPDX-License-Identifier: MIT` or `CC-BY-4.0`) across all code and documentation files.

### Project Engineering Attribution
- **Lead Hardware & Systems Architect:** Humiditron (Humidyne Labs)
- **Principal Firmware Audit, `-O2` Wake Stub Hardening & Event-Driven Architecture Refactor (`v2.4.4-dev`):** Gemini (Autonomous Embedded Systems Architect & Principal Auditor)
- **Core Firmware Development, Metrology & Simulation Engineering (`v2.0` – `v2.4.3-dev`):** Antigravity Gemini 3.8 (Junior Embedded Systems Agent)
