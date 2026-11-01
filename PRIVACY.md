# Privacy

## What this component collects

The component speaks RS-232 over a UART and makes no outbound network request
of its own. It has no cloud account, telemetry or analytics. ESPHome, Home
Assistant, backups and integrations you configure can transfer data elsewhere;
this component cannot enforce a local-only installation.

## What is stored

- Nothing is persisted by this component on the ESP beyond ESPHome's own
  normal behaviour for restored entity states.
- Receiver state (power, volume, input, DSP program, diagnostics counters)
  lives as ESPHome entity state and, once in Home Assistant, in Home
  Assistant's recorder database like any other entity.
- The receiver model string reported by the receiver is published as a
  `text_sensor`.

## What the diagnostics entities contain

The diagnostics entities exist to make a support report decidable. They
contain protocol-level data only:

- connection state, availability, last error and last parse-error reason;
  rejected frame bytes appear in local logs, not the text entity
- counters for commands sent, responses received, parse errors, timeouts and
  queue drops
- last response age

A rejected frame may contain custom input names, station/media text or other
user-supplied content, as well as receiver state. Treat logs as potentially
sensitive. Before sharing, redact names, addresses, credentials, tokens and
anything identifying people or usage patterns.

## What you are sharing when you share logs

A `uart: debug` trace shows every command sent and every report frame
received. It says when the receiver was switched on, how loud it was set and
which source was selected, so it says when somebody was at home and what they
were watching or listening to. That data stays in your installation; if you
export or share it, you are the one deciding to share it.

## Third parties

The optional Home Assistant companion integration runs inside your Home
Assistant instance and talks only to local ESPHome entities. No third-party
service is contacted by either part of this project.
