# Yamaha RS-232 Protocol Notes

This component implements Yamaha RS-232 communication as framed UART messages.

## Serial transport

Serial defaults used by Yamaha RX-Vx500 family:
- `9600 baud`
- `8 data bits`
- `No parity`
- `1 stop bit`
- full duplex
- hardware handshake documented by Yamaha (`RTS/CTS`)

## Framing

Frames are parsed non-blocking and may arrive fragmented or back-to-back.

Supported frame start bytes:
- `STX (0x02)` report/control responses
- `DC1 (0x11)` ready/text responses
- `DC2 (0x12)` configuration response
- `DC3 (0x13)` reset
- `DC4 (0x14)` extended protocol

Frame end byte:
- `ETX (0x03)`

The parser:
- resynchronizes on new start bytes
- tolerates multi-frame UART buffers
- enforces max frame length
- enforces frame assembly timeout
- validates DC4 checksums before dispatching extended payloads

`parse_errors` count malformed frames, checksum failures, impossible declared
payload lengths, interrupted frames and UART assembly timeouts. Normal Yamaha
status traffic should not increase the counter; repeated increments usually
point to logger/UART collisions, wrong wiring, wrong serial settings or a
receiver/model path sending a frame shape not covered by the active profile.

## Command queue and timing

The component uses a command queue with:
- configurable command spacing
- per-command timeout
- configurable retries
- single in-flight command
- poll command coalescing (duplicate poll tags are merged)
- queue overflow protection with `queue_drops` diagnostics counter
- no blocking sleeps in loop
- profile capability gating (prevents unsupported commands on mismatched receiver profiles)

## Ported command set

Primary commands in this port:

### Command coverage

| Command | Usage | ESPHome entity/action |
|---|---|---|
| `07E7E` | Power on | `switch.power` / `switch.main_zone_power` on |
| `07E7F` | Power off | `switch.power` / `switch.main_zone_power` off |
| `07EA2` | Mute on | `switch.mute` on |
| `07EA3` | Mute off | `switch.mute` off |
| `07A1A` | Volume up | `button.volume_up` |
| `07A1B` | Volume down | `button.volume_down` |
| `07ADA` | Zone2 volume up | `button.zone2_volume_up` |
| `07ADB` | Zone2 volume down | `button.zone2_volume_down` |
| `230xx` | Absolute main volume | `number.volume_db` |
| `231xx` | Absolute zone2 volume | `number.zone2_volume_db` |
| `07A14`..`07A55` | Main input select, including RX-V1600 `07A18` MD/TAPE | `select.input_source` |
| `07AC9` | RX-Vx500 MD/TAPE main input | `select.input_source` on x500 profiles |
| `07ACB`..`07AD9` | Zone2 input select | `select.zone2_input_source` |
| `07A87` | Multi CH input | `select.input_source` on x700/x800 profiles |
| `0F7F013FC0` / `0F7F0140BF` | NET/USB main / zone2 input | `select.input_source` on RX-V2700/RX-V3800-class profiles |
| `07AC8` / `07ACE` | BD/HD DVD main / zone2 input | `select.input_source` on x800 profile |
| `07EE0`..`07EFF` | DSP/Program select | `select.program` |
| `07E35`..`07E3A` | System Memory preset A-F | `select.scene` |
| `07EA6`/`07EA9`/`07EAA`/`07EDA` | Audio select mode | `select.audio_select` |
| `28B00`..`28B22` | Night mode | `select.night_mode` |
| `2B200`/`2B201` | Fan mode off/on | `switch.fan_mode` |
| `07E80`/`07E82` | Pure Direct on/off for RX-V1600-class models | `switch.pure_direct` |
| `07E82`/`07E83` | Pure Direct on/off for RX-V2500-class models | `switch.pure_direct` when model reports or is configured as RX-V2500 |
| `07EA0`/`07EA1` | Zone2 mute on/off | `switch.zone2_mute` |
| `07EAB`..`07EAE` | Speaker relay A/B on/off | `switch.speaker_a` / `switch.speaker_b` |
| `07EB3`..`07EB7` | Sleep timer | `select.sleep_timer` |
| `07EB8`/`07EB9`/`07E7C`/`07EDC`..`07EDE` | Extended surround mode | `select.extended_surround` |
| `07AE5`..`07AEC` | Tuner preset 1..8 | `select.tuner_preset` |
| `07AE0`..`07AE4` | Tuner preset page A-E | `select.tuner_preset_page` |
| `07EBC`/`07EBD` | Tuner band FM/AM | `select.tuner_band` |
| `07EBE`/`07EBF` | Tuner auto seek up/down | `button.tuner_auto_up` / `button.tuner_auto_down` |
| `07EDB`/`07EA8`/`07E3B` | Decoder mode Auto/DTS/AAC | `select.decoder_mode` |
| `07E28`/`07E29` | Speaker B assignment | `select.speaker_b_assignment` |
| `07E99`/`07E9A` | Zone2 amp mode | `select.zone2_amp` |
| `2006050000` | Current tuner station read | FM/AM frequency number state |
| `200D050100...` | Current tuner station write | FM/AM frequency number control |
| `2006033000`/`2006033001` | Bass/treble read | `number.bass`/`number.treble` state |
| `20090331001xx`/`20090331011xx` | Bass/treble write | `number.bass`/`number.treble` control |
| `20060410xx` | Speaker distance read | distance number states |
| `20090411xx` | Speaker distance write | distance number controls |
| `22001`/`22002` | Text volume poll | startup sync, manual refresh, optional periodic refresh |
| `22003`/`22004` | Text input poll | startup sync, manual refresh, optional periodic refresh |
| `20000`/`20100` | Report mode setup | startup bootstrap, manual refresh, optional periodic refresh |
| Named STX aliases | Finite RX-Vx500/x600/x700/x800 operation-table commands | `yamaha_serial.raw_command` using `power_on`, `zone3_power_on`, `gui_cursor_up`, `ipod_play`, `net_usb_play`, etc. |
| Named system aliases | Finite SW=2 setup/text/report commands | `yamaha_serial.raw_command` using `report_delay_100ms`, `firmware_version_request`, `speaker_center_small`, etc. |
| Named DC4 read aliases | Extended information block reads | `yamaha_serial.raw_command` using `ext_read_model_name`, `ext_read_dsp_user_parameters`, etc. |
| Any STX operation command | Standard RX-Vx600/Vx500 command not exposed as an entity | `yamaha_serial.raw_command` using `stx:<payload>` |
| Any DC4 extended command | Extended command matrix | `yamaha_serial.raw_command` using `dc4:<payload>` |
| `DC1 000` | ready/probe | `yamaha_serial.raw_command` using `dc1:000` |
| `DC3 DEL DEL DEL` | receiver reset | `yamaha_serial.raw_command` using `dc3:reset` |

### Symbolic raw command aliases

The runtime includes a named alias catalog for finite operation commands found
across the local RX-Vx500, RX-Vx600, RX-Vx700, RX-Vx800 and RX-V1900 command
tables. These aliases are routed through the same command queue, retry, timeout
and rate-limit handling as typed controls.

Accepted forms:

```yaml
on_...:
  then:
    - yamaha_serial.raw_command:
        id: yamaha
        command: zone3_power_on
    - yamaha_serial.raw_command:
        id: yamaha
        command: alias:gui_cursor_up
```

The optional API service in the full examples accepts the same command strings,
so Home Assistant service data can use:

```yaml
command: net_usb_play
```

For media-player dashboard shortcuts, the URI helper can use the raw alias form:

```yaml
media_content_id: yamaha://raw/alias/ipod_play
media_content_type: command
```

Alias families include main/Zone2/Zone3 power, input, volume, mute, DSP/program
direct select, tuner, speaker relay, sleep, decoder, extended-surround,
night-mode, system/home memory, volume memory, trigger, speaker setup, video and
HDMI setup, dynamic range, iPod/XM/NET/USB setup, Zone2/Zone3 tone, GUI cursor,
XM, iPod and NET/USB operation commands. Where Yamaha reused payloads with
different generation-specific names, both names are kept, for example
`program_hall_a` and `program_munich`, x500 `home_preset_memory_f` beside
x700/x800 `system_memory_save_6`, or x500 `language_english` beside x700/x800
`multi_ch_bgv_off`.

Generation-specific standard-command differences are kept explicit. RX-Vx600
profiles use the RX-V1600-practical `07A18` MD/TAPE input command. RX-Vx500
profiles use `07AC9` for MD/TAPE main input and `07ACF` for Zone2 MD/TAPE.
RX-V2500 Pure Direct uses `07E82` for ON and `07E83` for OFF, while
RX-V1600-class Pure Direct keeps `07E80` and `07E82`.

DC4 information block reads are also exposed as aliases, for example
`ext_read_model_name`, `ext_read_input_audio_optical`,
`ext_read_dsp_user_parameters` and `ext_read_xm_current_song_name`.

Parametric extended setup commands remain available through `dc4:<payload>`
because those frames carry caller-supplied values such as distance, trim,
frequency, rename text and assignment data.

See [command_matrix.md](command_matrix.md) for the practical split between HA
entities, named aliases and raw payload-only command families.

### Home Assistant media player mapping

ESPHome's current C++ `media_player` API exposes power, mute, volume and generic
media commands, but it does not expose persisted source or sound-mode state fields.
The component therefore keeps receiver source/DSP state in `select` and
`text_sensor` entities while routing practical media-player actions through the
same Yamaha command mappings:

| HA media-player action | Yamaha routing |
|---|---|
| turn on/off/toggle | `07E7E` / `07E7F` / local known-state toggle |
| mute/unmute | `07EA2` / `07EA3` |
| volume set | normalized HA volume converted to `230xx` dB volume |
| volume up/down | `07A1A` / `07A1B` |
| play | power on |
| next/previous | next/previous configured DSP program |
| play media `input:DVD`, `source:Tuner`, `select_source=DVD`, `yamaha://input/dvd`, `media-source://yamaha/source/dvd` | main-zone input select |
| play media `program:STRAIGHT`, `dsp:7ch Stereo`, `sound_mode=STRAIGHT`, `yamaha://program/straight`, `yamaha://sound_mode/straight` | DSP/program select |
| play media `scene:A`, `yamaha://scene/a` | System Memory preset |
| play media `yamaha://raw/stx/07E7E` | raw standard STX command |
| play media `yamaha://raw/dc4/20050000F` | raw extended DC4 command with checksum added by component |

When the receiver is powered on, the ESPHome media-player state is published as
`IDLE`: the AVR is available for commands, but the serial protocol does not
provide a reliable playback state comparable to a streaming speaker. The
component does not advertise native `SELECT_SOURCE` or `SELECT_SOUND_MODE`
feature flags until ESPHome exposes those fields in `MediaPlayerCall`; use the
select entities or the `play_media` shortcut strings above.

### Main power and mute
- `07E7E` main power on
- `07E7F` main power off
- `07EA2` mute on
- `07EA3` mute off

### Main volume
- `07A1A` volume up
- `07A1B` volume down
- `230XX` absolute master volume

Volume conversion:
- `raw = ((dB + 80.0) * 2) + 0x27`
- `dB = (raw - 0x27) / 2 - 80.0`

### Input select (main)
- `07A14` PHONO
- `07A15` CD
- `07A16` TUNER
- `07A19` CD-R
- `07A18` MD/TAPE
- `07AC1` DVD
- `07A54` D-TV/LD
- `07AC0` CBL/SAT
- `07A0F` VCR1
- `07A13` VCR2/DVR
- `07A55` V-AUX

### Zone2
- `07EBA` zone2 power on
- `07EBB` zone2 power off
- `07EA0` zone2 mute on
- `07EA1` zone2 mute off
- `07ADA` zone2 volume up
- `07ADB` zone2 volume down
- `231XX` zone2 absolute volume
- zone2 input commands (`07ADx`/`07ACx`) per Yamaha table

### Standard setup/control selectors
- `07EAB` / `07EAC` speaker relay A on/off
- `07EAD` / `07EAE` speaker relay B on/off
- `07EB3`..`07EB7` sleep timer off/120/90/60/30
- `07EBC` / `07EBD` tuner band FM/AM
- `07AE0`..`07AE4` tuner preset page A-E
- `07EBE` / `07EBF` tuner auto seek up/down
- `07EDB` / `07EA8` / `07E3B` decoder mode Auto/DTS/AAC
- `07EB8`, `07EB9`, `07E7C`, `07EDC`, `07EDD`, `07EDE` extended surround modes
- `07E28` / `07E29` speaker B assignment Main/Zone B
- `07E99` / `07E9A` Zone2 amp Internal/External

### Program / Surround
Ported from reference mapping:
- `07EE5` Vienna
- `07EEC` Bottom Line
- `07EED` Roxy Theater
- `07EFF` 7ch Stereo
- `07EE0` Straight
- `07EC2` THX Cinema
- `07EC8` THX Game
- `07EC0` 2ch Stereo
- `07EFD` ProLogic
- `07EFE` Standard
- `07EF0` Disco
- `07EF2` Game
- `07EF7` Mono Movie
- `07EF9` Spectacle
- `07EFA` Sci-Fi
- `07EFB` Adventure
- `07EF8` Sports
- `07EF3` Music Pop
- `07EFC` General
- `07EC3` THX Music

### System Memory preset
- `07E35` A
- `07E36` B
- `07E37` C
- `07E38` D
- `07E39` E
- `07E3A` F

## Status handling

Status is updated via:
- asynchronous report frames (`STX`)
- text responses (`DC1`) for volume/input names
- bootstrap/config response (`DC2`)
- extended query responses (`DC4`) for RX-Vx600 setup/status data

Parser diagnostics:
- `parse_errors` counts rejected or incomplete frames.
- `last_parse_error` stores the last rejection reason and, when available, the raw frame bytes as hex.
- A valid later response does not reset `last_parse_error`; it is intentionally kept for debugging intermittent startup/noise issues.

Startup/manual refresh queues:
- report enable (`20000`)
- main volume text request (`22001`)
- main input name text request (`22003`)
- zone2 input name text request (`22004`)
- zone2 volume text request (`22002`)

By default `poll_interval: never` disables periodic status polling after the startup
sync. RX-Vx600 receivers send asynchronous report frames after report mode has been
enabled, so physical front-panel or remote-control changes are expected to arrive
without a recurring poll burst. Set `poll_interval` to a time value only when a model
does not reliably report changes by itself.

## Extended protocol

The component now implements RX-Vx600 style extended control blocks (`DC4`) for:
- model query (`20050000F`)
- tone control (`033`: bass/treble)
- speaker distance (`041`)

The RX-Vx600 extended command set contains additional command groups that are model,
region, option-board, or setup-menu dependent: system information, input
information, DSP information, XM information, I/O assign, rename, volume trim,
user/maker DSP parameters, parameter initialize, graphic EQ, DC trigger, tuner
station and XM preset. Those are supported at transport level through the raw
command sender:

```text
stx:07E7E
dc4:20050000F
dc1:000
dc3:reset
```

For `dc4:` commands, pass the Yamaha payload without checksum and without frame
bytes; the component adds `DC4`, the checksum and `ETX`. This makes every command
sendable while keeping high-level HA entities limited to controls that have stable
semantics on RX-V1600-class hardware.

Model names from extended replies may include Yamaha's two-character prefix, for example `08RX-V1600`; the component normalizes that to `RX-V1600` for the public diagnostic entity.

Receiver-specific behavior is modularized by profile:
- `rx_vx500`: standard x500 command set baseline.
- `rx_vx500_extended`: auto-selected x500 profile slot; same typed capability as standard x500 until additional x500-only controls are validated.
- `rx_vx600`: standard x600-compatible baseline with DC4 setup entities disabled.
- `rx_vx600_extended`: auto-selected x600 profile with DC4 setup controls and RX-V1600 DC2 offset decode.
- `rx_vx700`: standard x700 input/DSP map with DC4 setup entities disabled.
- `rx_vx700_extended`: auto-selected x700 profile with DC4 raw/setup transport enabled.
- `rx_vx800`: standard x800 input/DSP map with DC4 setup entities disabled.
- `rx_vx800_extended`: auto-selected x800 profile with DC4 raw/setup transport enabled.

In `auto` mode, the configured `model` value is only a startup hint. Once a DC2
or DC4 model response is received, the normalized receiver model is used to
reselect the matching extended profile and refresh input/program options.

When adding new models, create or extend a profile and map only the model-specific command/report differences while keeping parser/queue core unchanged.

## RX-Vx700 / RX-Vx800 profile notes

The x700 and x800 profiles keep the same 9600 8N1 framed transport but add new
source/report values:

- x700 main input reports include `0D NET/USB`, `0E XM` and `10 Multi CH`.
- x800 main input reports include `0D NET/USB`, `0E XM`, `0F BD/HD DVD` and `10 Multi CH`.
- Because `10 Multi CH` needs both data nibbles, the parser decodes input reports
  using the complete `RDAT0,RDAT1` byte, not only the lower nibble.
- Power reports `04`..`07` represent Zone3 combinations; the component reduces
  those correctly to Main/Zone2 HA state while leaving Zone3 as raw/manual scope.

The extended operation tables for x700/x800 use 9-byte STX payloads such as
`0F7F013FC0` for NET/USB source selection. These are accepted by the normal command
queue and by `yamaha_serial.raw_command` with the `stx:` prefix.
