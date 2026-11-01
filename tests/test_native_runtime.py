"""Compile and execute production C++ with fake UART and deterministic time."""

from tools.run_native_tests import run


def test_production_parser_and_queue(tmp_path):
    run(tmp_path)
