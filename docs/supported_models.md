# Supported Receivers

This component controls Yamaha AV receivers through the RS-232 port on the
back. The **RX-V1600 is the reference receiver**: every configuration in
[`examples/`](../examples) is written for it. Other receivers use the same
component with a different receiver profile. This page lists them, shows which
entities each profile supports, and gives a configuration for each receiver
family.

## Models

| Model | Status | `auto` selects | Standard profile | Notes |
|---|---|---|---|---|
| RX-V1500 | expected | `rx_vx500_extended` | `rx_vx500` | x500 input/program map; MD/TAPE main input uses `07AC9`. See [RX-Vx500](#rx-v1500-and-rx-v2500-rx-vx500). |
| RX-V1600 | expected | `rx_vx600_extended` | `rx_vx600` | Reference receiver for the examples. MD/TAPE main input uses `07A18`; DC4 setup controls and DC2 startup decode in the extended profile. |
| RX-V1700 | expected | `rx_vx700_extended` | `rx_vx700` | x700 input/program map. See [RX-Vx700 and RX-Vx800](#rx-vx700-and-rx-vx800). |
| RX-V1800 | expected | `rx_vx800_extended` | `rx_vx800` | x800 input/program map; no night mode. |
| RX-V1900 | tested | `rx_v1900_extended` | `rx_v1900` | Tested on one G/E (UK/Europe) unit. See [RX-V1900](#rx-v1900). |
| RX-V2500 | expected | `rx_vx500_extended` | `rx_vx500` | As RX-V1500; Pure Direct uses the RX-V2500 `07E82`/`07E83` pair. |
| RX-V2600 | expected | `rx_vx600_extended` | `rx_vx600` | As RX-V1600. |
| RX-V2700 | expected | `rx_vx700_extended` | `rx_vx700` | As RX-V1700. |
| RX-V3800 | expected | `rx_vx800_extended` | `rx_vx800` | As RX-V1800. |
| Other RS-232 Yamaha receivers | unknown | `rx_vx500_extended` fallback | `rx_vx500` fallback | Verify the input and program mappings, and set an explicit profile if detection picks the wrong family. |

Status:

- `tested`: verified with real hardware using this component.
- `expected`: the command mapping follows the receiver family's RS-232
  protocol, but needs device validation.
- `unknown`: no validation yet.

No receiver is validated for the whole command surface; see
[SUPPORT.md](../SUPPORT.md#validated-hardware). A report with a `uart: debug`
trace of your receiver's report frames is what lets a profile be corrected.

## Choosing a Profile

Set `model:` to your receiver and keep `receiver_profile: auto`. The component
starts with the model you set, and switches to the matching extended profile
when the receiver reports its real model. Labels you set under `inputs:` are
kept across that switch.

Set `receiver_profile` explicitly when detection picks the wrong family, or
when you only want the standard command set. An explicit profile never changes
at runtime.

| Family | Standard profile | Extended profile | `auto` selects |
|---|---|---|---|
| RX-Vx500 | `rx_vx500` | `rx_vx500_extended` | `rx_vx500_extended` |
| RX-Vx600 | `rx_vx600` | `rx_vx600_extended` | `rx_vx600_extended` |
| RX-Vx700 | `rx_vx700` | `rx_vx700_extended` | `rx_vx700_extended` |
| RX-Vx800 | `rx_vx800` | `rx_vx800_extended` | `rx_vx800_extended` |
| RX-V1900 | `rx_v1900` | `rx_v1900_extended` | `rx_v1900_extended` |

A standard profile uses only the common operation commands (STX). An extended
profile adds the family's extended commands (DC4): tone, speaker distance,
dimmer and tuner frequency controls, and startup status decoding where the
family supports it.

## Entities per Profile

Every profile supports the media player, input and DSP program selects, System
Memory (`scene`), audio select, Pure Direct, the Zone2 controls (power, volume,
input, mute, amp), the tuner preset, page, band and seek controls, extended
surround, speaker B assignment and all diagnostics. The table lists the
entities that depend on the profile:

| Entity keys | RX-Vx500 | RX-Vx600 | RX-Vx700 | RX-Vx800 | RX-V1900 |
|---|---|---|---|---|---|
| `night_mode` | yes | yes | yes | no | no |
| `speaker_a`, `speaker_b` | yes | yes | yes | yes | no |
| `sleep_timer` (untested on x500-x800) | yes | yes | yes | yes | no |
| `decoder_mode` | yes | yes | yes | yes | no |
| `surround_decoder` | no | no | no | no | yes |
| `input_channels`, `bit_rate`, `signal_flags`, `dialogue_normalization` | no | no | yes | yes | yes |
| `fan_mode` | no | extended | no | no | no |
| `bass`, `treble`, speaker distances, `dimmer`, `tuner_fm_frequency`, `tuner_am_frequency`, `receiver_reset` | no | extended | extended | extended | extended |

"extended" means only the extended profile. A "yes" means the profile sends the
commands; the status column above says what was tested on hardware. An entity
whose command the profile rejects stays in Home Assistant, and using it sets
`last_error` to a "not supported by active profile" message. Leave such
entities out of your configuration. [Configuration](configuration.md#entities)
describes every entity key.

## Configuration per Receiver

### RX-V1600 and RX-V2600 (RX-Vx600)

Use the examples as they are:

- [`examples/yamaha_rx_v_basic.yaml`](../examples/yamaha_rx_v_basic.yaml):
  ESP32, the media player and diagnostics. Start here.
- [`examples/yamaha_rx_v_full.yaml`](../examples/yamaha_rx_v_full.yaml):
  ESP8266 on UART0, every entity the RX-V1600 supports, with the
  `rx_vx600_extended` profile.

For an RX-V2600, change `model:` to `"RX-V2600"`.

### RX-V1500 and RX-V2500 (RX-Vx500)

Start from [`examples/yamaha_rx_v_basic.yaml`](../examples/yamaha_rx_v_basic.yaml)
and replace its `yamaha_serial:`, `sensor:` and `text_sensor:` blocks with the
blocks below; add the `select:`, `switch:` and `button:` blocks. For an
RX-V2500, set `model: "RX-V2500"`. The x500 profiles have no fan, tone,
speaker distance, dimmer, tuner frequency or receiver reset controls.

<!-- ci-config: yamaha_rx_vx500.yaml from yamaha_rx_v_basic.yaml -->
```yaml
yamaha_serial:
  id: yamaha
  uart_id: uart_yamaha
  model: "RX-V1500"
  receiver_profile: rx_vx500_extended
  poll_interval: never
  diagnostics_interval: 60s
  allow_raw_commands: false
  command_timeout: 1500ms
  max_retries: 2
  command_spacing: 100ms
  power_on_delay: 2s
  inputs:
    phono: "PHONO"
    cd: "CD"
    tuner: "Tuner"
    cdr: "CD-R"
    md_tape: "MD/TAPE"
    dvd: "DVD"
    dtv_ld: "D-TV/LD"
    cbl_sat: "CBL/SAT"
    sat: "SAT"
    vcr1: "VCR1"
    vcr2_dvr: "VCR2/DVR"
    vcr3: "VCR3"
    vaux: "V-AUX"
  volume:
    min: -80
    max: 16
    step: 0.5

select:
  - platform: yamaha_serial
    yamaha_serial_id: yamaha
    input_source:
      name: Yamaha Input Source
    program:
      name: Yamaha DSP Program
    scene:
      name: Yamaha System Memory
    audio_select:
      name: Yamaha Audio Select
    night_mode:
      name: Yamaha Night Mode
    tuner_preset:
      name: Yamaha Tuner Preset
    tuner_preset_page:
      name: Yamaha Tuner Preset Page
    tuner_band:
      name: Yamaha Tuner Band
    sleep_timer:
      name: Yamaha Sleep Timer
    decoder_mode:
      name: Yamaha Decoder Mode
    extended_surround:
      name: Yamaha Extended Surround
    speaker_b_assignment:
      name: Yamaha Speaker B Assignment
    zone2_amp:
      name: Yamaha Zone2 Amp

switch:
  - platform: yamaha_serial
    yamaha_serial_id: yamaha
    zone2_power:
      name: Yamaha Zone2 Power
    zone2_mute:
      name: Yamaha Zone2 Mute
    pure_direct:
      name: Yamaha Pure Direct
    speaker_a:
      name: Yamaha Speaker A
    speaker_b:
      name: Yamaha Speaker B

button:
  - platform: yamaha_serial
    yamaha_serial_id: yamaha
    refresh:
      name: Yamaha Refresh
    tuner_auto_up:
      name: Yamaha Tuner Auto Up
    tuner_auto_down:
      name: Yamaha Tuner Auto Down

sensor:
  - platform: yamaha_serial
    yamaha_serial_id: yamaha
    sampling_rate:
      name: Yamaha Sampling Rate
    last_response_age:
      name: Yamaha Last Response Age
      disabled_by_default: true

text_sensor:
  - platform: yamaha_serial
    yamaha_serial_id: yamaha
    connection_state:
      name: Yamaha Connection State
    receiver_model:
      name: Yamaha Receiver Model
    playback_format:
      name: Yamaha Playback Format
    last_error:
      name: Yamaha Last Error
      disabled_by_default: true
    last_parse_error:
      name: Yamaha Last Parse Error
      disabled_by_default: true
```

This configuration leaves raw commands off. To reach receiver functions that
have no entity, set `allow_raw_commands: true`, which needs an encrypted native
API, and add an action or API service; see
[Raw commands](configuration.md#raw-commands).

### RX-Vx700 and RX-Vx800

The RX-V1700, RX-V2700, RX-V1800 and RX-V3800 are not validated on hardware
yet. Start from [`examples/yamaha_rx_v_basic.yaml`](../examples/yamaha_rx_v_basic.yaml),
set `model:` to your receiver and keep `receiver_profile: auto`. Remove the
`inputs:` block: it uses RX-V1600 keys such as `dtv_ld` and `vaux`, which the
x700 and x800 profiles name differently, and the profiles already carry their
own labels. Then add the entities you need from
[`examples/yamaha_rx_v_full.yaml`](../examples/yamaha_rx_v_full.yaml), following
[Entities per Profile](#entities-per-profile).

Zone3, XM, NET/USB/iPod operation, trigger outputs, memory save and DSP
parameter editing are not entities; they stay available through
[raw commands](configuration.md#raw-commands).

### RX-V1900

Tested on one RX-V1900 G/E (UK/Europe) unit. U/C/J models add regional decoder
modes, inputs and HD Radio, which are untested; see
[the RX-V1900 protocol notes](protocol.md#rx-v1900-profile-notes).

Before you start, set **ADVANCED SETUP > RS-232C STANDBY** to **YES** on the
receiver. The default is NO outside the US and Canada, and without it the
receiver cannot be controlled from standby.

Start from [`examples/yamaha_rx_v_basic.yaml`](../examples/yamaha_rx_v_basic.yaml)
and replace its `yamaha_serial:`, `sensor:` and `text_sensor:` blocks with the
blocks below; add the `switch:`, `button:`, `number:` and `select:` blocks.
This is a main-zone setup. Zone2, System Memory (labels 1-6) and bass/treble
work too; add them from
[`examples/yamaha_rx_v_full.yaml`](../examples/yamaha_rx_v_full.yaml).

<!-- ci-config: yamaha_rx_v1900.yaml from yamaha_rx_v_basic.yaml -->
```yaml
yamaha_serial:
  id: yamaha
  uart_id: uart_yamaha
  model: "RX-V1900"
  receiver_profile: rx_v1900_extended
  poll_interval: never
  diagnostics_interval: 60s
  allow_raw_commands: false
  command_timeout: 1500ms
  max_retries: 2
  command_spacing: 120ms
  power_on_delay: 2s
  volume:
    min: -80
    max: 16.5
    step: 0.5

switch:
  - platform: yamaha_serial
    yamaha_serial_id: yamaha
    pure_direct:
      name: Yamaha Pure Direct

button:
  - platform: yamaha_serial
    yamaha_serial_id: yamaha
    refresh:
      name: Yamaha Refresh

number:
  - platform: yamaha_serial
    yamaha_serial_id: yamaha
    tuner_fm_frequency:
      name: Yamaha FM Frequency
      mode: box
      min_value: 87.5
      max_value: 108
      step: 0.05
    tuner_am_frequency:
      name: Yamaha AM Frequency
      mode: box
      min_value: 531
      max_value: 1611
      step: 9

select:
  - platform: yamaha_serial
    yamaha_serial_id: yamaha
    input_source:
      name: Yamaha Input
    program:
      name: Yamaha Program
    audio_select:
      name: Yamaha Audio Select
    tuner_band:
      name: Yamaha Tuner Band
    surround_decoder:
      name: Yamaha Surround Decoder

sensor:
  - platform: yamaha_serial
    yamaha_serial_id: yamaha
    sampling_rate:
      name: Yamaha Sampling Rate
    last_response_age:
      name: Yamaha Last Response Age
      disabled_by_default: true

text_sensor:
  - platform: yamaha_serial
    yamaha_serial_id: yamaha
    playback_format:
      name: Yamaha Playback Format
    input_channels:
      name: Yamaha Input Channels
    bit_rate:
      name: Yamaha Bit Rate
    receiver_model:
      name: Yamaha Receiver Model
    connection_state:
      name: Yamaha Connection State
    last_error:
      name: Yamaha Last Error
      disabled_by_default: true
    last_parse_error:
      name: Yamaha Last Parse Error
      disabled_by_default: true
```

Notes for this configuration:

- There is no `inputs:` block. The profile already has the RX-V1900's inputs:
  PHONO, CD, Tuner, MD/CD-R, DVD, BD/HD DVD, TV, CBL/SAT, VCR, DVR, V-AUX,
  USB, Multi CH and DOCK. The basic example's RX-V1600 keys, such as
  `dtv_ld`, do not exist in this profile.
- The extended profile is needed for the FM/AM frequency controls.
- `volume.max: 16.5` is the full receiver range; lower it to cap the volume
  from Home Assistant.
- Tuner ranges are regional. Europe/UK: FM 87.5-108 MHz in 0.05 MHz steps, AM
  531-1611 kHz in 9 kHz steps. US/Canada (untested): FM 87.5-107.9 MHz in
  0.2 MHz steps, AM 530-1710 kHz in 10 kHz steps. Tuning on the front panel
  sends no serial report, so the frequency refreshes only on Refresh or after
  you set it from Home Assistant.
- The surround decoder (Pro Logic, PLII and Neo:6) applies to 2-channel
  sources, analog or digital. Selecting one switches the program to Surround
  Decode first. On a multichannel source the receiver keeps and reports the
  stored setting, but decodes the source format.
- Input Channels and Bit Rate describe the playing source, for example
  `5.1 (3/2.1)` at `640 kbps` for Dolby Digital. The `signal_flags` text
  sensor and the `dialogue_normalization` sensor are also available.

What was tested on the G/E unit: model detection (DC2 ID `R0240`), the DC2
startup values, input and program reports and select commands (main zone and
Zone2, including DOCK and 7ch Enhancer), playback format and sampling rate,
input channels, bit rate and dialogue normalization (analog, PCM, Dolby Digital
and DTS, from reports and from the DC2 reply at startup; signal flags were only
seen as `None`), tuner band, System Memory 1-6 (recall and remote reports), all
six surround decoder modes including Pro Logic, and bass/treble.

Not supported on the RX-V1900: the Auto/DTS/AAC decoder mode (the receiver
ignores it), the sleep timer (its Off is never reported back; use a Home
Assistant automation instead), Speaker A/B (the receiver has none), fan
control, night mode and Zone3. Speaker distances are not validated yet.
