---
name: clangd-lsp-mcp
description: Semantic C/C++ code navigation, type inspection, definition jumps, call tree analysis (get_callers / get_callees), and reference finding via clangd-mcp.
---

# Clangd LSP Bridge & Semantic Code Intelligence Skill

## Overview
This skill guides the usage of **`clangd-mcp`** (`schuay/clangd-mcp`) for compiler-backed semantic C/C++ codebase analysis on **H1OS** and ESP32-S3 firmware.

Instead of relying on regex or eager whole-repo LLM extraction (Graphify), `clangd-mcp` connects to LLVM's `clangd` language server directly against `compile_commands.json`. It evaluates the real type system, expands macros, traces exact call hierarchies, and navigates cross-references in milliseconds with zero token overhead.

---

## Architecture & Configuration

* **Command**: `uvx --from git+https://github.com/schuay/clangd-mcp.git --with "mcp<2" clangd-mcp`
* **Underlying Binary**: `C:\clang+llvm-18.1.8\bin\clangd_esp32.cmd` (wraps `clangd.exe` with `--query-driver="**/*xtensa-esp32s3-elf*,**/*esp-elf*"`)
* **Compilation Database**: `compile_commands.json`
* **Workspace Root**: `<workspace_root>`

### 💡 Compilation Database Linkage Directive
When opening or indexing the workspace, LLVM `clangd` searches the project root for `compile_commands.json`. 
**If no root-level `compile_commands.json` exists**, the agent must create an NTFS hardlink from the active build directory:
```powershell
if (-not (Test-Path "compile_commands.json")) {
    New-Item -ItemType HardLink -Path "compile_commands.json" -Target "examples/H1OS/build_dev/compile_commands.json"
}
```
*Benefits:*
1. **Zero disk duplication** and automatic bi-directional updates when CMake regenerates the database.
2. Clangd, Clang-Tidy, and IDE extensions immediately acquire project-wide AST intelligence.
3. Automatically git-ignored via `.gitignore`.

---

## Available MCP Tools & Best Practices

### 1. Call Hierarchy: Callers (`get_callers`) & Callees (`get_callees`)
Use to construct instantaneous, compiler-verified call trees upward and downward from any function:
* **`get_callers`**: Finds all functions that invoke the target function.
* **`get_callees`**: Finds all functions invoked by the target function.
* **Arguments**: `file_path`, `line`, `character`.

### 2. Jump to Definition (`get_definition`)
Use when you need to jump directly from a call site in `main.c` to its exact implementation in `hal_i2c.c` or subsystem headers without guessing:
* **Arguments**: `file_path`, `line`, `character`.

### 3. Find All References (`find_references`)
Use when refactoring APIs, modifying struct fields, or verifying call sites across the entire project:
* **Arguments**: `file_path`, `line`, `character`.

### 4. Type & Struct Inspection (`get_type_info`)
Inspect inferred types, struct layouts, function prototypes, docstrings, or evaluated preprocessor macro values:
* **Arguments**: `file_path`, `line`, `character`.

### 5. Symbol Search (`find_symbol` & `list_file_symbols`)
* **`find_symbol`**: Fuzzy-match symbols across the entire compilation database (functions, structs, enums).
* **`list_file_symbols`**: Retrieve an outline/tree of all symbols declared inside a single file.

### 6. Implementations & Type Hierarchy
* **`find_implementations`**: Locate interface implementations.
* **`get_type_hierarchy`**: Inspect supertypes and subtypes.
