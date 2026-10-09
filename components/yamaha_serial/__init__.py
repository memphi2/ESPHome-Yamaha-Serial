# SPDX-License-Identifier: GPL-3.0-only

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import uart
from esphome.const import CONF_ID, CONF_MODEL

CODEOWNERS = ["@memphi2"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = [
    "switch",
    "sensor",
    "text_sensor",
    "binary_sensor",
    "number",
    "select",
    "button",
    "media_player",
]
MULTI_CONF = True

DOMAIN = "yamaha_serial"
CONF_COMMAND = "command"
CONF_YAMAHA_SERIAL_ID = "yamaha_serial_id"
CONF_POLL_INTERVAL = "poll_interval"
CONF_COMMAND_TIMEOUT = "command_timeout"
CONF_MAX_RETRIES = "max_retries"
CONF_COMMAND_SPACING = "command_spacing"
CONF_POWER_ON_DELAY = "power_on_delay"
CONF_RECEIVER_PROFILE = "receiver_profile"
CONF_INPUTS = "inputs"
CONF_VOLUME = "volume"
CONF_VOLUME_MIN = "min"
CONF_VOLUME_MAX = "max"
CONF_VOLUME_STEP = "step"

yamaha_serial_ns = cg.esphome_ns.namespace("yamaha_serial")
YamahaSerialComponent = yamaha_serial_ns.class_(
    "YamahaSerialComponent", cg.PollingComponent, uart.UARTDevice
)
YamahaRawCommandAction = yamaha_serial_ns.class_(
    "YamahaRawCommandAction",
    automation.Action,
    cg.Parented.template(YamahaSerialComponent),
)

YAMAHA_SERIAL_COMPONENT_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_YAMAHA_SERIAL_ID): cv.use_id(YamahaSerialComponent),
    }
)

DEFAULT_INPUTS = {
    "phono": "PHONO",
    "cd": "CD",
    "tuner": "Tuner",
    "cdr": "CD-R",
    "md_tape": "MD/TAPE",
    "dvd": "DVD",
    "dtv_ld": "D-TV/LD",
    "cbl_sat": "CBL/SAT",
    "sat": "SAT",
    "vcr1": "VCR1",
    "vcr2_dvr": "VCR2/DVR",
    "vaux": "V-AUX",
}

RECEIVER_PROFILES = [
    "auto",
    "rx_vx500",
    "rx_vx500_extended",
    "rx_vx600",
    "rx_vx600_extended",
    "rx_vx700",
    "rx_vx700_extended",
    "rx_vx800",
    "rx_vx800_extended",
]

POLL_INTERVAL_SCHEMA = cv.Any(
    cv.one_of("never", "off", lower=True),
    cv.positive_time_period_milliseconds,
)

VOLUME_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_VOLUME_MIN, default=-80.0): cv.float_range(
            min=-99.5, max=16.5
        ),
        cv.Optional(CONF_VOLUME_MAX, default=16.5): cv.float_range(min=-99.5, max=16.5),
        cv.Optional(CONF_VOLUME_STEP, default=0.5): cv.float_range(min=0.5, max=10.0),
    }
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(YamahaSerialComponent),
            cv.Optional(CONF_MODEL, default="RX-Vx500"): cv.string_strict,
            cv.Optional(CONF_RECEIVER_PROFILE, default="auto"): cv.one_of(
                *RECEIVER_PROFILES, lower=True
            ),
            cv.Optional(CONF_POLL_INTERVAL, default="never"): POLL_INTERVAL_SCHEMA,
            cv.Optional(CONF_COMMAND_TIMEOUT, default="1500ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_MAX_RETRIES, default=2): cv.int_range(min=0, max=10),
            cv.Optional(CONF_COMMAND_SPACING, default="100ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_POWER_ON_DELAY, default="2s"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_INPUTS, default=DEFAULT_INPUTS): cv.Schema(
                {cv.string_strict: cv.string_strict}
            ),
            cv.Optional(CONF_VOLUME, default={}): VOLUME_SCHEMA,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.set_model(config[CONF_MODEL]))
    cg.add(var.set_receiver_profile(config[CONF_RECEIVER_PROFILE]))
    poll_interval = config[CONF_POLL_INTERVAL]
    poll_interval_ms = (
        0
        if isinstance(poll_interval, str)
        else poll_interval.total_milliseconds
    )
    cg.add(var.set_periodic_poll_enabled(poll_interval_ms > 0))
    cg.add(var.set_update_interval(poll_interval_ms if poll_interval_ms > 0 else 60000))
    cg.add(var.set_command_timeout(config[CONF_COMMAND_TIMEOUT].total_milliseconds))
    cg.add(var.set_max_retries(config[CONF_MAX_RETRIES]))
    cg.add(var.set_command_spacing(config[CONF_COMMAND_SPACING].total_milliseconds))
    cg.add(var.set_power_on_delay(config[CONF_POWER_ON_DELAY].total_milliseconds))

    volume = config[CONF_VOLUME]
    cg.add(
        var.set_volume_config(
            float(volume[CONF_VOLUME_MIN]),
            float(volume[CONF_VOLUME_MAX]),
            float(volume[CONF_VOLUME_STEP]),
        )
    )

    for key, label in config[CONF_INPUTS].items():
        cg.add(var.add_input_mapping(key, label))


@automation.register_action(
    "yamaha_serial.raw_command",
    YamahaRawCommandAction,
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.use_id(YamahaSerialComponent),
            cv.Required(CONF_COMMAND): cv.templatable(cv.string_strict),
        }
    ),
    synchronous=True,
)
async def raw_command_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    command = await cg.templatable(config[CONF_COMMAND], args, cg.std_string)
    cg.add(var.set_command(command))
    return var
