# Asset Optimization & Metadata Harmonization Invariants

This rule defines mandatory repository hygiene policies for binary assets, images, audio files, and PDF documentation in **Humid1-OS**.

---

## 1. Slash Commands & Triggers

* **Trigger Commands:** `/shrink` or `/quant`
* **Trigger Workflow:** Executes automated image EXIF stamping, progressive JPEG optimization, and lossless PDF metadata stamping across assets and documentation.

---

## 2. Core Directives & Quality Safeguards

1. **Pre-Merge Invariant (Shrink Before Release):**
   - The agent **MUST** execute the image optimization and PDF metadata harmonization pass before any release commit, push, merge to `main`, or tag creation.
   - Prevents unoptimized digital camera photos, uncompressed textures, or bulky audio files from accumulating in permanent Git history.

2. **Zero-Degradation PDF Policy (Metadata Only):**
   - PDFs are an inherently compressed container format. The agent **MUST NEVER** re-compress, downsample, or modify embedded PDF object streams or images.
   - `firmware/tools/agent/optimize_pdf.py` operates **strictly losslessly** to harmonize document metadata (`Author`, `Creator`, `Producer`).

3. **All-Caps Attribution Invariant:**
   - All processed photographic images and PDF documents must be stamped with project ownership metadata:
     - **EXIF Tags** (`Artist`, `Copyright`, `Software`, `ImageDescription`): `HUMIDITRON/HUMADYNE_LABS`
     - **PDF Metadata** (`Author`, `Creator`, `Producer`): `HUMIDITRON/HUMADYNE_LABS`

---

## 3. Standard Execution Commands

### Full Repository Asset Shrink & Harmonization Pass:
```powershell
# 1. Quantize & stamp photos with HUMIDITRON/HUMADYNE_LABS EXIF
python firmware/tools/agent/optimize_images.py assets/

# 2. Losslessly harmonize PDF metadata
python firmware/tools/agent/optimize_pdf.py docs/
```
