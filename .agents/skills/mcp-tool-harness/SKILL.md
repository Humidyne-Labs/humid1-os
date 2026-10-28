---
name: mcp-tool-harness
description: Operational reference and subagent orchestration for the 3-Finger ESP32-S3 and Humid1-OS MCP Tool Suite.
---

# 3-Finger MCP Architecture & Subagent Orchestration Skill

## Overview
This skill governs the operational procedures, subagent delegation patterns, and tool interaction models for the 3-Finger MCP embedded firmware development architecture supporting Humid1-OS on ESP32-S3.

## The 3-Finger Architecture

### Finger 1: Semantic Code Intelligence (`clangd`)
* **Engine:** `schuay/clangd-mcp` bound to LLVM 18 (`C:\clang+llvm-18.1.8\bin\clangd_esp32.cmd`) and `compile_commands.json`.
* **Zero-Hallucination Capabilities:**
  - `get_callers` & `get_callees`: Full incoming/outgoing call hierarchies across tasks, drivers, and HAL.
  - `get_definition`: Exact jumps to implementation or header declarations.
  - `find_references`: Deterministic list of all call sites and symbol uses.
  - `find_symbol`: Instant symbol lookups for structs, enums, typedefs, and functions.
  - `get_type_info`: Complete type layout, sizing, and struct member signatures.
* **Cost & Accuracy:** Runs locally at $0.00 token cost, replacing token-heavy eager whole-repo extraction.

### Finger 2: Hardware, Build & Serial Execution (`esp-idf`)
* **Engine:** Native ESP-IDF v6.1 profile script (`C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1`).
* **Capabilities:** Incremental compilation via Ninja (`idf.py -C firmware build`), returning line-numbered compiler diagnostics.

### Finger 3: Documentation & Technical Reference (`local-index` & `espressif-documentation`)
* **Local Indexer (`local-index`):** Runs `uvx code-index-mcp` targeting architecture docs, pinouts, and errata.
  - *Tool Quirk:* `local-index/search_code_advanced` strictly requires argument `pattern: str` (do NOT pass `query`).
* **Upstream Docs (`espressif-documentation`):** Official Espressif API docs, Technical Reference Manual (TRM) sections, and silicon errata via `https://mcp.espressif.com/docs`.

## Scientific Computing, Verification & Telemetry (Lazy-Loaded)

### 1. Telemetry Analytics Engine (`duckdb`)
* **Engine:** `uvx --with "mcp<2" mcp-server-duckdb --db-path :memory:`
* **Usage:** Queries disk logs and font CSV metrics directly using SQL (`read_csv_auto(...)`).

### 2. Science & Math Prover (`science`)
* **Tools:**
  - `derive_fixed_point_polynomial`: Computes fixed-point polynomials for FPU-less coprocessors.
  - `solve_z3_assertions`: Formal verification using Microsoft Z3 SMT solver.
  - `eval_symbolic_math`: SymPy symbolic simplification, factoring, and evaluation.

### 3. Embedded Operations & Hardware Diagnostics (`embedded-ops`)
* **Tools:**
  - `murdock_memory_budget`: Structured Linker & Section Auditor. Evaluates `.elf` to return deterministic JSON byte counts and headroom percentages for DRAM, DIRAM, RTC FAST/SLOW, Flash partition headroom.
  - `ba_serial_capture_and_decode`: Serial capture with automated panic backtrace decoding via `xtensa-esp32s3-elf-addr2line`.
  - `clangd_touch_reload`: Busts clangd AST cache after CMake reconfigures.

## Subagent Orchestration Guidelines

### 1. Subagent Specialization
* **Research Subagents (`research`):** Spawn for targeted, multi-step documentation searches, external datasheet verification, or deep header inspections that would clutter main conversation context.
* **Parallel Execution:** Launch concurrent tasks for independent work (e.g. running build verification alongside documentation review).
* **Reactive Wakeup:** Do NOT poll subagents in loops. Wait for background reactive wakeups from the agent messaging system.

### 2. Zero Token-Burn Policy
* **Never load entire repositories into context:** Always query `clangd` or `local-index` lazily.
* **Single-file surgical edits:** Use `edit_file` with precise targets; avoid re-emitting whole files unnecessarily.
