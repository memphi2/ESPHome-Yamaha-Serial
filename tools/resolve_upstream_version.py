"""Print the newest stable or beta release of ESPHome or Home Assistant on PyPI.

CI installs the exact version this prints, so a beta job pulls the beta itself
but no pre-release dependencies. ``beta`` prints the newest release of either
kind: between a stable release and the next beta it is the stable release.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import urllib.request

# Both projects publish YYYY.M.P and YYYY.M.PbN. Other forms (old dev builds,
# rc, two-part versions) are ignored.
FINAL = re.compile(r"(\d+)\.(\d+)\.(\d+)")
BETA = re.compile(r"(\d+)\.(\d+)\.(\d+)b(\d+)")


def release_key(version: str) -> tuple[int, int, int, int, int] | None:
    """Return a sort key; a final release sorts after every beta of it."""
    if match := FINAL.fullmatch(version):
        return (*map(int, match.groups()), 1, 0)
    if match := BETA.fullmatch(version):
        year, month, patch, beta = map(int, match.groups())
        return (year, month, patch, 0, beta)
    return None


def pick(releases: dict[str, list[dict]], channel: str) -> str:
    """Return the newest release of a channel from PyPI's ``releases`` mapping."""
    keys = {}
    for version, files in releases.items():
        key = release_key(version)
        # A release without files or with every file yanked cannot be installed.
        if key and files and not all(file.get("yanked") for file in files):
            keys[version] = key
    finals = [version for version, key in keys.items() if key[3] == 1]
    if not finals:
        raise ValueError("no stable release found")
    candidates = finals if channel == "stable" else keys
    return max(candidates, key=keys.__getitem__)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("package", choices=["esphome", "homeassistant"])
    parser.add_argument("channel", choices=["stable", "beta"])
    args = parser.parse_args()
    url = f"https://pypi.org/pypi/{args.package}/json"
    with urllib.request.urlopen(url, timeout=30) as response:
        releases = json.load(response)["releases"]
    print(pick(releases, args.channel))
    return 0


if __name__ == "__main__":
    sys.exit(main())
