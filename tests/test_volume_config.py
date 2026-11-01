import pytest
from esphome import config_validation as cv

from components.yamaha_serial import VOLUME_SCHEMA


@pytest.mark.parametrize("minimum,maximum", [(0, 0), (0, -20)])
def test_reject_empty_or_reversed_volume_range(minimum, maximum):
    with pytest.raises(cv.Invalid):
        VOLUME_SCHEMA({"min": minimum, "max": maximum})


def test_accept_volume_range():
    result = VOLUME_SCHEMA({"min": -80, "max": 0})
    assert result["min"] == -80
    assert result["max"] == 0
