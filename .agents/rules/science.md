# Mathematical & Empirical Grounding Policy

This rule governs the physical, thermodynamic, and mathematical models implemented across **Humid1-OS**.

---

## 1. Math Precedes Code

> *"Code solves a problem that math already did."*

1. **No Ad-Hoc Heuristics:** Never implement heuristic curve-fitting, synthetic sensor scaling, or arbitrary correction dividers when underlying physics formulations exist.
2. **Thermodynamic Invariants:**
   - Ground environmental algorithms in thermodynamic conservation laws.
   - For psychrometrics: actual water vapor pressure ($e$) and Dew Point ($T_{\text{dew}}$) are invariants unaffected by local PCB self-heating. Calculate true metrics using August-Roche-Magnus formulations.
   - For temperature unit conversions: base internal representation is **Kelvin**; convert dynamically for display using:
     $$T_{^\circ\text{F}} = (T_{\text{K}} - 273.15) \times \frac{9}{5} + 32$$
     $$T_{^\circ\text{C}} = T_{\text{K}} - 273.15$$
   - For power: calculate State of Charge (SoC) using empirical 13-point LiPo Open-Circuit Voltage (OCV) lookups rather than arbitrary linear approximations.

---

## 2. Reproducibility & Mathematical Verification

1. **Citation of Governing Equations:** Every non-trivial calculation in code or documentation must cite its physical formula, IEEE standard, or component datasheet equation.
2. **Deterministic & Unit-Testable:**
   - Keep mathematical algorithms in pure, stateless functions.
   - Pure math routines must be testable on host compilation test fixtures without hardware dependencies.
3. **Integer Fixed-Point in Coprocessor Domains:**
   - Express temperatures in centi-degrees ($1^\circ\text{C} = 100$) and relative humidity in centi-percent when interfacing with H1OS RTC memory to preserve precision without soft-float emulation.

---

## 3. Empirical Evidence vs. Speculative Deduction ("Sherlock Ohming")

1. **No Made-Up Numbers:** Never estimate or invent operational figures (e.g., deep sleep current, wake duration, quiescent drain, Wi-Fi connect latency). Every ballpark or baseline metric MUST be cited directly from manufacturer datasheets or physically verified on laboratory instruments (DMM, oscilloscope, power profiler, network logs). If an operating figure was not measured or read in official docs, it is **not a valid metric**.
2. **Hypothesis vs. Fact Boundary:** Observed symptoms may strongly support a theoretical hardware deduction (e.g., parasitic conduction through unpowered ESD clamping diodes, pull-up rail backfeeding, fast RTC cache hit), but without direct bench measurement or schematic corroboration, it is an **engineering hypothesis**, NOT empirical fact. Formulating physical deductions ("Sherlock Ohming") is encouraged, but they must be explicitly labeled as *unverified hypotheses / speculative deductions*, never presented as empirical ground truth.
