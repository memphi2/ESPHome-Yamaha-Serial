from __future__ import annotations

import json
import pathlib
import re
import subprocess
import sys

import yaml

ROOT = pathlib.Path(__file__).resolve().parents[1]


def declared_version() -> str:
    text = (ROOT / "pyproject.toml").read_text(encoding="utf-8")
    match = re.search(r'^version = "([^"]+)"', text, flags=re.M)
    assert match is not None, "pyproject.toml declares no version"
    return match.group(1)


def test_readme_badges_match_the_support_policy() -> None:
    # The compatibility badges are static text; keep them on the baseline SUPPORT.md states.
    readme = (ROOT / "README.md").read_text(encoding="utf-8")
    support = (ROOT / "SUPPORT.md").read_text(encoding="utf-8")
    esphome = re.search(r"^- Minimum ESPHome: `([^`]+)`", support, re.M)
    home_assistant = re.search(r"^- Minimum Home Assistant for the optional companion integration: `([^`]+)`", support, re.M)
    lts = re.search(r"^- LTS maintenance baseline: \*\*([^*]+)\*\*", support, re.M)
    assert esphome and home_assistant and lts
    assert f"badge/ESPHome-{esphome[1]}%2B-" in readme
    assert f"badge/Home%20Assistant-{home_assistant[1]}%2B-" in readme
    assert f"badge/LTS-{lts[1]}-" in readme
    hacs = json.loads((ROOT / "hacs.json").read_text(encoding="utf-8"))
    assert hacs["homeassistant"] == home_assistant[1]


def test_ci_pins_match_the_support_policy() -> None:
    # CI must test exactly the versions SUPPORT.md promises.
    support = (ROOT / "SUPPORT.md").read_text(encoding="utf-8")

    def stated(label: str) -> str:
        match = re.search(rf"^- {re.escape(label)}: `([^`]+)`", support, re.M)
        assert match, label
        return match[1]

    def pinned(name: str, package: str) -> str:
        text = (ROOT / "requirements" / name).read_text(encoding="utf-8")
        match = re.search(rf"^{package}==(\S+)$", text, re.M)
        assert match, name
        return match[1]

    minimum = stated("Minimum ESPHome")
    lts = stated("LTS build ESPHome")
    ha_minimum = stated("Minimum Home Assistant for the optional companion integration")
    ha_lts = re.search(r"^- LTS build Home Assistant \(companion runtime tests\): `([^`]+)`", support, re.M)
    assert ha_lts
    assert pinned("esphome-minimum.txt", "esphome") == minimum
    assert pinned("esphome-lts-2026.11.txt", "esphome") == lts
    assert pinned("ha-minimum.txt", "homeassistant") == ha_minimum
    assert pinned("ha-lts-2026.11.txt", "homeassistant") == ha_lts[1]
    # The CI table in SUPPORT.md repeats the pins.
    assert f"| LTS pin | `{lts}` | `{ha_lts[1]}` |" in support
    assert f"| Minimum | `{minimum}` | `{ha_minimum}` |" in support
    assert f'"esphome>={minimum}"' in (ROOT / "pyproject.toml").read_text(encoding="utf-8")
    for example in ("yamaha_rx_v_basic.yaml", "yamaha_rx_v_full.yaml"):
        text = (ROOT / "examples" / example).read_text(encoding="utf-8")
        assert re.search(rf"^  min_version: {re.escape(minimum)}$", text, re.M), example


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


def test_compile_pipelines_propagate_failures() -> None:
    for name in ("validate", "release"):
        workflow = yaml.safe_load((ROOT / ".github/workflows" / f"{name}.yml").read_text())
        pipelines = [
            step
            for job in workflow["jobs"].values()
            for step in job["steps"]
            if "esphome compile" in step.get("run", "") and "| tee" in step["run"]
        ]
        assert pipelines
        # Explicit bash selects GitHub's -eo pipefail shell invocation.
        assert all(step.get("shell") == "bash" for step in pipelines)
