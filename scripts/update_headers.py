#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Update koroFileHeader-style headers for existing source files using git metadata.
- Sets @Date to the file's first commit time (creation time)
- Sets @LastEditTime to the file's last commit time (last modification)
- Ensures keys use the '@' prefix so koroFileHeader can auto-update on save

Updates .cpp, .h, .hpp files by default.
"""
from __future__ import annotations
import subprocess
from pathlib import Path
import os
import stat
import re
import sys
from datetime import datetime
import argparse

REPO_ROOT = Path(__file__).resolve().parents[1]
TARGET_EXTS = {'.cpp', '.h', '.hpp'}  # adjust if needed
AUTHOR = 'wuxiaoxiao'
EMAIL = 'wuxiaoxiao@gmail.com'
DATE_FMT = '%Y-%m-%d %H:%M:%S'

# Skip folders we shouldn't touch
SKIP_DIRS = {
    'build', 'bin', 'CMakeFiles', 'DispCtrl_autogen', '.git', '.vscode'
}

HEADER_RE = re.compile(r"^/\*[\s\S]*?\*/\s*", re.M)  # leading block comment


def run_git(args: list[str], cwd: Path) -> str:
    try:
        out = subprocess.check_output(['git'] + args, cwd=str(cwd), stderr=subprocess.DEVNULL)
        return out.decode('utf-8', errors='ignore').strip()
    except subprocess.CalledProcessError:
        return ''


def get_git_dates(file_path: Path) -> tuple[str | None, str | None]:
    # First commit (creation)
    created = run_git(['log', '--diff-filter=A', '--follow', '--format=%ci', '--', str(file_path)], REPO_ROOT)
    if created:
        created = created.splitlines()[-1]  # last line (oldest) from creation filter; still safe
    # Last commit
    last = run_git(['log', '-1', '--format=%ci', '--', str(file_path)], REPO_ROOT)
    return (created or None, last or None)


def get_fs_dates(file_path: Path) -> tuple[str | None, str | None]:
    try:
        st = file_path.stat()
        # On Windows, st_ctime is creation time; on POSIX it's metadata change time.
        # Fallback heuristic: treat min(ctime, mtime) as creation-like time.
        created_ts = min(st.st_ctime, st.st_mtime)
        modified_ts = st.st_mtime
        created = datetime.fromtimestamp(created_ts).strftime('%Y-%m-%d %H:%M:%S')
        modified = datetime.fromtimestamp(modified_ts).strftime('%Y-%m-%d %H:%M:%S')
        return created, modified
    except Exception:
        return None, None


def normalize_datetime(dt: str | None) -> str | None:
    if not dt:
        return None
    # git %ci format: 'YYYY-MM-DD HH:MM:SS +0800'
    # We only keep 'YYYY-MM-DD HH:MM:SS'
    return dt.split(' +')[0].strip()


def detect_eol(text: str) -> str:
    # Detect predominant EOL
    if '\r\n' in text and text.count('\r\n') >= text.count('\n') - text.count('\r\n'):
        return '\r\n'
    return '\n'


def build_header(date_created: str | None, date_last: str | None, eol: str) -> str:
    dc = date_created or ''
    dl = date_last or ''
    lines = [
        '/*',
        f' * @Author: {AUTHOR}',
        f' * @Email: {EMAIL}',
        f' * @Date: {dc}',
        f' * @LastEditors: {AUTHOR}',
        f' * @LastEditTime: {dl}',
        ' * @Description: ',
        ' */',
        ''
    ]
    return eol.join(lines)


def should_skip(path: Path) -> bool:
    parts = set(p.name for p in path.parents)
    return any(name in SKIP_DIRS for name in parts)


def update_file(path: Path, mode: str = "git") -> bool:
    orig = path.read_text(encoding='utf-8', errors='ignore')
    eol = detect_eol(orig)

    created, last = get_git_dates(path)
    created = normalize_datetime(created)
    last = normalize_datetime(last)

    # Fallback to filesystem times if git history is unavailable (new/untracked files)
    if created is None or last is None:
        fs_created, fs_last = get_fs_dates(path)
        if created is None:
            created = fs_created
        if last is None:
            last = fs_last

    # In commit mode, stamp LastEditTime with current time to reflect this commit time
    if mode == "commit":
        last = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    header = build_header(created, last, eol)

    # If there's a leading block comment containing either Author or @Author, replace it; else insert.
    replaced = False
    m = HEADER_RE.match(orig)
    if m:
        block = m.group(0)
        if ('@Author' in block) or ('Author:' in block) or ('@LastEditTime' in block) or ('LastEditTime' in block):
            new_text = header + orig[m.end():]
            replaced = True
        else:
            # Keep existing first block (e.g., license) but still insert our header above it
            new_text = header + orig
    else:
        new_text = header + orig

    if new_text != orig:
        try:
            path.write_text(new_text, encoding='utf-8')
        except PermissionError:
            # Try to remove read-only attribute on Windows and retry
            try:
                os.chmod(path, stat.S_IWRITE)
                path.write_text(new_text, encoding='utf-8')
            except Exception:
                raise
        return True
    return False


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Update koroFileHeader headers with dates from git or commit time.")
    parser.add_argument("files", nargs="*", help="Specific files to update. If omitted, update all tracked files by extension.")
    parser.add_argument("--mode", choices=["git", "commit"], default="git", help="Use git history times or stamp LastEditTime=now for commits.")
    return parser.parse_args(argv)


def list_target_files(files: list[str]) -> list[Path]:
    paths: list[Path] = []
    if files:
        for f in files:
            p = (REPO_ROOT / f) if not os.path.isabs(f) else Path(f)
            if p.suffix in TARGET_EXTS and p.is_file() and not should_skip(p):
                paths.append(p)
        return paths
    # No files passed: scan repository
    return [p for p in REPO_ROOT.rglob('*') if p.suffix in TARGET_EXTS and p.is_file() and not should_skip(p)]


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    changed = 0
    files = list_target_files(args.files)
    for f in files:
        try:
            if update_file(f, mode=args.mode):
                changed += 1
        except Exception as e:
            print(f"[WARN] Failed updating {f}: {e}")
    print(f"Updated headers in {changed} file(s)")
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
