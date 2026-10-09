# Security Policy

## Supported Versions

Security fixes target the current release line. See [SUPPORT.md](SUPPORT.md).

## Reporting a Vulnerability

Report suspected vulnerabilities through a private GitHub security advisory on
this repository. Please do not open a public issue for an unfixed
vulnerability.

Include the component version, the ESPHome version, the receiver model and
profile (`receiver_profile`), and the smallest reproduction you have. A UART
log at `logger: level: VERY_VERBOSE` with `uart: debug` is usually what makes
a report decidable.

## Threat Model

This component drives an AV receiver over a point-to-point RS-232 link. It has
no cloud component, no inbound network listener of its own and no credentials.
What matters for security here is different from a cloud integration:

- **Physical serial access.** Anyone with access to the DB9 link can control
  the receiver regardless of Home Assistant. Treat the serial run as a
  trusted, physically protected segment.
- **Electrical safety is the real risk.** Yamaha DB9 ports carry true RS-232
  levels, not 3.3V TTL. Wiring an ESP GPIO directly to the receiver can
  destroy the ESP, the receiver port, or both. A MAX3232-class level converter
  is mandatory, not a recommendation. See [docs/wiring.md](docs/wiring.md).
- **Raw command authority.** The `yamaha_serial.raw_command` action and the
  optional `yamaha_raw_command` API service can send arbitrary frames,
  including setup-menu writes and `dc3:reset`. Anything that can call an
  ESPHome API service can therefore reconfigure the receiver. Home Assistant
  user permissions and the ESPHome API encryption key are the only things
  between a caller and those frames. Do not expose the API service on a device
  without `api: encryption:` configured.
- **Unvalidated payloads reach the receiver.** Raw payloads are checksummed
  and framed but not semantically validated. A wrong payload for a given model
  can leave the receiver in an unexpected setup state. Recovery is the
  receiver's own factory reset, not something this component can guarantee.
- **Transport exposure is ESPHome's.** The native API, OTA and any web server
  in your ESPHome configuration are ESPHome's attack surface, not this
  component's. Report those upstream.

## Out of Scope

- Firmware behaviour of the receiver itself. Report that to Yamaha.
- Attacks that require physical access to the RS-232 wiring or the ESP board.
- ESPHome platform vulnerabilities (native API, OTA, web server).
