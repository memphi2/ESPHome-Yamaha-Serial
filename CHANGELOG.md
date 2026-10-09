# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

## [0.2.0] - 2026-10-09

### Added
- Optional Home Assistant companion `media_player` wrapper with native `select_source`
  and `select_sound_mode` support backed by the ESPHome select entities.
- Native `media_player` platform (`platform: yamaha_serial`) for receiver-style control in Home Assistant.
- CI size-budget checker for ESPHome compile output.
- RX-Vx600 extended feature set:
  - Audio Select, Night Mode, Pure Direct, Fan Mode
  - Tuner preset select (`Preset 1`..`Preset 8`)
  - Tuner FM/AM frequency control through RX-Vx600 extended tuner-station command `050`
  - Bass/Treble controls
  - Speaker distance controls
  - Dimmer control
  - Receiver reset button
- Typed standard-table controls from the local Yamaha command tables:
  - Zone2 mute
  - Speaker relay A/B
  - Tuner preset page and tuner band
  - Tuner auto seek buttons
  - Sleep timer
  - Decoder mode
  - Extended surround mode
  - Speaker B assignment
  - Zone2 amp mode
- Additional diagnostics/status entities:
  - playback format
  - sampling rate sensor in Hz
  - last parse error text with rejected frame context
- Receiver profile option in component config:
  - `receiver_profile: auto | rx_vx500 | rx_vx500_extended | rx_vx600 | rx_vx600_extended | rx_vx700 | rx_vx700_extended | rx_vx800 | rx_vx800_extended`
- Profile-driven mapping/capability layout for future modular receiver support.
- RX-Vx700 and RX-Vx800 generation profiles with model auto-detection for RX-V1700/RX-V2700 and RX-V1800/RX-V3800.
- Profile-specific x700/x800 input and DSP/program mappings, including NET/USB, XM, Multi CH and BD/HD DVD where supported.
- Symbolic raw-command alias catalog for finite RX-Vx500/x600/x700/x800/RX-V1900 operation-table commands, including DSP/program, Zone3, memory, trigger, GUI, XM, iPod and NET/USB command families.
- Additional named aliases for finite SW=2 system/setup/text commands and DC4 information-block reads, plus a documented command-matrix split for entity, alias and raw payload layers.
- RX-Vx500/RX-Vx600 standard-command variants:
  - RX-V1600/RX-Vx600 MD/TAPE keeps the practical `07A18` mapping.
  - RX-Vx500 MD/TAPE uses `07AC9` for Main and `07ACF` for Zone2.
  - RX-V2500 Pure Direct uses `07E82`/`07E83`.
  - Additional finite x500 setup aliases cover main level, subwoofer config, Multi CH select, test tone, tone control, advanced setup, remote ID, fan control, speaker impedance and tuner setup.
- Media-player shortcuts for receiver-centric HA control:
  - play/power-on
  - next/previous DSP program
  - `media_player.play_media` routing for input, DSP/program, and System Memory presets
- Raw Yamaha command action/API service for complete RX-Vx600 command access:
  - `stx:<payload>` for standard operation commands
  - `dc4:<payload>` for RX-Vx600 extended commands with automatic checksum
  - `dc1:000` ready/probe command
  - `dc3:reset` receiver reset command
- Raw command routing through `media_player.play_media`, for example `yamaha://raw/stx/07E7E`.
- Parser replay tests for fragmented, coalesced and checksum-checked Yamaha frames.
- Repository release and support documentation: `SECURITY.md` with the RS-232 threat model,
  `SUPPORT.md` with the compatibility baseline and hardware-validation status, and `PRIVACY.md`.
- Issue forms for bug reports, feature requests and support questions, plus curated contact links.
- `hacs.json` so the optional Home Assistant companion integration can be installed through HACS.
- CodeQL analysis for Python and GitHub Actions, and Dependabot updates for Actions and pip.
- HACS and hassfest validation jobs for the companion integration in CI and in the release workflow.
- `tools/check_release_version.py`, which fails a release whose tag disagrees with the version in
  `pyproject.toml`, the companion `manifest.json` or the changelog.
- `tools/build_release_notes.py`, which prefers a curated `.github/release-notes/<tag>.md` over the
  changelog section.

### Changed
- Media-player state now reports powered-on receivers as `IDLE`, and `play_media` accepts additional HA-style source/sound-mode shortcut strings without advertising unsupported native source/sound-mode feature flags.
- Moved profile mapping tables and command lookup tables into flash-backed static storage on ESP8266 and compacted the raw-command alias table layout.
- Reduced command-queue heap pressure by replacing dynamic queue tags with static tag pointers and bounding the pending queue to 32 entries.
- Reduced ESP8266 RAM/flash pressure by replacing runtime lookup maps with static command/report tables and avoiding per-command TX packet heap allocation.
- `receiver_profile: auto` now reapplies the detected extended profile when the receiver reports its real model, while preserving YAML input labels.
- Receiver profile matrix now exposes both standard and extended profiles for RX-Vx500, RX-Vx600, RX-Vx700 and RX-Vx800 families; `auto` resolves recognized models to the extended profile.
- RX-Vx600 profile now explicitly preserves the RX-V1600-tested MD/TAPE standard input command instead of inheriting the x500 variant.
- Project license metadata clarified as `GPL-3.0-only`; added NOTICE with attribution, trademark, and manual-content notes.
- CI now compiles the ESP8266 full example and fails if RAM/flash usage crosses the configured budget.
- `DC2` extended offset decoding is now profile-gated.
- Receiver model names from extended replies are normalized, for example `08RX-V1600` becomes `RX-V1600`.
- Input report decoding now uses the complete two-nibble report value, required by RX-Vx700/RX-Vx800 sources such as `Multi CH`.
- Main/Zone2 power status decoding now handles Yamaha Zone3-combination report values `04`..`07`.
- The full example now exposes a receiver-style HA layout without hidden duplicate entities.
- Tuner preset moved from a number entity to a select entity.
- Polling behavior is profile-aware (zone2 and extended polls are capability-based).
- Startup/offline behavior is now receiver-driven: regular polling waits for first receiver response and falls back to lightweight ready probe while unavailable.
- Full and x500 examples now expose the additional typed standard-table controls.
- CI now lints `tools/` and `custom_components/` as well, with the bugbear, comprehension, pie,
  return, simplify and ruff rule sets enabled.
- CI now runs the lint and test suite on Python 3.12, 3.13 and 3.14, and reports the resolved
  ESPHome version before compiling.
- The release workflow now compiles both examples, enforces the ESP8266 size budget and requires the
  HACS and hassfest gates before publishing.
- GitHub Actions are pinned by commit SHA.
- Declared minimum Python raised to 3.12 and the ESPHome dev floor to 2026.9.0, matching what CI
  installs; `pyyaml` is now declared explicitly because the test suite parses the example YAML.
- Workflow renamed from `ci.yml` to `validate.yml` for consistency with the other repositories.


## [0.1.0] - 2026-05-22

### Added
- Initial ESPHome external component `yamaha_serial`.
- Non-blocking UART parser with framed STX/DC1/DC2/DC4 handling and ETX framing.
- Command queue with spacing, timeout handling, retry support, and diagnostics counters.
- Core Yamaha features:
  - Main/Zone power control
  - Mute control
  - Main and Zone2 volume control (step and absolute)
  - Input source select (Main and Zone2)
  - DSP program select
  - Scene (Home preset) select
- Periodic status refresh and startup bootstrap sequence.
- Diagnostics entities:
  - availability
  - last response age
  - commands sent
  - responses received
  - parse errors
  - timeouts
  - queue drops
  - last error text
- Documentation and examples for RS-232 wiring and ESPHome setup.
- CI workflows for static checks, YAML checks, and ESPHome config validation.
- Queue coalescing and queue overflow protection for long offline periods.
