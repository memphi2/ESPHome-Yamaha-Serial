import importlib.util
from pathlib import Path

import pytest

SPEC = importlib.util.spec_from_file_location(
    "resolve_upstream_version", Path(__file__).resolve().parents[1] / "tools/resolve_upstream_version.py"
)
resolve = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(resolve)

FILE = [{"yanked": False}]


def releases(*versions: str) -> dict[str, list[dict]]:
    return dict.fromkeys(versions, FILE)


def test_stable_is_the_newest_final_release():
    data = releases("2026.9.0", "2026.10.2", "2026.10.10", "2026.11.0b4")
    assert resolve.pick(data, "stable") == "2026.10.10"


def test_beta_is_a_beta_newer_than_stable():
    data = releases("2026.10.2", "2026.11.0b3", "2026.11.0b4", "2026.11.0b10")
    assert resolve.pick(data, "beta") == "2026.11.0b10"


def test_beta_falls_back_to_stable_once_the_release_is_out():
    data = releases("2026.11.0b4", "2026.11.0", "2026.11.1")
    assert resolve.pick(data, "beta") == "2026.11.1"


def test_other_version_forms_and_unusable_releases_are_ignored():
    data = releases("2026.10.2", "2026.12.0.dev20261113", "2026.12.0rc1", "0.7")
    data["2026.11.0"] = [{"yanked": True}]
    data["2026.11.1"] = []
    assert resolve.pick(data, "stable") == "2026.10.2"
    assert resolve.pick(data, "beta") == "2026.10.2"


def test_a_feed_without_stable_releases_is_an_error():
    with pytest.raises(ValueError):
        resolve.pick(releases("2026.11.0b1"), "beta")
