import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import ENTITY_CATEGORY_CONFIG

from . import CONF_YAMAHA_SERIAL_ID, YAMAHA_SERIAL_COMPONENT_SCHEMA, yamaha_serial_ns

DEPENDENCIES = ["yamaha_serial"]

YamahaButton = yamaha_serial_ns.class_("YamahaButton", button.Button)
ButtonFunction = yamaha_serial_ns.enum("ButtonFunction", is_class=True)

# Every button is a config entity: they are one-shot receiver actions, not
# state the dashboard should lead with.
_CONFIG = {"entity_category": ENTITY_CATEGORY_CONFIG}

# YAML key -> (component setter, receiver function, button_schema keyword
# arguments).
BUTTON_TYPES = {
    "refresh": (
        "set_refresh_button",
        ButtonFunction.REFRESH,
        {"icon": "mdi:refresh", **_CONFIG},
    ),
    "power_toggle": (
        "set_power_toggle_button",
        ButtonFunction.POWER_TOGGLE,
        {"icon": "mdi:power-cycle", **_CONFIG},
    ),
    "volume_up": (
        "set_volume_up_button",
        ButtonFunction.VOLUME_UP,
        {"icon": "mdi:volume-plus", **_CONFIG},
    ),
    "volume_down": (
        "set_volume_down_button",
        ButtonFunction.VOLUME_DOWN,
        {"icon": "mdi:volume-minus", **_CONFIG},
    ),
    "zone2_volume_up": (
        "set_zone2_volume_up_button",
        ButtonFunction.ZONE2_VOLUME_UP,
        {"icon": "mdi:volume-plus", **_CONFIG},
    ),
    "zone2_volume_down": (
        "set_zone2_volume_down_button",
        ButtonFunction.ZONE2_VOLUME_DOWN,
        {"icon": "mdi:volume-minus", **_CONFIG},
    ),
    "receiver_reset": (
        "set_receiver_reset_button",
        ButtonFunction.RECEIVER_RESET,
        {"icon": "mdi:restart-alert", **_CONFIG},
    ),
    "tuner_auto_up": (
        "set_tuner_auto_up_button",
        ButtonFunction.TUNER_AUTO_UP,
        {"icon": "mdi:radio-tower", **_CONFIG},
    ),
    "tuner_auto_down": (
        "set_tuner_auto_down_button",
        ButtonFunction.TUNER_AUTO_DOWN,
        {"icon": "mdi:radio-tower", **_CONFIG},
    ),
}

CONFIG_SCHEMA = YAMAHA_SERIAL_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(key): button.button_schema(YamahaButton, **kwargs)
        for key, (_, _, kwargs) in BUTTON_TYPES.items()
    }
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_YAMAHA_SERIAL_ID])

    for key, (setter, function, _) in BUTTON_TYPES.items():
        if conf := config.get(key):
            var = await button.new_button(conf)
            cg.add(var.set_parent(paren))
            cg.add(var.set_function(function))
            cg.add(getattr(paren, setter)(var))
