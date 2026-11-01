# Support Policy

## Supported Release Line

- LTS maintenance baseline: **11.2026**, reviewed on **2026-11-01**.
- `0.2.x`: active maintenance line; `0.2.3` is the November baseline.
- `0.1.x`: superseded; update to the current maintenance release.

LTS names this project's maintenance snapshot, not an upstream ESPHome or
Home Assistant LTS release. Maintenance covers bug, security and compatibility
fixes on the current minor line; no fixed end-of-support date is promised.

## Compatibility Baseline

- Minimum ESPHome: `2026.9.0`
- LTS build ESPHome: `2026.10.1`
- Python (for the repository's own tooling and tests): `3.12`, `3.13`, `3.14`
- Minimum Home Assistant for the optional companion integration: `2026.9.0`
- Runtime-tested Home Assistant companion: `2026.10.4` on Python `3.14`
- Targets: ESP32 (basic example) and ESP8266 (full example), both compiled in
  CI on every push

CI and releases constrain ESPHome using
`requirements/esphome-lts-2026.11.txt`. The HA companion runs in a separate
environment using `requirements/ha-lts-2026.11.txt`, so conflicting upstream
dependencies cannot affect firmware builds. These pins are reviewed as
maintenance updates arrive. On 2026-11-01 the latest stable releases are still
2026.10; the November label does not claim compatibility with unreleased 2026.11.

The minimum versions remain compatibility targets, not separately compiled
or runtime-tested baselines. Real receiver validation is still required for
model-specific behavior. The host C++ tests compile the production parser and
queue with UART/entity doubles and AddressSanitizer/UBSan. HA tests exercise
the real event dispatcher, service registry, availability and listener cleanup.

## ESPHome Media Player Limitation

ESPHome's `media_player` exposes `SELECT_SOURCE` and `SELECT_SOUND_MODE`
feature flags, but its C++ and native-API call path carries no source or
sound-mode value: `media_player::MediaPlayerCall` has only `command`,
`media_url`, `volume` and `announcement`, and `MediaPlayerTraits` has no
`source_list` or `sound_mode_list`. This is verified on ESPHome `2026.10.1`.

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

### Dependency Updates

The LTS line uses security-only Dependabot PRs for Python dependencies and
GitHub Actions. `open-pull-requests-limit: 0` disables routine version-update
PRs; it does not disable security updates. The required schedule field is not
a delay for security updates. Security fixes are grouped per ecosystem, with
no package or version exclusions. Repository-level Dependabot alerts and
security updates remain enabled; existing CI and review requirements apply.

Non-security dependency refreshes and baseline upgrades are reviewed during
deliberate maintenance work rather than recurring bot PRs. This policy does
not freeze dependencies declared with an open version range: CI still resolves
those ranges normally, while the ESPHome and HA baseline pins remain fixed.
