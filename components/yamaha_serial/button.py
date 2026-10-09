import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import ENTITY_CATEGORY_CONFIG

from . import CONF_YAMAHA_SERIAL_ID, YAMAHA_SERIAL_COMPONENT_SCHEMA, yamaha_serial_ns

DEPENDENCIES = ["yamaha_serial"]

CONF_REFRESH = "refresh"
CONF_POWER_TOGGLE = "power_toggle"
CONF_VOLUME_UP = "volume_up"
CONF_VOLUME_DOWN = "volume_down"
CONF_ZONE2_VOLUME_UP = "zone2_volume_up"
CONF_ZONE2_VOLUME_DOWN = "zone2_volume_down"
CONF_RECEIVER_RESET = "receiver_reset"
CONF_TUNER_AUTO_UP = "tuner_auto_up"
CONF_TUNER_AUTO_DOWN = "tuner_auto_down"

YamahaRefreshButton = yamaha_serial_ns.class_("YamahaRefreshButton", button.Button)
YamahaPowerToggleButton = yamaha_serial_ns.class_(
    "YamahaPowerToggleButton", button.Button
)
YamahaVolumeUpButton = yamaha_serial_ns.class_("YamahaVolumeUpButton", button.Button)
YamahaVolumeDownButton = yamaha_serial_ns.class_(
    "YamahaVolumeDownButton", button.Button
)
YamahaZone2VolumeUpButton = yamaha_serial_ns.class_(
    "YamahaZone2VolumeUpButton", button.Button
)
YamahaZone2VolumeDownButton = yamaha_serial_ns.class_(
    "YamahaZone2VolumeDownButton", button.Button
)
YamahaReceiverResetButton = yamaha_serial_ns.class_(
    "YamahaReceiverResetButton", button.Button
)
YamahaTunerAutoUpButton = yamaha_serial_ns.class_(
    "YamahaTunerAutoUpButton", button.Button
)
YamahaTunerAutoDownButton = yamaha_serial_ns.class_(
    "YamahaTunerAutoDownButton", button.Button
)

CONFIG_SCHEMA = YAMAHA_SERIAL_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(CONF_REFRESH): button.button_schema(
            YamahaRefreshButton,
            icon="mdi:refresh",
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
        cv.Optional(CONF_POWER_TOGGLE): button.button_schema(
            YamahaPowerToggleButton,
            icon="mdi:power-cycle",
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
        cv.Optional(CONF_VOLUME_UP): button.button_schema(
            YamahaVolumeUpButton,
            icon="mdi:volume-plus",
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
        cv.Optional(CONF_VOLUME_DOWN): button.button_schema(
            YamahaVolumeDownButton,
            icon="mdi:volume-minus",
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
        cv.Optional(CONF_ZONE2_VOLUME_UP): button.button_schema(
            YamahaZone2VolumeUpButton,
            icon="mdi:volume-plus",
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
        cv.Optional(CONF_ZONE2_VOLUME_DOWN): button.button_schema(
            YamahaZone2VolumeDownButton,
            icon="mdi:volume-minus",
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
        cv.Optional(CONF_RECEIVER_RESET): button.button_schema(
            YamahaReceiverResetButton,
            icon="mdi:restart-alert",
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
        cv.Optional(CONF_TUNER_AUTO_UP): button.button_schema(
            YamahaTunerAutoUpButton,
            icon="mdi:radio-tower",
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
        cv.Optional(CONF_TUNER_AUTO_DOWN): button.button_schema(
            YamahaTunerAutoDownButton,
            icon="mdi:radio-tower",
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
    }
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_YAMAHA_SERIAL_ID])

    if conf := config.get(CONF_REFRESH):
        var = await button.new_button(conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_refresh_button(var))

    if conf := config.get(CONF_POWER_TOGGLE):
        var = await button.new_button(conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_power_toggle_button(var))

    if conf := config.get(CONF_VOLUME_UP):
        var = await button.new_button(conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_volume_up_button(var))

    if conf := config.get(CONF_VOLUME_DOWN):
        var = await button.new_button(conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_volume_down_button(var))

    if conf := config.get(CONF_ZONE2_VOLUME_UP):
        var = await button.new_button(conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_zone2_volume_up_button(var))

    if conf := config.get(CONF_ZONE2_VOLUME_DOWN):
        var = await button.new_button(conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_zone2_volume_down_button(var))

    if conf := config.get(CONF_RECEIVER_RESET):
        var = await button.new_button(conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_receiver_reset_button(var))

    if conf := config.get(CONF_TUNER_AUTO_UP):
        var = await button.new_button(conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_tuner_auto_up_button(var))

    if conf := config.get(CONF_TUNER_AUTO_DOWN):
        var = await button.new_button(conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_tuner_auto_down_button(var))
