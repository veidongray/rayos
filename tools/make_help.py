#!/usr/bin/env python3
import re
import sys
import unicodedata
from pathlib import Path


def display_width(s: str) -> int:
    """计算字符串在终端中的实际显示宽度(CJK字符占2列)"""
    width = 0
    for ch in s:
        if unicodedata.east_asian_width(ch) in ("W", "F"):
            width += 2
        else:
            width += 1
    return width


def pad_to_width(s: str, target_width: int) -> str:
    """按终端显示宽度右填充空格，替代 str.ljust()"""
    current = display_width(s)
    return s + " " * max(0, target_width - current)


def parse_makefile(path: str) -> list[tuple[str, str]]:
    entries = []
    lines = Path(path).read_text(encoding="utf-8").splitlines()
    pending_desc = None

    for line in lines:
        stripped = line.strip()

        # Makefile 构建目标的注释必须满足以下格式才能被正确识别:
        #
        # ## 这是一个注释
        # build:
        #
        if stripped.startswith("## "):
            pending_desc = stripped[3:].strip()
            continue

        m = re.match(r"^([a-zA-Z_][a-zA-Z0-9_-]*)\s*:", stripped)
        if m and pending_desc is not None:
            entries.append((m.group(1), pending_desc))
            pending_desc = None
            continue

        if not stripped.startswith("#"):
            pending_desc = None

    return entries


def print_help(entries: list[tuple[str, str]]) -> None:
    if not entries:
        print("Can't found anythings")
        return

    CYAN = "\033[36m"
    RESET = "\033[0m"
    BOLD = "\033[1m"

    max_width = max(display_width(name) for name, _ in entries)
    col_width = min(max_width + 2, 30)

    for name, desc in entries:
        padded_name = pad_to_width(name, col_width)
        print(f"  {CYAN}{padded_name}{RESET} {desc}")
    print()


def main():
    makefile = sys.argv[1] if len(sys.argv) > 1 else "Makefile"
    entries = parse_makefile(makefile)
    print_help(entries)


if __name__ == "__main__":
    main()
