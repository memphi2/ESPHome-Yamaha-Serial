# RS-232 Wiring

## Do Not Connect RS-232 Directly to ESP GPIO

Yamaha receiver serial ports use true RS-232 voltage levels. ESP8266 and ESP32
UART pins are 3.3 V TTL only.

Always use an RS-232 <-> TTL converter, for example:

- a MAX3232-based module (recommended for 3.3 V logic);
- another known-good TTL-RS232 converter for the right voltage.

## Minimum Wiring

- ESP `TX` -> converter `TX TTL`
- ESP `RX` -> converter `RX TTL`
- ESP `GND` -> converter `GND`
- Converter RS-232 side -> Yamaha DB9 serial port

The DB9 direction stays crossed end-to-end through the converter. If the
receiver never answers, swap TX and RX first.

## Pins in the Examples

| Example | Board | ESP TX | ESP RX | Note |
|---|---|---|---|---|
| [`yamaha_rx_v_basic.yaml`](../examples/yamaha_rx_v_basic.yaml) | ESP32 (`esp32dev`) | `GPIO17` | `GPIO16` | The logger stays on its default port. |
| [`yamaha_rx_v_full.yaml`](../examples/yamaha_rx_v_full.yaml) | ESP8266 (`nodemcuv2`) | `GPIO1` | `GPIO3` | UART0: set `logger: baud_rate: 0` so log output does not collide with receiver traffic; logs still work over the API. |

The serial settings are `9600` baud, 8 data bits, no parity, 1 stop bit, as in
the examples' `uart:` block.

## Tested Setups

Setups that users have run against a receiver. They are not CI-built: CI
compiles only the ESP32 and ESP8266 examples above.

**ESP32-C3 RS232 Adapter (RX-V1900).** The
[ESP32-C3 RS232 Adapter](https://github.com/Leotro-Engineering/ESP32-C3-RS232-Adapter)
by LEOTRO combines an ESP32-C3 and an RS-232 transceiver with a DB9 male
connector on one board, so no separate converter or TTL wiring is needed. It
was tested with an RX-V1900 G/E:

- a null-modem (crossed) female-to-female DB9 cable to the receiver;
- ESP TX `GPIO10`, ESP RX `GPIO4`;
- no handshake wiring or flow-control settings;
- the logger stays on the ESP32-C3's built-in USB, so the UART is free for
  the receiver.

```yaml
esp32:
  variant: esp32c3
  framework:
    type: esp-idf

uart:
  id: uart_yamaha
  tx_pin: GPIO10
  rx_pin: GPIO4
  baud_rate: 9600
```

## Handshake

Yamaha documents hardware handshaking (`RTS/CTS`). If your converter needs the
handshake lines, wire or strap them. On RX-Vx600/RX-V1600 setups this is often
critical:

- hold `CTS` high; many MAX3232 DB9 modules do this by bridging `RTS` and `CTS`;
- if `CTS` floats or is low, the receiver may stay completely silent.

## Receiver Settings

Some receivers only accept serial control in standby when an RS-232 standby
option is enabled in the advanced setup. On the RX-V1900, set
**ADVANCED SETUP > RS-232C STANDBY** to **YES**; the default is NO outside the
US and Canada.

## Flashing

On many boards, disconnect the converter from the ESP's flashing UART while
uploading firmware if the upload is unstable.
