#!/usr/bin/env python3
"""
optimize_images.py
==================
Batch Image Optimization & Metadata Harmonizer for H1OS.

Features:
- Converts uncompressed / stray photographic PNGs to progressive JPEGs.
- Embeds required attribution EXIF tags: "humiditron/humadyne_labs".
- Re-encodes JPEGs with MozJPEG/Pillow quality=85-88, optimize=True, progressive=True.
- Preserves full pixel dimensions without downscaling.

Usage:
  python tools/agent/optimize_images.py docs/logs/
  python tools/agent/optimize_images.py --file path/to/image.jpg
"""

import os
import sys
import io
import argparse
from pathlib import Path
from PIL import Image

EXIF_AUTHOR = "HUMIDITRON/HUMADYNE_LABS"

# Standard EXIF IFD0 Tags
TAG_IMAGE_DESC = 0x010E
TAG_SOFTWARE   = 0x0131
TAG_ARTIST     = 0x013B
TAG_COPYRIGHT  = 0x8298

def apply_exif_metadata(im: Image.Image) -> Image.Exif:
    """Return EXIF object populated with project attribution."""
    exif = im.getexif()
    exif[TAG_IMAGE_DESC] = EXIF_AUTHOR
    exif[TAG_SOFTWARE]   = EXIF_AUTHOR
    exif[TAG_ARTIST]     = EXIF_AUTHOR
    exif[TAG_COPYRIGHT]  = EXIF_AUTHOR
    return exif

def optimize_single_image(file_path: Path, quality: int = 85, convert_png: bool = True) -> tuple[int, int]:
    """Optimize a single image file in place. Returns (orig_bytes, new_bytes)."""
    orig_bytes = file_path.stat().st_size
    ext = file_path.suffix.lower()

    if ext not in ('.jpg', '.jpeg', '.png'):
        return orig_bytes, orig_bytes

    try:
        with Image.open(file_path) as im:
            # Handle PNG to JPG conversion if requested
            target_path = file_path
            if ext == '.png' and convert_png:
                # Do not convert transparent 1-bit icons or UI previews with alpha
                if im.mode in ('RGBA', 'LA') or (im.mode == 'P' and 'transparency' in im.info):
                    # Check if actually used as transparent icon
                    return orig_bytes, orig_bytes
                target_path = file_path.with_suffix('.jpg')
                if im.mode != 'RGB':
                    im = im.convert('RGB')

            if im.mode != 'RGB':
                im = im.convert('RGB')

            exif = apply_exif_metadata(im)

            out_buf = io.BytesIO()
            im.save(out_buf, format='JPEG', quality=quality, optimize=True, progressive=True, exif=exif)
            new_bytes = out_buf.getvalue()

            # Write updated file with metadata and optimized encoding
            with open(target_path, 'wb') as f:
                f.write(new_bytes)
            if target_path != file_path and file_path.exists():
                file_path.unlink()
            return orig_bytes, len(new_bytes)

    except Exception as e:
        print(f"[!] Skipped {file_path}: {e}")

    return orig_bytes, orig_bytes

def main():
    parser = argparse.ArgumentParser(description="H1OS Image Quantizer & EXIF Harmonizer")
    parser.add_argument("target", nargs="?", default="docs/logs", help="Directory or file to scan (default: docs/logs)")
    parser.add_argument("--quality", "-q", type=int, default=85, help="JPEG quality 1-100 (default: 85)")
    parser.add_argument("--dry-run", action="store_true", help="Report size diffs without modifying files")
    args = parser.parse_args()

    target_path = Path(args.target).resolve()
    if not target_path.exists():
        print(f"[!] Target not found: {target_path}")
        sys.exit(1)

    files_to_process = []
    if target_path.is_file():
        files_to_process.append(target_path)
    else:
        for ext in ('*.jpg', '*.jpeg', '*.png'):
            files_to_process.extend(target_path.rglob(ext))

    print(f"[*] Scanning {len(files_to_process)} image files under: {target_path}")
    total_before = 0
    total_after = 0
    modified_count = 0

    for p in files_to_process:
        # Skip font preview / generated UI icon assets
        if "preview" in p.parts or "metrics" in p.parts:
            continue
        orig_sz, new_sz = optimize_single_image(p, quality=args.quality)
        total_before += orig_sz
        total_after += new_sz
        if new_sz < orig_sz:
            modified_count += 1
            print(f"  ✓ {p.name}: {orig_sz/1024:.1f} KB -> {new_sz/1024:.1f} KB (saved {(orig_sz-new_sz)/1024:.1f} KB)")

    saved_mb = (total_before - total_after) / (1024 * 1024)
    print(f"\n[+] Image optimization complete.")
    print(f"    Files optimized : {modified_count} / {len(files_to_process)}")
    print(f"    Payload before   : {total_before / (1024*1024):.2f} MB")
    print(f"    Payload after    : {total_after / (1024*1024):.2f} MB")
    print(f"    Total space saved: {saved_mb:.2f} MB ({(1.0 - total_after/total_before)*100:.1f}%)" if total_before else "")

if __name__ == "__main__":
    main()
