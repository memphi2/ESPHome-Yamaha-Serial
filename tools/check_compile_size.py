#!/usr/bin/env python3
from __future__ import annotations

import argparse
import pathlib
import re
import sys

ANSI_RE = re.compile(r"\x1b\[[0-9;]*[A-Za-z]")
SIZE_RE = re.compile(r"^(RAM|Flash):.*?\(used\s+(\d+)\s+bytes", re.MULTILINE)


def parse_sizes(log_text: str) -> dict[str, int]:
    log_text = ANSI_RE.sub("", log_text)
    return {name.lower(): int(used) for name, used in SIZE_RE.findall(log_text)}


def main() -> int:
    parser = argparse.ArgumentParser(description="Check ESPHome compile RAM/Flash budgets.")
    parser.add_argument("log_file", type=pathlib.Path)
    parser.add_argument("--max-ram", type=int, required=True)
    parser.add_argument("--max-flash", type=int, required=True)
    args = parser.parse_args()

    sizes = parse_sizes(args.log_file.read_text(encoding="utf-8", errors="replace"))
    missing = sorted({"ram", "flash"} - set(sizes))
    if missing:
        print(f"Missing size lines in compile log: {', '.join(missing)}", file=sys.stderr)
        return 2

    failures = []
    if sizes["ram"] > args.max_ram:
        failures.append(f"RAM {sizes['ram']} > {args.max_ram}")
    if sizes["flash"] > args.max_flash:
        failures.append(f"Flash {sizes['flash']} > {args.max_flash}")

    print(f"RAM: {sizes['ram']} / {args.max_ram} bytes")
    print(f"Flash: {sizes['flash']} / {args.max_flash} bytes")
    if failures:
        print("Size budget exceeded: " + "; ".join(failures), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
