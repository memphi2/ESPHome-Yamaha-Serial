#!/usr/bin/env python3
"""Build the release notes for a tag.

A curated note in .github/release-notes/<tag>.md wins, because a release note
is written for a reader and a changelog entry is written for a diff. When no
curated note exists, the matching changelog section is used instead.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def curated_notes(tag: str) -> str | None:
    path = ROOT / ".github" / "release-notes" / f"{tag}.md"
    if not path.is_file():
        return None
    body = path.read_text(encoding="utf-8").strip()
    return body or None


def changelog_section(tag: str) -> str | None:
    version = tag[1:] if tag.startswith("v") else tag
    text = (ROOT / "CHANGELOG.md").read_text(encoding="utf-8")
    pattern = rf"## \[{re.escape(version)}\] - .*?(?=\n## \[|\Z)"
    match = re.search(pattern, text, flags=re.S)
    if match is None:
        return None
    return match.group(0).strip()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tag", help="the release tag, for example v0.1.0")
    parser.add_argument(
        "--output",
        type=Path,
        help="write the notes to this file instead of standard output",
    )
    args = parser.parse_args(argv)

    body = curated_notes(args.tag) or changelog_section(args.tag) or f"Release {args.tag}"

    if args.output is None:
        print(body)
    else:
        args.output.write_text(body + "\n", encoding="utf-8")
        print(f"Wrote {args.output} ({len(body.splitlines())} lines)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
