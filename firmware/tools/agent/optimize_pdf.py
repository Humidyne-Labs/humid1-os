#!/usr/bin/env python3
"""
optimize_pdf.py
===============
Lossless PDF Metadata Harmonizer for H1OS.

Features:
- STRICTLY LOSSLESS: Zero stream modification, zero raster downsampling.
- Preserves 100% of original vector schematics, embedded images, and fonts.
- Sets PDF author, creator, and producer metadata to "HUMIDITRON/HUMADYNE_LABS".

Usage:
  python tools/agent/optimize_pdf.py path/to/datasheet.pdf
  python tools/agent/optimize_pdf.py docs/hardware_docs/datasheets/
"""

import os
import sys
import argparse
from pathlib import Path
import fitz  # PyMuPDF

METADATA_CREATOR = "HUMIDITRON/HUMADYNE_LABS"

def update_pdf_metadata(pdf_path: Path) -> bool:
    """Losslessly update PDF author, creator, and producer metadata in place."""
    try:
        doc = fitz.open(pdf_path)
    except Exception as e:
        print(f"[!] Failed to open {pdf_path}: {e}")
        return False

    try:
        meta = doc.metadata or {}
        # Check if already tagged
        if (
            meta.get("author") == METADATA_CREATOR
            and meta.get("creator") == METADATA_CREATOR
            and meta.get("producer") == METADATA_CREATOR
        ):
            doc.close()
            return False

        meta["author"] = METADATA_CREATOR
        meta["creator"] = METADATA_CREATOR
        meta["producer"] = METADATA_CREATOR
        doc.set_metadata(meta)

        # Write to temporary file losslessly without recompressing streams
        tmp_out = pdf_path.with_suffix(".tmp.pdf")
        doc.save(str(tmp_out))
        doc.close()

        tmp_out.replace(pdf_path)
        return True

    except Exception as e:
        print(f"[!] Error updating metadata in {pdf_path}: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(description="H1OS Lossless PDF Metadata Harmonizer")
    parser.add_argument("target", nargs="?", default="docs/hardware_docs/datasheets", help="PDF file or directory")
    args = parser.parse_args()

    target_path = Path(args.target).resolve()
    if not target_path.exists():
        print(f"[!] Target not found: {target_path}")
        sys.exit(1)

    files = [target_path] if target_path.is_file() else list(target_path.rglob("*.pdf"))
    print(f"[*] Scanning {len(files)} PDF file(s) for metadata harmonization...")

    updated_count = 0
    for f in files:
        if update_pdf_metadata(f):
            updated_count += 1
            print(f"  ✓ {f.name}: metadata stamped with '{METADATA_CREATOR}'")

    print(f"\n[+] PDF metadata pass complete. Updated {updated_count} / {len(files)} file(s).")

if __name__ == "__main__":
    main()
