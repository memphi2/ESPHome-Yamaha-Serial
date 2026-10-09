import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import ENTITY_CATEGORY_CONFIG

from . import CONF_YAMAHA_SERIAL_ID, YAMAHA_SERIAL_COMPONENT_SCHEMA, yamaha_serial_ns

DEPENDENCIES = ["yamaha_serial"]

CONF_POWER = "power"
CONF_MAIN_ZONE_POWER = "main_zone_power"
CONF_MUTE = "mute"
CONF_ZONE2_POWER = "zone2_power"
CONF_PURE_DIRECT = "pure_direct"
CONF_FAN_MODE = "fan_mode"
CONF_ZONE2_MUTE = "zone2_mute"
CONF_SPEAKER_A = "speaker_a"
CONF_SPEAKER_B = "speaker_b"

YamahaPowerSwitch = yamaha_serial_ns.class_("YamahaPowerSwitch", switch.Switch)
YamahaMainZonePowerSwitch = yamaha_serial_ns.class_(
    "YamahaMainZonePowerSwitch", switch.Switch
)
YamahaMuteSwitch = yamaha_serial_ns.class_("YamahaMuteSwitch", switch.Switch)
YamahaZone2PowerSwitch = yamaha_serial_ns.class_(
    "YamahaZone2PowerSwitch", switch.Switch
)
YamahaPureDirectSwitch = yamaha_serial_ns.class_("YamahaPureDirectSwitch", switch.Switch)
YamahaFanModeSwitch = yamaha_serial_ns.class_("YamahaFanModeSwitch", switch.Switch)
YamahaZone2MuteSwitch = yamaha_serial_ns.class_("YamahaZone2MuteSwitch", switch.Switch)
YamahaSpeakerASwitch = yamaha_serial_ns.class_("YamahaSpeakerASwitch", switch.Switch)
YamahaSpeakerBSwitch = yamaha_serial_ns.class_("YamahaSpeakerBSwitch", switch.Switch)

CONFIG_SCHEMA = YAMAHA_SERIAL_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(CONF_POWER): switch.switch_schema(
            YamahaPowerSwitch, icon="mdi:power"
        ),
        cv.Optional(CONF_MAIN_ZONE_POWER): switch.switch_schema(
            YamahaMainZonePowerSwitch, icon="mdi:power"
        ),
        cv.Optional(CONF_MUTE): switch.switch_schema(
            YamahaMuteSwitch, icon="mdi:volume-mute"
        ),
        cv.Optional(CONF_ZONE2_POWER): switch.switch_schema(
            YamahaZone2PowerSwitch, icon="mdi:power"
        ),
        cv.Optional(CONF_PURE_DIRECT): switch.switch_schema(
            YamahaPureDirectSwitch, icon="mdi:music-note-off"
        ),
        cv.Optional(CONF_FAN_MODE): switch.switch_schema(
            YamahaFanModeSwitch,
            icon="mdi:fan",
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
        cv.Optional(CONF_ZONE2_MUTE): switch.switch_schema(
            YamahaZone2MuteSwitch, icon="mdi:volume-mute"
        ),
        cv.Optional(CONF_SPEAKER_A): switch.switch_schema(
            YamahaSpeakerASwitch, icon="mdi:speaker"
        ),
        cv.Optional(CONF_SPEAKER_B): switch.switch_schema(
            YamahaSpeakerBSwitch, icon="mdi:speaker"
        ),
    }
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_YAMAHA_SERIAL_ID])

    if conf := config.get(CONF_POWER):
        var = await switch.new_switch(conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_power_switch(var))

    if conf := config.get(CONF_MAIN_ZONE_POWER):
        var = await switch.new_switch(conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_main_zone_power_switch(var))

    if conf := config.get(CONF_MUTE):
        var = await switch.new_switch(conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_mute_switch(var))

    if conf := config.get(CONF_ZONE2_POWER):
        var = await switch.new_switch(conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_zone2_power_switch(var))

    if conf := config.get(CONF_PURE_DIRECT):
        var = await switch.new_switch(conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_pure_direct_switch(var))

    if conf := config.get(CONF_FAN_MODE):
        var = await switch.new_switch(conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_fan_mode_switch(var))

    if conf := config.get(CONF_ZONE2_MUTE):
        var = await switch.new_switch(conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_zone2_mute_switch(var))

    if conf := config.get(CONF_SPEAKER_A):
        var = await switch.new_switch(conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_speaker_a_switch(var))

    if conf := config.get(CONF_SPEAKER_B):
        var = await switch.new_switch(conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_speaker_b_switch(var))
