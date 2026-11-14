from __future__ import annotations

import ast
import importlib.util
import pathlib
import re

import yaml

ROOT = pathlib.Path(__file__).resolve().parents[1]


# examples/ holds the RX-V1600 reference configurations; the other receivers'
# configurations are overlays in docs/supported_models.md (see prepare_ci_configs).
EXAMPLES = [
    ROOT / "examples/yamaha_rx_v_basic.yaml",
    ROOT / "examples/yamaha_rx_v_full.yaml",
]

_SPEC = importlib.util.spec_from_file_location("prepare_ci_configs", ROOT / "tools/prepare_ci_configs.py")
_prepare = importlib.util.module_from_spec(_SPEC)
_SPEC.loader.exec_module(_prepare)
DOC_CONFIGS = _prepare.doc_configs(ROOT)


class _SecretSafeLoader(yaml.SafeLoader):
    pass


def _secret_constructor(loader, node):
    return loader.construct_scalar(node)


_SecretSafeLoader.add_constructor("!secret", _secret_constructor)
_SecretSafeLoader.add_constructor("!lambda", _secret_constructor)


def _configs() -> dict[str, dict]:
    """Every user-facing configuration: the examples and the documented overlays.

    Both are parsed with the CI generator's loader, so tags read the same everywhere.
    """
    configs = {example.name: _prepare.load_config(example.read_text(encoding="utf-8")) for example in EXAMPLES}
    configs.update(DOC_CONFIGS)
    return configs


def test_documented_receiver_configurations_are_found() -> None:
    assert set(DOC_CONFIGS) == {"yamaha_rx_vx500.yaml", "yamaha_rx_v1900.yaml"}


EXCERPT = re.compile(r"<!-- excerpt: (?P<source>examples/[\w.-]+\.yaml) -->\n```yaml\n(?P<body>.*?)\n```", re.S)


def test_documented_excerpts_match_their_example() -> None:
    # A YAML block marked as an excerpt must show the example's blocks unchanged, so
    # the reference cannot drift from a configuration CI validates and compiles.
    found = 0
    for doc in [*ROOT.glob("*.md"), *(ROOT / "docs").glob("*.md")]:
        for match in EXCERPT.finditer(doc.read_text(encoding="utf-8")):
            found += 1
            excerpt = _prepare.load_config(match["body"])
            example = _prepare.load_config((ROOT / match["source"]).read_text(encoding="utf-8"))
            for key, block in excerpt.items():
                assert block == example[key], f"{doc.name}: `{key}:` differs from {match['source']}"
    assert found >= 1


def test_python_modules_are_syntax_valid() -> None:
    python_files = sorted((ROOT / "components/yamaha_serial").glob("*.py"))
    assert python_files, "No python files found in component folder"
    for file in python_files:
        ast.parse(file.read_text(encoding="utf-8"), filename=str(file))


def test_examples_have_external_component_reference() -> None:
    for name, data in _configs().items():
        (component,) = data["external_components"]
        assert component["source"] == "github://memphi2/ESPHome-Yamaha-Serial", name
        assert component["components"] == ["yamaha_serial"], name


def test_examples_require_ota_authentication() -> None:
    # An encrypted API only makes encrypted OTA available; it does not require it.
    for name, data in _configs().items():
        for entry in data.get("ota", []):
            if entry.get("platform") == "esphome":
                assert "encryption" in entry or "password" in entry, name


def test_examples_have_minimum_expected_blocks() -> None:
    for data in _configs().values():
        assert "uart" in data
        assert "yamaha_serial" in data
        assert "external_components" in data
        assert "media_player" in data

        yamaha = data["yamaha_serial"]
        assert "id" in yamaha
        assert "uart_id" in yamaha
        assert "poll_interval" in yamaha
        assert "command_timeout" in yamaha
        assert "receiver_profile" in yamaha


def test_examples_use_report_based_polling_by_default() -> None:
    for data in _configs().values():
        assert data["yamaha_serial"]["poll_interval"] == "never"


def test_factory_reset_button_is_disabled_by_default() -> None:
    for name, data in _configs().items():
        for block in data.get("button", []):
            if "receiver_reset" in block:
                assert block["receiver_reset"]["disabled_by_default"] is True, name


def test_poll_interval_supports_never_mode() -> None:
    init_source = (ROOT / "components/yamaha_serial/__init__.py").read_text(encoding="utf-8")
    cpp_source = (ROOT / "components/yamaha_serial/yamaha_serial.cpp").read_text(encoding="utf-8")

    assert 'cv.one_of("never", "off", lower=True)' in init_source
    assert "var.set_periodic_poll_enabled(poll_interval_ms > 0)" in init_source
    assert "if (!this->periodic_poll_enabled_)" in cpp_source


def test_full_example_contains_extended_entities() -> None:
    full = yaml.load((ROOT / "examples/yamaha_rx_v_full.yaml").read_text(encoding="utf-8"), Loader=_SecretSafeLoader)
    yamaha = full["yamaha_serial"]
    assert yamaha["receiver_profile"] == "rx_vx600_extended"

    select_block = full.get("select", [{}])[0]
    for key in (
        "audio_select",
        "night_mode",
        "tuner_preset",
        "tuner_preset_page",
        "tuner_band",
        "sleep_timer",
        "decoder_mode",
        "extended_surround",
        "speaker_b_assignment",
        "zone2_amp",
    ):
        assert key in select_block

    sensor_block = full.get("sensor", [{}])[0]
    assert "sampling_rate" in sensor_block
    assert "volume_db" not in sensor_block
    assert "zone2_volume_db" not in sensor_block

    number_block = full.get("number", [{}])[0]
    for key in (
        "zone2_volume_db",
        "bass",
        "treble",
        "center_distance",
        "front_left_distance",
        "front_right_distance",
        "surround_left_distance",
        "surround_right_distance",
        "subwoofer_distance",
        "dimmer",
        "tuner_fm_frequency",
        "tuner_am_frequency",
    ):
        assert key in number_block

    services = full["api"]["services"]
    assert services[0]["service"] == "yamaha_raw_command"
    assert services[0]["then"][0]["yamaha_serial.raw_command"]["id"] == "yamaha"


def test_full_example_uses_rx_v1600_consistent_entity_layout() -> None:
    full = yaml.load((ROOT / "examples/yamaha_rx_v_full.yaml").read_text(encoding="utf-8"), Loader=_SecretSafeLoader)

    inputs = full["yamaha_serial"]["inputs"]
    assert "hdmi1" not in inputs
    assert "hdmi2" not in inputs
    assert "av1" not in inputs
    assert inputs["dvd"] == "DVD"
    assert inputs["dtv_ld"] == "D-TV/LD"
    assert inputs["cbl_sat"] == "CBL/SAT"

    assert full["select"][0]["scene"]["name"] == "Yamaha System Memory"

    switch_block = full["switch"][0]
    for key in ("power", "main_zone_power", "mute"):
        assert key not in switch_block
    for key in ("zone2_mute", "speaker_a", "speaker_b"):
        assert key in switch_block

    button_block = full["button"][0]
    for key in ("power_toggle", "volume_up", "volume_down", "zone2_volume_up", "zone2_volume_down"):
        assert key not in button_block
    for key in ("tuner_auto_up", "tuner_auto_down"):
        assert key in button_block

    number_block = full["number"][0]
    assert "volume_db" not in number_block
    assert number_block["zone2_volume_db"]["name"] == "Yamaha Zone2 Volume dB"
    assert "disabled_by_default" not in number_block["zone2_volume_db"]

    text_block = full["text_sensor"][0]
    assert "sampling_rate" not in text_block
    assert "last_parse_error" in text_block
    for key in (
        "power_state",
        "input_source",
        "zone2_input_source",
        "program",
        "main_volume_text",
        "audio_select_state",
        "night_mode_state",
    ):
        assert key not in text_block

    sensor_block = full["sensor"][0]
    for key in (
        "last_response_age",
        "commands_sent",
        "responses_received",
        "parse_errors",
        "timeouts",
        "queue_drops",
    ):
        assert sensor_block[key]["disabled_by_default"] is True

    for key in ("last_error", "last_parse_error"):
        assert text_block[key]["disabled_by_default"] is True

    assert "disabled_by_default" not in number_block["zone2_volume_db"]
    assert "disabled_by_default" not in full["select"][0]["tuner_preset"]


def test_x500_configuration_uses_explicit_x500_profile_and_inputs() -> None:
    data = DOC_CONFIGS["yamaha_rx_vx500.yaml"]

    yamaha = data["yamaha_serial"]
    assert yamaha["model"] == "RX-V1500"
    assert yamaha["receiver_profile"] == "rx_vx500_extended"
    assert yamaha["poll_interval"] == "never"
    assert yamaha["inputs"]["dtv_ld"] == "D-TV/LD"
    assert yamaha["inputs"]["vcr3"] == "VCR3"

    select_block = data["select"][0]
    for key in (
        "input_source",
        "program",
        "scene",
        "audio_select",
        "night_mode",
        "tuner_preset",
        "tuner_preset_page",
        "tuner_band",
        "sleep_timer",
        "decoder_mode",
        "extended_surround",
        "speaker_b_assignment",
        "zone2_amp",
    ):
        assert key in select_block

    # The x500 profiles have no fan or DC4 setup controls; their commands would be rejected.
    assert "fan_mode" not in data["switch"][0]
    assert "number" not in data
    assert "receiver_reset" not in data["button"][0]
    assert data["yamaha_serial"]["allow_raw_commands"] is False


def test_rx_v1900_configuration_uses_rx_v1900_profile_without_unsupported_entities() -> None:
    data = DOC_CONFIGS["yamaha_rx_v1900.yaml"]

    yamaha = data["yamaha_serial"]
    assert yamaha["model"] == "RX-V1900"
    assert yamaha["receiver_profile"] == "rx_v1900_extended"
    assert yamaha["poll_interval"] == "never"
    assert yamaha["allow_raw_commands"] is False
    # The profile carries the verified RX-V1900 input labels; RX-V1600 overrides would undo them.
    assert "inputs" not in yamaha

    # A main-zone example: exactly these controls, nothing the RX-V1900 lacks or ignores.
    assert data["switch"][0].keys() - {"platform", "yamaha_serial_id"} == {"pure_direct"}
    select_block = data["select"][0]
    assert select_block.keys() - {"platform", "yamaha_serial_id"} == {
        "input_source",
        "program",
        "audio_select",
        "tuner_band",
        "surround_decoder",
    }
    assert data["number"][0].keys() - {"platform", "yamaha_serial_id"} == {
        "tuner_fm_frequency",
        "tuner_am_frequency",
    }
    assert data["button"][0].keys() - {"platform", "yamaha_serial_id"} == {"refresh"}
    assert data["media_player"][0]["receiver"]["name"] == "Yamaha Receiver"
    number_block = data["number"][0]
    for key in ("tuner_fm_frequency", "tuner_am_frequency"):
        assert number_block[key]["mode"] == "box"
    # Read-only sensors only, main zone; diagnostics hidden by default.
    # No mirrors of state the media player and selects already show.
    for key in ("volume_db", "zone2_volume_db"):
        assert key not in data["sensor"][0]
    for key in ("input_source", "zone2_input_source"):
        assert key not in data["text_sensor"][0]
    assert data["sensor"][0]["last_response_age"]["disabled_by_default"] is True
    for key in ("last_error", "last_parse_error"):
        assert data["text_sensor"][0][key]["disabled_by_default"] is True
    # Source information decoded from the x700+ reports 0x12-0x14.
    for key in ("playback_format", "input_channels", "bit_rate"):
        assert key in data["text_sensor"][0]
    assert data["binary_sensor"][0]["availability"]["name"] == "Yamaha Availability"


def _number_entry(key: str) -> ast.Dict:
    """Return the NUMBER_TYPES entry for one YAML key, read from the source.

    The table is the single place that decides a control's icon, unit and
    entity category, so the tests read it instead of matching repeated schema
    calls. Parsing beats importing here: the module pulls in esphome.codegen.
    """
    source = (ROOT / "components/yamaha_serial/number.py").read_text(encoding="utf-8")
    tree = ast.parse(source)
    for node in tree.body:
        if not isinstance(node, ast.Assign):
            continue
        if not any(
            isinstance(target, ast.Name) and target.id == "NUMBER_TYPES"
            for target in node.targets
        ):
            continue
        assert isinstance(node.value, ast.Dict)
        for entry_key, entry_value in zip(node.value.keys, node.value.values, strict=True):
            if isinstance(entry_key, ast.Constant) and entry_key.value == key:
                assert isinstance(entry_value, ast.Tuple)
                kwargs = entry_value.elts[2]
                assert isinstance(kwargs, ast.Dict)
                return kwargs
    raise AssertionError(f"NUMBER_TYPES has no entry for {key!r}")


def _kwarg_keys(kwargs: ast.Dict) -> set[str]:
    keys: set[str] = set()
    for key in kwargs.keys:
        if key is None:  # a ** spread, for example the shared _DISTANCE block
            continue
        assert isinstance(key, ast.Constant)
        keys.add(key.value)
    return keys


def _spread_names(kwargs: ast.Dict) -> set[str]:
    return {
        value.id
        for key, value in zip(kwargs.keys, kwargs.values, strict=True)
        if key is None and isinstance(value, ast.Name)
    }


def test_tone_numbers_are_controls_not_config_entities() -> None:
    for key in ("bass", "treble"):
        kwargs = _number_entry(key)
        assert "entity_category" not in _kwarg_keys(kwargs)
        assert not _spread_names(kwargs)


def test_zone2_volume_is_control_not_config_entity() -> None:
    kwargs = _number_entry("zone2_volume_db")
    assert "entity_category" not in _kwarg_keys(kwargs)
    assert not _spread_names(kwargs)


def test_subwoofer_distance_uses_available_icon() -> None:
    kwargs = _number_entry("subwoofer_distance")
    icons = [
        value.value
        for key, value in zip(kwargs.keys, kwargs.values, strict=True)
        if isinstance(key, ast.Constant) and key.value == "icon"
    ]
    assert icons == ["mdi:speaker"], icons
    assert "mdi:subwoofer" not in ast.dump(kwargs)