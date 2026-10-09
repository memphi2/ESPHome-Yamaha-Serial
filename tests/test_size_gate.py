from __future__ import annotations

import importlib.util
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "tools" / "check_compile_size.py"


def load_size_module():
    spec = importlib.util.spec_from_file_location("check_compile_size", SCRIPT)
    assert spec is not None
    assert spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_parse_esphome_size_lines() -> None:
    module = load_size_module()
    log = """
\x1b[0mRAM:   [=======   ]  73.5% (used 60224 bytes from 81920 bytes)
Flash: [=====     ]  49.9% (used 522071 bytes from 1044464 bytes)
"""
    assert module.parse_sizes(log) == {"ram": 60224, "flash": 522071}


def test_size_gate_fails_when_budget_is_exceeded(tmp_path: pathlib.Path) -> None:
    log_path = tmp_path / "compile.log"
    log_path.write_text(
        "RAM:   [=======   ]  73.5% (used 60224 bytes from 81920 bytes)\n"
        "Flash: [=====     ]  49.9% (used 522071 bytes from 1044464 bytes)\n",
        encoding="utf-8",
    )

    result = subprocess.run(
        [
            sys.executable,
            str(SCRIPT),
            str(log_path),
            "--max-ram",
            "60000",
            "--max-flash",
            "560000",
        ],
        check=False,
        text=True,
        capture_output=True,
    )

    assert result.returncode == 1
    assert "RAM 60224 > 60000" in result.stderr
