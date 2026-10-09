"""Media player wrapper for ESPHome Yamaha Serial receiver entities."""

from __future__ import annotations

from collections.abc import Iterable
from typing import Any

import voluptuous as vol
from homeassistant.components.media_player import PLATFORM_SCHEMA, MediaPlayerEntity
from homeassistant.components.media_player.const import (
    ATTR_MEDIA_VOLUME_LEVEL,
    MediaPlayerEntityFeature,
    MediaPlayerState,
)
from homeassistant.components.media_player.const import (
    DOMAIN as MEDIA_PLAYER_DOMAIN,
)
from homeassistant.components.select import DOMAIN as SELECT_DOMAIN
from homeassistant.const import (
    ATTR_ENTITY_ID,
    CONF_NAME,
    CONF_UNIQUE_ID,
    STATE_OFF,
    STATE_ON,
    STATE_UNAVAILABLE,
    STATE_UNKNOWN,
)
from homeassistant.core import Event, HomeAssistant
from homeassistant.helpers import config_validation as cv
from homeassistant.helpers.entity_platform import AddEntitiesCallback
from homeassistant.helpers.event import async_track_state_change_event
from homeassistant.helpers.typing import ConfigType, DiscoveryInfoType

from .const import (
    CONF_AVAILABILITY_ENTITY,
    CONF_MEDIA_PLAYER_ENTITY,
    CONF_MODEL,
    CONF_MUTE_ENTITY,
    CONF_POWER_ENTITY,
    CONF_SOUND_MODE_ENTITY,
    CONF_SOURCE_ENTITY,
    CONF_VOLUME_ENTITY,
)

ATTR_OPTIONS = "options"
ATTR_VOLUME_MUTED = "is_volume_muted"

SERVICE_SELECT_OPTION = "select_option"
SERVICE_TURN_ON = "turn_on"
SERVICE_TURN_OFF = "turn_off"
SERVICE_VOLUME_MUTE = "volume_mute"
SERVICE_VOLUME_SET = "volume_set"
SERVICE_VOLUME_UP = "volume_up"
SERVICE_VOLUME_DOWN = "volume_down"

DEFAULT_NAME = "Yamaha Receiver"
ATTRIBUTION = "ESPHome Yamaha Serial companion media player"

PLATFORM_SCHEMA = PLATFORM_SCHEMA.extend(
    {
        vol.Optional(CONF_NAME, default=DEFAULT_NAME): cv.string,
        vol.Optional(CONF_UNIQUE_ID): cv.string,
        vol.Required(CONF_MEDIA_PLAYER_ENTITY): cv.entity_id,
        vol.Optional(CONF_POWER_ENTITY): cv.entity_id,
        vol.Optional(CONF_MUTE_ENTITY): cv.entity_id,
        vol.Optional(CONF_VOLUME_ENTITY): cv.entity_id,
        vol.Optional(CONF_SOURCE_ENTITY): cv.entity_id,
        vol.Optional(CONF_SOUND_MODE_ENTITY): cv.entity_id,
        vol.Optional(CONF_AVAILABILITY_ENTITY): cv.entity_id,
        vol.Optional(CONF_MODEL): cv.string,
    }
)


async def async_setup_platform(
    hass: HomeAssistant,
    config: ConfigType,
    async_add_entities: AddEntitiesCallback,
    discovery_info: DiscoveryInfoType | None = None,
) -> None:
    """Set up a YAML-configured Yamaha Serial Receiver media player."""
    async_add_entities([YamahaSerialReceiverMediaPlayer(hass, config)])


class YamahaSerialReceiverMediaPlayer(MediaPlayerEntity):
    """Expose several ESPHome Yamaha entities as one native HA media player."""

    _attr_icon = "mdi:audio-video"
    _attr_has_entity_name = False
    _attr_should_poll = False
    _attr_attribution = ATTRIBUTION

    def __init__(self, hass: HomeAssistant, config: ConfigType) -> None:
        """Initialize the wrapper entity."""
        self.hass = hass
        self._attr_name = config[CONF_NAME]
        self._attr_unique_id = config.get(CONF_UNIQUE_ID)
        self._model = config.get(CONF_MODEL)
        self._media_player_entity = config[CONF_MEDIA_PLAYER_ENTITY]
        self._power_entity = config.get(CONF_POWER_ENTITY)
        self._mute_entity = config.get(CONF_MUTE_ENTITY)
        self._volume_entity = config.get(CONF_VOLUME_ENTITY)
        self._source_entity = config.get(CONF_SOURCE_ENTITY)
        self._sound_mode_entity = config.get(CONF_SOUND_MODE_ENTITY)
        self._availability_entity = config.get(CONF_AVAILABILITY_ENTITY)
        self._tracked_entities = tuple(
            _defined(
                (
                    self._media_player_entity,
                    self._power_entity,
                    self._mute_entity,
                    self._volume_entity,
                    self._source_entity,
                    self._sound_mode_entity,
                    self._availability_entity,
                )
            )
        )

    async def async_added_to_hass(self) -> None:
        """Subscribe to state changes from all wrapped entities."""
        self.async_on_remove(
            async_track_state_change_event(
                self.hass,
                self._tracked_entities,
                self._handle_wrapped_entity_change,
            )
        )

    @property
    def available(self) -> bool:
        """Return whether the wrapped receiver should be considered available."""
        availability = self._state(self._availability_entity)
        if availability is not None and availability.state in {STATE_OFF, STATE_UNAVAILABLE}:
            return False

        media_player = self._state(self._media_player_entity)
        return media_player is not None and media_player.state not in {STATE_UNAVAILABLE, STATE_UNKNOWN}

    @property
    def state(self) -> MediaPlayerState | None:
        """Return the current receiver state."""
        power = self._state(self._power_entity)
        if power is not None and power.state in {STATE_ON, STATE_OFF}:
            return MediaPlayerState.IDLE if power.state == STATE_ON else MediaPlayerState.OFF

        media_player = self._state(self._media_player_entity)
        if media_player is None:
            return None
        try:
            return MediaPlayerState(media_player.state)
        except ValueError:
            return None

    @property
    def supported_features(self) -> MediaPlayerEntityFeature:
        """Return the feature flags this wrapper can execute reliably."""
        features = (
            MediaPlayerEntityFeature.TURN_ON
            | MediaPlayerEntityFeature.TURN_OFF
            | MediaPlayerEntityFeature.VOLUME_SET
            | MediaPlayerEntityFeature.VOLUME_STEP
            | MediaPlayerEntityFeature.VOLUME_MUTE
        )
        if self._source_entity is not None:
            features |= MediaPlayerEntityFeature.SELECT_SOURCE
        if self._sound_mode_entity is not None:
            features |= MediaPlayerEntityFeature.SELECT_SOUND_MODE
        return features

    @property
    def volume_level(self) -> float | None:
        """Return normalized volume in Home Assistant's 0..1 range."""
        media_player = self._state(self._media_player_entity)
        if media_player is not None:
            value = media_player.attributes.get(ATTR_MEDIA_VOLUME_LEVEL)
            if isinstance(value, int | float):
                return max(0.0, min(1.0, float(value)))

        volume = self._state(self._volume_entity)
        if volume is None:
            return None
        try:
            raw = float(volume.state)
        except (TypeError, ValueError):
            return None
        minimum = _float_attr(volume.attributes, "min", -80.0)
        maximum = _float_attr(volume.attributes, "max", 16.0)
        if maximum <= minimum:
            return None
        return max(0.0, min(1.0, (raw - minimum) / (maximum - minimum)))

    @property
    def is_volume_muted(self) -> bool | None:
        """Return whether the receiver is muted."""
        mute = self._state(self._mute_entity)
        if mute is not None and mute.state in {STATE_ON, STATE_OFF}:
            return mute.state == STATE_ON

        media_player = self._state(self._media_player_entity)
        if media_player is not None:
            value = media_player.attributes.get(ATTR_VOLUME_MUTED)
            if isinstance(value, bool):
                return value
        return None

    @property
    def source(self) -> str | None:
        """Return the active receiver input source."""
        return self._select_state(self._source_entity)

    @property
    def source_list(self) -> list[str] | None:
        """Return available receiver input sources."""
        return self._select_options(self._source_entity)

    @property
    def sound_mode(self) -> str | None:
        """Return the active receiver sound mode."""
        return self._select_state(self._sound_mode_entity)

    @property
    def sound_mode_list(self) -> list[str] | None:
        """Return available receiver sound modes."""
        return self._select_options(self._sound_mode_entity)

    @property
    def extra_state_attributes(self) -> dict[str, Any]:
        """Return additional diagnostic attributes for the wrapper."""
        attrs: dict[str, Any] = {
            "wrapped_media_player": self._media_player_entity,
        }
        if self._model is not None:
            attrs["model"] = self._model
        return attrs

    async def async_turn_on(self) -> None:
        """Turn the receiver on."""
        await self._call_power_or_media_player(True)

    async def async_turn_off(self) -> None:
        """Turn the receiver off."""
        await self._call_power_or_media_player(False)

    async def async_mute_volume(self, mute: bool) -> None:
        """Mute or unmute the receiver."""
        if self._mute_entity is not None:
            await self._call_service(
                "switch",
                SERVICE_TURN_ON if mute else SERVICE_TURN_OFF,
                {ATTR_ENTITY_ID: self._mute_entity},
            )
            return
        await self._call_service(
            MEDIA_PLAYER_DOMAIN,
            SERVICE_VOLUME_MUTE,
            {ATTR_ENTITY_ID: self._media_player_entity, ATTR_VOLUME_MUTED: mute},
        )

    async def async_set_volume_level(self, volume: float) -> None:
        """Set normalized receiver volume."""
        await self._call_service(
            MEDIA_PLAYER_DOMAIN,
            SERVICE_VOLUME_SET,
            {ATTR_ENTITY_ID: self._media_player_entity, ATTR_MEDIA_VOLUME_LEVEL: volume},
        )

    async def async_volume_up(self) -> None:
        """Increase receiver volume."""
        await self._call_service(
            MEDIA_PLAYER_DOMAIN,
            SERVICE_VOLUME_UP,
            {ATTR_ENTITY_ID: self._media_player_entity},
        )

    async def async_volume_down(self) -> None:
        """Decrease receiver volume."""
        await self._call_service(
            MEDIA_PLAYER_DOMAIN,
            SERVICE_VOLUME_DOWN,
            {ATTR_ENTITY_ID: self._media_player_entity},
        )

    async def async_select_source(self, source: str) -> None:
        """Select an input source."""
        await self._select_option(self._source_entity, source)

    async def async_select_sound_mode(self, sound_mode: str) -> None:
        """Select a sound mode."""
        await self._select_option(self._sound_mode_entity, sound_mode)

    async def _call_power_or_media_player(self, turn_on: bool) -> None:
        """Route power through a switch when configured, otherwise media_player."""
        service = SERVICE_TURN_ON if turn_on else SERVICE_TURN_OFF
        if self._power_entity is not None:
            await self._call_service("switch", service, {ATTR_ENTITY_ID: self._power_entity})
            return
        await self._call_service(
            MEDIA_PLAYER_DOMAIN,
            service,
            {ATTR_ENTITY_ID: self._media_player_entity},
        )

    async def _select_option(self, entity_id: str | None, option: str) -> None:
        """Call select.select_option for a configured select entity."""
        if entity_id is None:
            return
        await self._call_service(
            SELECT_DOMAIN,
            SERVICE_SELECT_OPTION,
            {ATTR_ENTITY_ID: entity_id, "option": option},
        )

    async def _call_service(self, domain: str, service: str, data: dict[str, Any]) -> None:
        """Call a Home Assistant service and wait for command acceptance."""
        await self.hass.services.async_call(domain, service, data, blocking=True)

    def _handle_wrapped_entity_change(self, event: Event) -> None:
        """Refresh wrapper state after one wrapped entity changes."""
        self.async_write_ha_state()

    def _state(self, entity_id: str | None):
        """Return a wrapped entity state object."""
        if entity_id is None:
            return None
        return self.hass.states.get(entity_id)

    def _select_state(self, entity_id: str | None) -> str | None:
        """Return current state of a select entity."""
        state = self._state(entity_id)
        if state is None or state.state in {STATE_UNKNOWN, STATE_UNAVAILABLE}:
            return None
        return state.state

    def _select_options(self, entity_id: str | None) -> list[str] | None:
        """Return the options attribute from a select entity."""
        state = self._state(entity_id)
        if state is None:
            return None
        options = state.attributes.get(ATTR_OPTIONS)
        if not isinstance(options, list):
            return None
        return [option for option in options if isinstance(option, str)]


def _defined(values: Iterable[str | None]) -> Iterable[str]:
    """Yield non-empty entity IDs while preserving order."""
    seen: set[str] = set()
    for value in values:
        if value is None or value in seen:
            continue
        seen.add(value)
        yield value


def _float_attr(attrs: dict[str, Any], key: str, default: float) -> float:
    """Read a numeric attribute with a safe fallback."""
    try:
        return float(attrs.get(key, default))
    except (TypeError, ValueError):
        return default
