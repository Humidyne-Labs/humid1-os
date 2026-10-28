#!/usr/bin/env python3
"""
H1OS Font String Width & Kerning Validation Engine.
Calculates exact sub-pixel string widths using LVGL 1bpp Montserrat font metrics and kerning classes,
validating against the 200x200 display's 188.0 px usable margin.
"""

import argparse
import csv
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
GENERATED_DIR = REPO_ROOT / "examples" / "H1OS" / "build_dev" / "esp-idf" / "h1os" / "generated_fonts"
OUTPUT_DIR = REPO_ROOT / "tools" / "fonts"
SCREEN_DIR = REPO_ROOT / "examples" / "H1OS" / "main" / "screens"
MAX_USABLE_WIDTH_PX = 188.0  # 200px width - 6px left margin - 6px right margin

class LVGLFontMetrics:
    def __init__(self, font_name: str, c_file: Path):
        self.font_name = font_name
        self.chars = {}            # char_code -> dict(glyph)
        self.glyph_to_code = {}    # glyph_id -> char_code
        self.kern_pairs = {}       # (code_left, code_right) -> kern_px
        self._load_from_c_file(c_file)

    def _load_from_c_file(self, c_path: Path):
        with open(c_path, "r", encoding="utf-8") as f:
            content = f.read()

        # Parse glyph_dsc
        marker = "static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {"
        start = content.find(marker)
        if start == -1:
            raise ValueError(f"glyph_dsc not found in {c_path}")
        end = content.find("};\n\n/*---------------------", start)
        if end == -1:
            end = content.find("};", start)
        glyph_block = content[start + len(marker):end]

        glyph_entries = []
        entry_pat = re.compile(r"\{\.bitmap_index\s*=\s*(\d+),\s*\.adv_w\s*=\s*(\d+),\s*\.box_w\s*=\s*(\d+),\s*\.box_h\s*=\s*(\d+),\s*\.ofs_x\s*=\s*(-?\d+),\s*\.ofs_y\s*=\s*(-?\d+)\}")
        for m in entry_pat.finditer(glyph_block):
            glyph_entries.append({
                "adv_w": int(m.group(2)),
                "box_w": int(m.group(3)),
                "box_h": int(m.group(4)),
                "ofs_x": int(m.group(5)),
                "ofs_y": int(m.group(6)),
            })

        # Parse unicode_list_1
        unicode_list = []
        u_marker = "static const uint16_t unicode_list_1[] = {"
        u_start = content.find(u_marker)
        if u_start != -1:
            u_end = content.find("};", u_start)
            u_block = content[u_start + len(u_marker):u_end]
            for val in re.findall(r"0x[0-9a-fA-F]+", u_block):
                unicode_list.append(int(val, 16))

        # Build cmap
        for code in range(32, 127):
            glyph_id = code - 32 + 1
            if glyph_id < len(glyph_entries):
                self.chars[code] = glyph_entries[glyph_id]
                self.glyph_to_code[glyph_id] = code

        for i, offset in enumerate(unicode_list):
            code = 176 + offset
            glyph_id = 96 + i
            if glyph_id < len(glyph_entries):
                self.chars[code] = glyph_entries[glyph_id]
                self.glyph_to_code[glyph_id] = code

        # Parse kerning classes
        self._parse_kerning(content)

    def _parse_kerning(self, content: str):
        # Extract kern_left_class_mapping
        left_match = re.search(r"static const uint8_t kern_left_class_mapping\[\]\s*=\s*\{([^}]+)\};", content)
        right_match = re.search(r"static const uint8_t kern_right_class_mapping\[\]\s*=\s*\{([^}]+)\};", content)
        vals_match = re.search(r"static const int8_t kern_class_values\[\]\s*=\s*\{([^}]+)\};", content)

        if not (left_match and right_match and vals_match):
            return

        left_map = [int(x.strip()) for x in left_match.group(1).split(",") if x.strip()]
        right_map = [int(x.strip()) for x in right_match.group(1).split(",") if x.strip()]
        kern_vals = [int(x.strip()) for x in vals_match.group(1).split(",") if x.strip()]

        if not left_map or not right_map or not kern_vals:
            return

        right_class_cnt = max(right_map) + 1
        left_class_cnt = max(left_map) + 1

        # Build map for all valid pairs
        for g_left, c_left in enumerate(left_map):
            if c_left == 0 or g_left not in self.glyph_to_code:
                continue
            code_l = self.glyph_to_code[g_left]
            for g_right, c_right in enumerate(right_map):
                if c_right == 0 or g_right not in self.glyph_to_code:
                    continue
                code_r = self.glyph_to_code[g_right]
                idx = (c_left * right_class_cnt) + c_right
                if idx < len(kern_vals):
                    kv = kern_vals[idx]
                    if kv != 0:
                        self.kern_pairs[(code_l, code_r)] = kv / 16.0

    def calc_width(self, text: str):
        total_px = 0.0
        details = []
        prev_code = None

        for ch in text:
            code = ord(ch)
            adv = self.chars[code]["adv_w"] / 16.0 if code in self.chars else 8.0
            kern = 0.0
            if prev_code is not None and (prev_code, code) in self.kern_pairs:
                kern = self.kern_pairs[(prev_code, code)]

            char_w = adv + kern
            total_px += char_w
            details.append({
                "char": ch,
                "code": code,
                "adv_px": adv,
                "kern_px": kern,
                "total_char_px": char_w
            })
            prev_code = code

        return round(total_px, 2), details

    def export_kerning_csv(self, out_path: Path):
        with open(out_path, "w", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(["left_char", "left_code", "right_char", "right_code", "kern_raw_16th", "kern_px"])
            for (cl, cr), kpx in sorted(self.kern_pairs.items()):
                try:
                    sl = chr(cl) if cl >= 32 else f"<U+{cl:04X}>"
                    sr = chr(cr) if cr >= 32 else f"<U+{cr:04X}>"
                except Exception:
                    sl = str(cl)
                    sr = str(cr)
                writer.writerow([sl, cl, sr, cr, int(kpx * 16), kpx])
        print(f"Exported {len(self.kern_pairs)} kerning pairs to {out_path}")


LVGL_SYMBOL_MAP = {
    "LV_SYMBOL_AUDIO": "\uf001",
    "LV_SYMBOL_VIDEO": "\uf008",
    "LV_SYMBOL_LIST": "\uf00b",
    "LV_SYMBOL_OK": "\uf00c",
    "LV_SYMBOL_CLOSE": "\uf00d",
    "LV_SYMBOL_POWER": "\uf011",
    "LV_SYMBOL_SETTINGS": "\uf013",
    "LV_SYMBOL_HOME": "\uf015",
    "LV_SYMBOL_DOWNLOAD": "\uf019",
    "LV_SYMBOL_DRIVE": "\uf01c",
    "LV_SYMBOL_REFRESH": "\uf021",
    "LV_SYMBOL_MUTE": "\uf026",
    "LV_SYMBOL_VOLUME_MID": "\uf027",
    "LV_SYMBOL_VOLUME_MAX": "\uf028",
    "LV_SYMBOL_IMAGE": "\uf03e",
    "LV_SYMBOL_TINT": "\uf043",
    "LV_SYMBOL_PREV": "\uf048",
    "LV_SYMBOL_PLAY": "\uf04b",
    "LV_SYMBOL_PAUSE": "\uf04c",
    "LV_SYMBOL_STOP": "\uf04d",
    "LV_SYMBOL_NEXT": "\uf051",
    "LV_SYMBOL_PLUS": "\uf067",
    "LV_SYMBOL_MINUS": "\uf068",
    "LV_SYMBOL_WARNING": "\uf071",
    "LV_SYMBOL_CHARGE": "\uf0e7",
    "LV_SYMBOL_BELL": "\uf0f3",
    "LV_SYMBOL_WIFI": "\uf1eb",
    "LV_SYMBOL_BATTERY_FULL": "\uf240",
    "LV_SYMBOL_BATTERY_3": "\uf241",
    "LV_SYMBOL_BATTERY_2": "\uf242",
    "LV_SYMBOL_BATTERY_1": "\uf243",
    "LV_SYMBOL_BATTERY_EMPTY": "\uf244",
    "LV_SYMBOL_USB": "\uf287",
    "LV_SYMBOL_BLUETOOTH": "\uf293",
    "LV_SYMBOL_SD_CARD": "\uf7c2",
}

def audit_screen_files(font14: LVGLFontMetrics):
    print("\n" + "=" * 78)
    print("SCREEN SOURCE FILE AUTOMATED STRING WIDTH AUDIT (Threshold: <= 188.0 px)")
    print("=" * 78)

    c_files = sorted(SCREEN_DIR.glob("screen_*.c"))
    total_checked = 0
    overflows = 0

    str_pat = re.compile(r'lv_label_set_text(?:_fmt)?\s*\([^,]+,\s*"([^"\\]*(?:\\.[^"\\]*)*)"', re.DOTALL)

    for c_file in c_files:
        with open(c_file, "r", encoding="utf-8") as f:
            code = f.read()

        # Handle LVGL symbol concatenation (e.g. LV_SYMBOL_TINT " text" -> "\uf043 text")
        for sym_name, sym_char in LVGL_SYMBOL_MAP.items():
            code = re.sub(rf'\b{sym_name}\s*"', f'"{sym_char}', code)
            code = re.sub(rf'"\s*{sym_name}\b', f'{sym_char}"', code)
            code = re.sub(rf'\b{sym_name}\b', f'"{sym_char}"', code)

        matches = str_pat.findall(code)
        for raw_str in matches:
            # Replace format specifiers with worst-case realistic text
            sim_str = raw_str
            sim_str = sim_str.replace("%lu", "625").replace("%u", "100").replace("%d", "100")
            sim_str = sim_str.replace("%.2f", "4.00").replace("%.1f", "72.5").replace("%.0f", "120")
            sim_str = sim_str.replace("%+.1f", "+0.0").replace("%3lu", "240")
            sim_str = sim_str.replace("%02X", "00").replace("%X", "00").replace("%%", "%")
            sim_str = sim_str.replace("%s", "CHARGED")

            # Clean escape sequences
            sim_str = sim_str.replace("\\n", " ").replace('\\"', '"')

            w, _ = font14.calc_width(sim_str)
            total_checked += 1
            status = "PASS" if w <= MAX_USABLE_WIDTH_PX else "FAIL"

            if status == "FAIL":
                overflows += 1
                print(f"[{status:4s}] {w:6.2f} px > 188 px | {c_file.name:24s} | Template: '{raw_str}' -> Sim: '{sim_str}'")

    if overflows == 0:
        print(f"\nAUDIT VERDICT: ALL {total_checked} STRINGS PASS! Zero screen boundaries breached.")
    else:
        print(f"\nAUDIT VERDICT: {overflows} of {total_checked} STRINGS EXCEED 188px!")


def main():
    parser = argparse.ArgumentParser(description="H1OS Font Metric and String Width Checker")
    parser.add_argument("string", nargs="?", help="String to calculate width for")
    parser.add_argument("--size", type=int, default=14, choices=[14, 16, 18, 20, 24, 32], help="Font size to evaluate (default: 14)")
    parser.add_argument("--audit-screens", action="store_true", help="Audit all screen string templates")
    args = parser.parse_args()

    font_path = GENERATED_DIR / f"h1os_font_montserrat_{args.size}_1bpp.c"
    if not font_path.exists():
        print(f"Error: {font_path} not found.")
        sys.exit(1)

    font = LVGLFontMetrics(f"Montserrat-{args.size}", font_path)

    if args.audit_screens:
        audit_screen_files(font)
        return

    if args.string:
        w, details = font.calc_width(args.string)
        status = "PASS" if w <= MAX_USABLE_WIDTH_PX else "OVERFLOW"
        print(f"\nFont:   Montserrat-{args.size} 1bpp")
        print(f"String: '{args.string}'")
        print(f"Width:  {w} px (Max: {MAX_USABLE_WIDTH_PX} px) -> [{status}]")
        print("-" * 50)
        print(f"{'Char':<6} {'Adv(px)':<10} {'Kern(px)':<10} {'Total(px)':<10}")
        print("-" * 50)
        for d in details:
            print(f"{d['char']:<6} {d['adv_px']:<10.2f} {d['kern_px']:<10.2f} {d['total_char_px']:<10.2f}")
    else:
        # Default behavior: run screen audit
        audit_screen_files(font)

if __name__ == "__main__":
    main()
