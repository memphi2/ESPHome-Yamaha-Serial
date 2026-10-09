# Home Assistant Companion Media Player

ESPHome exposes `SELECT_SOURCE` and `SELECT_SOUND_MODE` feature flags, but its
C++ media player call path carries no selected source or sound-mode value:
`media_player::MediaPlayerCall` has only `command`, `media_url`, `volume` and
`announcement`, and `MediaPlayerTraits` has no `source_list` or
`sound_mode_list`. This is still true on ESPHome `dev` as of 2026.9. This
repository therefore keeps the ESPHome media player conservative and offers
this optional Home Assistant companion integration when you want a single
receiver-style media player in the Home Assistant UI.

The companion wraps the ESPHome entities created by this project:

- the ESPHome `media_player` entity for power, mute and normalized volume
- `select.input_source` for Home Assistant's native source selector
- `select.program` for Home Assistant's native sound-mode selector
- optional power, mute and availability helper entities

## Installation

Requires Home Assistant `2026.9.0` or newer.

Through HACS: add this repository as a custom repository with category
`Integration`, install it, then restart Home Assistant.

Manually: copy `custom_components/yamaha_serial_receiver` into your Home
Assistant configuration directory and restart Home Assistant.

Then add a YAML media player entry like this:

```yaml
media_player:
  - platform: yamaha_serial_receiver
    name: Yamaha RX-V1600
    unique_id: yamaha_rxv1600_receiver
    model: RX-V1600
    media_player_entity: media_player.yamaha_receiver
    power_entity: switch.yamaha_power
    mute_entity: switch.yamaha_mute
    source_entity: select.yamaha_input
    sound_mode_entity: select.yamaha_program
    availability_entity: binary_sensor.yamaha_availability
```

Use the real entity IDs from your ESPHome device. The full example is available
in `examples/home_assistant_companion.yaml`.

## Behavior

- `media_player.turn_on` and `media_player.turn_off` call the configured power
  switch when present, otherwise the wrapped ESPHome media player.
- `media_player.volume_set`, `volume_up`, `volume_down` and `volume_mute` call
  the wrapped ESPHome media player, except mute can optionally use a dedicated
  mute switch.
- `media_player.select_source` calls `select.select_option` on the configured
  input-source select.
- `media_player.select_sound_mode` calls `select.select_option` on the
  configured DSP/program select.

The wrapper is local-push and does not poll. It updates whenever one of the
wrapped ESPHome entities changes.
