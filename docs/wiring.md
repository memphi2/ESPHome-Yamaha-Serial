# RS-232 Wiring

## Do not connect RS-232 directly to ESP GPIO

Yamaha receiver serial ports use true RS-232 voltage levels.
ESP8266/ESP32 UART pins are TTL-level only.

Always use a proper RS-232 <-> TTL converter, for example:
- MAX3232-based module (recommended for 3.3V logic)
- known-good TTL-RS232 converter with correct voltage domain

## Minimum wiring

Typical setup:
- ESP `TX` -> converter `TX TTL`
- ESP `RX` -> converter `RX TTL`
- ESP `GND` -> converter `GND`
- Converter RS-232 side -> Yamaha DB9 serial port

DB9 direction stays crossed end-to-end through the converter.

## Handshake note

Yamaha docs describe hardware handshaking (`RTS/CTS`).
If your converter/module requires handshake lines, wire or strap as needed.
For RX-Vx600/RX-V1600 style setups this is often critical:
- hold `CTS` high (many MAX3232 DB9 modules do this by bridging `RTS` and `CTS`)
- if `CTS` is floating/low, receiver responses may stay completely silent

## Flashing note

On many boards, disconnect receiver TX/RX from ESP flashing UART during firmware upload if upload stability is affected.
