"""Compile and execute production C++ with fake UART and deterministic time."""

import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]


def test_production_parser_and_queue(tmp_path):
    includes = ["core/automation.h", "core/component.h", "core/helpers.h", "core/log.h"]
    includes += [f"components/{name}/{name}.h" for name in (
        "binary_sensor", "button", "media_player", "number", "select",
        "sensor", "switch", "text_sensor", "uart",
    )]
    for include in includes:
        path = tmp_path / "esphome" / include
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text('#include "esphome_stubs.h"\n')
    executable = tmp_path / "test_yamaha"
    compiled = subprocess.run([
        "g++", "-std=c++17", "-g", "-fsanitize=address,undefined",
        "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        "-I", str(tmp_path), "-I", str(ROOT / "tests/native"),
        "-I", str(ROOT / "components/yamaha_serial"),
        str(ROOT / "tests/native/test_yamaha.cpp"),
        str(ROOT / "components/yamaha_serial/yamaha_serial.cpp"),
        "-o", str(executable),
    ], check=False, capture_output=True, text=True, timeout=120)
    assert compiled.returncode == 0, compiled.stdout + compiled.stderr
    result = subprocess.run(
        [str(executable)], check=False, capture_output=True, text=True, timeout=30
    )
    assert result.returncode == 0, result.stdout + result.stderr
