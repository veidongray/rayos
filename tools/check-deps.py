#!/usr/bin/env python3
import shutil
import sys
from dataclasses import dataclass


@dataclass(frozen=True)
class CheckResult:
    name: str
    found: bool
    path: str | None = None


def check_executables(commands: list[str], search_path: str | None = None) -> list[CheckResult]:
    results = []
    for cmd in commands:
        resolved = shutil.which(cmd, path=search_path)
        results.append(CheckResult(name=cmd, found=resolved is not None, path=resolved))
    return results


def print_report(results: list[CheckResult]) -> None:
    GREEN = "\033[92m"
    RED = "\033[91m"
    RESET = "\033[0m"
    BOLD = "\033[1m"

    missing = [r for r in results if not r.found]
    found = [r for r in results if r.found]

    for r in results:
        if r.found:
            print(f"  {GREEN}[OK]{RESET}      {r.name:<20} -> {r.path}")
            pass
        else:
            print(f"  {RED}[MISSING]{RESET} {r.name:<20} -> not found")

    if missing:
        print(f"\n{RED}{BOLD}Missing {len(missing)} dependencies.{RESET}\n")
    else:
        print(f"\n{GREEN}{BOLD}All dependencies are available.{RESET}\n")


def main():
    REQUIRED_TOOLS = [
        "clang-format",
        "gcc",
        "make",
        "qemu-system-x86_64",
        "xorriso",
        "grub-mkrescue",
        "mkfs.fat",
        "gdb",
        "ld",
        "mtools",
    ]

    CUSTOM_PATH = None

    results = check_executables(REQUIRED_TOOLS, search_path=CUSTOM_PATH)
    print_report(results)
    sys.exit(1 if any(not r.found for r in results) else 0)


if __name__ == "__main__":
    main()
