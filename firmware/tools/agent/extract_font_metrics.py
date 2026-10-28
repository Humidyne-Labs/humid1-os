#!/usr/bin/env python3
"""
H1OS Font Metrics & Kerning Extractor.
Parses all generated LVGL 1bpp font C files (sizes 14 to 32) and exports:
1. char_widths_montserrat_<size>.csv (advance widths, bounding boxes, offsets)
2. kern_pairs_montserrat_<size>.csv (class-based kerning pair adjustments)
"""

import csv
import re
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
GENERATED_DIR = REPO_ROOT / "examples" / "H1OS" / "build_dev" / "esp-idf" / "h1os" / "generated_fonts"
OUTPUT_DIR = REPO_ROOT / "tools" / "fonts" / "metrics"

FONT_SIZES = [14, 16, 18, 20, 24, 32]


def parse_font_c_file(c_path: Path):
    with open(c_path, "r", encoding="utf-8") as f:
        content = f.read()

    # 1. Parse glyph_dsc block
    marker = "static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {"
    start = content.find(marker)
    if start == -1:
        raise ValueError(f"glyph_dsc table not found in {c_path}")
    end = content.find("};\n\n/*---------------------", start)
    if end == -1:
        end = content.find("};", start)
    glyph_block = content[start + len(marker):end]

    glyph_entries = []
    entry_pat = re.compile(
        r"\{\.bitmap_index\s*=\s*(\d+),\s*\.adv_w\s*=\s*(\d+),\s*\.box_w\s*=\s*(\d+),\s*\.box_h\s*=\s*(\d+),\s*\.ofs_x\s*=\s*(-?\d+),\s*\.ofs_y\s*=\s*(-?\d+)\}"
    )
    for m in entry_pat.finditer(glyph_block):
        glyph_entries.append({
            "bitmap_index": int(m.group(1)),
            "adv_w": int(m.group(2)),
            "box_w": int(m.group(3)),
            "box_h": int(m.group(4)),
            "ofs_x": int(m.group(5)),
            "ofs_y": int(m.group(6)),
        })

    # 2. Parse unicode_list_1 (unicode offsets beyond basic ASCII)
    unicode_list = []
    u_marker = "static const uint16_t unicode_list_1[] = {"
    u_start = content.find(u_marker)
    if u_start != -1:
        u_end = content.find("};", u_start)
        u_block = content[u_start + len(u_marker):u_end]
        for val in re.findall(r"0x[0-9a-fA-F]+", u_block):
            unicode_list.append(int(val, 16))

    # 3. Map glyph_ids to unicode character codes
    chars = {}
    glyph_to_code = {}

    # cmap 0: range 32..126 -> glyph_id 1..95
    for code in range(32, 127):
        glyph_id = code - 32 + 1
        if glyph_id < len(glyph_entries):
            chars[code] = glyph_entries[glyph_id]
            glyph_to_code[glyph_id] = code

    # cmap 1: range_start = 176 (offset mapping)
    for i, offset in enumerate(unicode_list):
        code = 176 + offset
        glyph_id = 96 + i
        if glyph_id < len(glyph_entries):
            chars[code] = glyph_entries[glyph_id]
            glyph_to_code[glyph_id] = code

    # 4. Parse kerning classes
    kern_pairs = {}
    left_match = re.search(r"static const uint8_t kern_left_class_mapping\[\]\s*=\s*\{([^}]+)\};", content)
    right_match = re.search(r"static const uint8_t kern_right_class_mapping\[\]\s*=\s*\{([^}]+)\};", content)
    vals_match = re.search(r"static const int8_t kern_class_values\[\]\s*=\s*\{([^}]+)\};", content)

    if left_match and right_match and vals_match:
        left_map = [int(x.strip()) for x in left_match.group(1).split(",") if x.strip()]
        right_map = [int(x.strip()) for x in right_match.group(1).split(",") if x.strip()]
        kern_vals = [int(x.strip()) for x in vals_match.group(1).split(",") if x.strip()]

        if left_map and right_map and kern_vals:
            right_class_cnt = max(right_map) + 1
            for g_left, c_left in enumerate(left_map):
                if c_left == 0 or g_left not in glyph_to_code:
                    continue
                code_l = glyph_to_code[g_left]
                for g_right, c_right in enumerate(right_map):
                    if c_right == 0 or g_right not in glyph_to_code:
                        continue
                    code_r = glyph_to_code[g_right]
                    idx = (c_left * right_class_cnt) + c_right
                    if idx < len(kern_vals):
                        kv = kern_vals[idx]
                        if kv != 0:
                            kern_pairs[(code_l, code_r)] = kv

    return chars, kern_pairs


def export_char_csv(chars: dict, out_csv: Path, font_size: int):
    fieldnames = ["char", "unicode_hex", "unicode_dec", "adv_w_raw", "width_px", "box_w", "box_h", "ofs_x", "ofs_y"]
    with open(out_csv, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for code, g in sorted(chars.items()):
            try:
                ch_repr = chr(code)
                if code == 32:
                    ch_repr = "<SPACE>"
                elif code < 32 or (126 < code < 160):
                    ch_repr = f"<U+{code:04X}>"
            except Exception:
                ch_repr = f"<U+{code:04X}>"

            writer.writerow({
                "char": ch_repr,
                "unicode_hex": f"0x{code:04X}",
                "unicode_dec": code,
                "adv_w_raw": g["adv_w"],
                "width_px": round(g["adv_w"] / 16.0, 4),
                "box_w": g["box_w"],
                "box_h": g["box_h"],
                "ofs_x": g["ofs_x"],
                "ofs_y": g["ofs_y"],
            })
    print(f"Exported {len(chars):3d} glyph metrics to {out_csv.name}")


def export_kern_csv(kern_pairs: dict, out_csv: Path, font_size: int):
    fieldnames = ["left_char", "left_code", "right_char", "right_code", "kern_raw_16th", "kern_px"]
    with open(out_csv, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for (cl, cr), kv in sorted(kern_pairs.items()):
            try:
                sl = chr(cl) if cl >= 32 else f"<U+{cl:04X}>"
            except Exception:
                sl = f"<U+{cl:04X}>"
            try:
                sr = chr(cr) if cr >= 32 else f"<U+{cr:04X}>"
            except Exception:
                sr = f"<U+{cr:04X}>"

            writer.writerow({
                "left_char": sl,
                "left_code": f"0x{cl:04X}",
                "right_char": sr,
                "right_code": f"0x{cr:04X}",
                "kern_raw_16th": kv,
                "kern_px": round(kv / 16.0, 4),
            })
    print(f"Exported {len(kern_pairs):4d} kerning pairs to {out_csv.name}")


def main():
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    print("=" * 70)
    print("EXTRACTING METRICS & KERNING FOR ALL FONTS (14..32)")
    print("=" * 70)

    for sz in FONT_SIZES:
        font_file = GENERATED_DIR / f"h1os_font_montserrat_{sz}_1bpp.c"
        if not font_file.exists():
            print(f"Warning: {font_file.name} not found, skipping.")
            continue

        chars, kern_pairs = parse_font_c_file(font_file)
        char_csv = OUTPUT_DIR / f"char_widths_montserrat_{sz}.csv"
        kern_csv = OUTPUT_DIR / f"kern_pairs_montserrat_{sz}.csv"

        export_char_csv(chars, char_csv, sz)
        export_kern_csv(kern_pairs, kern_csv, sz)

    print("=" * 70)
    print("ALL FONT METRICS SUCCESSFULLY GENERATED!")
    print("=" * 70)


if __name__ == "__main__":
    main()
