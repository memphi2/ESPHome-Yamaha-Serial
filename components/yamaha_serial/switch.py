import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import ENTITY_CATEGORY_CONFIG

from . import CONF_YAMAHA_SERIAL_ID, YAMAHA_SERIAL_COMPONENT_SCHEMA, yamaha_serial_ns

DEPENDENCIES = ["yamaha_serial"]

YamahaSwitch = yamaha_serial_ns.class_("YamahaSwitch", switch.Switch)
SwitchFunction = yamaha_serial_ns.enum("SwitchFunction", is_class=True)

# YAML key -> (component setter, receiver function, switch_schema keyword
# arguments). One table instead of one class per control: the C++ side is a
# single YamahaSwitch that dispatches on the function.
SWITCH_TYPES = {
    "power": ("set_power_switch", SwitchFunction.POWER, {"icon": "mdi:power"}),
    "main_zone_power": (
        "set_main_zone_power_switch",
        SwitchFunction.MAIN_ZONE_POWER,
        {"icon": "mdi:power"},
    ),
    "mute": ("set_mute_switch", SwitchFunction.MUTE, {"icon": "mdi:volume-mute"}),
    "zone2_power": (
        "set_zone2_power_switch",
        SwitchFunction.ZONE2_POWER,
        {"icon": "mdi:power"},
    ),
    "pure_direct": (
        "set_pure_direct_switch",
        SwitchFunction.PURE_DIRECT,
        {"icon": "mdi:music-note-off"},
    ),
    "fan_mode": (
        "set_fan_mode_switch",
        SwitchFunction.FAN_MODE,
        {"icon": "mdi:fan", "entity_category": ENTITY_CATEGORY_CONFIG},
    ),
    "zone2_mute": (
        "set_zone2_mute_switch",
        SwitchFunction.ZONE2_MUTE,
        {"icon": "mdi:volume-mute"},
    ),
    "speaker_a": (
        "set_speaker_a_switch",
        SwitchFunction.SPEAKER_A,
        {"icon": "mdi:speaker"},
    ),
    "speaker_b": (
        "set_speaker_b_switch",
        SwitchFunction.SPEAKER_B,
        {"icon": "mdi:speaker"},
    ),
}

CONFIG_SCHEMA = YAMAHA_SERIAL_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(key): switch.switch_schema(YamahaSwitch, **kwargs)
        for key, (_, _, kwargs) in SWITCH_TYPES.items()
    }
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_YAMAHA_SERIAL_ID])

    for key, (setter, function, _) in SWITCH_TYPES.items():
        if conf := config.get(key):
            var = await switch.new_switch(conf)
            cg.add(var.set_parent(paren))
            cg.add(var.set_function(function))
            cg.add(getattr(paren, setter)(var))
