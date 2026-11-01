#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build production firmware logic with deterministic ESPHome/UART stubs."""

import argparse
import json
import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def run(build, *, build_only=False, fuzz=False):
    build.mkdir(parents=True, exist_ok=True)
    includes = ["core/automation.h", "core/component.h", "core/helpers.h", "core/log.h"]
    includes += [f"components/{name}/{name}.h" for name in (
        "binary_sensor", "button", "media_player", "number", "select",
        "sensor", "switch", "text_sensor", "uart",
    )]
    includes.append("components/api/api_server.h")
    for include in includes:
        path = build / "esphome" / include
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text('#include "esphome_stubs.h"\n')
    catalog = json.loads((ROOT / "components/yamaha_serial/command_catalog.json").read_text())
    (build / "aliases.tsv").write_text("".join(
        f"{entry['name']}\t{entry['payload']}\t{2 if entry['frame'] == 'STX' else 20}\n"
        for entry in catalog["commands"]
    ))
    executable = build / ("fuzz_yamaha" if fuzz else "test_yamaha")
    sources = ["fuzz_yamaha.cpp"] if fuzz else ["test_yamaha.cpp", "test_regressions.cpp"]
    sanitizers = [] if build_only else ["-fsanitize=" + ("fuzzer," if fuzz else "") + "address,undefined"]
    subprocess.run([
        os.environ.get("CXX", "clang++" if fuzz else "g++"), "-std=c++17", "-g", *sanitizers,
        "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        "-I", str(build), "-I", str(ROOT / "tests/native"),
        "-I", str(ROOT / "components/yamaha_serial"),
        *(str(ROOT / "tests/native" / source) for source in sources),
        *map(str, sorted((ROOT / "components/yamaha_serial").glob("*.cpp"))),
        "-o", str(executable),
    ], check=True, timeout=180)
    if not build_only:
        args = [str(executable)]
        if fuzz:
            corpus = build / "corpus"
            corpus.mkdir(exist_ok=True)
            body = b"20009990" + b"A" * 252
            dc2 = b"R0210A00" + b"0" * 256
            seeds = (
                b"\x02002301\x03", b"dc4:2011011100508Hi Fi PC",
                b"\x14" + body + f"{sum(body) & 255:02X}".encode() + b"\x03",
                b"\x12" + dc2 + f"{sum(dc2) & 255:02X}".encode() + b"\x03",
                b"dc4:" + body, b"\x02" + b"A" * 269,
            )
            for index, data in enumerate(seeds):
                (corpus / str(index)).write_bytes(data)
            args += [str(corpus), "-runs=10000", "-max_len=1024", "-seed=1600"]
        subprocess.run(args, cwd=build, check=True, timeout=90)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--build-only", action="store_true")
    parser.add_argument("--fuzz", action="store_true")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="yamaha-native-") as temp:
        run((args.build_dir or Path(temp)).resolve(), build_only=args.build_only, fuzz=args.fuzz)


if __name__ == "__main__":
    main()
