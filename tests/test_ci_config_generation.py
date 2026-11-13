import importlib.util
from pathlib import Path

import pytest
import yaml

SPEC = importlib.util.spec_from_file_location(
    "prepare_ci_configs", Path(__file__).resolve().parents[1] / "tools/prepare_ci_configs.py"
)
prepare = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(prepare)


def test_round_trip_keeps_lambda_and_resolves_only_secrets():
    data = yaml.load("password: !secret password\ncommand: !lambda return command;\n", Loader=prepare.SecretSafeLoader)
    resolved = prepare.replace_secrets(data, {"password": "dummy"})
    output = yaml.dump(resolved, Dumper=prepare.ConfigDumper)
    loaded = yaml.load(output, Loader=prepare.SecretSafeLoader)
    assert loaded["password"] == "dummy"
    assert isinstance(loaded["command"], prepare.Lambda)
    assert loaded["command"] == "return command;"


def test_missing_secret_fails_instead_of_compiling_literal_text():
    with pytest.raises(KeyError):
        prepare.replace_secrets({"password": "!secret missing"}, {})


def test_documented_configs_are_overlays_on_their_base_example():
    root = Path(__file__).resolve().parents[1]
    configs = prepare.doc_configs(root)
    assert set(configs) == {"yamaha_rx_vx500.yaml", "yamaha_rx_v1900.yaml"}

    base = prepare.load_config((root / "examples/yamaha_rx_v_basic.yaml").read_text(encoding="utf-8"))
    for name, config in configs.items():
        # Blocks the overlay does not mention come from the example unchanged.
        for key in ("esphome", "wifi", "api", "ota", "uart", "logger"):
            assert config[key] == base[key], (name, key)
        # The overlay replaces the yamaha_serial block as a whole.
        assert config["yamaha_serial"] != base["yamaha_serial"], name
        assert config["yamaha_serial"]["uart_id"] == base["yamaha_serial"]["uart_id"], name


def test_documented_config_names_must_not_shadow_examples(tmp_path):
    (tmp_path / "examples").mkdir()
    (tmp_path / "docs").mkdir()
    (tmp_path / "examples/yamaha_rx_v_basic.yaml").write_text("uart: []\n", encoding="utf-8")
    overlay = "<!-- ci-config: yamaha_rx_v_basic.yaml from yamaha_rx_v_basic.yaml -->\n```yaml\nlogger:\n```\n"
    (tmp_path / "docs/supported_models.md").write_text(overlay, encoding="utf-8")
    with pytest.raises(ValueError):
        prepare.doc_configs(tmp_path)
