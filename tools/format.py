#!/usr/bin/env python3
import shutil
import argparse
import os
import subprocess
import sys
from concurrent.futures import ProcessPoolExecutor, as_completed
from pathlib import Path

EXTENSIONS = {".c", ".h", ".cpp", ".hpp", ".cc", ".cxx", ".hh"}
EXCLUDE_DIRS = {"build", ".git", "third_party", "vendor", "generated"}

CLANG_FORMAT_STYLE = (
    "{BasedOnStyle: LLVM, IndentWidth: 8, TabWidth: 8, "
    "UseTab: ForIndentation, ContinuationIndentWidth: 8, "
    "BreakBeforeBraces: Linux, AllowShortIfStatementsOnASingleLine: false, "
    "AllowShortLoopsOnASingleLine: false, ColumnLimit: 80}"
)


def find_files(root: Path) -> list[Path]:
    return [
        p for p in root.rglob("*")
        if p.is_file() and p.suffix in EXTENSIONS
        and not any(excl in p.parts for excl in EXCLUDE_DIRS)
    ]


def format_file(file: Path, clang_format: str, check_only: bool) -> tuple[str, str | None]:
    if check_only:
        cmd = [clang_format, "--style", CLANG_FORMAT_STYLE, "--dry-run", "-Werror", str(file)]
    else:
        cmd = [clang_format, "--style", CLANG_FORMAT_STYLE, "-i", str(file)]

    try:
        subprocess.run(cmd, check=True, capture_output=True, text=True)
        return str(file), None
    except subprocess.CalledProcessError as e:
        error_msg = e.stderr.strip() or e.stdout.strip() or "Unknown format error"
        return str(file), error_msg
    except Exception as e:
        return str(file), f"Execution failed: {e}"


def main():
    parser = argparse.ArgumentParser(description="Concurrent clang-format wrapper with Linux kernel style")
    parser.add_argument("--check", action="store_true", help="Check only, do not modify files")
    parser.add_argument("--dir", type=Path, default=".", help="Root directory to search")
    parser.add_argument("-j", "--jobs", type=int, default=0, help="Parallel workers (default: CPU count)")
    parser.add_argument("--clang-format", default="clang-format", help="Path to clang-format binary")
    args = parser.parse_args()

    files = find_files(args.dir)
    if not files:
        print("No files found")
        return

    workers = args.jobs or os.cpu_count() or 4
    mode = "check" if args.check else "format"
    print(f"Files: {len(files)}, Workers: {workers}, Mode: {mode}")

    failed = []
    with ProcessPoolExecutor(max_workers=workers) as pool:
        futures = {
            pool.submit(format_file, f, args.clang_format, args.check): f
            for f in files
        }
        for future in as_completed(futures):
            filepath, error = future.result()
            if error is not None:
                failed.append((filepath, error))

    if args.check and failed:
        print(f"\nFailed ({len(failed)}):")
        for f, err in sorted(failed):
            print(f"  {f}")
        sys.exit(1)
    elif args.check:
        print(f"All {len(files)} files passed")
    else:
        if failed:
            print(f"\nFormatted {len(files) - len(failed)} files, {len(failed)} failed:")
            for f, _ in sorted(failed):
                print(f"  {f}")
            sys.exit(1)
        else:
            print(f"Formatted {len(files)} files")


if __name__ == "__main__":
    if shutil.which("clang-format") is not None:
        main()
    else:
        print("Can't find 'clang-format', you can to install used 'apt install clang-format'")
    
    