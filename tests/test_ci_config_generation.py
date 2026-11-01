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
