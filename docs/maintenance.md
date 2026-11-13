# Maintenance and Migration

## Migration

These changes ship in `v0.2.4`. Review them when upgrading from `v0.2.3`
or earlier.

- Raw actions, aliases and raw `play_media` shortcuts need
  `allow_raw_commands: true`. The full example opts in explicitly and uses an
  encrypted API connection; Basic leaves raw access off.
- `volume.min/max/step` now govern typed volume steps and provide the defaults
  for volume numbers. Remove redundant number limits or keep them within the
  parent limits. Values must be finite half-dB increments, within -80..16.5 dB.
- Typed volume steps wait for a known volume. They queue absolute targets so
  retries cannot double a step. Raw `volume_up/down` remain relative commands.
- Diagnostics default to disabled in HA. Existing entity-registry choices stay
  as they were; remove unwanted entities from YAML rather than adding mirrors.
- Parse-error entities contain a short reason, not a raw hex dump. Find the
  full rejected frame in bounded log lines. No live parse-error diagnosis can
  be inferred from the counter alone.
- A changed auto-detected input/program list causes one native-API reconnect
  for fresh HA metadata. Explicit profiles never change on detection.
- Event-driven mode has no continuous heartbeat. A silent failed explicit
  command marks the link unavailable; a new report restores it. With no
  command or report, an unplugged idle receiver is indistinguishable from a
  quiet receiver. Readiness probes run only while waiting for a receiver.

## Recorder Load

The component and HA companion have no database client. Receiver reports update
entities; HA's recorder decides what to persist. Identical entity states are
not automatically new `state_changed` records, but an increasing response-age
sensor produces genuinely changing values.

Age previously updated every second: up to 86,400 changed values per day when
enabled. At the new 60-second default this is up to 1,440, before recorder
filtering, restarts or disconnections. Diagnostic counters publish only when
changed. Age/counters no longer request measurement statistics; volume and
sampling-rate measurements retain their normal classifications.

Leave noisy diagnostics disabled outside investigations. For diagnostics that
must be visible but need no history, exclude their actual entity IDs in HA:

```yaml
recorder:
  exclude:
    entities:
      - sensor.yamaha_last_response_age
      - sensor.yamaha_commands_sent
      - sensor.yamaha_responses_received
```

This affects future recording, not existing history. Disabling receiver polling
and changing the diagnostics interval are separate operations; the latter sends
no serial command. Avoid redundant number/sensor/control mirrors.

## Memory Measurement

The ESP8266 copies all `.rodata`, including every string literal, into RAM at
boot. The component therefore keeps its constant text in flash:

- The 666 command aliases live in one deduplicated flash string pool with 16-bit
  offsets and binary-search lookup, generated from `command_catalog.json`.
- Profile input/program/scene tables, select command and report tables,
  playback format and sampling tables, response-policy rules and input aliases
  are `YAMAHA_SERIAL_FLASH_TABLE` arrays whose strings are inline `char` arrays.
  `flash_read()` copies one entry, strings included, into RAM. A string longer
  than its field fails to compile; widen the field in the struct.
- Error and parse-error messages are flash strings (`YAMAHA_SERIAL_PSTR`).
  `set_last_error_` formats them with printf-style arguments only when an error
  occurs. Host tests build with `-Werror=format`, so a mismatched argument
  fails CI.

`yamaha_serial_flash.h` holds these helpers. Never pass a flash string or a
flash table field to `strlen`, `strcmp` or a `std::string` constructor; on the
ESP8266 that faults. Tags, fixed command payloads, `snprintf` formats and the
media player shortcut prefixes are still RAM literals.

Local comparisons with ESPHome 2026.10.1 and the default example configurations:

| Change | Example | Before RAM | After RAM | Before Flash | After Flash |
|---|---|---:|---:|---:|---:|
| Alias string pool (0.2.4) | ESP8266 Full | 58,756 B | 44,144 B | 475,360 B | 483,540 B |
| Alias string pool (0.2.4) | ESP32 Basic | 45,428 B | 45,404 B | 815,587 B | 820,943 B |
| Flash tables and messages | ESP8266 Full | 45,240 B | 38,324 B | 487,772 B | 494,140 B |
| Flash tables and messages | ESP32 Basic | 45,444 B | 45,444 B | 824,047 B | 828,835 B |

The second change was measured on top of the RX-V1900 profiles. ESP8266 static
RAM falls by 6,916 bytes (15.3%); the change costs 6,368 bytes of flash,
mostly for the inline table fields. On the ESP32, `.rodata` is already read
from flash, so it gains no RAM and only pays the flash. CI budgets are 48,000
bytes RAM and 520,000 bytes flash for Full.

RX is bounded at 268 bytes, raw input at 320 bytes, and the queue at 32 entries.
Payloads and runtime profile/select lists still use dynamic storage. Static RAM
reported by the linker is not free heap, stack headroom or proof against
fragmentation. Do not extrapolate it to runtime stability.

RX processing has a per-loop byte budget and expires incomplete frames even
when no more bytes arrive. TX uses ESPHome's UART driver: software UART or an
unbuffered driver may block during a write, so this is not a hard real-time TX
guarantee. Prefer hardware UART and test long raw frames on the actual board.

Reproduce size checks with the same ESPHome/toolchain and YAML:

```sh
python tools/prepare_ci_configs.py
esphome compile .ci/yamaha_rx_v_full.yaml 2>&1 | tee /tmp/yamaha-size.log
python tools/check_compile_size.py /tmp/yamaha-size.log --max-ram 48000 --max-flash 520000
```

For real-device soak testing, use ESPHome's optional `debug` component to record
free heap, largest free block and loop time. Exercise sustained reports, a full
queue, receiver power cycling and API reconnects. This repository's host tests
and firmware builds do not replace that hardware test; no new hardware result
is claimed for the maintenance changes.

## Code Layout

- `yamaha_serial.cpp`: lifecycle, bootstrap and diagnostic helpers.
- `yamaha_serial_transport.cpp`: bounded UART assembly and queue scheduling.
- `yamaha_serial_protocol.cpp`: report/checksum decoding.
- `yamaha_serial_policy.cpp`: response matching and retry safety.
- `yamaha_serial_commands.cpp`: typed controls and raw entry points.
- `yamaha_serial_profiles.cpp`: family mappings and metadata updates.
- `yamaha_serial_entities.cpp`: ESPHome bindings and report publication.
- `yamaha_serial_flash.h`: flash placement of constant tables and messages.
- `command_catalog.json`: public alias/payload catalog.
- `select_catalog.json`: shared select options, command keys and report labels.

Run `python tools/generate_catalog.py` after changing catalogs; generated headers
are committed and checked in tests. Add a production C++ regression whenever a
command policy, parser shape or model mapping changes. Do not replace runtime
checks with source-substring assertions.
