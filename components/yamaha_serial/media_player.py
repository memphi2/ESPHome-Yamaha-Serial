import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import media_player

from . import CONF_YAMAHA_SERIAL_ID, YAMAHA_SERIAL_COMPONENT_SCHEMA, yamaha_serial_ns

DEPENDENCIES = ["yamaha_serial"]

CONF_RECEIVER = "receiver"

YamahaMediaPlayer = yamaha_serial_ns.class_(
    "YamahaMediaPlayer", media_player.MediaPlayer
)

CONFIG_SCHEMA = YAMAHA_SERIAL_COMPONENT_SCHEMA.extend(
    {
        cv.Optional(CONF_RECEIVER): media_player.media_player_schema(
            YamahaMediaPlayer, icon="mdi:audio-video"
        )
    }
)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_YAMAHA_SERIAL_ID])

    if conf := config.get(CONF_RECEIVER):
        var = await media_player.new_media_player(conf)
        cg.add(var.set_parent(paren))
        cg.add(paren.set_media_player(var))
