# ESPHome-Yamaha-Serial

[![Release](https://img.shields.io/github/v/release/memphi2/ESPHome-Yamaha-Serial?sort=semver&display_name=tag)](https://github.com/memphi2/ESPHome-Yamaha-Serial/releases/latest)
[![Validate](https://github.com/memphi2/ESPHome-Yamaha-Serial/actions/workflows/validate.yml/badge.svg?branch=main)](https://github.com/memphi2/ESPHome-Yamaha-Serial/actions/workflows/validate.yml)
[![CodeQL](https://github.com/memphi2/ESPHome-Yamaha-Serial/actions/workflows/codeql.yml/badge.svg?branch=main)](https://github.com/memphi2/ESPHome-Yamaha-Serial/actions/workflows/codeql.yml)
[![License: GPL-3.0-only](https://img.shields.io/badge/license-GPL--3.0--only-blue)](LICENSE)
<br>
[![ESPHome 2026.9.0+](https://img.shields.io/badge/ESPHome-2026.9.0%2B-000000?logo=esphome&logoColor=white)](SUPPORT.md#compatibility-baseline)
[![Listed on esphome.io](https://img.shields.io/badge/esphome.io-listed-000000?logo=esphome&logoColor=white)](https://esphome.io/guides/diy/#external-components--code)
[![Home Assistant 2026.9.0+](https://img.shields.io/badge/Home%20Assistant-2026.9.0%2B-18BCF2?logo=homeassistant&logoColor=white)](docs/home_assistant_companion.md)
[![HACS: Custom](https://img.shields.io/badge/HACS-Custom-41BDF5?logo=homeassistantcommunitystore&logoColor=white)](docs/home_assistant_companion.md#installation)
[![LTS 11.2026](https://img.shields.io/badge/LTS-11.2026-2EA44F)](SUPPORT.md)

Control a Yamaha AV receiver from Home Assistant through its RS-232 port, with
an ESP32 or ESP8266 running ESPHome.

The component gives you a media player (power, volume, mute), selects for the
input, DSP program and System Memory, Zone2 and tuner controls, sound and setup
controls, status sensors such as playback format and sampling rate, and
diagnostics. It reads the receiver's own status reports, so most changes made
with the remote or on the front panel show up in Home Assistant.

## Status

- LTS maintenance baseline **11.2026** (`0.4.0`); see
  [SUPPORT.md](SUPPORT.md).
- **Reference receiver: RX-V1600.** All [examples](examples) are written for it.
- Profiles exist for the RX-Vx500, RX-Vx600, RX-Vx700 and RX-Vx800 families and
  the RX-V1900. Hardware validation is partial; see
  [Supported Receivers](docs/supported_models.md).

## Safety

Yamaha receivers use **true RS-232 voltage levels** on their DB9 port, not
3.3 V logic. Never connect an ESP pin directly to the receiver. Use an RS-232
level converter (MAX3232 or equivalent); see [Wiring](docs/wiring.md).

## What You Need

- An ESP32 or ESP8266 board.
- An RS-232 level converter and a cable to the receiver's DB9 port.
- ESPHome `2026.9.0` or newer.
- For the optional companion integration, Home Assistant `2026.9.0` or newer.

## Quick Start

1. Wire the ESP, the converter and the receiver as described in
   [Wiring](docs/wiring.md).
2. Copy [`examples/yamaha_rx_v_basic.yaml`](examples/yamaha_rx_v_basic.yaml)
   into your ESPHome configuration, and fill in a `secrets.yaml` from
   [`examples/secrets.example.yaml`](examples/secrets.example.yaml). Adjust the
   UART pins if your wiring differs.
3. If your receiver is not an RX-V1600, change `model:` and follow its section
   in [Supported Receivers](docs/supported_models.md#configuration-per-receiver).
4. Install the firmware and add the device in Home Assistant.

The example loads the component from this repository:

```yaml
external_components:
  - source: github://memphi2/ESPHome-Yamaha-Serial
    components: [yamaha_serial]
```

Pin a release tag for reproducible builds, for example
`github://memphi2/ESPHome-Yamaha-Serial@v0.4.0`.

If nothing happens, start with [Troubleshooting](docs/troubleshooting.md).

## Examples

| File | Board | Contents |
|---|---|---|
| [`yamaha_rx_v_basic.yaml`](examples/yamaha_rx_v_basic.yaml) | ESP32 | Media player and diagnostics. The recommended start. |
| [`yamaha_rx_v_full.yaml`](examples/yamaha_rx_v_full.yaml) | ESP8266 (NodeMCU, UART0) | Every entity the RX-V1600 supports, including Zone2, tuner, tone and speaker setup, and the raw command service. |
| [`home_assistant_companion.yaml`](examples/home_assistant_companion.yaml) | Home Assistant | The companion media player with source and sound-mode lists. |

Configurations for the RX-Vx500 family, the RX-V1900 and the other receivers
are in [Supported Receivers](docs/supported_models.md#configuration-per-receiver).

## Documentation

For users:

- [Supported Receivers](docs/supported_models.md): models, profiles, which
  entities each profile supports, and a configuration per receiver.
- [Configuration](docs/configuration.md): every option and entity, media player
  shortcuts and raw commands.
- [Wiring](docs/wiring.md): converter, pins and handshake.
- [Troubleshooting](docs/troubleshooting.md).
- [Home Assistant companion](docs/home_assistant_companion.md): one receiver
  media player with native source and sound-mode selection.

For contributors:

- [Receiver profiles](docs/profiles.md), [RS-232 protocol](docs/protocol.md)
  and the [command matrix](docs/command_matrix.md).
- [Maintenance](docs/maintenance.md) and [CONTRIBUTING.md](CONTRIBUTING.md).

## Support, Security and Privacy

- [SUPPORT.md](SUPPORT.md): the supported release line, the ESPHome and Home
  Assistant baseline, and validated hardware.
- [SECURITY.md](SECURITY.md): threat model and how to report a vulnerability.
- [PRIVACY.md](PRIVACY.md): what is collected, stored and shared (nothing
  leaves your network).

## License

Licensed as `GPL-3.0-only`; see [LICENSE](LICENSE) and [NOTICE](NOTICE).

No Yamaha manual PDFs, manual images, vendor diagrams or copied vendor tables
are included. Product names are used only to describe compatibility and may be
trademarks of their owners.
