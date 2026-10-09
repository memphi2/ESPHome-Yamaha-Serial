# Troubleshooting

## No response from receiver

1. Verify UART settings: `9600 8N1`.
2. Check RS-232 converter wiring and power.
3. Confirm TX/RX are crossed.
4. Confirm handshake line behavior (`RTS/CTS`): on many Yamaha + MAX3232 setups `CTS` must be held high (often by bridging `RTS` to `CTS` on DB9 side).
5. Confirm receiver RS-232 control is enabled (model-specific menu option, e.g. Wake on RS-232).
6. Enable debug logs over API; if using ESP8266 UART0 (`GPIO1/GPIO3`) set `logger.baud_rate: 0` (see section below).

## Wrong baud rate or serial config

Symptoms:
- parse errors increase
- availability toggles to unavailable
- no valid state updates

Fix:
- set correct UART baud/parity/data bits/stop bits for the receiver.

## Parse errors increase

`parse_errors` should stay stable during normal report traffic. Increases mean
the UART parser rejected a frame before it could be interpreted.

Common causes:
- serial logger bytes on the Yamaha UART
- wrong baud rate or serial parameters
- RS-232 level/wiring noise
- interrupted frame due to TX/RX/ground issues
- DC4 checksum mismatch
- extended payload length that does not match the received bytes

Check `last_parse_error` first. It includes the last reason and, when available,
the rejected frame bytes.

## TX/RX swapped

Symptoms:
- component sends commands but receives nothing
- command timeout counter increases

Fix:
- swap UART `tx_pin`/`rx_pin` at the ESP or converter side.

## ESP8266 UART0 logger conflict

Symptoms:
- no responses while commands are sent
- intermittent parse failures
- ESPHome warning that logger and UART component use the same serial port

Cause:
- on ESP8266, Yamaha RS-232 is often connected to `GPIO1`/`GPIO3` (UART0)
- if serial logger is active on UART0, log bytes interfere with Yamaha traffic

Fix:
- disable serial logger output on that port:

```yaml
logger:
  level: DEBUG
  baud_rate: 0
```

- keep logs via native API (`esphome logs`) instead of UART console

## RS-232 level mismatch (critical)

Symptoms:
- unstable responses
- no responses
- potential GPIO damage risk

Fix:
- never connect receiver DB9 RS-232 pins directly to ESP GPIO.
- use MAX3232 or equivalent RS-232 <-> TTL converter.

## Receiver in standby

Some models reject most control commands while standby is active.
Use:
- RS-232 wake/standby settings in AVR menu
- explicit power-on command

## Commands partially work

Likely causes:
- model-specific command differences
- wrong `receiver_profile` selected
- unsupported input/program IDs
- guard status (Yamaha command rejected by current mode)

Check:
- `last_error` text sensor
- raw behavior for program/input selection
- set an explicit extended profile such as `receiver_profile: rx_vx600_extended` for known receivers
- use `receiver_profile: auto` only if model auto-detection is correct for your AVR

## A raw command has no dedicated entity

Use the `yamaha_raw_command` API service from the full example:

```text
stx:07E7E
dc4:20050000F
dc1:000
dc3:reset
```

For `dc4:` use the payload from Yamaha's RX-Vx600 extended table without frame
bytes and without checksum. The component adds `DC4`, checksum and `ETX`.
Keep rarely used setup commands disabled in normal dashboards; many of them are
model, region or setup-menu dependent.

## Extended entities stay idle (bass/treble/distances/dimmer)

Likely causes:
- active profile does not support extended (`DC4`) commands
- receiver/firmware has extended block differences

Check:
- component log line `Receiver profile: ...`
- set `receiver_profile` explicitly
- verify `command_timeout`/`command_spacing` if receiver answers slowly

## Entities stay unavailable

Availability is derived from recent valid responses.
If it stays unavailable:
- check cable and converter
- validate UART params
- use the refresh button or temporarily set `poll_interval` to a time value to test responses
- increase `command_timeout` for slow links

Note:
- when unavailable/offline, the component only sends a lightweight ready probe and waits for a receiver response before startup sync resumes.
- with `poll_interval: never`, the component stays online after startup and relies on Yamaha report frames instead of periodic heartbeat traffic.

If `queue_drops` increases:
- receiver is not responding fast enough for current command rate
- increase `command_spacing`
- increase `command_timeout`
- disable periodic polling with `poll_interval: never` or increase `poll_interval`

## Enable debug logging

Use ESPHome logger with `level: DEBUG` while testing:

```yaml
logger:
  level: DEBUG
```

For heavy line traffic, temporarily use `VERBOSE` and inspect frame timing.
