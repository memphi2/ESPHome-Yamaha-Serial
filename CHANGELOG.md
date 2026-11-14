# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added
- [Tested Setups](docs/wiring.md#tested-setups) in the wiring guide: the
  ESP32-C3 RS232 Adapter used for the RX-V1900 tests, with its pins, cable and
  a minimal `esp32:` and `uart:` configuration.

### Fixed
- Power-on after a long standby no longer waits 3.5 s to retry. The receiver
  can answer the first power-on with a bare `0x00` byte (its RS-232 waking up)
  and drop the command; the component now retries 500 ms after that byte
  (seen on an RX-V1900 G/E unit). Other commands and later attempts are
  unchanged.

## [0.3.1] - 2026-11-13

### Added
- README badges for the latest release, the Validate and CodeQL workflows, the
  license, the minimum ESPHome and Home Assistant versions, the
  [esphome.io listing](https://esphome.io/guides/diy/#external-components--code),
  HACS and the LTS line. A test keeps the version badges on the baseline in
  SUPPORT.md.
- CI tests four versions of ESPHome and of Home Assistant: the LTS pins, the
  minimums, the newest stable releases and the newest betas, which
  `tools/resolve_upstream_version.py` reads from PyPI on each run. A new
  weekly workflow also starts Validate, so a new upstream release is tested
  without a push. A failing beta job warns but does not fail the workflow.
- The examples set `esphome: min_version: 2026.9.0`, so an older ESPHome stops
  with a clear message instead of a build error.

### Changed
- The minimum ESPHome `2026.9.0` is now compiled in CI, not only validated, and
  the minimum Home Assistant `2026.9.0` runs the companion runtime tests.
  `2026.9.0` stays the minimum: older releases reject the examples'
  `ota: encryption:`, and on `2026.5.0` the component does not compile.
- LTS build pin ESPHome `2026.10.1` to `2026.10.2`. Both releases declare the
  same dependency pins, so the host-dependency findings in SECURITY.md are
  unchanged.

### Fixed
- The ESP32 build no longer warns about a possible truncation when it formats
  an AM tuner frequency. The value always fitted; GCC could not see that.

## [0.3.0] - 2026-11-13

### Added
- `rx_v1900` and `rx_v1900_extended` receiver profiles. `auto` selects the
  extended profile when the receiver reports DC2 model ID `R0240` (RX-V1900).
- A main-zone RX-V1900 configuration in
  [Supported Receivers](docs/supported_models.md#rx-v1900) (media player,
  input, program, audio select, Pure Direct, surround decoder and tuner
  controls, with status sensors). CI also validates the `rx_v1900` and
  `rx_v1900_extended` profile variants.
- [Configuration](docs/configuration.md), a reference for every option and
  entity, the media player shortcuts and raw commands.
- `surround_decoder` select for the RX-V1900's 2ch Pro Logic / PLII / Neo:6
  decoder used by Surround Decode (report `6E`; direct commands for all six,
  including Pro Logic `07EC9`, verified on a G/E unit). Selecting a decoder
  always selects Surround Decode first. The setting applies to 2-channel
  sources; on a multichannel source the receiver stores and reports it but
  decodes the source format. A decoder command with no reply sets `last_error`
  but does not mark the receiver unavailable. Only the RX-V1900 profile has it (new `surround_decoder`
  capability): the x700/x800 manuals document a different decoder table, and the
  regional modes (CS II on U/J models, Neural-THX on U/C models) are untested
  and not included.

### Changed
- ESP8266 static RAM for the full example falls by 6,916 bytes (45,240 to
  38,324 bytes; flash grows by 6,368 bytes). Error and parse-error messages,
  the profile input/program/scene tables, select command and report tables,
  playback format and sampling tables, response-policy rules and input aliases
  now stay in flash instead of being copied to RAM at boot. Messages, labels
  and behavior are unchanged: `last_error` is still cut at 252 bytes, and the
  log still gets the whole message.
- The user documentation is reorganized. The README is a short quick start;
  the option and entity reference moved to `docs/configuration.md`, and
  `docs/supported_models.md` lists which entities each profile supports and
  has a configuration section per receiver. `docs/wiring.md` has the pins of
  each example and the receiver's RS-232 standby setting.
- `examples/` holds only RX-V1600 configurations, the reference receiver. The
  RX-Vx500 example moved to `docs/supported_models.md` without the reset
  button and with raw commands off. CI checks the documented configurations
  with `esphome config` and `yamllint`, so a renamed or removed option fails
  the build.
- The `receiver_reset` button and `dc3:reset` are documented as what they are:
  a factory reset that deletes every user setting on the receiver. The full
  example keeps the button but disables it by default.

### Fixed
- DC2 configuration input decoding is now per profile: RX-Vx500/RX-Vx600 keep the
  single-nibble input ID, while RX-V1900 reads the two-nibble ID so startup
  reports the correct input instead of PHONO.
- RX-Vx700, RX-Vx800 and RX-V1900 profiles decode playback format and sampling
  rate reports with the RX-Vx700 RS-232C tables and the full report byte (for
  example `04`/`09` is now Dolby Digital at 48 kHz, not "Dolby Digital 2.0" at
  176.4 kHz). RX-Vx500/RX-Vx600 are unchanged.
- RX-V1900 reports Straight Enhancer as `44` and 7ch Enhancer as `45`; the
  program select now follows both, and 7ch Enhancer is selected with the
  remote-code form `0F7E81146B`.
- RX-V1900 input map checked on hardware: a single `MD/CD-R` input (`03`, key
  `cdr`) replaces the separate CD-R and MD/TAPE entries, and BD/HD DVD reports
  `11` in the main zone (`0F` in zone 2).
- On RX-V1900 extended, more entities now get a value at startup instead of
  showing unknown: zone 2 input, mute and volume, Extended Surround, tuner
  preset page, preset and band, and playback format and sampling rate. They're
  read from the DC2 configuration reply, using offsets checked against real
  captures. The RX-V1600 night-mode position is no longer read for the RX-V1900,
  because it holds other data there.
- The RX-V1900 has its own DSP program list, checked on hardware and in
  front-panel order. It drops Disco, Game and the THX programs, which the
  receiver doesn't have and which reverted when selected.
- The RX-V1900 decodes the tuner band report (`35`) as `00` AM / `01` FM, as
  measured on a G/E unit (new `tuner_band_fm_is_1` capability). Previously the
  band showed the wrong value and band changes from Home Assistant were never
  confirmed, so the select reverted. RX-Vx500 to RX-Vx800 keep `00` = FM, as
  their manuals specify.
- The RX-V1900 profile rejects Speaker A/B relay commands before sending them
  (new `supports_speaker_relays` capability): the receiver has no Speaker A/B
  and never answers `07EAB`-`07EAE`, so they timed out and could mark it
  unavailable.
- RX-V1900 DOCK is selectable in both zones (`0F7F014AB5` / `0F7F014BB4`, the
  remote-code form) and follows zone 2, where it reports `08`.
- RX-V1900 System Memory is numbered 1-6, and recalling a memory from the remote
  (report `30`) now updates the System Memory select. The scene select takes its
  options from the active profile; other profiles keep A-F.
- DC4 setup writes acknowledged with a bare status and no value echo (RX-V1900
  bass/treble) are now accepted, so they no longer time out, retry and log
  "Tone-control payload shorter than minimum length", and the value is read
  back straight away.
- The RX-V1900 profile rejects the Auto/DTS/AAC decoder mode commands, which the
  receiver ignores.
- The RX-V1900 profile has no sleep timer control (new `sleep_control`
  capability, still on for the other profiles, where it is untested). Its serial
  sleep commands drive "System Sleep", whose Off is never reported, so the
  select showed the wrong state; use a Home Assistant automation instead.

## [0.2.4] - 2026-11-01

### Fixed
- Match command acknowledgements to frame/report IDs, expected values and
  Extended selectors; unrelated reports no longer complete a pending write.
- Accept every mapped DSP/input report for shared operation commands; test
  program and input acknowledgements across all eight explicit profiles.
- Apply one retry/holdoff policy to typed controls, aliases and raw commands.
  Relative/toggle/unknown writes are never retried; typed volume steps use
  bounded absolute targets and honor `volume.step`.
- Mark an event-driven connection unavailable after an unanswered command;
  preserve availability when unrelated valid reports prove receiver activity.
- Validate DC2/DC4 lengths and checksums, preserve DC4 text/padding, handle
  maximum protocol frames, and reject invalid numeric reports/controls.
- Keep parse-error text within HA state limits; log rejected bytes in chunks.
- Canonicalize audio/surround report labels, allow standard-profile tuner
  presets, preserve model hints for unknown IDs, and refresh HA option metadata
  after late model detection without stale select indices.
- Inherit volume number limits from the parent; reject inconsistent ranges.

### Added
- Explicit `allow_raw_commands` opt-in, including raw media-player shortcuts;
  require API encryption when exposing raw access via the native API.
- `diagnostics_interval` (default 60s), with noisy diagnostics disabled by
  default and no long-term statistics classification for age/counters.
- Production C++ regression/fuzz tests, generated catalog checks, C++ CodeQL,
  minimum-version YAML checks and all-family profile validation.
- Migration, recorder-load, memory-measurement and contributor documentation.
- Security floors for compatible build dependencies; documented unresolved
  Pillow/filelock upstream pins instead of overriding ESPHome requirements.

### Changed
- Split the firmware into command, profile, entity, protocol, transport and
  acknowledgement-policy modules. Generate shared select metadata and a
  flash-resident string pool for all 666 command aliases.
- Reduce the ESP8266 full-example static RAM budget from 64,000 to 48,000 bytes;
  tighten the flash budget from 560,000 to 520,000 bytes.
- Limit Dependabot to grouped security-update PRs for Python and GitHub Actions;
  routine version updates are handled during planned LTS maintenance.

## [0.2.3] - 2026-11-01

### Fixed
- Preserve underscores in profile IDs, input keys and raw command aliases.
- Input selects use the active profile's labels and exclude entries without a
  command for their zone; unknown overrides no longer create unusable controls.
- HA companion state updates run on the HA event loop and preserve service context.
- Missing or unknown configured availability sensors no longer report online.
- Invalid entity domains, self references and nonfinite volume states are rejected.
- Companion examples use the existing media player for power/mute.
- Invalid UART frames no longer mark the receiver online or complete pending commands.
- Receiver-reported errors remain visible; retries respect command spacing.
- Typed volume steps are not retried after a lost report, avoiding double increments.
- CI preserves executable `!lambda` actions instead of compiling literal secret text.
- Firmware compile pipelines propagate failures through the log capture step.

### Added
- LTS maintenance baseline 11.2026 with ESPHome 2026.10.1 and HA 2026.10.4 pins.
- Real HA runtime tests and production C++ parser/queue tests with sanitizers.
- Release publication requires the HA runtime tests as well as firmware checks.

## [0.2.2] - 2026-10-21

### Changed
- The examples authenticate OTA with the API encryption key instead of a
  separate OTA password. ESPHome reports that the password costs roughly
  3.5 KB of flash and 60 bytes of RAM on a device that already carries the
  key; measured on the ESP8266 full example, which CI compiles against a size
  budget, it is 3 256 bytes of flash and 32 bytes of RAM.
  The now unused `ota_password` is gone from `examples/secrets.example.yaml`.
  Adopting it on a running device is an ordinary OTA install: the OTA
  component already offers the encrypted protocol because the examples
  configure `api: encryption:`, and an `ota: encryption:` block only adds the
  requirement, so the device never reaches its password phase.
- The compatibility baseline names ESPHome `2026.10.x` as validated. CI
  installs the newest release that satisfies the `>=2026.9.0` floor, and
  2026.10.0 compiles both examples green, including the switch of the ESP8266
  build to the native Arduino toolchain.
- The media-player limitation is restated against ESPHome `2026.10` instead of
  `dev` as of 2026.9. `media_player.h` is byte-identical between 2026.9.1 and
  2026.10.0, so the missing source and sound-mode values in the call path are
  unchanged.

## [0.2.1] - 2026-10-11

### Changed
- Collapsed the 45 per-control C++ entity classes into one class per ESPHome
  entity kind that dispatches on a function enum, and made the Python codegen
  table-driven. The YAML surface is unchanged. Measured on the ESP8266 full
  example, flash drops by 912 bytes and RAM rises by 280.

### Fixed
- The select function enum spells its first entry `INPUT_SOURCE`: the Arduino
  ESP8266 core defines `INPUT` as a macro, which rewrites a bare enumerator
  into a numeric constant and breaks the build. ESP32 builds against ESP-IDF
  and never includes `Arduino.h`, so only the ESP8266 compile catches it.

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
