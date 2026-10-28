#!/usr/bin/env python3
"""
H1OS Production Firmware Release Packager & Distribution Tool
Version: 1.0.0
Author: Humidyne Labs / H1OS Firmware Engineering

Auto-discovers ESP-IDF build artifacts (build_prod / build_dev),
merges bootloader, partition table, OTA data, and application binaries
into a single monolithic 0x0 factory image, generates 1-click 'flash_now.bat'
and 'flash_now.sh' flasher scripts, computes SHA-256 checksums, and packages
a complete zip distribution bundle.
"""

import argparse
import datetime
import hashlib
import os
import re
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

VERSION = "1.0.0"
CANDIDATE_BUILD_DIRS = ["build_prod", "prod_build", "build_dev", "dev_build", "build"]
DEFAULT_CHIP = "esp32s3"
DEFAULT_BAUD = 460800


def get_idf_python_exe() -> str:
    """Locate the Python executable within the active or canonical ESP-IDF virtual environment."""
    env_path = os.environ.get("IDF_PYTHON_ENV_PATH")
    if env_path:
        for sub in [("Scripts", "python.exe"), ("bin", "python")]:
            p = Path(env_path) / sub[0] / sub[1]
            if p.exists():
                return str(p)

    candidates = [
        Path(r"C:\Espressif\tools\python\v6.1\venv\Scripts\python.exe"),
        Path(r"C:\Espressif\tools\python\v5.3\venv\Scripts\python.exe"),
        Path(r"C:\Espressif\tools\python\v5.1\venv\Scripts\python.exe"),
    ]
    for c in candidates:
        if c.exists():
            return str(c)

    return sys.executable


def find_project_root(start_dir: Path) -> Path:
    """Climb up directories until components/h1os is found."""
    current = start_dir.resolve()
    for _ in range(6):
        if (current / "components" / "h1os").exists():
            return current
        if (current / ".git").exists():
            return current
        if current.parent == current:
            break
        current = current.parent
    return start_dir.resolve()


def resolve_h1os_version(repo_root: Path) -> str:
    """Extract semantic version string from h1os_version.h."""
    header_candidates = [
        repo_root / "components" / "h1os" / "include" / "h1os" / "h1os_version.h",
        repo_root.parent / "components" / "h1os" / "include" / "h1os" / "h1os_version.h",
        repo_root.parent.parent / "components" / "h1os" / "include" / "h1os" / "h1os_version.h",
    ]
    for h in header_candidates:
        if h.exists():
            try:
                content = h.read_text(encoding="utf-8")
                m = re.search(r'#define\s+H1OS_VERSION_STRING\s+"([^"]+)"', content)
                if m:
                    return m.group(1).strip()
            except Exception:
                pass
    try:
        res = subprocess.run(["git", "describe", "--tags", "--always"], cwd=repo_root, capture_output=True, text=True, check=True)
        tag = res.stdout.strip().lstrip("v")
        if tag:
            return tag
    except Exception:
        pass
    raise RuntimeError(f"Could not locate h1os_version.h from root {repo_root}")


def get_git_commit(repo_root: Path) -> str:
    """Retrieve short git commit hash."""
    try:
        res = subprocess.run(
            ["git", "rev-parse", "--short", "HEAD"],
            cwd=str(repo_root),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False,
        )
        if res.returncode == 0:
            return res.stdout.strip()
    except Exception:
        pass
    return "unknown"


def find_build_dir(search_roots: list, preferred: str = None) -> Path:
    """Find the most appropriate build directory."""
    if preferred:
        p = Path(preferred)
        if p.exists() and (p / "h1os.bin").exists():
            return p.resolve()

    for root in search_roots:
        if not root.exists():
            continue
        for name in CANDIDATE_BUILD_DIRS:
            cand = root / name
            if cand.exists() and (cand / "h1os.bin").exists():
                return cand.resolve()
    return None


def parse_flash_args(build_dir: Path) -> dict:
    """Parse flash_args file in the build directory."""
    flash_args_file = build_dir / "flash_args"
    info = {
        "flash_mode": "dio",
        "flash_freq": "80m",
        "flash_size": "8MB",
        "binaries": [],  # list of tuples: (offset_int, offset_hex_str, relative_path, absolute_path)
    }

    if flash_args_file.exists():
        lines = flash_args_file.read_text(encoding="utf-8").splitlines()
        for line in lines:
            line = line.strip()
            if not line or line.startswith("#"):
                continue

            # Flags line: --flash-mode dio --flash-freq 80m --flash-size 8MB
            if line.startswith("--"):
                parts = line.split()
                for i in range(len(parts)):
                    if parts[i] in ["--flash-mode", "--flash_mode"] and i + 1 < len(parts):
                        info["flash_mode"] = parts[i + 1]
                    elif parts[i] in ["--flash-freq", "--flash_freq"] and i + 1 < len(parts):
                        info["flash_freq"] = parts[i + 1]
                    elif parts[i] in ["--flash-size", "--flash_size"] and i + 1 < len(parts):
                        info["flash_size"] = parts[i + 1]
                continue

            # Offset + binary line: 0x20000 h1os.bin
            tokens = line.split(maxsplit=1)
            if len(tokens) == 2:
                offset_str, bin_rel = tokens[0], tokens[1]
                try:
                    offset_int = int(offset_str, 16)
                    bin_abs = (build_dir / bin_rel).resolve()
                    info["binaries"].append((offset_int, offset_str, bin_rel, bin_abs))
                except ValueError:
                    pass

    # Fallback to standard ESP32-S3 layout if flash_args incomplete
    if not info["binaries"]:
        default_layout = [
            (0x0, "0x0", "bootloader/bootloader.bin"),
            (0x8000, "0x8000", "partition_table/partition-table.bin"),
            (0x1D000, "0x1d000", "ota_data_initial.bin"),
            (0x20000, "0x20000", "h1os.bin"),
        ]
        for off_int, off_hex, rel in default_layout:
            abs_p = (build_dir / rel).resolve()
            if abs_p.exists():
                info["binaries"].append((off_int, off_hex, rel, abs_p))

    # Sort binaries by offset
    info["binaries"].sort(key=lambda x: x[0])
    return info


def sha256_file(filepath: Path) -> str:
    """Compute SHA-256 hash of a file."""
    h = hashlib.sha256()
    with open(filepath, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()


def generate_flash_now_bat(version: str, flash_info: dict, factory_bin_name: str, app_bin_name: str) -> str:
    """Generate interactive Windows 1-click batch flasher."""
    flash_mode = flash_info.get("flash_mode", "dio")
    flash_freq = flash_info.get("flash_freq", "80m")
    flash_size = flash_info.get("flash_size", "8MB")

    return f"""@echo off
setlocal EnableDelayedExpansion
title H1OS {version} Firmware Flasher
color 0A

echo =========================================================================
echo               H1OS Firmware Flasher Tool - {version}
echo          Target Hardware: Waveshare ESP32-S3 ePaper 1.54 V2
echo =========================================================================
echo.

rem --- Detect Python Interpreter ---
set "PY_CMD="
where python >nul 2>&1
if not errorlevel 1 (
    set "PY_CMD=python"
    goto :check_esptool
)
where python3 >nul 2>&1
if not errorlevel 1 (
    set "PY_CMD=python3"
    goto :check_esptool
)

rem Check canonical Espressif IDF venv python
if exist "C:\\Espressif\\tools\\python\\v6.1\\venv\\Scripts\\python.exe" (
    set "PY_CMD=C:\\Espressif\\tools\\python\\v6.1\\venv\\Scripts\\python.exe"
    goto :check_esptool
)

echo [ERROR] Python was not found in PATH or at C:\\Espressif.
echo Please install Python 3.8 or higher.
echo.
pause
exit /b 1

:check_esptool
%PY_CMD% -m esptool version >nul 2>&1
if not errorlevel 1 goto :detect_port

echo [WARNING] esptool module not found. Installing via pip...
%PY_CMD% -m pip install esptool
if errorlevel 1 (
    echo [ERROR] Failed to install esptool. Please run: pip install esptool
    echo.
    pause
    exit /b 1
)

:detect_port
echo Scanning serial COM ports...
set "PORT="
for /f "tokens=1" %%p in ('%PY_CMD% -m serial.tools.list_ports 2^>nul') do (
    set "PORT=%%p"
)

if not "%PORT%"=="" (
    echo [INFO] Detected device port: %PORT%
    set /p "USER_PORT=Press ENTER to use %PORT%, or enter COM port manually: "
    if not "!USER_PORT!"=="" set "PORT=!USER_PORT!"
) else (
    set /p "PORT=Enter ESP32-S3 COM port [e.g. COM3]: "
)

if "%PORT%"=="" (
    echo [ERROR] No COM port provided.
    echo.
    pause
    exit /b 1
)

:show_menu
echo.
echo =========================================================================
echo Select Flash Option:
echo   [1] Full Factory Flash  - Monolithic Image [0x0, Clean Reinstall]
echo   [2] Fast App Update     - Firmware Only [0x20000, Keeps Settings]
echo   [3] Full Chip Erase     - Erase Entire Flash, then Factory Flash
echo =========================================================================
set "CHOICE=1"
set /p "CHOICE=Enter choice 1, 2, or 3 [default 1]: "

echo.
if "%CHOICE%"=="1" goto :flash_factory
if "%CHOICE%"=="2" goto :flash_app
if "%CHOICE%"=="3" goto :erase_and_flash

echo [ERROR] Invalid selection "%CHOICE%".
goto :show_menu

:flash_factory
echo [INFO] Flashing complete factory binary at 0x0 on %PORT%...
%PY_CMD% -m esptool --chip esp32s3 -p %PORT% -b {DEFAULT_BAUD} --before default-reset --after hard-reset write-flash --flash-mode {flash_mode} --flash-freq {flash_freq} --flash-size {flash_size} 0x0 {factory_bin_name}
goto :flash_result

:flash_app
echo [INFO] Flashing application payload only at 0x20000 on %PORT%...
%PY_CMD% -m esptool --chip esp32s3 -p %PORT% -b {DEFAULT_BAUD} --before default-reset --after hard-reset write-flash --flash-mode {flash_mode} --flash-freq {flash_freq} --flash-size {flash_size} 0x20000 {app_bin_name}
goto :flash_result

:erase_and_flash
echo [WARNING] Erasing entire flash memory on %PORT%...
%PY_CMD% -m esptool --chip esp32s3 -p %PORT% erase-flash
if errorlevel 1 goto :flash_result
echo.
echo [INFO] Flashing factory binary at 0x0...
%PY_CMD% -m esptool --chip esp32s3 -p %PORT% -b {DEFAULT_BAUD} --before default-reset --after hard-reset write-flash --flash-mode {flash_mode} --flash-freq {flash_freq} --flash-size {flash_size} 0x0 {factory_bin_name}
goto :flash_result

:flash_result
if errorlevel 1 (
    echo.
    echo =========================================================================
    echo [FAILED] Flash operation encountered an error.
    echo Please verify USB connection, driver, and selected COM port.
    echo =========================================================================
) else (
    echo.
    echo =========================================================================
    echo [SUCCESS] Flash completed successfully! Device is rebooting into H1OS.
    echo =========================================================================
)
echo.
pause
"""


def generate_flash_now_sh(version: str, flash_info: dict, factory_bin_name: str, app_bin_name: str) -> str:
    """Generate 1-click Linux/macOS shell flasher."""
    flash_mode = flash_info.get("flash_mode", "dio")
    flash_freq = flash_info.get("flash_freq", "80m")
    flash_size = flash_info.get("flash_size", "8MB")

    return f"""#!/usr/bin/env bash
set -e

echo "========================================================================="
echo "               H1OS Firmware Flasher Tool - {version}"
echo "          Target Hardware: Waveshare ESP32-S3 ePaper 1.54 V2"
echo "========================================================================="
echo ""

PYTHON_CMD="python3"
if ! command -v python3 &> /dev/null; then
    if command -v python &> /dev/null; then
        PYTHON_CMD="python"
    else
        echo "[ERROR] python3 could not be found."
        exit 1
    fi
fi

if ! $PYTHON_CMD -m esptool version &> /dev/null; then
    echo "[WARNING] esptool not found. Attempting install..."
    $PYTHON_CMD -m pip install esptool
fi

PORT="${{1}}"
if [ -z "$PORT" ]; then
    echo "Available serial ports:"
    $PYTHON_CMD -m serial.tools.list_ports || true
    echo ""
    read -p "Enter serial port (e.g. /dev/ttyACM0 or /dev/cu.usbmodem*): " PORT
fi

if [ -z "$PORT" ]; then
    echo "[ERROR] No port provided."
    exit 1
fi

echo ""
echo "Select Flash Option:"
echo "[1] Full Factory Flash  - Monolithic Image (0x0, Clean Reinstall) [Recommended]"
echo "[2] Fast App Update     - Firmware Only (0x20000, Keeps Settings/NVS)"
echo "[3] Full Chip Erase     - Erase Entire Flash, then Full Factory Flash"
read -p "Enter choice (1, 2, or 3, default is 1): " CHOICE
CHOICE="${{CHOICE:-1}}"

echo ""
if [ "$CHOICE" = "1" ]; then
    echo "[INFO] Flashing factory image at 0x0..."
    $PYTHON_CMD -m esptool --chip esp32s3 -p "$PORT" -b {DEFAULT_BAUD} --before default-reset --after hard-reset write-flash --flash-mode {flash_mode} --flash-freq {flash_freq} --flash-size {flash_size} 0x0 "{factory_bin_name}"
elif [ "$CHOICE" = "2" ]; then
    echo "[INFO] Flashing app payload at 0x20000..."
    $PYTHON_CMD -m esptool --chip esp32s3 -p "$PORT" -b {DEFAULT_BAUD} --before default-reset --after hard-reset write-flash --flash-mode {flash_mode} --flash-freq {flash_freq} --flash-size {flash_size} 0x20000 "{app_bin_name}"
elif [ "$CHOICE" = "3" ]; then
    echo "[WARNING] Erasing full flash..."
    $PYTHON_CMD -m esptool --chip esp32s3 -p "$PORT" erase-flash
    echo "[INFO] Flashing factory image at 0x0..."
    $PYTHON_CMD -m esptool --chip esp32s3 -p "$PORT" -b {DEFAULT_BAUD} --before default-reset --after hard-reset write-flash --flash-mode {flash_mode} --flash-freq {flash_freq} --flash-size {flash_size} 0x0 "{factory_bin_name}"
else
    echo "[ERROR] Invalid selection."
    exit 1
fi

echo ""
echo "[SUCCESS] Flash completed successfully!"
"""


def main():
    parser = argparse.ArgumentParser(description=f"H1OS Production Release Packager v{VERSION}")
    parser.add_argument("--build-dir", "-b", "-B", help="Specific build directory containing binaries (e.g. -B build_prod)")
    parser.add_argument("positional_build_dir", nargs="?", default=None, help="Optional positional build directory (e.g. build_prod)")
    parser.add_argument("--out-dir", "-o", help="Custom output directory for release artifacts")
    parser.add_argument("--no-zip", action="store_true", help="Skip zip bundle generation")
    parser.add_argument("--version", "-V", help="Explicit version string override (e.g. 2.4.3)")
    parser.add_argument("--prod", action="store_true", help="Force production naming (strip -dev tag)")
    parser.add_argument("--dev", action="store_true", help="Force dev naming (keep -dev tag)")
    args = parser.parse_args()

    target_build_dir = args.build_dir or args.positional_build_dir

    script_dir = Path(__file__).resolve().parent
    repo_root = find_project_root(script_dir)
    git_commit = get_git_commit(repo_root)
    py_exe = get_idf_python_exe()

    # Locate build directory
    search_dirs = [repo_root / "examples" / "H1OS", repo_root, script_dir]
    build_dir = find_build_dir(search_dirs, preferred=target_build_dir)

    if not build_dir:
        print("[ERROR] Could not find a valid build directory containing 'h1os.bin'.")
        print(f"Searched in: {[str(d) for d in search_dirs]}")
        print("Please compile first with: idf.py -B build_prod build (or build_dev)")
        sys.exit(1)

    raw_version = resolve_h1os_version(repo_root)
    is_prod_build = "prod" in build_dir.name.lower()

    if args.version:
        version = args.version.lstrip("v")
    elif args.prod or (is_prod_build and not args.dev):
        # When packaging from a production build directory (build_prod), strip any -dev/-alpha/-beta pre-release suffix
        version = re.sub(r"-(dev|alpha|beta|rc\d*)$", "", raw_version, flags=re.IGNORECASE)
    else:
        version = raw_version

    build_flavor = "Production" if is_prod_build else "Development"

    print(f"\n{'='*72}")
    print(f"  H1OS {build_flavor} Firmware Packager v{VERSION}")
    print(f"  Target Release: v{version} (Commit: {git_commit})")
    print(f"{'='*72}\n")

    print(f"[*] Discovered Build Directory : {build_dir} [{build_flavor}]")
    flash_info = parse_flash_args(build_dir)
    print(f"[*] Flash Configuration       : Mode={flash_info['flash_mode']}, Freq={flash_info['flash_freq']}, Size={flash_info['flash_size']}")
    print(f"[*] Target Binaries Found      : {len(flash_info['binaries'])} parts")
    for off_int, off_hex, rel, abs_p in flash_info["binaries"]:
        sz_kb = abs_p.stat().st_size / 1024.0
        print(f"    - {off_hex:>7} : {rel:<35} ({sz_kb:7.1f} KB)")

    # Prepare output distribution directory
    dist_root = Path(args.out_dir) if args.out_dir else (repo_root / "dist" / f"h1os-v{version}")
    dist_root.mkdir(parents=True, exist_ok=True)
    bins_dir = dist_root / "bins"
    bins_dir.mkdir(parents=True, exist_ok=True)

    factory_bin_name = f"h1os-v{version}-factory.bin"
    factory_bin_path = dist_root / factory_bin_name
    app_bin_name = "h1os.bin"

    # Step 1: Copy individual binaries into dist/bins/ and dist/
    for _, _, rel, abs_p in flash_info["binaries"]:
        dest_filename = Path(rel).name
        shutil.copy2(abs_p, bins_dir / dest_filename)
        # Also copy main app binary directly into dist root for fast access
        if dest_filename == "h1os.bin":
            shutil.copy2(abs_p, dist_root / app_bin_name)

    # Step 2: Merge into monolithic 0x0 factory binary via esptool
    print(f"\n[*] Generating monolithic factory binary: {factory_bin_name}...")
    merge_args = [
        py_exe,
        "-m",
        "esptool",
        "--chip",
        DEFAULT_CHIP,
        "merge-bin",
        "-o",
        str(factory_bin_path),
        "--flash-mode",
        flash_info["flash_mode"],
        "--flash-freq",
        flash_info["flash_freq"],
        "--flash-size",
        flash_info["flash_size"],
    ]
    for _, off_hex, _, abs_p in flash_info["binaries"]:
        merge_args.extend([off_hex, str(abs_p)])

    try:
        res = subprocess.run(merge_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=False)
        if res.returncode != 0:
            # Fallback to merge_bin if newer command syntax failed
            merge_args[5] = "merge_bin"
            res = subprocess.run(merge_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=False)
        
        if res.returncode == 0:
            sz = factory_bin_path.stat().st_size / 1024.0
            print(f"    [+] Created: {factory_bin_name} ({sz:.1f} KB)")
        else:
            print(f"[!] Warning: esptool merge-bin failed: {res.stderr.strip()}")
            print("[!] Generating python fallback pad-concatenation...")
            # Fallback manual merge padding with 0xFF
            max_offset = max(off for off, _, _, _ in flash_info["binaries"])
            last_bin = [abs_p for off, _, _, abs_p in flash_info["binaries"] if off == max_offset][0]
            total_size = max_offset + last_bin.stat().st_size
            buf = bytearray(b"\xFF" * total_size)
            for off_int, _, _, abs_p in flash_info["binaries"]:
                data = abs_p.read_bytes()
                buf[off_int : off_int + len(data)] = data
            factory_bin_path.write_bytes(buf)
            print(f"    [+] Created (manual fallback): {factory_bin_name} ({len(buf)/1024.0:.1f} KB)")
    except Exception as e:
        print(f"[ERROR] Failed to generate merged binary: {e}")
        sys.exit(1)

    # Step 3: Generate flash_now.bat and flash_now.sh
    print("[*] Generating 1-click flasher scripts (flash_now.bat, flash_now.sh)...")
    bat_content = generate_flash_now_bat(version, flash_info, factory_bin_name, app_bin_name)
    (dist_root / "flash_now.bat").write_text(bat_content, encoding="utf-8")

    sh_content = generate_flash_now_sh(version, flash_info, factory_bin_name, app_bin_name)
    sh_file = dist_root / "flash_now.sh"
    sh_file.write_text(sh_content, encoding="utf-8")
    try:
        sh_file.chmod(0o755)
    except Exception:
        pass

    # Step 4: Write flash instructions README
    readme_content = f"""# H1OS {version} Firmware Release Bundle
Target: Waveshare ESP32-S3 ePaper 1.54" V2 (ESP32-S3-PICO-1-N8R8)
Commit: {git_commit}
Date  : {datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")}

## Quick Start Flashing:

### On Windows:
1. Connect ESP32-S3 via USB-C.
2. Double-click `flash_now.bat`.
3. Choose option [1] for Full Factory Flash.

### On Linux / macOS:
```bash
chmod +x flash_now.sh
./flash_now.sh /dev/ttyACM0
```

### Manual Command via esptool:
```bash
python -m esptool --chip esp32s3 -b 460800 write-flash 0x0 {factory_bin_name}
```

## Binary Contents:
- `{factory_bin_name}`: Complete monolithic flash image (starts at 0x0).
- `{app_bin_name}`: Application payload only (starts at 0x20000).
- `bins/`: Individual modular partitions (bootloader, partition-table, ota_data).
"""
    (dist_root / "README.txt").write_text(readme_content, encoding="utf-8")

    # Step 5: Compute SHA-256 Checksums
    checksums_file = dist_root / "checksums.sha256"
    checksum_lines = []
    print("\n[*] Generating SHA-256 Checksums:")
    for p in sorted(dist_root.rglob("*")):
        if p.is_file() and p.name != "checksums.sha256":
            rel_name = p.relative_to(dist_root).as_posix()
            sha = sha256_file(p)
            checksum_lines.append(f"{sha}  {rel_name}")
            if p.parent == dist_root:
                print(f"    - {sha[:12]}...  {rel_name:<32} ({p.stat().st_size / 1024.0:6.1f} KB)")
    checksums_file.write_text("\n".join(checksum_lines) + "\n", encoding="utf-8")

    # Step 6: Create Zip Bundle
    if not args.no_zip:
        zip_path = dist_root.parent / f"h1os-v{version}-bundle.zip"
        print(f"\n[*] Compressing Release Archive: {zip_path.name}...")
        with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as z:
            for p in sorted(dist_root.rglob("*")):
                if p.is_file():
                    z.write(p, arcname=p.relative_to(dist_root.parent))
        zip_sz_mb = zip_path.stat().st_size / (1024.0 * 1024.0)
        print(f"    [+] Created Release Zip: {zip_path.name} ({zip_sz_mb:.2f} MB)")

    print(f"\n{'='*72}")
    print(f"  [SUCCESS] H1OS {build_flavor} Release v{version} Packaged Successfully!")
    print(f"  Distribution Directory : {dist_root}")
    if not args.no_zip:
        print(f"  Release Archive (.zip) : {zip_path}")
    print(f"{'='*72}\n")


if __name__ == "__main__":
    main()
