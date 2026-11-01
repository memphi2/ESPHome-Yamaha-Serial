# Maintenance and Migration

## Migration

These changes are in the unreleased maintenance branch, not the `v0.2.3` tag.

- Raw actions, aliases and raw `play_media` shortcuts need
  `allow_raw_commands: true`. The full/x500 examples opt in explicitly and use
  encrypted API connections; Basic leaves raw access off.
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

The full ESP8266 example has 666 aliases. Their names and payloads now live in
one deduplicated flash string pool with 16-bit offsets and binary-search lookup,
instead of RAM-resident literals behind a flash pointer table. No alias was
removed. Unused button backpointers were removed, DC4 dispatch uses string views,
and queue dispatch moves payloads instead of copying them.

Local comparison on 2026-11-01, ESPHome 2026.10.1, default example configurations:

| Example | Before RAM | After RAM | Before Flash | After Flash |
|---|---:|---:|---:|---:|
| ESP8266 Full | 58,756 B | 44,144 B | 475,360 B | 483,508 B |
| ESP32 Basic | 45,428 B | 45,404 B | 815,587 B | 820,907 B |

ESP8266 static RAM falls by 14,612 bytes (24.9%); the additional checks increase
flash by 8,148 bytes (1.7%). The flash string placement primarily benefits
ESP8266; it is not a claim of equivalent ESP32 RAM savings. CI budgets are
48,000 bytes RAM and 520,000 bytes flash for Full.

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
- `command_catalog.json`: public alias/payload catalog.
- `select_catalog.json`: shared select options, command keys and report labels.

Run `python tools/generate_catalog.py` after changing catalogs; generated headers
are committed and checked in tests. Add a production C++ regression whenever a
command policy, parser shape or model mapping changes. Do not replace runtime
checks with source-substring assertions.
