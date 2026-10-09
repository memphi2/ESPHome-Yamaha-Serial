from __future__ import annotations

import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]


def test_project_declares_gpl_3_only() -> None:
    pyproject = (ROOT / "pyproject.toml").read_text(encoding="utf-8")
    license_text = (ROOT / "LICENSE").read_text(encoding="utf-8")
    notice = (ROOT / "NOTICE").read_text(encoding="utf-8")

    assert 'license = { text = "GPL-3.0-only" }' in pyproject
    assert "GPL-3.0-or-later" not in pyproject
    assert "GNU GENERAL PUBLIC LICENSE" in license_text
    assert "Version 3, 29 June 2007" in license_text
    assert "GPL-3.0-only" in notice


def test_notice_covers_attribution_and_trademarks() -> None:
    notice = (ROOT / "NOTICE").read_text(encoding="utf-8")

    assert "memphi2/homie-yamaha-rs232" in notice
    assert "No Yamaha manual PDFs" in notice
    assert "not affiliated with, endorsed by, or sponsored by Yamaha" in notice


def test_public_docs_do_not_bundle_or_quote_manual_sources() -> None:
    scan_files = [
        ROOT / "README.md",
        ROOT / "docs" / "protocol.md",
        ROOT / "docs" / "supported_models.md",
        ROOT / "docs" / "troubleshooting.md",
        ROOT / "docs" / "wiring.md",
    ]
    forbidden = ("RX-Vx600_RS232C", "RX-V1500_RS232C", ".pdf", "verbatim manual")
    offenders: list[str] = []
    for file in scan_files:
        text = file.read_text(encoding="utf-8")
        for token in forbidden:
            if token in text:
                offenders.append(f"{file.relative_to(ROOT)} contains {token!r}")
    assert not offenders, "\n".join(offenders)
