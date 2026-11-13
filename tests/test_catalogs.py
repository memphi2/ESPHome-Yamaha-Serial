"""Generated lookup data and configuration limits are part of the public contract."""

import json
import re
import subprocess
import sys
from pathlib import Path

import pytest
from esphome import config_validation as cv

from components.yamaha_serial import VOLUME_SCHEMA
from tools.generate_catalog import render_selects

ROOT = Path(__file__).resolve().parents[1]


def test_generated_catalogs_are_current():
    subprocess.run([sys.executable, str(ROOT / "tools/generate_catalog.py"), "--check"], check=True)


def test_select_reports_are_valid_control_options():
    catalog = json.loads((ROOT / "components/yamaha_serial/select_catalog.json").read_text())
    render_selects(catalog)
    for group in catalog["selects"].values():
        report_only = set(group.get("report_only", []))
        assert report_only <= set(group["options"])
        for label in group["options"]:
            # Same normalization as YamahaSerialComponent::normalize_token_.
            key = re.sub(r"[ /:._-]+", "_", label.lower()).strip("_")
            if label in report_only:
                assert key not in group["commands"]
            else:
                assert key in group["commands"]


@pytest.mark.parametrize("values", [
    {"min": -80.5}, {"max": 17}, {"max": float("nan")},
    {"min": -0.25}, {"min": 0, "max": -10}, {"step": 0}, {"step": 0.7},
])
def test_invalid_volume_configuration(values):
    with pytest.raises(cv.Invalid):
        VOLUME_SCHEMA(values)


def test_valid_volume_configuration():
    config = VOLUME_SCHEMA({"min": -60, "max": -10, "step": 2})
    assert config == {"min": -60, "max": -10, "step": 2}
