from __future__ import annotations

import ast
import json
import pathlib

import yaml

ROOT = pathlib.Path(__file__).resolve().parents[1]
COMPANION = ROOT / "custom_components/yamaha_serial_receiver"


def test_companion_manifest_is_local_push_and_dependency_free() -> None:
    manifest = json.loads((COMPANION / "manifest.json").read_text(encoding="utf-8"))

    assert manifest["domain"] == "yamaha_serial_receiver"
    assert manifest["iot_class"] == "local_push"
    assert manifest["requirements"] == []


def test_companion_python_files_are_syntax_valid() -> None:
    for file in sorted(COMPANION.glob("*.py")):
        ast.parse(file.read_text(encoding="utf-8"), filename=str(file))


def test_companion_media_player_advertises_native_ha_source_and_sound_mode() -> None:
    source = (COMPANION / "media_player.py").read_text(encoding="utf-8")

    assert "MediaPlayerEntityFeature.SELECT_SOURCE" in source
    assert "MediaPlayerEntityFeature.SELECT_SOUND_MODE" in source
    assert "async_select_source" in source
    assert "async_select_sound_mode" in source
    assert "source_list" in source
    assert "sound_mode_list" in source
    assert "select_option" in source


def test_companion_example_wraps_esphome_entities() -> None:
    example = yaml.safe_load((ROOT / "examples/home_assistant_companion.yaml").read_text(encoding="utf-8"))
    config = example["media_player"][0]

    assert config["platform"] == "yamaha_serial_receiver"
    assert config["media_player_entity"].startswith("media_player.")
    assert config["source_entity"].startswith("select.")
    assert config["sound_mode_entity"].startswith("select.")
    assert config["availability_entity"].startswith("binary_sensor.")
