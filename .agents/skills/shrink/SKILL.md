---
name: shrink
description: Quantizes photographic bench test images with all-caps HUMIDITRON/HUMADYNE_LABS EXIF attribution and losslessly harmonizes PDF metadata without modifying compressed PDF streams for Humid1-OS. Triggered by /shrink or /quant.
---

# /shrink & /quant: Repository Asset Optimizer & Metadata Harmonizer

This skill automates the compression and EXIF attribution of photographic evidence and losslessly stamps author metadata on PDF documentation for **Humid1-OS**.

---

## 1. Quick Reference: Commands

| Command | Purpose | Tools Involved |
| :--- | :--- | :--- |
| **`/shrink` or `/quant`** | Run full repository image & PDF harmonization pass | `optimize_images.py`, `optimize_pdf.py` |
| **`python firmware/tools/agent/optimize_images.py assets/`** | Progressive JPEG encode & EXIF tagging | Pillow, EXIF tags 0x013B/0x8298/0x0131/0x010E |
| **`python firmware/tools/agent/optimize_pdf.py docs/`** | Losslessly stamp author/creator metadata on PDFs | PyMuPDF (`fitz`), zero stream modification |

---

## 2. Invariants & Rules

1. **Lossless PDF Invariant:** PDFs are already a compressed format. Never downsample raster images or re-compress page streams inside PDFs. `optimize_pdf.py` touches metadata only.
2. **Metadata Invariant:** EXIF tags (`Artist`, `Copyright`, `Software`, `ImageDescription`) and PDF metadata (`Author`, `Creator`, `Producer`) MUST be set to `HUMIDITRON/HUMADYNE_LABS`.
3. **Pre-Merge Requirement:** Must be run before committing, pushing, merging into `main`, or releasing new version tags.

---

## 3. Automated Execution

Run the complete pipeline from the repository root:

```powershell
# Step 1: Optimize and stamp asset photos
python firmware/tools/agent/optimize_images.py assets/

# Step 2: Losslessly harmonize PDF metadata
python firmware/tools/agent/optimize_pdf.py docs/
```
