#!/usr/bin/env python3
"""
ESP-IDF Binary Size & Memory Footprint Diagnostic Inspector
Version: 2.2.0
"""
import argparse
import datetime
import os
import re
import subprocess
import sys
from pathlib import Path

VERSION = "2.2.0"
CANDIDATE_DIRS = ["build_prod", "prod_build", "build_dev", "dev_build", "build"]
TIMESTAMP = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
OUTPUT_STREAMS = {}
LOG_FILE = f"size_diagnostic_{TIMESTAMP}.log"


def log_write(text):
    print(text, end="", flush=True)
    try:
        with open(LOG_FILE, "a", encoding="utf-8") as f:
            f.write(text)
    except Exception:
        pass


def get_idf_python_exe():
    """Locate the Python executable within the active or canonical ESP-IDF virtual environment."""
    env_path = os.environ.get("IDF_PYTHON_ENV_PATH")
    if env_path:
        p = Path(env_path) / "Scripts" / "python.exe"
        if p.exists():
            return str(p)
        p = Path(env_path) / "bin" / "python"
        if p.exists():
            return str(p)

    candidates = [
        Path(r"C:\Espressif\tools\python\v6.1\venv\Scripts\python.exe"),
        Path(r"C:\Espressif\tools\python\v5.3\venv\Scripts\python.exe"),
    ]
    for c in candidates:
        if c.exists():
            return str(c)

    return sys.executable


def get_idf_cmd_prefix(py_exe):
    """Resolve the idf.py launcher prefix with explicit python interpreter."""
    idf_path = os.environ.get("IDF_PATH")
    if idf_path:
        cand = Path(idf_path) / "tools" / "idf.py"
        if cand.exists():
            return f'"{py_exe}" "{cand}"'

    candidates = [
        Path(r"C:\esp\v6.1\esp-idf\tools\idf.py"),
        Path(r"C:\esp\v5.3\esp-idf\tools\idf.py"),
        Path(r"C:\Espressif\frameworks\esp-idf\tools\idf.py"),
    ]
    for c in candidates:
        if c.exists():
            return f'"{py_exe}" "{c}"'

    return "idf.py"


def find_check_sizes_script():
    """Locate ESP-IDF partition size verification script."""
    idf_path = os.environ.get("IDF_PATH")
    if idf_path:
        p = Path(idf_path) / "components" / "partition_table" / "check_sizes.py"
        if p.exists():
            return p

    candidates = [
        Path(r"C:\esp\v6.1\esp-idf\components\partition_table\check_sizes.py"),
        Path(r"C:\esp\v5.3\esp-idf\components\partition_table\check_sizes.py"),
        Path(r"C:\Espressif\frameworks\esp-idf\components\partition_table\check_sizes.py"),
    ]
    for c in candidates:
        if c.exists():
            return c

    return None


def run_command(key, title, cmd, cwd=None):
    banner = f"\n{'='*65}\n{title}\nCommand: {cmd}\n{'='*65}\n"
    log_write(banner)
    try:
        result = subprocess.run(
            cmd,
            shell=True,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            cwd=cwd
        )
        log_write(result.stdout)
        OUTPUT_STREAMS[key] = result.stdout
        return result.returncode
    except Exception as e:
        err_msg = f"Error executing command: {e}\n"
        log_write(err_msg)
        OUTPUT_STREAMS[key] = err_msg
        return 1


def find_active_build_dir(search_roots=None):
    """Find the most recently modified active build directory among candidate paths."""
    if search_roots is None:
        search_roots = [
            Path("."),
            Path("examples/H1OS"),
            Path("../examples/H1OS"),
            Path(__file__).resolve().parents[2] / "examples" / "H1OS",
            Path(__file__).resolve().parents[2],
        ]

    valid_candidates = []
    for root in search_roots:
        if not root.exists():
            continue
        for folder_name in CANDIDATE_DIRS:
            candidate = root / folder_name
            if candidate.is_dir():
                has_artifacts = (
                    (candidate / "compile_commands.json").exists()
                    or any(candidate.glob("*.map"))
                    or any(candidate.glob("*.bin"))
                )
                if has_artifacts:
                    try:
                        mtime = candidate.stat().st_mtime
                    except Exception:
                        mtime = 0
                    valid_candidates.append((mtime, candidate))

    if valid_candidates:
        valid_candidates.sort(key=lambda x: x[0], reverse=True)
        return valid_candidates[0][1]

    for root in search_roots:
        if not root.exists():
            continue
        for folder_name in CANDIDATE_DIRS:
            candidate = root / folder_name
            if candidate.is_dir():
                return candidate

    return None


def find_target_bin(build_dir_path):
    p = Path(build_dir_path)
    ignored = {"bootloader.bin", "partition-table.bin"}
    bins = [f for f in p.glob("*.bin") if f.name.lower() not in ignored]
    return bins[0] if bins else None


def find_target_map(build_dir_path):
    p = Path(build_dir_path)
    maps = [f for f in p.glob("*.map") if not f.name.startswith(".")]
    if not maps:
        return None
    maps.sort(key=lambda x: x.stat().st_mtime, reverse=True)
    return maps[0]


def format_bytes(b):
    if b >= 1024 * 1024:
        return f"{b / (1024*1024):.2f} MB ({b:,} B)"
    elif b >= 1024:
        return f"{b / 1024:.2f} KB ({b:,} B)"
    return f"{b:,} B"


def render_summary():
    part_str = OUTPUT_STREAMS.get("partition", "") + "\n" + OUTPUT_STREAMS.get("size", "")
    size_out = OUTPUT_STREAMS.get("size", "")
    comp_out = OUTPUT_STREAMS.get("components", "")

    summary_lines = []
    summary_lines.append(f"\n{'='*65}")
    summary_lines.append("                EXECUTIVE SIZE METRICS SUMMARY")
    summary_lines.append(f"{'='*65}\n")

    # 1. Partition Check Parsing
    part_match = re.search(
        r'binary size\s+(0x[0-9a-fA-F]+)\s+bytes\.\s+Smallest app partition is\s+(0x[0-9a-fA-F]+)\s+bytes\.\s+(0x[0-9a-fA-F]+)\s+bytes\s+\((\d+)%\)\s+free',
        part_str
    )

    if part_match:
        bin_sz = int(part_match.group(1), 16)
        part_sz = int(part_match.group(2), 16)
        free_sz = int(part_match.group(3), 16)
        free_pct = float(part_match.group(4))
        used_pct = 100.0 - free_pct

        summary_lines.append("1. APP PARTITION HEADROOM (Flash):")
        summary_lines.append(f"   * Partition Max : {format_bytes(part_sz)}")
        summary_lines.append(f"   * Firmware Used : {format_bytes(bin_sz)} [{used_pct:.1f}%]")
        summary_lines.append(f"   * Space Left    : {format_bytes(free_sz)} [{free_pct:.1f}% FREE]")
    else:
        summary_lines.append("1. APP PARTITION HEADROOM: [Data not detected]")

    # 2. Memory Section Usage Parsing
    summary_lines.append("\n2. CHIP INTERNAL RAM ALLOCATION:")
    ram_targets = ["DIRAM", "IRAM", "RTC FAST", "RTC SLOW"]
    for ram_sec in ram_targets:
        m = re.search(rf'│\s+{re.escape(ram_sec)}\s+│\s+(\d+)\s+│\s+([\d\.]+)\s+│\s+(\d+)\s+│\s+(\d+)\s+│', size_out)
        if m:
            used = int(m.group(1))
            pct = float(m.group(2))
            remain = int(m.group(3))
            total = int(m.group(4))
            if ram_sec == "IRAM" and remain == 0:
                status = "100.0% (overflow into DIRAM)"
            elif remain == 0:
                status = "CRITICAL (FULL)"
            else:
                status = f"{pct:.1f}% used"
            summary_lines.append(f"   * {ram_sec:<9}: {format_bytes(used):<20} / {format_bytes(total):<18} ({status}, {format_bytes(remain)} free)")

    # 3. Component Flash Attribution Top 6
    summary_lines.append("\n3. TOP FLASH CONSUMING ARCHIVES:")
    comp_matches = re.findall(r'│\s+([\w\.\-]+)\s+│\s+(\d+)\s+│', comp_out)
    if comp_matches:
        sorted_comps = sorted([(name, int(sz)) for name, sz in comp_matches], key=lambda x: x[1], reverse=True)
        valid_comps = [c for c in sorted_comps if c[0].endswith(".a")][:6]
        for name, sz in valid_comps:
            summary_lines.append(f"   * {name:<26}: {format_bytes(sz):<18}")

    summary_lines.append(f"\n{'='*65}\n")
    return "\n".join(summary_lines)


def parse_args():
    parser = argparse.ArgumentParser(
        description=f"ESP-IDF Binary Size & Memory Footprint Diagnostic Inspector v{VERSION}",
        epilog="Examples:\n"
               "  python inspect_size.py -B build_prod              (instant direct inspection)\n"
               "  python inspect_size.py -B build_prod --rebuild    (rebuild target then inspect)\n"
               "  python inspect_size.py -B examples/H1OS/build_prod\n"
               "  python inspect_size.py                            (auto-selects newest build)",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "-v", "--version",
        action="version",
        version=f"%(prog)s {VERSION}"
    )
    parser.add_argument(
        "-B", "--build-dir",
        dest="build_dir",
        default=None,
        help="Specify the target build directory path or name (e.g. build_prod, build_dev, build)"
    )
    parser.add_argument(
        "--rebuild",
        action="store_true",
        default=False,
        help="Force rebuilding firmware via 'idf.py build' before running size diagnostics"
    )
    parser.add_argument(
        "positional_build_dir",
        nargs="?",
        default=None,
        help="Optional positional build directory (fallback if -B is not provided)"
    )
    return parser.parse_args()


def main():
    global LOG_FILE
    args = parse_args()

    log_write(f"ESP-IDF Binary Size Diagnostic v{VERSION} (Windows)\n")
    log_write(f"Timestamp: {datetime.datetime.now()}\n")

    chosen_dir = args.build_dir or args.positional_build_dir
    build_dir_path = None

    if chosen_dir:
        p = Path(chosen_dir)
        if p.is_dir():
            build_dir_path = p
        else:
            candidates = [
                Path(".") / chosen_dir,
                Path("examples/H1OS") / chosen_dir,
                Path(__file__).resolve().parent.parent / "examples" / "H1OS" / chosen_dir
            ]
            for c in candidates:
                if c.is_dir():
                    build_dir_path = c
                    break

        if not build_dir_path:
            log_write(f"\n[ERROR] Specified build directory does not exist: {chosen_dir}\n")
            sys.exit(1)
    else:
        build_dir_path = find_active_build_dir()
        if not build_dir_path:
            log_write(f"\n[ERROR] None of the candidate build directories exist: {', '.join(CANDIDATE_DIRS)}\n")
            sys.exit(1)

    build_dir_path = build_dir_path.resolve()
    log_write(f"Target Build Directory: {build_dir_path}\n")

    project_root = build_dir_path.parent if (build_dir_path.parent / "CMakeLists.txt").exists() else Path(".")

    py_exe = get_idf_python_exe()
    log_write(f"Active Python: {py_exe}\n")

    idf_py = get_idf_cmd_prefix(py_exe)
    idf_flag = f"-B \"{build_dir_path}\""
    if project_root.resolve() != Path(".").resolve() and (project_root / "CMakeLists.txt").exists():
        idf_flag = f"-C \"{project_root}\" -B \"{build_dir_path}\""

    # 1. Optional explicit rebuild
    if args.rebuild:
        log_write("\n[REBUILD] Executing full firmware build as requested...\n")
        run_command("build", "0. REBUILD FIRMWARE", f"{idf_py} {idf_flag} build")

    # 2. Check for existing map and bin artifacts
    map_path = find_target_map(build_dir_path)
    bin_path = find_target_bin(build_dir_path)
    part_bin = build_dir_path / "partition_table" / "partition-table.bin"
    check_sizes_script = find_check_sizes_script()

    # Fast Mode: directly parse map and bin files without invoking Ninja
    if map_path and not args.rebuild:
        log_write("\n[MODE] Instant Direct Inspection (0% CPU, no Ninja dependency check)\n")

        # Step A: Partition table check
        if check_sizes_script and part_bin.exists() and bin_path:
            cmd = f'"{py_exe}" "{check_sizes_script}" --offset 0x8000 partition --type app "{part_bin}" "{bin_path}"'
            run_command("partition", "1. APP PARTITION HEADROOM CHECK", cmd)

        # Step B: Overall memory breakdown
        cmd_size = f'"{py_exe}" -m esp_idf_size "{map_path}"'
        run_command("size", "2. OVERALL MEMORY BREAKDOWN", cmd_size)

        # Step C: Per-component breakdown
        cmd_comp = f'"{py_exe}" -m esp_idf_size --archives "{map_path}"'
        run_command("components", "3. PER-COMPONENT BREAKDOWN", cmd_comp)

        # Step D: Per-file breakdown
        cmd_files = f'"{py_exe}" -m esp_idf_size --files "{map_path}"'
        run_command("files", "4. PER-FILE BREAKDOWN", cmd_files)

        # Step E: Binary image headers
        if bin_path:
            cmd_hdr = f'"{py_exe}" -m esptool image-info "{bin_path}"'
            run_command("headers", "5. BINARY IMAGE HEADERS (esptool)", cmd_hdr)
        else:
            log_write(f"\n[WARNING] No application binary found in {build_dir_path}\n")

    else:
        # Fallback Mode: idf.py commands
        log_write("\n[MODE] Full idf.py Execution Mode\n")
        run_command("size", "1. OVERALL MEMORY BREAKDOWN", f"{idf_py} {idf_flag} size")
        run_command("components", "2. PER-COMPONENT BREAKDOWN", f"{idf_py} {idf_flag} size-components")
        run_command("files", "3. PER-FILE BREAKDOWN", f"{idf_py} {idf_flag} size-files")

        bin_path = find_target_bin(build_dir_path)
        if bin_path:
            run_command("headers", "4. BINARY IMAGE HEADERS (esptool)", f'"{py_exe}" -m esptool image-info "{bin_path}"')
        else:
            log_write(f"\n[WARNING] No main application binary found in {build_dir_path}\n")

    # Render metrics dashboard
    summary_text = render_summary()
    log_write(summary_text)

    log_write(f"Diagnostics completed successfully.\nFull scroll-back log saved to: {LOG_FILE}\n")


if __name__ == "__main__":
    main()