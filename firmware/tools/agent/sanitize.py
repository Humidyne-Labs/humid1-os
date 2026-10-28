#!/usr/bin/env python3
"""
H1OS Repository Identity & Name Sanitizer
==========================================
Scans and sanitizes personal names, usernames, and workstation paths across
the repository codebase, documentation, compressed archives, document metadata,
and Git identity records.

Features:
  - Recursive search & replace across source, headers, docs, and configs.
  - Case-agnostic pattern matching ('target', 'Target', 'TARGET') with case-sensitive replacement.
  - Strict whole-word boundary protection to prevent accidental substring corruption.
  - Deep archive inspection & sanitization (.zip, .tar, .tgz, .whl) including file names, contents,
    and Tar member ownership headers (uname/gname).
  - Git commit history & git config author verification.
  - PDF document metadata inspection (/Author, /Creator, /Producer).
  - Configurable, git-ignored name file ('sanitize_names.txt' or '.sanitize_names').
"""

import argparse
import io
import os
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import zipfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple


# ==============================================================================
# Repository Root Discovery
# ==============================================================================

def find_repo_root(start_dir: Optional[Path] = None) -> Path:
    """Climb up from start_dir to find repository root (.git or components/h1os)."""
    current = (start_dir or Path(__file__)).resolve().parent
    for _ in range(8):
        if (current / ".git").exists() or (current / "components" / "h1os").exists():
            return current
        if current.parent == current:
            break
        current = current.parent
    return Path(__file__).resolve().parents[2]


REPO_ROOT = find_repo_root()


# ==============================================================================
# Configuration & Name Rules Parser
# ==============================================================================

@dataclass
class NameRule:
    target: str
    replacement: str
    pattern: re.Pattern
    is_path_or_literal: bool = False


class NameConfig:
    """Loads and compiles target names and ignore rules from configuration file."""

    DEFAULT_CONFIG_NAMES = [
        "tools/agent/sanitize_names.txt",
        "tools/agent/.sanitize_names",
        ".sanitize_names",
        "sanitize_names.txt",
    ]

    def __init__(self, config_path: Optional[Path] = None, default_replacement: str = "Humiditron"):
        self.config_path: Optional[Path] = None
        self.default_replacement: str = default_replacement
        self.rules: List[NameRule] = []
        self.ignore_words: Set[str] = set()
        self.raw_targets: List[str] = []

        self._discover_and_load(config_path)

    def _discover_and_load(self, explicit_path: Optional[Path]):
        target_file: Optional[Path] = None
        if explicit_path and explicit_path.exists():
            target_file = explicit_path
        else:
            for cand_rel in self.DEFAULT_CONFIG_NAMES:
                cand = REPO_ROOT / cand_rel
                if cand.exists():
                    target_file = cand
                    break

        if target_file and target_file.exists():
            self.config_path = target_file
            self._parse_file(target_file)
        else:
            # Fallback default rules from local environment if no config file exists
            env_user = os.environ.get("USERNAME") or os.environ.get("USER")
            self.raw_targets = [env_user] if env_user else []
            self._compile_rules()

    def _parse_file(self, file_path: Path):
        with open(file_path, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#") or line.startswith("//"):
                    continue

                # Check for replacement directive: REPLACEMENT = <Name>
                rep_match = re.match(r'^(?:DEFAULT_)?REPLACEMENT\s*[:=]\s*(.+)$', line, re.IGNORECASE)
                if rep_match:
                    self.default_replacement = rep_match.group(1).strip()
                    continue

                # Check for ignore directive: IGNORE = <word>
                ign_match = re.match(r'^(?:IGNORE|WHITELIST)\s*[:=]\s*(.+)$', line, re.IGNORECASE)
                if ign_match:
                    word = ign_match.group(1).strip().lower()
                    if word:
                        self.ignore_words.add(word)
                    continue

                # Check for explicit mapping: <target> => <replacement>
                if "=>" in line or "->" in line:
                    sep = "=>" if "=>" in line else "->"
                    parts = line.split(sep, 1)
                    target = parts[0].strip()
                    rep = parts[1].strip()
                    if target and rep:
                        self._add_rule(target, rep)
                    continue

                # Plain target word / name
                self.raw_targets.append(line)

        self._compile_rules()

    def _add_rule(self, target: str, replacement: str):
        if not target or len(target) < 2:
            return

        is_word = bool(re.match(r'^[A-Za-z0-9_]+$', target))
        if is_word:
            # Whole word boundary matching (case-insensitive search)
            pattern = re.compile(rf'\b{re.escape(target)}\b', re.IGNORECASE)
            self.rules.append(NameRule(target=target, replacement=replacement, pattern=pattern, is_path_or_literal=False))
        else:
            # Literal / path matching (case-insensitive)
            pattern = re.compile(re.escape(target), re.IGNORECASE)
            self.rules.append(NameRule(target=target, replacement=replacement, pattern=pattern, is_path_or_literal=True))

    def _compile_rules(self):
        for target in self.raw_targets:
            # Skip if target is explicitly in ignore list
            if target.lower() in self.ignore_words:
                continue
            self._add_rule(target, self.default_replacement)

        # Sort rules so longest multi-word targets are evaluated before single words
        self.rules.sort(key=lambda r: len(r.target), reverse=True)

    def is_ignored(self, word: str) -> bool:
        return word.lower() in self.ignore_words


# ==============================================================================
# Violation Models
# ==============================================================================

@dataclass
class TextMatch:
    line_number: int
    matched_text: str
    replacement: str
    preview: str


@dataclass
class FileViolation:
    file_path: Path
    relative_path: str
    matches: List[TextMatch] = field(default_factory=list)


@dataclass
class ArchiveViolation:
    archive_path: Path
    relative_path: str
    member_name: str
    violation_type: str  # 'name', 'content', 'owner_header'
    details: str


@dataclass
class GitViolation:
    commit_hash: str
    author: str
    email: str
    subject: str


# ==============================================================================
# Scanner Engine
# ==============================================================================

class SanitizerEngine:
    """Core scanning and in-place sanitization engine."""

    IGNORED_DIRS = {
        ".git",
        "build",
        "build_dev",
        "build_prod",
        "dev_build",
        "prod_build",
        ".cache",
        ".pytest_cache",
        "__pycache__",
        ".espressif",
        "venv",
        ".venv",
        "node_modules",
        "dist",
        "release",
        "releases",
        "managed_components",
    }

    IGNORED_FILES = {
        "compile_commands.json",
        "sanitize.py",
        "sanitize_names.txt",
        "sanitize_names.example.txt",
        ".sanitize_names",
    }

    BINARY_EXTENSIONS = {
        ".png", ".jpg", ".jpeg", ".gif", ".ico", ".bmp", ".webp",
        ".ttf", ".otf", ".woff", ".woff2", ".eot",
        ".bin", ".elf", ".map", ".a", ".o", ".obj", ".so", ".dll", ".dylib",
        ".exe", ".hex", ".pcap",
    }

    ARCHIVE_EXTENSIONS = {
        ".zip", ".tar", ".gz", ".tgz", ".bz2", ".whl"
    }

    def __init__(self, config: NameConfig, root_dir: Path):
        self.config = config
        self.root_dir = root_dir

        self.text_violations: List[FileViolation] = []
        self.archive_violations: List[ArchiveViolation] = []
        self.git_violations: List[GitViolation] = []
        self.pdf_violations: List[Tuple[Path, str]] = []

        self.total_scanned_files = 0
        self.total_scanned_archives = 0

    # --------------------------------------------------------------------------
    # Text File Scanning & Fixing
    # --------------------------------------------------------------------------

    def is_binary_file(self, path: Path) -> bool:
        if path.suffix.lower() in self.BINARY_EXTENSIONS:
            return True
        try:
            with open(path, "rb") as f:
                chunk = f.read(8192)
                if b"\0" in chunk:
                    return True
        except Exception:
            return True
        return False

    def scan_text_file(self, path: Path) -> Optional[FileViolation]:
        rel = path.relative_to(self.root_dir).as_posix()
        matches: List[TextMatch] = []

        try:
            with open(path, "r", encoding="utf-8", errors="replace") as f:
                lines = f.readlines()
        except Exception:
            return None

        for idx, line in enumerate(lines, start=1):
            for rule in self.config.rules:
                for m in rule.pattern.finditer(line):
                    matched = m.group(0)
                    if self.config.is_ignored(matched):
                        continue

                    matches.append(TextMatch(
                        line_number=idx,
                        matched_text=matched,
                        replacement=rule.replacement,
                        preview=line.strip()[:120]
                    ))

        if matches:
            return FileViolation(file_path=path, relative_path=rel, matches=matches)
        return None

    def normalize_paths(self, content: str, file_path: Path) -> Tuple[str, int]:
        """Normalizes hardcoded developer workstation paths to portable workspace/environment variables."""
        count = 0
        rel_posix = file_path.relative_to(self.root_dir).as_posix()
        ext = file_path.suffix.lower()

        # Build dynamic pattern of target user folder tokens from config and environment
        target_usernames = set()
        for t in self.config.raw_targets:
            for part in t.split():
                if re.match(r'^[A-Za-z0-9_\-]+$', part):
                    target_usernames.add(part)
        env_u = os.environ.get("USERNAME") or os.environ.get("USER")
        if env_u:
            target_usernames.add(env_u)

        if target_usernames:
            user_pat = "(?:" + "|".join(re.escape(u) for u in sorted(target_usernames, key=len, reverse=True)) + ")"
        else:
            user_pat = r'[^\\/]+'

        # 1. VSCode Configuration Files (.vscode/*.json)
        if ".vscode" in rel_posix or ext == ".json":
            def rep_esp_tools(m):
                nonlocal count
                count += 1
                delim = m.group(1)
                if delim == "\\\\":
                    return "${env:USERPROFILE}\\\\.espressif"
                elif delim == "\\":
                    return "${env:USERPROFILE}\\.espressif"
                else:
                    return "${env:USERPROFILE}/.espressif"

            content = re.sub(
                rf'[A-Za-z]:(\\{{1,2}}|/)Users\1{user_pat}\1\.espressif',
                rep_esp_tools,
                content,
                flags=re.IGNORECASE
            )

            def rep_ws(m):
                nonlocal count
                count += 1
                return "${workspaceFolder}"

            content = re.sub(
                rf'[A-Za-z]:(?:\\{{1,2}}|/)Users(?:\\{{1,2}}|/){user_pat}(?:\\{{1,2}}|/)Documents(?:\\{{1,2}}|/)GitHub(?:\\{{1,2}}|/)H1OS',
                rep_ws,
                content,
                flags=re.IGNORECASE
            )

        # 2. Markdown & Skill Links (file:///c:/Users/.../H1OS/...)
        if ext == ".md" or ".agents" in rel_posix:
            def rep_md_link(m):
                nonlocal count
                count += 1
                rel_target = m.group("rel")
                return f"file:///{rel_target}"

            content = re.sub(
                rf'file:///[A-Za-z]:/Users/{user_pat}/Documents/GitHub/H1OS/(?P<rel>[^)"\'\s]+)',
                rep_md_link,
                content,
                flags=re.IGNORECASE
            )

            def rep_cmd_path(m):
                nonlocal count
                count += 1
                rel_target = m.group("rel")
                return rel_target

            content = re.sub(
                rf'[A-Za-z]:(?:\\{{1,2}}|/)Users(?:\\{{1,2}}|/){user_pat}(?:\\{{1,2}}|/)Documents(?:\\{{1,2}}|/)GitHub(?:\\{{1,2}}|/)H1OS(?:\\{{1,2}}|/)(?P<rel>[a-zA-Z0-9_\-\./\\]+)',
                rep_cmd_path,
                content,
                flags=re.IGNORECASE
            )

            def rep_ws_doc(m):
                nonlocal count
                count += 1
                return "<workspace_root>"

            content = re.sub(
                rf'[A-Za-z]:(?:\\{{1,2}}|/)Users(?:\\{{1,2}}|/){user_pat}(?:\\{{1,2}}|/)Documents(?:\\{{1,2}}|/)GitHub(?:\\{{1,2}}|/)H1OS',
                rep_ws_doc,
                content,
                flags=re.IGNORECASE
            )

        return content, count

    def fix_text_file(self, path: Path) -> int:
        """Applies replacements in-place preserving original line endings. Returns count of fixes."""
        try:
            with open(path, "rb") as f:
                raw_bytes = f.read()

            newline = b"\r\n" if b"\r\n" in raw_bytes else b"\n"
            content = raw_bytes.decode("utf-8", errors="replace")

            # 1. Apply smart path normalization first
            mod_content, path_replacements = self.normalize_paths(content, path)
            total_replacements = path_replacements

            # 2. Apply name rules (longest first)
            for rule in self.config.rules:
                def repl_cb(match):
                    nonlocal total_replacements
                    word = match.group(0)
                    if self.config.is_ignored(word):
                        return word
                    total_replacements += 1
                    return rule.replacement

                mod_content = rule.pattern.sub(repl_cb, mod_content)

            if total_replacements > 0:
                with open(path, "wb") as f:
                    f.write(mod_content.encode("utf-8"))

            return total_replacements
        except Exception as e:
            print(f"Error fixing {path}: {e}", file=sys.stderr)
            return 0

    # --------------------------------------------------------------------------
    # Archive Scanning & Sanitization
    # --------------------------------------------------------------------------

    def scan_archive(self, path: Path):
        rel = path.relative_to(self.root_dir).as_posix()
        ext = path.suffix.lower()

        if ext in [".zip", ".whl"]:
            self._scan_zip(path, rel)
        elif ext in [".tar", ".gz", ".tgz", ".bz2"]:
            self._scan_tar(path, rel)

    def _scan_zip(self, path: Path, rel: str):
        try:
            with zipfile.ZipFile(path, "r") as zf:
                for info in zf.infolist():
                    # Check member path
                    for rule in self.config.rules:
                        for m in rule.pattern.finditer(info.filename):
                            matched = m.group(0)
                            if not self.config.is_ignored(matched):
                                self.archive_violations.append(ArchiveViolation(
                                    archive_path=path,
                                    relative_path=rel,
                                    member_name=info.filename,
                                    violation_type="name",
                                    details=f"Member filename matches '{matched}' -> '{rule.replacement}'"
                                ))

                    # Check member text content
                    member_ext = Path(info.filename).suffix.lower()
                    if member_ext not in self.BINARY_EXTENSIONS and info.file_size < 1024 * 1024:
                        try:
                            with zf.open(info) as mf:
                                raw = mf.read()
                                if b"\0" not in raw:
                                    text = raw.decode("utf-8", errors="replace")
                                    for rule in self.config.rules:
                                        for m in rule.pattern.finditer(text):
                                            matched = m.group(0)
                                            if not self.config.is_ignored(matched):
                                                self.archive_violations.append(ArchiveViolation(
                                                    archive_path=path,
                                                    relative_path=rel,
                                                    member_name=info.filename,
                                                    violation_type="content",
                                                    details=f"Contains matched token '{matched}' -> '{rule.replacement}'"
                                                ))
                                                break
                        except Exception:
                            pass
        except Exception as e:
            pass

    def _scan_tar(self, path: Path, rel: str):
        try:
            mode = "r:*" if path.suffix in [".gz", ".tgz", ".bz2"] else "r:"
            with tarfile.open(path, mode) as tf:
                for member in tf.getmembers():
                    # 1. Header ownership check (uname / gname)
                    for rule in self.config.rules:
                        for name_val, header_field in [(member.uname, "uname"), (member.gname, "gname")]:
                            if name_val:
                                for m in rule.pattern.finditer(name_val):
                                    matched = m.group(0)
                                    if not self.config.is_ignored(matched):
                                        self.archive_violations.append(ArchiveViolation(
                                            archive_path=path,
                                            relative_path=rel,
                                            member_name=member.name,
                                            violation_type="owner_header",
                                            details=f"Tar member header {header_field}='{name_val}' matches '{matched}'"
                                        ))

                    # 2. Member path check
                    for rule in self.config.rules:
                        for m in rule.pattern.finditer(member.name):
                            matched = m.group(0)
                            if not self.config.is_ignored(matched):
                                self.archive_violations.append(ArchiveViolation(
                                    archive_path=path,
                                    relative_path=rel,
                                    member_name=member.name,
                                    violation_type="name",
                                    details=f"Tar member filename matches '{matched}' -> '{rule.replacement}'"
                                ))

                    # 3. Content check
                    if member.isfile() and member.size < 1024 * 1024:
                        m_ext = Path(member.name).suffix.lower()
                        if m_ext not in self.BINARY_EXTENSIONS:
                            try:
                                f = tf.extractfile(member)
                                if f:
                                    raw = f.read()
                                    if b"\0" not in raw:
                                        text = raw.decode("utf-8", errors="replace")
                                        for rule in self.config.rules:
                                            for m in rule.pattern.finditer(text):
                                                matched = m.group(0)
                                                if not self.config.is_ignored(matched):
                                                    self.archive_violations.append(ArchiveViolation(
                                                        archive_path=path,
                                                        relative_path=rel,
                                                        member_name=member.name,
                                                        violation_type="content",
                                                        details=f"Contains matched token '{matched}' -> '{rule.replacement}'"
                                                    ))
                                                    break
                            except Exception:
                                pass
        except Exception:
            pass

    def fix_archive(self, path: Path) -> bool:
        """Sanitizes an archive in-place (rewriting member names, contents, and Tar uname/gname)."""
        ext = path.suffix.lower()
        if ext in [".zip", ".whl"]:
            return self._fix_zip(path)
        elif ext in [".tar", ".gz", ".tgz", ".bz2"]:
            return self._fix_tar(path)
        return False

    def _fix_zip(self, path: Path) -> bool:
        temp_file = path.with_suffix(path.suffix + ".tmp_sanitized")
        try:
            with zipfile.ZipFile(path, "r") as zin:
                with zipfile.ZipFile(temp_file, "w", compression=zin.compression) as zout:
                    for item in zin.infolist():
                        raw = zin.read(item.filename)
                        # Sanitize filename
                        new_name = item.filename
                        for rule in self.config.rules:
                            new_name = rule.pattern.sub(rule.replacement, new_name)

                        # Sanitize content if text
                        member_ext = Path(item.filename).suffix.lower()
                        if member_ext not in self.BINARY_EXTENSIONS and b"\0" not in raw:
                            try:
                                text = raw.decode("utf-8")
                                for rule in self.config.rules:
                                    text = rule.pattern.sub(rule.replacement, text)
                                raw = text.encode("utf-8")
                            except Exception:
                                pass

                        zout.writestr(new_name, raw)

            shutil.move(str(temp_file), str(path))
            return True
        except Exception as e:
            if temp_file.exists():
                temp_file.unlink()
            return False

    def _fix_tar(self, path: Path) -> bool:
        temp_file = path.with_suffix(path.suffix + ".tmp_sanitized")
        read_mode = "r:*" if path.suffix in [".gz", ".tgz", ".bz2"] else "r:"
        write_mode = "w:gz" if path.suffix in [".gz", ".tgz"] else ("w:bz2" if path.suffix == ".bz2" else "w:")

        try:
            with tarfile.open(path, read_mode) as tin:
                with tarfile.open(temp_file, write_mode) as tout:
                    for member in tin.getmembers():
                        # Reset ownership header
                        member.uname = self.config.default_replacement.lower()
                        member.gname = self.config.default_replacement.lower()
                        member.uid = 1000
                        member.gid = 1000

                        # Sanitize member name
                        for rule in self.config.rules:
                            member.name = rule.pattern.sub(rule.replacement, member.name)

                        if member.isfile():
                            f = tin.extractfile(member)
                            raw = f.read() if f else b""
                            m_ext = Path(member.name).suffix.lower()
                            if m_ext not in self.BINARY_EXTENSIONS and b"\0" not in raw:
                                try:
                                    text = raw.decode("utf-8")
                                    for rule in self.config.rules:
                                        text = rule.pattern.sub(rule.replacement, text)
                                    raw = text.encode("utf-8")
                                except Exception:
                                    pass

                            member.size = len(raw)
                            tout.addfile(member, io.BytesIO(raw))
                        else:
                            tout.addfile(member)

            shutil.move(str(temp_file), str(path))
            return True
        except Exception as e:
            if temp_file.exists():
                temp_file.unlink()
            return False

    # --------------------------------------------------------------------------
    # Git History & Document Metadata
    # --------------------------------------------------------------------------

    def scan_git_history(self, max_commits: int = 200):
        try:
            cmd = ["git", "log", f"-n{max_commits}", "--format=%H|%an|%ae|%cn|%ce|%s"]
            res = subprocess.run(cmd, cwd=str(self.root_dir), capture_output=True, text=True, check=False)
            if res.returncode != 0:
                return

            for line in res.stdout.splitlines():
                if not line:
                    continue
                parts = line.split("|", 5)
                if len(parts) >= 6:
                    h, an, ae, cn, ce, subj = parts
                    for rule in self.config.rules:
                        for field_val in [an, ae, cn, ce]:
                            for m in rule.pattern.finditer(field_val):
                                matched = m.group(0)
                                if not self.config.is_ignored(matched):
                                    self.git_violations.append(GitViolation(
                                        commit_hash=h[:8],
                                        author=f"{an} <{ae}>",
                                        email=ae,
                                        subject=subj[:60]
                                    ))
                                    break
        except Exception:
            pass

    def scan_pdf_metadata(self, path: Path):
        rel = path.relative_to(self.root_dir).as_posix()
        try:
            with open(path, "rb") as f:
                header = f.read(1024 * 256)  # Read first 256KB
            text = header.decode("latin1", errors="ignore")
            for rule in self.config.rules:
                for tag in ["/Author", "/Creator", "/Producer"]:
                    m = re.search(rf'{tag}\s*\(([^)]+)\)', text)
                    if m:
                        val = m.group(1)
                        if rule.pattern.search(val) and not self.config.is_ignored(val):
                            self.pdf_violations.append((path, f"PDF Tag {tag}: '{val}' matches target"))
        except Exception:
            pass

    # --------------------------------------------------------------------------
    # Repository Walker
    # --------------------------------------------------------------------------

    def scan_all(self, include_git_history: bool = True):
        self.text_violations.clear()
        self.archive_violations.clear()
        self.git_violations.clear()
        self.pdf_violations.clear()
        self.total_scanned_files = 0
        self.total_scanned_archives = 0

        for root, dirs, files in os.walk(self.root_dir):
            # Prune ignored directories
            dirs[:] = [d for d in dirs if d not in self.IGNORED_DIRS and not d.startswith(".git")]

            for file_name in files:
                file_path = Path(root) / file_name

                # Skip the names config file itself
                if self.config.config_path and file_path.resolve() == self.config.config_path.resolve():
                    continue

                if file_name in self.IGNORED_FILES:
                    continue

                ext = file_path.suffix.lower()

                if ext in self.ARCHIVE_EXTENSIONS:
                    self.total_scanned_archives += 1
                    self.scan_archive(file_path)
                elif ext == ".pdf":
                    self.total_scanned_files += 1
                    self.scan_pdf_metadata(file_path)
                elif not self.is_binary_file(file_path):
                    self.total_scanned_files += 1
                    violation = self.scan_text_file(file_path)
                    if violation:
                        self.text_violations.append(violation)

        if include_git_history:
            self.scan_git_history()


# ==============================================================================
# Reporting & CLI Presentation
# ==============================================================================

def print_audit_report(engine: SanitizerEngine):
    print("\n" + "=" * 78)
    print("H1OS IDENTITY & NAME SANITIZATION AUDIT REPORT")
    print("=" * 78)

    cfg = engine.config
    print(f"Config File       : {cfg.config_path.name if cfg.config_path else 'Default Rule Set'}")
    print(f"Default Replacement: {cfg.default_replacement}")
    print(f"Target Rules      : {len(cfg.rules)} active rules ({', '.join(r.target for r in cfg.rules)})")
    print(f"Ignored Words     : {len(cfg.ignore_words)} words whitelisted")
    print(f"Files Scanned     : {engine.total_scanned_files} text/docs, {engine.total_scanned_archives} archives")
    print("-" * 78)

    # 1. Text Violations
    total_text_matches = sum(len(v.matches) for v in engine.text_violations)
    print(f"\n[1] CODE & DOCUMENT TEXT LEAKS: {len(engine.text_violations)} files ({total_text_matches} matches)")
    if engine.text_violations:
        for v in engine.text_violations:
            print(f"  • {v.relative_path} ({len(v.matches)} matches)")
            for m in v.matches[:3]:  # Show top 3 per file
                print(f"      L{m.line_number:4d}: '{m.matched_text}' -> '{m.replacement}' | {m.preview}")
            if len(v.matches) > 3:
                print(f"      ... and {len(v.matches) - 3} more match(es)")
    else:
        print("  ✓ Zero personal name leaks detected across source files and documentation.")

    # 2. Archive Violations
    print(f"\n[2] ARCHIVE CHECKS (.zip, .tar, .tgz): {len(engine.archive_violations)} issues")
    if engine.archive_violations:
        for a in engine.archive_violations:
            print(f"  • [{a.violation_type.upper()}] {a.relative_path} :: {a.member_name}")
            print(f"      {a.details}")
    else:
        print("  ✓ Zero leaks inside compressed archives (clean file names, contents, headers).")

    # 3. PDF Document Metadata
    print(f"\n[3] DOCUMENT METADATA (PDF / DOC): {len(engine.pdf_violations)} issues")
    if engine.pdf_violations:
        for p, desc in engine.pdf_violations:
            rel = p.relative_to(engine.root_dir).as_posix()
            print(f"  • {rel} :: {desc}")
    else:
        print("  ✓ Zero document metadata leaks detected.")

    # 4. Git History & Commits
    print(f"\n[4] GIT IDENTITY & COMMIT HISTORY: {len(engine.git_violations)} commits flagged")
    if engine.git_violations:
        for g in engine.git_violations[:5]:
            print(f"  • [{g.commit_hash}] {g.author} - {g.subject}")
        if len(engine.git_violations) > 5:
            print(f"  ... and {len(engine.git_violations) - 5} more commit(s)")
    else:
        print("  ✓ Git commit author / committer logs are clean.")

    # Final Summary Banner
    total_issues = total_text_matches + len(engine.archive_violations) + len(engine.pdf_violations) + len(engine.git_violations)
    print("\n" + "=" * 78)
    if total_issues == 0:
        print("SANITY CHECK VERDICT: [PASS] Repository is 100% clean of target identities!")
        print("=" * 78 + "\n")
        return 0
    else:
        print(f"SANITY CHECK VERDICT: [FAIL] Found {total_issues} identity reference(s) across {len(engine.text_violations)} files.")
        print("Run with '--fix' to automatically sanitize text files and rewrite archives.")
        print("=" * 78 + "\n")
        return 1


# ==============================================================================
# Main Entry Point
# ==============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="H1OS Repository Identity & Name Sanitizer",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""Examples:
  python tools/agent/sanitize.py --check               (Audit repository for leaks, exit 0 on clean)
  python tools/agent/sanitize.py --fix                 (Apply sanitization in-place)
  python tools/agent/sanitize.py --names-file custom.txt
  python tools/agent/sanitize.py --target "DeveloperName" --replacement "Humiditron"
"""
    )
    parser.add_argument("--check", action="store_true", help="Audit mode: scan and report without modifying (default)")
    parser.add_argument("--fix", action="store_true", help="Apply sanitization in-place across text files and archives")
    parser.add_argument("--names-file", type=Path, default=None, help="Path to custom name configuration file")
    parser.add_argument("--target", type=str, default=None, help="Quick target word/name (case-agnostic)")
    parser.add_argument("--replacement", type=str, default="Humiditron", help="Replacement string (case-sensitive)")
    parser.add_argument("--root", type=Path, default=REPO_ROOT, help="Root directory of repository to scan")
    parser.add_argument("--skip-git-history", action="store_true", help="Skip auditing git commit history")

    args = parser.parse_args()

    # Determine mode: default is check if fix not requested
    is_fix_mode = args.fix

    # Initialize configuration
    config = NameConfig(config_path=args.names_file, default_replacement=args.replacement)
    if args.target:
        config.default_replacement = args.replacement
        config._add_rule(args.target, args.replacement)

    # Initialize engine
    engine = SanitizerEngine(config=config, root_dir=args.root)

    print(f"Initiating Repository Scan from: {args.root}")
    engine.scan_all(include_git_history=not args.skip_git_history)

    if not is_fix_mode:
        ret = print_audit_report(engine)
        sys.exit(ret)

    # FIX MODE: Apply fixes in-place
    print("\n" + "=" * 78)
    print("APPLYING IN-PLACE SANITIZATION...")
    print("=" * 78)

    fixed_files_count = 0
    total_replaced_count = 0

    # Fix text files
    for v in engine.text_violations:
        count = engine.fix_text_file(v.file_path)
        if count > 0:
            fixed_files_count += 1
            total_replaced_count += count
            print(f"  ✓ Sanitized: {v.relative_path} ({count} replacements)")

    # Fix archives
    fixed_archives_count = 0
    for a in set(v.archive_path for v in engine.archive_violations):
        if engine.fix_archive(a):
            fixed_archives_count += 1
            rel = a.relative_to(args.root).as_posix()
            print(f"  ✓ Sanitized archive & ownership headers: {rel}")

    print("-" * 78)
    print(f"Sanitization complete: {total_replaced_count} tokens replaced across {fixed_files_count} files, {fixed_archives_count} archives updated.")
    print("=" * 78 + "\n")

    # Re-verify after fix
    engine.scan_all(include_git_history=not args.skip_git_history)
    print_audit_report(engine)


if __name__ == "__main__":
    main()
