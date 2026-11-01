"""Validate cross-entity safety constraints independently of code generation."""

import pytest
from esphome import config_validation as cv
from esphome import final_validate as fv

from components.yamaha_serial import validate_raw_access
from components.yamaha_serial.number import validate_ranges


class Config(dict):
    def get_path_for_id(self, _):
        return ["yamaha_serial", 0, "id"]

    def get_config_for_path(self, _):
        return {"volume": {"min": -60.0, "max": -10.0, "step": 2.0}}


@pytest.fixture
def final_config():
    config = Config()
    token = fv.full_config.set(config)
    yield config
    fv.full_config.reset(token)


def test_volume_number_inherits_parent(final_config):
    result = validate_ranges({"yamaha_serial_id": "yamaha", "volume_db": {}})
    assert result["volume_db"] == {"min_value": -60.0, "max_value": -10.0, "step": 2.0}


@pytest.mark.parametrize("key,values", [
    ("volume_db", {"min_value": -80}),
    ("volume_db", {"min_value": -10, "max_value": -30}),
    ("zone2_volume_db", {"max_value": 0}),
    ("volume_db", {"step": 60}),
    ("bass", {"min_value": 100, "max_value": 0, "step": 1}),
    ("bass", {"min_value": 0, "max_value": float("nan"), "step": 1}),
])
def test_reject_inconsistent_number_ranges(final_config, key, values):
    with pytest.raises(cv.Invalid):
        validate_ranges({"yamaha_serial_id": "yamaha", key: values})


def test_raw_access_requires_encryption_only_when_api_exposed(final_config):
    validate_raw_access({"allow_raw_commands": True})
    final_config["api"] = {}
    validate_raw_access({"allow_raw_commands": False})
    with pytest.raises(cv.Invalid):
        validate_raw_access({"allow_raw_commands": True})
    final_config["api"] = {"encryption": {}}
    validate_raw_access({"allow_raw_commands": True})
