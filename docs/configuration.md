# Configuration

This page describes the `yamaha_serial` options and every entity the component
offers. The [examples](../examples) show them in context for the RX-V1600;
[Supported Receivers](supported_models.md) has the configuration for other
receivers and lists which entities each profile supports.

## The `yamaha_serial` Block

From [`examples/yamaha_rx_v_basic.yaml`](../examples/yamaha_rx_v_basic.yaml):

<!-- excerpt: examples/yamaha_rx_v_basic.yaml -->
```yaml
uart:
  - id: uart_yamaha
    tx_pin: GPIO17
    rx_pin: GPIO16
    baud_rate: 9600
    data_bits: 8
    parity: NONE
    stop_bits: 1
    rx_buffer_size: 512

yamaha_serial:
  id: yamaha
  uart_id: uart_yamaha
  model: "RX-V1600"
  receiver_profile: auto
  poll_interval: never
  command_timeout: 1500ms
  max_retries: 2
  command_spacing: 100ms
  power_on_delay: 2s
  diagnostics_interval: 60s
  allow_raw_commands: false
  inputs:
    tuner: "Tuner"
    cd: "CD"
    dvd: "DVD"
    dtv_ld: "D-TV/LD"
    vaux: "V-AUX"
  volume:
    min: -80
    max: 16
    step: 0.5
```

The receiver's serial port runs at 9600 baud, 8 data bits, no parity, 1 stop
bit. See [Wiring](wiring.md) for the pins.

| Option | Default | Description |
|---|---|---|
| `model` | `RX-Vx500` | Your receiver model. With `receiver_profile: auto` it selects the starting profile; it also appears in diagnostics. |
| `receiver_profile` | `auto` | `auto`, or one of `rx_vx500`, `rx_vx500_extended`, `rx_vx600`, `rx_vx600_extended`, `rx_vx700`, `rx_vx700_extended`, `rx_vx800`, `rx_vx800_extended`, `rx_v1900`, `rx_v1900_extended`. `auto` switches to the matching extended profile when the receiver reports its model. See [Choosing a Profile](supported_models.md#choosing-a-profile). |
| `poll_interval` | `never` | Periodic status refresh, as a time or `never`/`off`. With `never`, the component syncs at startup and then follows the receiver's own reports. |
| `command_timeout` | `1500ms` | How long to wait for a reply to a command. |
| `max_retries` | `2` | Retries for reads and for setters that are safe to repeat. Relative, toggle and unknown writes are never retried. |
| `command_spacing` | `100ms` | Minimum gap between two commands. |
| `power_on_delay` | `2s` | Pause after power-on before the next queued command. |
| `diagnostics_interval` | `60s` | How often the diagnostic sensors publish, 1s to 7d. It sends nothing to the receiver. |
| `allow_raw_commands` | `false` | Allows [raw commands](#raw-commands). Requires API encryption when the native API is configured. |
| `inputs` | profile labels | Changes the labels of the profile's input keys. Unknown keys are logged and ignored; the Zone2 input list only shows inputs with a Zone2 command. |
| `volume.min` | `-80` | Lowest dB the volume controls accept, in 0.5 dB steps. |
| `volume.max` | `16.5` | Highest dB the volume controls accept. Set a lower ceiling for your speakers; raw commands bypass it. |
| `volume.step` | `0.5` | Volume step, 0.5 to 10 dB. Number entities inherit it unless they set their own. |

Command timeout, spacing and power-on delay are limited to 60s, the poll
interval to 7d. A volume step starts from the last reported volume plus any
pending absolute target; before the receiver has reported a volume, a step is
rejected instead of guessing. Number ranges must fit inside the `volume:` range.

Before updating an existing installation, read the
[migration notes](maintenance.md#migration).

## Entities

Each platform takes `yamaha_serial_id: yamaha` and one sub-key per entity, each
with the usual ESPHome entity options such as `name` and `disabled_by_default`:

```yaml
select:
  - platform: yamaha_serial
    yamaha_serial_id: yamaha
    input_source:
      name: Yamaha Input Source
    program:
      name: Yamaha DSP Program
```

Start with the media player and the diagnostics, as in the basic example. Add
other entities only for controls you use, and do not add an entity that only
mirrors a state Home Assistant already shows. Some entities depend on the
receiver profile; see [Entities per Profile](supported_models.md#entities-per-profile).

### Media Player

| Key | Description |
|---|---|
| `receiver` | Power, mute, volume, volume step, play (power on) and next/previous DSP program, plus [`play_media` shortcuts](#media-player-shortcuts). |

The ESPHome media player has no source or sound-mode list. Use the
`input_source` and `program` selects for those, or the optional
[Home Assistant companion](home_assistant_companion.md) for one media player
with both.

### Select

| Key | Description |
|---|---|
| `input_source`, `zone2_input_source` | Input of the main zone and Zone2. |
| `program` | DSP program. |
| `scene` | System Memory recall, A-F (1-6 on the RX-V1900). The key is named `scene` for backward compatibility. |
| `audio_select` | Audio input: Auto, Coax/Opt, Analog, HDMI. |
| `night_mode` | Night listening mode: off, or Cinema/Music low, mid, high. |
| `tuner_preset` | Tuner preset, Preset 1 to 8. |
| `tuner_preset_page` | Tuner preset page, A-E. |
| `tuner_band` | FM or AM. |
| `sleep_timer` | Sleep timer. |
| `decoder_mode` | Input decoder mode: Auto, DTS, AAC. |
| `extended_surround` | Extended surround mode. |
| `speaker_b_assignment` | Speaker B terminals: main zone or Zone B. |
| `zone2_amp` | Internal or external Zone2 amplifier. |
| `surround_decoder` | RX-V1900 2-channel decoder: Pro Logic, PLII, Neo:6. |

### Switch

| Key | Description |
|---|---|
| `power`, `main_zone_power` | Receiver power and main zone power. The media player already covers power. |
| `mute` | Main zone mute. The media player already covers mute. |
| `zone2_power`, `zone2_mute` | Zone2 power and mute. |
| `pure_direct` | Pure Direct. |
| `speaker_a`, `speaker_b` | Speaker relays A and B. |
| `fan_mode` | Fan mode (RX-Vx600 extended). |

### Number

| Key | Description |
|---|---|
| `volume_db`, `zone2_volume_db` | Absolute volume in dB. For the main zone, the media player volume is the everyday control. |
| `bass`, `treble` | Tone controls. |
| `center_distance`, `front_left_distance`, `front_right_distance`, `surround_left_distance`, `surround_right_distance`, `subwoofer_distance` | Speaker distances. |
| `dimmer` | Front display dimmer. |
| `tuner_fm_frequency`, `tuner_am_frequency` | Tuner frequency; set `min_value`, `max_value` and `step` to your region. |

Tone, distance, dimmer and tuner frequency controls need an extended profile.

### Button

| Key | Description |
|---|---|
| `refresh` | Re-reads the receiver state. |
| `power_toggle` | Toggles power. |
| `volume_up`, `volume_down`, `zone2_volume_up`, `zone2_volume_down` | One volume step. |
| `tuner_auto_up`, `tuner_auto_down` | Tuner seek. |
| `receiver_reset` | **Factory reset** (extended profiles): the receiver recalls its factory presets and deletes every user setting. Keep it disabled unless you need it. |

### Sensor

| Key | Description |
|---|---|
| `volume_db`, `zone2_volume_db` | Current volume in dB. |
| `sampling_rate` | Sampling rate of the playing source in Hz. |
| `last_response_age` | Milliseconds since the last receiver reply. |
| `commands_sent`, `responses_received`, `parse_errors`, `timeouts`, `queue_drops` | Diagnostic counters. |

### Text Sensor

| Key | Description |
|---|---|
| `playback_format` | Format of the playing source, for example Dolby Digital or PCM. |
| `sampling_rate` | Sampling rate as the receiver's label. |
| `power_state`, `input_source`, `zone2_input_source`, `program`, `main_volume_text`, `audio_select_state`, `night_mode_state` | Text mirrors of states. Only useful when you do not use the matching control entity. |
| `connection_state` | Connection to the receiver, for example `waiting_for_receiver` or `online`. |
| `receiver_model` | Model the receiver reports. |
| `last_error` | Last command error, for example a command the profile does not support. |
| `last_parse_error` | Why the last frame was rejected, for example `DC4 checksum mismatch`. The frame itself is only in the log. |

### Binary Sensor

| Key | Description |
|---|---|
| `availability` | On while the receiver is reachable, judged from its valid replies. |

Keep the counters, `last_response_age`, `last_error` and `last_parse_error`
disabled by default (`disabled_by_default: true`) unless you are debugging;
[Maintenance](maintenance.md) shows how to keep them out of the recorder.

## Media Player Shortcuts

`media_player.play_media` on the receiver accepts these values as the media
content ID:

| Action | Examples |
|---|---|
| Input | `input:DVD`, `source:Tuner`, `select_source=DVD`, `yamaha://input/dvd`, `media-source://yamaha/source/dvd` |
| DSP program | `program:STRAIGHT`, `dsp:7ch Stereo`, `sound_mode=STRAIGHT`, `yamaha://program/straight`, `yamaha://sound_mode/straight` |
| System Memory | `scene:A`, `yamaha://scene/a` |
| Raw command | `yamaha://raw/stx/07E7E`, `yamaha://raw/dc4/20050000F` (needs `allow_raw_commands: true`) |

Inputs and programs match the same labels and keys as the selects.

## Raw Commands

Raw commands reach receiver functions that have no entity, such as I/O assign,
rename, volume trim, graphic EQ, DC trigger, Zone3, NET/USB/iPod/XM operation,
the tuner station block and DSP parameters. They are off by default; set
`allow_raw_commands: true` to use them, which needs an encrypted native API.

The `yamaha_serial.raw_command` action takes one of:

- a named alias, for example `power_on`, `program_munich`, `zone3_power_on`,
  `gui_cursor_up`, `ipod_play`, `net_usb_play` or `system_memory_save_3`;
- `stx:<payload>` for a standard operation command;
- `dc4:<payload>` for an extended command: the payload from the receiver's
  extended command table, without frame bytes or checksum, which the component
  adds;
- `dc1:000` for the ready probe;
- `dc3:reset`, a **factory reset** that deletes every user setting on the
  receiver.

The full example exposes it to Home Assistant as the API service
`yamaha_raw_command`, called with data such as `{"command": "power_on"}`,
`{"command": "alias:zone3_power_on"}` or `{"command": "stx:07E7E"}`.
[docs/command_matrix.md](command_matrix.md) lists which commands are entities,
aliases or raw only.

## Refresh and Polling

- At startup the component waits for the receiver's first reply, then reads its
  state.
- After that it follows the receiver's own report frames; with
  `poll_interval: never` nothing is polled.
- `poll_interval` adds a periodic refresh, and the `refresh` button reads the
  state on demand.
- While the receiver does not answer, the component only sends a short ready
  probe, and resumes the startup read on the next valid reply.

## Known Limitations

- Input and program IDs and region-specific blocks (XM, NET/USB) differ between
  models.
- Zone2 report behavior may vary by model and firmware.
- Tone, distance and dimmer controls are implemented for RX-Vx600-style
  receivers; on the RX-V1900, bass and treble were tested. The x700 and x800
  profiles need broader hardware validation.
