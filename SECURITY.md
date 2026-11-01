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
  optional `yamaha_raw_command` API service, as well as
  `media_player.play_media` with a `yamaha://raw/...` URI, can send arbitrary frames,
  including setup-menu writes and `dc3:reset`. Anything that can call an
  ESPHome API service can therefore reconfigure the receiver. Home Assistant
  user permissions and the ESPHome API encryption key are the only things
  between a caller and those frames. Do not expose the API service on a device
  without `api: encryption:` configured. All these raw entry points default to
  disabled. Enabling `allow_raw_commands: true` with an unencrypted native API
  fails configuration validation. API encryption authenticates a client, not an
  individual receiver operation; it is not per-command authorization.
  The separate reset button is an explicitly configured control and should not
  be exposed to untrusted users either. Keep OTA authenticated and API ports off
  the public internet.
- **Limited payload validation.** Input size, printable bytes and DC4 length
  are checked before framing/checksumming. Receiver-specific semantics are not
  fully validated. Raw commands bypass typed volume ceilings. A wrong payload for a given model
  can leave the receiver in an unexpected setup state. Recovery is the
  receiver's own factory reset, not something this component can guarantee.
- **Transport exposure is ESPHome's.** The native API, OTA and any web server
  in your ESPHome configuration are ESPHome's attack surface, not this
  component's. Report those upstream.

## Out of Scope

- Firmware behaviour of the receiver itself. Report that to Yamaha.
- Attacks that require physical access to the RS-232 wiring or the ESP board.
- ESPHome platform vulnerabilities (native API, OTA, web server).

## Build Dependency Review (2026-11-01)

The local Python dependency audit found affected build-tool dependencies even
though the repository's Dependabot, secret-scanning and CodeQL alert lists were
empty. These are different checks; neither green CI nor an empty GitHub alert
list is a clean bill of health for every installed transitive dependency.

Compatible fixes are enforced by `requirements/build-security.txt` for AnyIO,
Brotli, certifi, pyasn1, Starlette, Tornado and urllib3. Optional packages only
receive constraints if installed. Firmware test/lint tool versions are pinned;
the full transitive environment is not hash-locked across Python/platforms.

Two upstream pins remain unresolved in the latest stable ESPHome baseline
available on this review date (`2026.10.1`):

| Dependency | ESPHome pin | Patched version | Relevant boundary |
|---|---|---|---|
| filelock | 4.0.10 | 4.2.1 | Build/cache filesystem locking, not ESP firmware. |
| Pillow | 12.3.0 | 13.0.0 | Host-side image processing; these receiver examples do not use image components. |

The upstream [filelock advisory](https://github.com/tox-dev/filelock/security/advisories/GHSA-v4x8-3wr5-3wq8)
describes attacks through attacker-writable lock directories. The upstream
[Pillow advisory](https://github.com/python-pillow/Pillow/security/advisories/GHSA-78g4-xqwv-q3g8)
includes a bundled image decoder vulnerability. Other advisories affect these
versions too; the table is not a complete CVE inventory. The separately tested
HA baseline also pins Pillow 12.3.0. Updating this component cannot repair
dependencies inside an existing HA/ESPHome installation.

Do not force incompatible dependency versions with `--no-deps` and call the
result supported. Build as an unprivileged user, keep caches/build directories
private, process only trusted YAML/assets, and do not expose the build dashboard
publicly. CI compiles with dummy credentials on disposable runners. These are
exposure reductions, not fixes for the upstream vulnerabilities. Recheck the
pins when upstream publishes a compatible security update; the project is not
declared vulnerability-free in the meantime.

To repeat the audit, install `pip-audit` in a separate tools environment and run
`pip-audit --path <build-venv>/lib/pythonX.Y/site-packages`. Audit the HA test
environment separately. Review package-level results and primary advisories;
raw advisory counts can contain duplicate identifiers. No scanner exclusions
are used to hide the remaining findings.
