# Contributing

Use a focused branch and keep the five standard/extended profile pairs distinct.
Do not add redundant state/control entities or advertise unsupported HA features.
Confirm mappings against the receiver's exact protocol revision and capture
sanitized receiver feedback when testing hardware. Transport access is not proof
of a typed feature or hardware support.

## Checks

```sh
python -m pip install '.[dev]' -c requirements/esphome-lts-2026.11.txt
python tools/generate_catalog.py --check
ruff check components tests tools custom_components
pytest
yamllint -c .yamllint.yml examples .github
python tools/run_native_tests.py --fuzz
python tools/prepare_ci_configs.py
```

Native tests need GCC; fuzzing needs Clang with libFuzzer. Both execute production
C++ with deterministic UART/entity doubles and ASan/UBSan. Firmware validation
and compilation are separate CI gates. HA companion tests use a separate virtual
environment installed from `requirements/ha-lts-2026.11.txt`.

See [maintenance notes](docs/maintenance.md) for module ownership and size checks.

## License and Privacy

Contribute only code and data you have the right to license under this project's
GPL-3.0-only terms. Preserve copyright/license notices and attribution in NOTICE.
Do not submit vendor manual files, scans, images or copied explanatory text.
Independent protocol mappings are not a license to redistribute vendor works.
Do not include secrets, private addresses, tokens, raw personal logs or firmware
containing Wi-Fi/API credentials in commits or release attachments.

When redistributing binaries, review the corresponding-source and notice
requirements in [LICENSE](LICENSE), including the components linked into that
firmware. A GitHub source archive alone does not certify compliance for an
arbitrary compiled image. This repository does not provide a legal clearance
opinion or a warranty about third-party rights.
