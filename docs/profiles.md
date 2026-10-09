# Receiver Profiles

Profiles isolate model-family differences from the UART parser and command
queue. Each supported generation should have two profiles:

| Profile kind | Purpose |
|---|---|
| Standard | Stable STX command surface, input table, DSP/program table and report IDs. |
| Extended | Standard profile plus model-family DC4 support, extended status decoding and setup controls that are known to match that family. |

`receiver_profile: auto` must always select an extended profile for a recognized
model. Explicit standard profiles are reserved for conservative installs where
only the common STX command surface should be used.

## Contract

A profile is responsible for:

- `ProfileCapabilities` flags
- main and Zone2 input mappings
- DSP/program mappings
- System Memory/scene mappings
- generation-specific command differences such as MD/TAPE or Pure Direct
- whether DC2 offset decoding is valid
- whether typed DC4 setup entities may send commands

A profile must not own:

- UART byte framing
- DC4 checksum handling
- command queue timing/retry behavior
- Home Assistant entity registration
- diagnostics counters

## Adding A Receiver Family

1. Add `rx_vxNNN` and `rx_vxNNN_extended` values to the config schema.
2. Add both profile IDs to `resolve_profile_()`.
3. Map normalized model tokens to the extended profile in auto mode.
4. Add `apply_profile_rx_vxNNN_()` for the standard command table.
5. Add `apply_profile_rx_vxNNN_extended_()` that calls the standard profile and only enables validated extended capabilities.
6. Keep user configured `inputs:` labels working by relying on `apply_input_overrides_()` after profile load.
7. Update `docs/supported_models.md`, `docs/protocol.md`, examples if useful, and command coverage tests.
8. Validate at least `esphome config` for all examples and compile the smallest realistic ESP8266 profile if the profile targets ESP8266-era hardware.

## Runtime Detection

The component starts with the configured `model` hint. When a DC2/DC4 model
response arrives, `handle_detected_model_()` normalizes the reported model and,
in `auto` mode, reapplies the matching extended profile. Select options are
then refreshed so Home Assistant sees the correct input and DSP/program choices.

Explicit `receiver_profile` values do not change at runtime.
