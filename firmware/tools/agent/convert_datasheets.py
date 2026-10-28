import os
import sys
import fnmatch
from pathlib import Path

# Silence Hugging Face symlink warnings on Windows
os.environ["HF_HUB_DISABLE_SYMLINKS_WARNING"] = "1"

from docling.datamodel.base_models import InputFormat
from docling.datamodel.pipeline_options import PdfPipelineOptions
from docling.document_converter import DocumentConverter, PdfFormatOption

# Resolve directory anchors
REPO_ROOT = Path(__file__).resolve().parents[2]
PDF_DIR = REPO_ROOT / "docs" / "hardware_docs"
OUTPUT_DIR = REPO_ROOT / "docs" / "hardware_docs" / "markdown"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

# Allow manual override: python .\tools\convert_datasheets.py --force
FORCE_REBUILD = "--force" in sys.argv


def load_ignore_patterns(root_dir: Path) -> list[str]:
    """Load exclusion patterns from .doclingignore or .docklingignore."""
    patterns = []
    ignore_files = [root_dir / ".doclingignore", root_dir / ".docklingignore"]

    for ignore_file in ignore_files:
        if ignore_file.exists():
            print(f"Loaded ignore rules from: {ignore_file.name}")
            with open(ignore_file, "r", encoding="utf-8") as f:
                for line in f:
                    line = line.strip()
                    if line and not line.startswith("#"):
                        patterns.append(line.replace("\\", "/"))
            break
    else:
        print("No .doclingignore file found. Processing all PDFs.")

    return patterns


def is_ignored(file_path: Path, patterns: list[str], root_dir: Path) -> bool:
    """Check if a file matches any pattern against its name or relative path."""
    rel_path = file_path.relative_to(root_dir).as_posix()
    file_name = file_path.name

    for pattern in patterns:
        pattern = pattern.rstrip("/")
        if fnmatch.fnmatch(file_name, pattern):
            return True
        if fnmatch.fnmatch(rel_path, pattern) or fnmatch.fnmatch(rel_path, f"{pattern}/*"):
            return True
        if pattern in rel_path.split("/"):
            return True
    return False


def needs_conversion(pdf_path: Path, md_path: Path) -> bool:
    """Return True if Markdown is missing or PDF has been updated since conversion."""
    if FORCE_REBUILD or not md_path.exists():
        return True
    # If the source PDF was modified after the existing .md was created, re-convert
    return pdf_path.stat().st_mtime > md_path.stat().st_mtime


# Configure Docling: Disable slow OCR, keep table structure model
pipeline_options = PdfPipelineOptions()
pipeline_options.do_ocr = False
pipeline_options.do_table_structure = True

converter = DocumentConverter(
    format_options={
        InputFormat.PDF: PdfFormatOption(pipeline_options=pipeline_options)
    }
)

ignore_patterns = load_ignore_patterns(REPO_ROOT)
pdf_files = list(PDF_DIR.rglob("*.pdf"))

converted_count = 0
skipped_ignore_count = 0
skipped_cached_count = 0

print(f"Discovered {len(pdf_files)} PDF(s) in {PDF_DIR.relative_to(REPO_ROOT)}.\n")

for pdf_path in pdf_files:
    rel_pdf = pdf_path.relative_to(REPO_ROOT).as_posix()
    out_file = OUTPUT_DIR / f"{pdf_path.stem}.md"

    # 1. Skip if ignored
    if is_ignored(pdf_path, ignore_patterns, REPO_ROOT):
        print(f"[IGNORED] {rel_pdf}")
        skipped_ignore_count += 1
        continue

    # 2. Skip if already converted and up to date
    if not needs_conversion(pdf_path, out_file):
        print(f"[UP-TO-DATE] {out_file.name}")
        skipped_cached_count += 1
        continue

    # 3. Convert new or modified files
    print(f"[CONVERTING] {rel_pdf}...")
    try:
        result = converter.convert(str(pdf_path))
        md_text = result.document.export_to_markdown()

        out_file.write_text(md_text, encoding="utf-8")
        print(f" -> Saved: {out_file.relative_to(REPO_ROOT)}\n")
        converted_count += 1
    except Exception as e:
        print(f" -> [ERROR] Failed to convert {pdf_path.name}: {e}\n")

print(
    f"Done. Converted: {converted_count} | Up-to-date: {skipped_cached_count} | Ignored: {skipped_ignore_count}"
)