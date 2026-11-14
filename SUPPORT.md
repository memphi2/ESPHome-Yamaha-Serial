# Support Policy

## Supported Release Line

- LTS maintenance baseline: **11.2026**, reviewed on **2026-11-01**.
- `0.4.x`: active maintenance line; `0.4.1` is the current release.
- `0.3.x`: superseded; update to `0.4.1`, which keeps `0.3.x` configurations
  working.
- `0.2.x`: superseded; update to `0.4.1`, which keeps `0.2.4` configurations
  working.
- `0.1.x`: superseded; update to the current maintenance release.

LTS names this project's maintenance snapshot, not an upstream ESPHome or
Home Assistant LTS release. Maintenance covers bug, security and compatibility
fixes on the current minor line; no fixed end-of-support date is promised.

## Compatibility Baseline

- Minimum ESPHome: `2026.9.0`
- LTS build ESPHome: `2026.10.2`
- Python (for the repository's own tooling and tests): `3.12`, `3.13`, `3.14`
- Minimum Home Assistant for the optional companion integration: `2026.9.0`
- LTS build Home Assistant (companion runtime tests): `2026.10.4` on Python `3.14`
- Targets: ESP32 (basic example) and ESP8266 (full example), both compiled in
  CI on every push

The LTS jobs and releases constrain ESPHome using
`requirements/esphome-lts-2026.11.txt`, which also applies the
`requirements/build-security.txt` floors; the other ESPHome jobs install only
the exact ESPHome version. The HA companion always runs in a separate
environment from ESPHome (`requirements/ha-lts-2026.11.txt`,
`requirements/ha-minimum.txt` or `requirements/ha-test.txt`), so conflicting
upstream dependencies cannot affect firmware builds. These pins are reviewed as
maintenance updates arrive.

CI tests four versions of ESPHome and of Home Assistant on every push and
pull request, and once a week through `.github/workflows/weekly.yml`. GitHub
pauses scheduled workflows after 60 days without repository activity; re-enable
the weekly workflow in the Actions tab if that happens.

| Version | ESPHome | Home Assistant |
|---|---|---|
| LTS pin | `2026.10.2` | `2026.10.4` |
| Minimum | `2026.9.0` | `2026.9.0` |
| Newest stable | from PyPI | from PyPI |
| Newest beta | from PyPI | from PyPI |

Each ESPHome version validates every configuration, compiles both examples and
checks the ESP8266 full example against the RAM and flash budget; each HA
version runs the companion runtime tests.
`tools/resolve_upstream_version.py` reads the newest stable and beta from PyPI
on each run; when no beta is newer than the stable release, the beta job tests
the stable release again. A failing beta job is an early warning and does not
fail the workflow; the other versions are what this project promises.

`2026.9.0` is the oldest ESPHome release that runs the examples: older releases
reject `ota: encryption:`, and on `2026.5.0` the component itself does not
compile. Real receiver validation is still required for
model-specific behavior. The host C++ tests compile the production parser and
queue with UART/entity doubles and AddressSanitizer/UBSan. HA tests exercise
the real event dispatcher, service registry, availability and listener cleanup.
CI also fuzzes the production parser/raw input, validates all ten explicit
standard/extended profiles, checks generated catalogs, and runs C++ CodeQL.

## ESPHome Media Player Limitation

ESPHome's `media_player` exposes `SELECT_SOURCE` and `SELECT_SOUND_MODE`
feature flags, but its C++ and native-API call path carries no source or
sound-mode value: `media_player::MediaPlayerCall` has only `command`,
`media_url`, `volume` and `announcement`, and `MediaPlayerTraits` has no
`source_list` or `sound_mode_list`. This is verified on ESPHome `2026.10.2`;
the `2026.11.0b4` beta and `dev` on 2026-11-13 have no such fields either.

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
- The RX-V1900 profile was tested on one RX-V1900 G/E (UK/Europe) unit, with
  an ESP32-C3 RS232 Adapter (see [Tested Setups](docs/wiring.md#tested-setups)),
  for the controls listed in
  [Supported Receivers](docs/supported_models.md#rx-v1900). Regional variants
  (U/C/J) are untested.
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
transitive ranges normally, while ESPHome, HA and the firmware test/lint tools
remain pinned. This is not a cross-platform dependency lockfile. Changes to
resolved dependencies or framework packages can change binary sizes; compare
builds using the same configuration, platform, ESPHome and toolchain.
