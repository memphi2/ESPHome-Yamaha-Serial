#!/usr/bin/env python3
"""Check that a release tag agrees with every version declared in the repository.

A release is only reproducible if the tag, the Python project version, the Home
Assistant companion manifest and the changelog all name the same version. This
runs in the release workflow before anything is published.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def version_from_tag(tag: str) -> str:
    if not tag.startswith("v"):
        raise SystemExit(f"Release tag must start with 'v', got {tag!r}")
    version = tag[1:]
    if not re.fullmatch(r"\d+\.\d+\.\d+(?:[-.][0-9A-Za-z.]+)?", version):
        raise SystemExit(f"Release tag {tag!r} is not a version tag")
    return version


def pyproject_version() -> str:
    text = (ROOT / "pyproject.toml").read_text(encoding="utf-8")
    match = re.search(r'^version = "([^"]+)"', text, flags=re.M)
    if match is None:
        raise SystemExit("pyproject.toml declares no version")
    return match.group(1)


def manifest_version() -> str:
    manifest = json.loads(
        (ROOT / "custom_components" / "yamaha_serial_receiver" / "manifest.json").read_text(
            encoding="utf-8"
        )
    )
    version = manifest.get("version")
    if not version:
        raise SystemExit("companion manifest.json declares no version")
    return version


def changelog_has_section(version: str) -> bool:
    text = (ROOT / "CHANGELOG.md").read_text(encoding="utf-8")
    return re.search(rf"^## \[{re.escape(version)}\] - ", text, flags=re.M) is not None


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tag", help="the release tag, for example v0.1.0")
    args = parser.parse_args(argv)

    version = version_from_tag(args.tag)
    problems: list[str] = []

    if (declared := pyproject_version()) != version:
        problems.append(f"pyproject.toml declares {declared}, tag says {version}")
    if (declared := manifest_version()) != version:
        problems.append(f"companion manifest.json declares {declared}, tag says {version}")
    if not changelog_has_section(version):
        problems.append(f"CHANGELOG.md has no released '## [{version}] - <date>' section")

    if problems:
        for problem in problems:
            print(f"error: {problem}", file=sys.stderr)
        return 1

    print(f"Release {args.tag} is consistent across pyproject, manifest and changelog")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
