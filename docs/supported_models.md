# Supported Models

| Model | Status | Auto Profile | Standard Profile | Notes |
|---|---|---|---|---|
| RX-V1500 | expected | `rx_vx500_extended` | `rx_vx500` | x500 input/program map with typed standard setup controls; MD/TAPE main input uses `07AC9`. |
| RX-V1600 | expected | `rx_vx600_extended` | `rx_vx600` | x600 input/program map with RX-V1600-practical standard controls; MD/TAPE main input uses `07A18`; DC4 setup controls and DC2 offset decode in extended profile. |
| RX-V1700 | expected | `rx_vx700_extended` | `rx_vx700` | x700 input/program map with typed standard setup controls and extended raw transport. |
| RX-V1800 | expected | `rx_vx800_extended` | `rx_vx800` | x800 input/program map with typed standard setup controls; Night Listening Mode disabled by profile. |
| RX-V2500 | expected | `rx_vx500_extended` | `rx_vx500` | x500 input/program map with typed standard setup controls; MD/TAPE main input uses `07AC9`; Pure Direct uses the RX-V2500 `07E82`/`07E83` pair. |
| RX-V2600 | expected | `rx_vx600_extended` | `rx_vx600` | x600 input/program map with typed standard controls, DC4 setup controls, and DC2 offset decode in extended profile. |
| RX-V2700 | expected | `rx_vx700_extended` | `rx_vx700` | x700 input/program map with typed standard setup controls and extended raw transport. |
| RX-V3800 | expected | `rx_vx800_extended` | `rx_vx800` | x800 input/program map with typed standard setup controls and extended raw transport. |
| RX-Vx500 family | expected | `rx_vx500_extended` | `rx_vx500` | Family profile pair available; standard entities cover the shared STX command surface. |
| RX-Vx600 family | expected | `rx_vx600_extended` | `rx_vx600` | Family profile pair available; use extended profile for RX-V1600-class setup controls. |
| RX-Vx700 family | expected | `rx_vx700_extended` | `rx_vx700` | Family profile pair available; standard entities cover the shared STX command surface. |
| RX-Vx800 family | expected | `rx_vx800_extended` | `rx_vx800` | Family profile pair available; standard entities cover the shared STX command surface. |
| Other RS-232 Yamaha AVRs | unknown | `rx_vx500_extended` fallback | `rx_vx500` fallback | Verify input/program mappings and set an explicit profile if auto detection is wrong. |

## Validation status definitions

- `tested`: verified with real hardware using this ESPHome component.
- `expected`: command mapping is implemented from the receiver command families, but needs device validation.
- `unknown`: no validation yet.

## Profile selection policy

Every supported model family has a standard and an extended profile:

| Family | Standard Profile | Extended Profile | Auto Chooses |
|---|---|---|---|
| RX-Vx500 | `rx_vx500` | `rx_vx500_extended` | `rx_vx500_extended` |
| RX-Vx600 | `rx_vx600` | `rx_vx600_extended` | `rx_vx600_extended` |
| RX-Vx700 | `rx_vx700` | `rx_vx700_extended` | `rx_vx700_extended` |
| RX-Vx800 | `rx_vx800` | `rx_vx800_extended` | `rx_vx800_extended` |

Use a standard profile when you want the stable STX command surface only.
Use an extended profile when you want the model-family extended command path
enabled. `auto` selects the extended profile for recognized model names.

## Command coverage notes

The stable standard surface is exposed as typed Home Assistant entities across
the supported profile families: power, mute, volume, input, DSP/program, System
Memory, audio select, Pure Direct, Zone2 mute, speaker relay A/B, tuner
preset page, tuner band, sleep timer, decoder mode, extended surround, speaker
B assignment, Zone2 amp mode, and diagnostics where the receiver reports the
state.

RX-Vx600 uses the RX-V1600-practical `07A18` MD/TAPE standard command. RX-Vx500
uses the x500 table `07AC9` MD/TAPE standard command. Keep the explicit profile
setting if a receiver does not match the automatic family choice.

The RX-Vx600 extended profile currently has the deepest typed setup coverage:
tuner frequency, bass/treble, speaker distances, dimmer, fan mode, DC4 setup
polls, and RX-V1600 DC2 offset decode.

The RX-Vx700 and RX-Vx800 extended profiles expose their generation-specific
input/DSP maps and enable the raw DC4 command path. Region/model-dependent
blocks such as XM, NET/USB/iPod operation, Zone3, trigger outputs, memory
save/load, and DSP parameter editing remain available through
`yamaha_serial.raw_command` until validated as typed HA controls.
