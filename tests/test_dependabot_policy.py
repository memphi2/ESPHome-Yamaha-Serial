"""Keep LTS dependency maintenance quiet without suppressing security fixes."""

from pathlib import Path

import pytest
import yaml

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize("ecosystem", ["github-actions", "pip"])
def test_dependabot_only_opens_security_prs(ecosystem):
    config = yaml.safe_load((ROOT / ".github/dependabot.yml").read_text(encoding="utf-8"))
    assert config["version"] == 2
    assert {item["package-ecosystem"] for item in config["updates"]} == {"github-actions", "pip"}
    entries = [item for item in config["updates"] if item["package-ecosystem"] == ecosystem]
    assert len(entries) == 1
    entry = entries[0]
    assert entry["directory"] == "/"
    assert entry["open-pull-requests-limit"] == 0
    assert "schedule" in entry
    # Do not redirect security configuration or exclude vulnerable packages.
    for key in ("target-branch", "ignore", "allow", "exclude-paths"):
        assert key not in entry
    groups = list(entry["groups"].values())
    assert len(groups) == 1
    assert groups[0] == {"applies-to": "security-updates", "patterns": ["*"]}
