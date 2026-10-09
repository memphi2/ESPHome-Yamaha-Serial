# Support Policy

## Supported Release Line

- `0.1.x`: active

## Compatibility Baseline

- Minimum ESPHome: `2026.9.0`
- Validated ESPHome: `2026.9.x`
- Python (for the repository's own tooling and tests): `3.12`, `3.13`, `3.14`
- Minimum Home Assistant for the optional companion integration: `2026.9.0`
- Targets: ESP32 (basic example) and ESP8266 (full example), both compiled in
  CI on every push

The minimum is the oldest ESPHome the CI really installs and compiles against,
not the oldest one the code might import successfully. Older `2026.x` releases
are likely to work but are not claimed, because they are not tested.

## ESPHome Media Player Limitation

ESPHome's `media_player` exposes `SELECT_SOURCE` and `SELECT_SOUND_MODE`
feature flags, but its C++ and native-API call path carries no source or
sound-mode value: `media_player::MediaPlayerCall` has only `command`,
`media_url`, `volume` and `announcement`, and `MediaPlayerTraits` has no
`source_list` or `sound_mode_list`. This is still true on ESPHome `dev` as of
this release.

Consequences, which are deliberate and not bugs:

- The ESPHome `media_player` entity does not advertise source or sound-mode
  selection.
- Receiver source and DSP program are exposed as `select` entities, and
  through `media_player.play_media` shortcuts such as `select_source=DVD` and
  `sound_mode=STRAIGHT`.
- The optional Home Assistant companion integration exists to provide one
  native receiver `media_player` with `source_list`, `sound_mode_list`,
  `media_player.select_source` and `media_player.select_sound_mode`.

If ESPHome gains source and sound-mode values in the media-player call path,
the companion becomes optional sugar rather than the supported route, and the
ESPHome entity will advertise the flags directly.

## Validated Hardware

Hardware validation is still the weak point of this project and is stated
honestly:

- No receiver model is claimed as fully hardware-validated for the whole
  command surface.
- Profile mappings for RX-Vx500, RX-Vx600, RX-Vx700 and RX-Vx800 families are
  derived from publicly known RS-232 behaviour and reimplementation, with
  partial hardware testing.
- Extended setup controls (bass, treble, speaker distances, dimmer) are
  implemented for RX-Vx600-style devices. The x700 and x800 profile maps need
  broader hardware validation.
- Zone2 report behaviour varies by model and firmware.

Issues from untested models are welcome and will be labelled as unvalidated
hardware. A report that includes a `uart: debug` trace of the receiver's own
report frames is what lets a profile be corrected.

## What Is Not Exposed As Entities

The normal entities cover stable receiver controls. Model-specific or rarely
used blocks (I/O assign, rename, volume trim, graphic EQ, DC trigger, Zone3,
NET/USB/iPod/XM operation, tuner station, DSP parameter blocks) stay reachable
through the raw command action and its symbolic aliases rather than becoming
entities. See [docs/command_matrix.md](docs/command_matrix.md). That split is
intentional: entities cost flash on ESP8266, and the full example already runs
against a size budget enforced in CI.

## Maintenance Scope

Security and compatibility fixes target the current minor release line only,
unless a release note states otherwise.
