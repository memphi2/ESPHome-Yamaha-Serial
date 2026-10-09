# ESPHome-Yamaha-Serial

ESPHome external component for Yamaha AV receivers controlled via RS-232/UART.

This project provides a native ESPHome external component for Yamaha RS-232 receivers with non-blocking parsing, queueing, retries, diagnostics entities, and ESPHome-native switches/selects/sensors.

## Project status

- Current status: `beta`
- Focus: RX-Vx500 / RX-Vx600 / RX-Vx700 / RX-Vx800 RS-232 command families
- Profile architecture: `auto` + standard/extended profiles for RX-Vx500, RX-Vx600, RX-Vx700, and RX-Vx800 families
- Hardware validation: still required on multiple receiver models

See [docs/supported_models.md](docs/supported_models.md).

## Safety and hardware warning (important)

Yamaha DB9 ports usually use **true RS-232 voltage levels**, not 3.3V TTL.
Do **not** connect ESP GPIO directly to the receiver serial port.

Use a proper RS-232 level converter (MAX3232 class or equivalent).
See [docs/wiring.md](docs/wiring.md).

## Requirements

- ESPHome `2026.9.0` or newer (see [SUPPORT.md](SUPPORT.md))
- An RS-232 level converter between the ESP and the receiver — see
  [docs/wiring.md](docs/wiring.md)
- Home Assistant `2026.9.0` or newer for the optional companion integration

## Installation (External Component)

```yaml
external_components:
  - source: github://memphi2/ESPHome-Yamaha-Serial
    components: [yamaha_serial]
```

Pin a tag for reproducible builds:

```yaml
external_components:
  - source: github://memphi2/ESPHome-Yamaha-Serial@v0.2.0
    components: [yamaha_serial]
```

## Minimal example

See [examples/yamaha_rx_v_basic.yaml](examples/yamaha_rx_v_basic.yaml).
This recommended baseline uses:
- one `media_player` entity for daily receiver control
- diagnostics entities for availability, connection state, and error visibility
- optional `media_player.play_media` shortcuts for receiver input/program selection

## Full example

See [examples/yamaha_rx_v_full.yaml](examples/yamaha_rx_v_full.yaml).

## Native Home Assistant media player wrapper

ESPHome has `SELECT_SOURCE` and `SELECT_SOUND_MODE` feature flags, but its
C++/native-API media-player call path carries no source or sound-mode value:
`media_player::MediaPlayerCall` has only `command`, `media_url`, `volume` and
`announcement`, and `MediaPlayerTraits` has no `source_list` or
`sound_mode_list`. This is still true on ESPHome `dev` as of 2026.9. This
repository therefore keeps the ESPHome media-player entity conservative and
provides an optional Home Assistant companion wrapper for native
`media_player.select_source` and `media_player.select_sound_mode`.

See [docs/home_assistant_companion.md](docs/home_assistant_companion.md) and
[examples/home_assistant_companion.yaml](examples/home_assistant_companion.yaml).

## RX-Vx500 example

See [examples/yamaha_rx_vx500.yaml](examples/yamaha_rx_vx500.yaml) for RX-V1500/RX-V2500-style receivers using the explicit `rx_vx500_extended` profile.

## Core configuration

```yaml
yamaha_serial:
  id: yamaha
  uart_id: uart_yamaha
  model: "RX-Vxxx"
  receiver_profile: auto
  poll_interval: never
  command_timeout: 1500ms
  max_retries: 2
  command_spacing: 100ms
  power_on_delay: 2s
  inputs:
    tuner: "Tuner"
    cd: "CD"
    dvd: "DVD"
    dtv_ld: "D-TV/LD"
    cbl_sat: "CBL/SAT"
    vaux: "V-AUX"
  volume:
    min: -80
    max: 16
    step: 0.5

media_player:
  - platform: yamaha_serial
    yamaha_serial_id: yamaha
    receiver:
      name: Yamaha Receiver
```

## Configuration options

| Option | Type | Default | Description |
|---|---|---|---|
| `model` | string | `RX-Vx500` | Friendly model hint for diagnostics/logging. |
| `receiver_profile` | `auto` \| `rx_vx500` \| `rx_vx500_extended` \| `rx_vx600` \| `rx_vx600_extended` \| `rx_vx700` \| `rx_vx700_extended` \| `rx_vx800` \| `rx_vx800_extended` | `auto` | Selects receiver-specific mapping/capability profile. Explicit standard profiles keep DC4 setup entities disabled; `auto` starts from the model hint and reapplies the matching extended profile when the receiver reports its real model. |
| `poll_interval` | time \| `never` \| `off` | `never` | Optional periodic status refresh interval. `never` uses startup sync plus Yamaha report frames only. |
| `command_timeout` | time | `1500ms` | Timeout for in-flight command responses. |
| `max_retries` | int | `2` | Retries after timeout before command fails. |
| `command_spacing` | time | `100ms` | Minimum gap between transmitted commands. |
| `power_on_delay` | time | `2s` | Holdoff after power-on before further queued commands. |
| `inputs` | map | built-in set | Key/label mapping for input selects and text states. |
| `volume.min` | float | `-80` | Minimum dB accepted by number control. |
| `volume.max` | float | `16.5` | Maximum dB accepted by number control. |
| `volume.step` | float | `0.5` | Step size used by number entity. |

## Implemented entities and features

### Power / Zone
- Native `media_player` entity for receiver core control: power, mute, volume, volume step, play/power-on, next/previous DSP program
- Main power on/off
- Main zone power on/off
- Zone2 power on/off (where supported)
- Power toggle button

### Audio
- Mute on/off
- Volume up/down
- Absolute dB volume controls (main + zone2, config entities; daily control is the `media_player` volume)
- Optional current volume sensors (main + zone2)
- Optional main volume text state
- Audio select mode (Auto/Coax/Opt/Analog/HDMI)
- Night mode (Cinema/Music low-mid-high)
- Pure Direct on/off
- Fan mode on/off (profile dependent, config entity)
- Tuner preset select (`Preset 1`..`Preset 8`)
- Tuner FM/AM frequency controls for RX-Vx600 extended profiles

### Source / program
- Input source select (main + zone2)
- Optional current input text state (main + zone2)
- Program/DSP select
- Optional program text state
- System Memory preset select (A-F, exposed as the `scene` config key for backward compatibility)
- Zone2 mute
- Speaker relay A/B
- Tuner preset page, tuner band and auto seek
- Sleep timer
- Decoder mode
- Extended surround mode
- Speaker B assignment
- Zone2 amp mode
- Playback format text sensor
- Sampling-rate sensor in Hz, plus optional text state for raw Yamaha labels
- `media_player.play_media` shortcuts:
  - `input:DVD`, `source:Tuner`, `select_source=DVD`, `yamaha://input/dvd`, `media-source://yamaha/source/dvd`
  - `program:STRAIGHT`, `dsp:7ch Stereo`, `sound_mode=STRAIGHT`, `yamaha://program/straight`, `yamaha://sound_mode/straight`
  - `scene:A`, `yamaha://scene/a`
  - `yamaha://raw/stx/07E7E`, `yamaha://raw/dc4/20050000F`

### Polling / refresh
- Startup bootstrap sequence
- Yamaha report-frame based updates after startup
- Optional periodic refresh queue via `poll_interval`
- Manual refresh button
- Extended status polling for compatible profiles
- Offline-safe startup: waits for first receiver response before startup sync/manual refresh

### Extended setup controls (RX-Vx600 profile)
- Bass
- Treble
- Speaker distances (center/front/surround/subwoofer)
- Dimmer
- Receiver reset button
- Raw Yamaha command action/API service for complete raw command access:
  - named aliases for finite table commands, for example `program_munich`, `zone3_power_on`, `gui_cursor_up`, `ipod_play`, `net_usb_play`, `system_memory_save_3`
  - `stx:<payload>` for standard operation commands
  - `dc4:<payload>` for RX-Vx600 extended commands, checksum added automatically
  - `dc1:000` for ready/probe
  - `dc3:reset` for receiver reset

The full examples expose an ESPHome API service named `yamaha_raw_command`.
Call it from Home Assistant with a payload such as `{"command": "power_on"}`,
`{"command": "alias:zone3_power_on"}` or `{"command": "stx:07E7E"}`.

### Receiver profiles

| Profile | Auto-detected models | Notes |
|---|---|---|
| `rx_vx500` | never by `auto` | Standard x500 input/program map; standard setup controls; no typed DC4 setup controls. |
| `rx_vx500_extended` | RX-V1500, RX-V2500 | Extended x500 slot for profile consistency; standard setup controls, with raw extended frames available through the raw command action. |
| `rx_vx600` | never by `auto` | Standard x600-compatible baseline; DC4 setup entities disabled. |
| `rx_vx600_extended` | RX-V1600, RX-V2600 | Extended x600 profile with DC4 setup controls and RX-V1600 DC2 offset decode. |
| `rx_vx700` | never by `auto` | Standard x700 input/program map; DC4 setup entities disabled. |
| `rx_vx700_extended` | RX-V1700, RX-V2700 | Extended x700 profile with x700 input/program map and DC4 raw/setup transport enabled. |
| `rx_vx800` | never by `auto` | Standard x800 input/program map; DC4 setup entities disabled. |
| `rx_vx800_extended` | RX-V1800, RX-V3800 | Extended x800 profile with x800 input/program map and DC4 raw/setup transport enabled. |

With `receiver_profile: auto`, the component uses `model` as a startup hint and
then reselects the matching extended profile after a DC2/DC4 model response.
Labels configured under `inputs:` are preserved when that profile switch happens.

### Diagnostics
- availability binary sensor
- connection_state text sensor
- receiver_model text sensor
- last_error text sensor
- last_parse_error text sensor with the last rejected frame/reason
- last_response_age sensor
- counters:
  - commands_sent
  - responses_received
  - parse_errors
  - timeouts
  - queue_drops

## Recommended HA layout

For a clean Home Assistant device model:
- start with `media_player` + diagnostics only (the basic example)
- use `select.input_source`/`select.program` for visible receiver source and DSP state; ESPHome's current C++ media_player API does not expose source or sound-mode state fields
- enable the optional Home Assistant companion wrapper when you want one native receiver `media_player` with `source_list`, `sound_mode_list`, `media_player.select_source`, and `media_player.select_sound_mode`
- use `media_player.play_media` for dashboard/service shortcuts when you want source/program changes from the media-player entity; HA-style strings such as `select_source=DVD` and `sound_mode=STRAIGHT` are routed through the same mappings
- do not add redundant state mirrors when HA already has a select/switch/number/media_player state
- keep noisy diagnostics such as counters and last parse/error details disabled by default unless actively debugging
- add fine-grained `switch`/`select`/`number` entities only when you need advanced tuning controls

## RS-232 protocol notes

See [docs/protocol.md](docs/protocol.md).
For the entity/alias/raw split across the receiver command surface, see
[docs/command_matrix.md](docs/command_matrix.md).
For the receiver-family profile contract, see [docs/profiles.md](docs/profiles.md).

The normal HA entities intentionally expose stable receiver controls; the `yamaha_serial.raw_command` action and the optional API service in the full examples keep the broader raw command surface reachable for model-specific or rarely used functions such as I/O assign, rename, volume trim, graphic EQ, DC trigger, Zone3, NET/USB/iPod/XM operation, tuner station and DSP parameter blocks. Finite RX-Vx500/x600/x700/x800 operation commands are also available as symbolic aliases so automations do not need to carry raw hexadecimal payloads.

## Known limitations

- Cross-model behavior differences exist, especially for input/program IDs and region-specific XM/NET/USB blocks.
- Zone2 report behavior may vary by model/firmware.
- Extended setup controls are implemented for RX-Vx600 style devices; x700/x800 profile maps need broader hardware validation.

## Troubleshooting

See [docs/troubleshooting.md](docs/troubleshooting.md).

## Support, security and privacy

- [SUPPORT.md](SUPPORT.md) — supported release line, ESPHome and Home Assistant
  baseline, validated hardware and what is deliberately not an entity
- [SECURITY.md](SECURITY.md) — threat model and how to report a vulnerability
- [PRIVACY.md](PRIVACY.md) — what is collected, stored and shared (nothing
  leaves your network)

## License and legal notes

This repository is licensed as `GPL-3.0-only`. See [LICENSE](LICENSE) and [NOTICE](NOTICE).

No Yamaha manual PDFs, manual images, vendor diagrams, or copied vendor tables are included. Product and project names are used only for compatibility description and may be trademarks of their respective owners.

## Architecture notes

- MQTT property transport is not used.
- ESPHome entities are now first-class (`switch`, `number`, `select`, `sensor`, `text_sensor`, `binary_sensor`, `button`).
- UART parser and command dispatch are non-blocking and queue-driven.
- Retry/timeout/spacing and diagnostics are integrated.
- Receiver behavior is profile-driven, so additional Yamaha model families can be added modularly.
