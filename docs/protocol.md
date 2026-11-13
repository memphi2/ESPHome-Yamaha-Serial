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

Only successfully parsed frames update availability. A pending command completes
only on its matching frame type, report/command ID, and known value or Extended
selector (for example speaker channel). An unrelated unsolicited report cannot
acknowledge a mute or volume command. Matching guarded/error replies end the wait
but remain errors. Invalid frames never acknowledge commands.

There is no transaction identifier: a coincident matching unsolicited report
cannot be distinguished from a solicited reply. An acknowledgement is not a
transactional guarantee. Unclassified STX operations have no inferred
acknowledgement. Unknown DC4 writes match only the command ID, without claiming
selector/value verification. Both are sent once. Reads and recognized absolute setters may retry; relative
steps, toggles and unclassified writes never retry. This policy applies equally
to aliases and typed controls. Typed volume steps use absolute targets respecting
the configured limits and step. Raw relative-volume aliases remain available.

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
- validates DC2/DC4 lengths and checksums before dispatch
- accepts DC4 data up to 256 bytes (`00` length), preserving spaces and case
- bounds RX at 268 bytes (maximum DC2 frame) and raw input at 320 bytes

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
- poll coalescing by frame type and exact payload, never across a pending write
- queue overflow protection with `queue_drops` diagnostics counter
- no blocking sleeps in loop
- profile capability gating for typed controls (raw commands bypass capabilities)

Power-on holdoff also applies to raw aliases, and the response timeout includes
the configured startup delay. The queue is limited to 32 pending commands;
overflow discards an old refresh first, otherwise the oldest pending command.

## Ported command set

Primary commands in this port:

### Command coverage

| Command | Usage | ESPHome entity/action |
|---|---|---|
| `07E7E` | Power on | `switch.power` / `switch.main_zone_power` on |
| `07E7F` | Power off | `switch.power` / `switch.main_zone_power` off |
| `07EA2` | Mute on | `switch.mute` on |
| `07EA3` | Mute off | `switch.mute` off |
| `07A1A` | Relative volume up | raw alias `volume_up` |
| `07A1B` | Relative volume down | raw alias `volume_down` |
| `07ADA` | Relative Zone2 volume up | raw alias `zone2_volume_up` |
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
| `07E35`..`07E3A` | System Memory preset A-F (1-6 on RX-V1900) | `select.scene` |
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
| `07E67`/`07E68`/`07EC7`/`07E69`/`07E6A` | 2ch surround decoder PLII Movie/Music/Game, Neo:6 Cinema/Music (x700+) | `select.surround_decoder` |
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
- `last_parse_error` stores the rejection reason, limited to HA's 255-byte state bound. Rejected bytes are logged separately in bounded hex chunks, including offset and total length.
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

Raw entry points require `allow_raw_commands: true`. DC4 bodies keep case and
spaces exactly, including trailing padding; the declared length must match.
For example, `dc4:2011011100508Hi Fi PC` preserves the eight-character name.
Length/framing checks do not establish that a write is safe for your model.

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
- `rx_v1900`: standard RX-V1900 input and DSP program maps; DC4 setup entities and DC2 offset decode disabled.
- `rx_v1900_extended`: auto-selected RX-V1900 profile with DC4 raw/setup transport and DC2 offset decode.

In `auto` mode, the configured `model` value is only a startup hint. Once a DC2
or DC4 recognized model response is received, the normalized receiver model is
used to reselect the matching extended profile and refresh input/program options.
Known internal DC2 IDs are translated (for example `R0240` is RX-V1900); `R0177` identifies the x500 family, not a
unique model. Unknown IDs keep the configured model hint. Explicit profiles stay fixed.
Changed option lists invalidate removed selections and request one native-API
client reconnect so HA fetches fresh metadata. Repeated identical reports do not reconnect.

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

## RX-V1900 profile notes

Everything in this section was measured on one RX-V1900 G/E (UK/Europe) unit.
Other regional variants differ; see the last item. The RX-V1900 reports DC2
model ID `R0240` and follows the x800 command generation, with its own main
input map:

- Main input reports (checked on hardware): `00 PHONO`, `01 CD`, `02 Tuner`,
  `03 MD/CD-R`, `05 DVD`, `06 TV`, `07 CBL/SAT`, `09 VCR`, `0A DVR`, `0C V-AUX`,
  `0D USB`, `10 Multi CH`, `11 BD/HD DVD` and `12 DOCK`. There is a single
  MD/CD-R input (`07A19` / zone 2 `07AD4`); `04` MD/TAPE is never reported.
- BD/HD DVD reports `11` in the main zone but `0F` in zone 2 (commands `07AC8` /
  `07ACE`).
- Zone 2 input commands are ignored while zone 2 is powered off.
- DOCK reports `12` in the main zone but `08` in zone 2. It is selected with the
  long remote-code form `0F7F014AB5` (main) and `0F7F014BB4` (zone 2), the codes
  the remote and Yamaha's Receiver Manager send. `07A55` selects V-AUX, not DOCK.
- In the DC2 configuration reply, the main input ID spans two hex nibbles
  (DC2 data DT9/DT10, frame bytes 18-19). RX-Vx500/RX-Vx600 use a single nibble
  at frame byte 18, so the two-nibble decode is a per-profile capability.
  The RX-Vx700 RS-232C documentation uses the same two-nibble layout, but the
  x700/x800 profiles do not enable DC2 offset decode yet.
- The remaining DC2 offsets used by the extended decode (power, audio select,
  mute, volume, program, dimmer, fan, Pure Direct) match the RX-V1600 layout.
- The DC2 reply also gives these startup values, checked against RX-V1900
  captures with known settings. DTn is frame byte 9+n.

  | Field | DT | Frame byte(s) |
  |---|---|---|
  | Zone 2 input (one nibble) | DT13 | 22 |
  | Zone 2 mute | DT14 | 23 |
  | Zone 2 volume | DT17-18 | 26-27 |
  | Extended Surround | DT22 | 31 |
  | System Sleep, last time set (not used, see below) | DT24 | 33 |
  | Tuner preset page | DT25 | 34 |
  | Tuner preset | DT26 | 35 |
  | Playback format | DT31-32 | 40-41 |
  | Sampling rate | DT33-34 | 42-43 |
  | Tuner band | DT37 | 46 |
  | Sleep, remote timer (not used, see below) | DT186 | 195 |

  Each value uses the same encoding as the matching STX report.
- Night mode isn't read from the DC2 reply on the RX-V1900, because its RX-V1600
  position (frame bytes 36-37) holds DT27-28 in this layout. The RX-V1900 has no
  night mode anyway.
- DSP program reports use x700 codes except the enhancers: Straight Enhancer is
  `44` and 7ch Enhancer is `45`. `07ED8` selects Straight Enhancer. 7ch Enhancer
  is only accepted in the long remote-code form `0F7E81146B` (captured from
  Receiver Manager); the short form `07E14` and `07ED9` get no reply.
- DSP program reports and select commands, checked on hardware, in front-panel
  order: `25` Sci-Fi, `28` Adventure, `29` Drama, `20` Mono Movie, `34` 2ch
  Stereo, `17` 7ch Stereo, `44` Straight Enhancer, `45` 7ch Enhancer, `2C`
  Surround Decode, `00` Munich, `05` Vienna, `07` Amsterdam, `09` Freiburg, `0B`
  Chamber, `0D` Village Vanguard, `11` Warehouse Loft, `0F` Cellar Club, `10` The
  Roxy Theatre, `0E` The Bottom Line, `21` Sports, `1E` Action Game, `1F`
  Roleplaying Game, `18` Music Video, `1C` Recital/Opera, `2D` Standard, `24`
  Spectacle. STRAIGHT (`80`) is reached with the EFFECT key. The RX-V1900 has no
  Disco, Game or THX programs, so its profile leaves them out.
- Playback format (`10`) and sampling rate (`11`) reports use the RX-Vx700 tables
  with the full report byte; this also applies to the x700 and x800 profiles.
- The tuner band report (`35`) is `00` AM / `01` FM on this unit, the reverse of
  the x500-x800 manuals (`00` FM). Only the RX-V1900 profile uses the reversed
  mapping (`tuner_band_fm_is_1`). The `07EBC` (FM) and `07EBD` (AM) commands are
  unchanged. The DC4 tuner-station field (band `1` = FM) is a separate encoding.
- The tuner-station write (`050`) is sent without the trailing Program field that
  the RX-V1900 Extended command table lists for HD Radio (US) models. FM and AM
  station writes (for example `200D0501001095.00` and `200D0501000000900`) tune
  the G/E unit and read back correctly; HD Radio models are untested.
- Tuning from the front panel sends no serial report, so the tuner frequency is
  only refreshed by the extended tuner-station read (`2006050000`). That read
  returns only the current station, so the AM frequency stays unknown while the
  tuner is on FM, and the FM frequency while it is on AM.
- The RX-V1900 has two sleep timers, and the profile exposes neither
  (`sleep_control` is off; use a Home Assistant automation instead):
  - System Sleep ("System Sleep" on the display) is set by `07EB4`-`07EB7`
    (120/90/60/30) and `07EB3` (Off), acknowledged with `2C`. Its on/off state is
    never reported: after Off, the `2C` reply, the DC2 reply (DT24 keeps the last
    time) and all thirteen DC4 information blocks are identical to sleep running.
  - Sleep (the remote SLEEP key, "Sleep" on the display) reports `C5` and its
    state is in DC2 DT186 (`00` 120 min, `01` 90, `02` 60, `03` 30, `04` Off), but
    it has no direct command. The remote key `07A30` steps it: from Off the first
    press sets 120; within about two seconds each press advances one step, after
    that a press only shows the current value. `07A31` and `07A32` appear to be the
    zone 2 and zone 3 keys (untested).
- System Memory is numbered 1-6 (same `07E35`-`07E3A` commands). Recalling one
  from the remote reports `30` with the 1-based memory number, followed by a full
  DC2 configuration reply.
- The 2ch surround decoder used by Surround Decode on 2-channel sources (analog
  or digital) reports `6E`: `00` Pro Logic, `01` PLII Movie, `02` PLII Music, `03`
  PLII Game, `04` Neo:6 Cinema, `05` Neo:6 Music. `07E67`, `07E68`, `07EC7`, `07E69`
  and `07E6A` select the last five, but only while Surround Decode is active on a
  2-channel source; otherwise they are ignored. Pro Logic is `07EC9` (`07E66` is
  refused with a guarded reply). The remote's decoder key `07A8D` steps through
  all six. Selecting a decoder from Home Assistant always selects Surround Decode
  first; re-selecting it while active keeps the current decoder. On a
  multichannel source (tested with Dolby Digital) the receiver still accepts the
  command and reports `6E`, but the stored setting only applies to 2-channel
  sources: Surround Decode decodes the multichannel format itself (shown by
  Playback Format). The select therefore shows the stored 2ch decoder, like the
  receiver's own setting. If a decoder command gets no reply, the component sets
  `last_error` but does not treat that as a lost connection. The decoder's DC2
  position is not known, so it shows unknown until the first `6E` report. This
  catalog is RX-V1900-specific (`surround_decoder`): the x700/x800 manuals
  document a different table (for example Neural `07ECC` / `6E 09`, which this
  unit refuses), and that table is not implemented.
- The input decoder mode commands (`07EDB`/`07EA8`/`07E3B`, Auto/DTS/AAC) are
  ignored on both analog and digital inputs, so the RX-V1900 profile rejects them.
- Extended Surround accepted Off, EX/ES and Auto on a setup without surround
  back speakers; EX and PLIIx Movie/Music need surround back speakers.
- DC4 setup writes such as bass/treble (`033`) are acknowledged with the status
  alone (`20 04 0330`), with no value echo. The component accepts that as success
  and reads the value back. Front-panel tone changes send no serial report.
- `07E6B`-`07E70` are Volume Memory 1-6, as in the older manuals: `07E6C`
  showed "Volume Memory 2" on the display and reported `33 02`; `07E70` reported
  `33 06`. They are not exposed.
- There is no Speaker A/B: `07EAB` gets no reply (the A/B remote keys do
  nothing), so the profile rejects the relay controls (`supports_speaker_relays`).
- Other remote keys, converted from IR captures and checked over serial (not
  used by the component): ENHANCER `07A94` toggles Straight Enhancer and 7ch
  Enhancer; STRAIGHT `07A56` toggles STRAIGHT and the previous program; the
  program category keys `07A88` (Classical), `07A89` (Live/Club), `07A8A`
  (Entertain), `07A8B` (Movie) and `07A8C` (Stereo) cycle within their category.
  Captured but untested zone 3 keys: TV `07AF6`, DOCK `0F7F014CB3`, power `07AED`.
- Regional variants (from the service manual; none tested): U and J models add
  the CS II Cinema and CS II Music decoders, and U and C models add Neural-THX and
  Neural-THX Digital Music. None of these are in the decoder catalog, so the
  select can't show or set them on those models. U models also list XM/SIRIUS
  inputs and HD Radio, and use US tuner ranges. Other regions may report a model
  ID other than `R0240`, in which case select the profile explicitly.
