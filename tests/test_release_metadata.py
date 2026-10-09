from __future__ import annotations

import json
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]


def declared_version() -> str:
    text = (ROOT / "pyproject.toml").read_text(encoding="utf-8")
    match = re.search(r'^version = "([^"]+)"', text, flags=re.M)
    assert match is not None, "pyproject.toml declares no version"
    return match.group(1)


def test_companion_manifest_matches_project_version() -> None:
    manifest = json.loads(
        (ROOT / "custom_components" / "yamaha_serial_receiver" / "manifest.json").read_text(
            encoding="utf-8"
        )
    )
    assert manifest["version"] == declared_version()


def test_changelog_has_a_released_section_for_the_declared_version() -> None:
    changelog = (ROOT / "CHANGELOG.md").read_text(encoding="utf-8")
    version = declared_version()
    assert re.search(rf"^## \[{re.escape(version)}\] - \d{{4}}-\d{{2}}-\d{{2}}$", changelog, re.M), (
        f"CHANGELOG.md has no released '## [{version}] - <date>' section"
    )


def test_changelog_keeps_an_unreleased_section() -> None:
    changelog = (ROOT / "CHANGELOG.md").read_text(encoding="utf-8")
    assert "## [Unreleased]" in changelog


def test_release_version_gate_accepts_the_declared_version() -> None:
    result = subprocess.run(
        [sys.executable, str(ROOT / "tools" / "check_release_version.py"), f"v{declared_version()}"],
        capture_output=True,
        text=True,
        check=False,
    )
    assert result.returncode == 0, result.stderr


def test_release_version_gate_rejects_a_mismatched_tag() -> None:
    result = subprocess.run(
        [sys.executable, str(ROOT / "tools" / "check_release_version.py"), "v99.99.99"],
        capture_output=True,
        text=True,
        check=False,
    )
    assert result.returncode == 1


def test_release_notes_exist_for_the_declared_version() -> None:
    notes = ROOT / ".github" / "release-notes" / f"v{declared_version()}.md"
    assert notes.is_file(), f"missing curated release notes: {notes.relative_to(ROOT)}"
    assert notes.read_text(encoding="utf-8").strip(), "release notes file is empty"


def test_release_notes_builder_prefers_the_curated_file() -> None:
    version = declared_version()
    result = subprocess.run(
        [sys.executable, str(ROOT / "tools" / "build_release_notes.py"), f"v{version}"],
        capture_output=True,
        text=True,
        check=True,
    )
    curated = (
        (ROOT / ".github" / "release-notes" / f"v{version}.md").read_text(encoding="utf-8").strip()
    )
    assert result.stdout.strip() == curated


def test_hacs_manifest_is_consistent() -> None:
    hacs = json.loads((ROOT / "hacs.json").read_text(encoding="utf-8"))
    assert hacs["name"]
    assert re.fullmatch(r"\d+\.\d+\.\d+", hacs["homeassistant"])
