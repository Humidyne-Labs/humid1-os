---
name: main-merge-protocol
description: Operational reference, validation checklists, and execution commands for merging code into main for Humid1-OS. Enforces the pre-merge protocol: identity sanitization, /shrink asset optimization, synchronous architecture documentation, version synchronization across canonical locations, verified zero-warning ESP-IDF builds, and changelog updates.
---

# Pre-Merge to Main Protocol & Release Execution

This skill provides step-by-step procedures, one-liner automation commands, and validation scripts for preparing, merging, and releasing code into the `main` branch of **Humid1-OS**.

---

## 1. Quick Reference: Pre-Merge Checklist

| Step | Action | Tools / Files Involved | Verification |
| :--- | :--- | :--- | :--- |
| **0a. Identity Audit** | Sanitize & Strip Personal Paths | `python firmware/tools/agent/sanitize.py --check` | Zero leaks in code, archives, or git logs |
| **0b. Asset Shrink Gate** | Quantize Assets & PDFs | `/shrink` or `firmware/tools/agent/optimize_*.py` | Lossless PDF meta, EXIF stamped |
| **1. Invariant Check** | Firmware Safety Audit | `.agents/rules/coding.md`, `.agents/skills/h1os-firmware-standards` | 10 Safety Invariants validated |
| **2. Clean Build** | Compile firmware | `idf.py -C firmware build` via ESP-IDF v6.1 profile | Exit code 0, 0 warnings/errors |
| **3. Rev Version** | Bump semantic version | `firmware/main/app_config.h`, `idf_component.yml`, `README.md` | Synchronized version strings |
| **4. Sync Docs** | Synchronous Architecture Docs | `docs/architecture/APP_01..07` | Architecture docs match code |
| **5. Update Changelog**| Document modifications | `CHANGELOG.md` | Keep a Changelog entry with date |
| **6. Git Commit & Push**| Push feature branch | `git commit`, `git push origin <branch>` | Branch clean on remote |
| **7. Merge & Tag** | Merge to main & tag | `git checkout main`, `git merge`, `git tag -a` | Main and tag pushed to remote |

---

## 2. Step-by-Step Execution Guide

### Step 0: Identity Hygiene & Repository Sanitization

Run the automated identity and name sanitizer before staging or committing changes:

```powershell
python firmware/tools/agent/sanitize.py --check
```

If un-sanitized author names, personal workstation paths, or archive leaks are reported, apply automatic in-place sanitization:

```powershell
python firmware/tools/agent/sanitize.py --fix
```

---

### Step 1: Rev Semantic Version Strings

When bumping version:
1. `firmware/main/app_config.h`: Update `APP_FIRMWARE_VERSION_STRING`.
2. `firmware/main/idf_component.yml`: Update `version: "X.Y.Z"`.
3. `README.md`: Update version badge and headings.
4. `CHANGELOG.md`: Add release section header `## [X.Y.Z] - YYYY-MM-DD`.

---

### Step 2: Synchronize Architecture Documentation

Ensure any changes to state machine, telemetry schema, dynamic attributes, screens, or H1OS API mandates are reflected in `docs/architecture/`.

---

### Step 3: Touch & Update `CHANGELOG.md`

Insert the new release ledger block directly above the previous release in `CHANGELOG.md`:

```markdown
## [X.Y.Z] - YYYY-MM-DD

### 🚀 Added
* **Feature Name** (`firmware/main/...`): Description of new functionality.

### 🔄 Changed
* **Architectural Refactor** (`firmware/main/...`): Description of behavior change.

### 🛡️ Fixed & Hardened
* **Bug Fix / Safety Invariant** (`firmware/main/...`): Specific resolution details.
```

---

### Step 4: Verify Clean Build

Run incremental build via canonical profile:

```powershell
& "C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1"; idf.py -C firmware build
```
Verify build finishes with exit code 0 and zero warnings.
