# Command Matrix

This component exposes Yamaha receiver commands through three layers:

- Home Assistant entities for stable, user-facing controls.
- Named raw-command aliases for finite table commands.
- Raw `stx:` / `dc4:` payloads for parameterized command families.

The named alias catalog is intended for automation and service calls where a
full HA entity would be noisy or misleading. It is routed through the same
non-blocking command queue, spacing, timeout and retry handling as the typed
controls.

## Entity Layer

These command families are exposed as HA/ESPHome controls where the state model
is stable and useful:

| Family | Entity layer |
|---|---|
| Main power, mute, volume | `media_player` |
| Main input | `media_player.play_media`, `select.input_source`, optional HA companion `media_player.select_source` |
| DSP/program | `media_player.play_media`, `select.program`, optional HA companion `media_player.select_sound_mode` |
| Zone2 power, mute, input, volume | `switch`, `select`, `number` |
| Tuner preset, preset page, band, seek | `select`, `button` |
| Audio Select, Night Mode, Extended Surround, Decoder Mode | `select` |
| Speaker A/B relay, Speaker B assignment, Zone2 amp | `switch`, `select` |
| Bass, treble, speaker distances, dimmer | `number` |
| Availability, model, last error, counters | `binary_sensor`, `sensor`, `text_sensor` |

## Named Alias Layer

Named aliases can be sent through the raw command action:

```yaml
on_...:
  then:
    - yamaha_serial.raw_command:
        id: yamaha
        command: zone3_power_on
```

The full examples also expose the same path through the ESPHome API service:

```yaml
service: esphome.yamaha_rxv_full_yamaha_raw_command
data:
  command: program_munich
```

Alias families currently include:

| Alias family | Examples |
|---|---|
| Power and zone power | `power_on`, `all_zone_power_off`, `zone3_power_on` |
| Input and zone input | `input_dvd`, `input_md_tape_rx_v1600`, `input_md_tape_x500`, `zone2_input_vaux`, `zone3_input_net_usb` |
| Volume and mute steps | `volume_up`, `zone3_volume_down`, `mute_20db_on` |
| DSP/program direct codes | `program_hall_a`, `program_munich`, `program_surround_decode` |
| Tuner operation | `tuner_preset_page_a`, `tuner_preset_8`, `tuner_auto_up` |
| Speaker and setup toggles | `speaker_a_on`, `speaker_center_small`, `memory_guard_on`, `speaker_impedance_6ohm_x500` |
| System/report/text commands | `report_enable`, `report_delay_100ms`, `firmware_version_request` |
| Amp/output modes | `zone2_amplifier_both`, `zone3_volume_out_fixed` |
| Dynamic range and audio setup | `dynamic_range_speaker_min`, `audio_select_set_last` |
| Video/HDMI/OSD setup | `video_conversion_on`, `hdmi_upscaling_1080p`, `on_screen_always` |
| iPod, XM, NET/USB | `ipod_play`, `xm_display_hold`, `net_usb_shuffle_on` |
| DC trigger and memory | `trigger2_zone3_high`, `system_memory_save_6`, `home_preset_memory_f` |
| DC4 information reads | `ext_read_model_name`, `ext_read_input_audio_optical`, `ext_read_dsp_user_parameters` |

RX-Vx600/RX-V1600 aliases keep the practical standard mapping used by the
receiver family, for example `input_md_tape_rx_v1600` -> `07A18` and
`pure_direct_off_rx_v1600` -> `07E82`. RX-Vx500/RX-V2500 variants are explicit
where the standard table differs, for example `input_md_tape_x500` -> `07AC9`
and `pure_direct_off_rx_v2500` -> `07E83`.

Use the `alias:` prefix when a command name might otherwise be parsed as a
prefixed raw command:

```yaml
command: alias:ext_read_model_name
```

## Raw Payload Layer

Every operation command can still be sent directly:

```yaml
command: stx:07E7E
```

Every extended command can also be sent directly without frame bytes and without
checksum. The component adds `DC4`, checksum and `ETX`:

```yaml
command: dc4:20060410A0
```

Keep these families on the raw payload layer unless a local installation needs a
specific typed control:

| Family | Why it remains raw |
|---|---|
| I/O assign | Requires type, connector number and input ID. |
| Rename text | Requires receiver-specific text length and character payload. |
| Volume trim | Requires input ID and encoded dB value. |
| User/maker DSP parameters | Requires DSP ID, parameter ID and range-specific value. |
| Graphic EQ and Parametric EQ | Requires channel, band, frequency/gain/Q values. |
| Channel mute blocks | Can carry a variable number of channel/status pairs. |
| DC trigger matrix | Requires terminal, zone, input and state. |
| Tuner station memory | Requires page, preset, band and formatted frequency text. |
| XM preset memory | Requires page, preset and station ID. |

That split keeps Home Assistant clean while still making the complete command
surface reachable for automation, testing and model-specific tuning.
