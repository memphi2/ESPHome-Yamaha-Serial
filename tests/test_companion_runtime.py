"""Exercise the companion with real HA Core, including its event dispatcher."""

import shutil
import threading
from pathlib import Path

import pytest

pytest.importorskip("homeassistant")
import voluptuous as vol
from homeassistant import auth, bootstrap, config_entries, loader
from homeassistant.core import Context, HomeAssistant, callback
from homeassistant.exceptions import ServiceValidationError
from homeassistant.setup import async_setup_component

from custom_components.yamaha_serial_receiver.media_player import (
    PLATFORM_SCHEMA,
    YamahaSerialReceiverMediaPlayer,
)


def config(**overrides):
    return PLATFORM_SCHEMA({
        "platform": "yamaha_serial_receiver",
        "name": "Receiver",
        "media_player_entity": "media_player.transport",
        "source_entity": "select.input",
        "sound_mode_entity": "select.program",
        **overrides,
    })


@pytest.mark.parametrize("key,value", [
    ("media_player_entity", "switch.power"),
    ("source_entity", "sensor.input"),
    ("sound_mode_entity", "sensor.program"),
    ("mute_entity", "sensor.mute"),
    ("power_entity", "binary_sensor.power"),
    ("volume_entity", "sensor.volume"),
])
def test_reject_wrong_entity_domains(key, value):
    with pytest.raises(vol.Invalid):
        config(**{key: value})


@pytest.mark.asyncio
async def test_push_updates_run_on_event_loop_and_unsubscribe(tmp_path):
    hass = HomeAssistant(str(tmp_path))
    entity = YamahaSerialReceiverMediaPlayer(hass, config())
    entity.entity_id = "media_player.receiver"
    loop_thread = threading.get_ident()
    threads = []
    entity.async_write_ha_state = lambda: threads.append(threading.get_ident())
    try:
        await entity.async_added_to_hass()
        hass.states.async_set("select.input", "DVD", {"options": ["DVD", "Tuner"]})
        await hass.async_block_till_done()
        assert threads == [loop_thread]
        assert entity.source == "DVD"
        assert entity.source_list == ["DVD", "Tuner"]
        assert entity.should_poll is False
        await entity.async_remove()
        threads.clear()
        hass.states.async_set("select.input", "Tuner", {"options": ["DVD", "Tuner"]})
        await hass.async_block_till_done()
        assert threads == []
    finally:
        await hass.async_stop()


@pytest.mark.asyncio
async def test_services_use_real_registry_and_preserve_context(tmp_path):
    hass = HomeAssistant(str(tmp_path))
    entity = YamahaSerialReceiverMediaPlayer(hass, config())
    entity.entity_id = "media_player.receiver"
    context = Context()
    entity.async_set_context(context)
    calls = []

    @callback
    def record(call):
        calls.append(call)

    for domain, service in [
        ("select", "select_option"), ("media_player", "turn_on"),
        ("media_player", "turn_off"), ("media_player", "volume_set"),
        ("media_player", "volume_mute"), ("media_player", "volume_up"),
        ("media_player", "volume_down"),
    ]:
        hass.services.async_register(domain, service, record)
    hass.states.async_set("select.input", "DVD", {"options": ["DVD", "Tuner"]})
    hass.states.async_set("select.program", "STRAIGHT", {"options": ["STRAIGHT", "7ch Stereo"]})
    try:
        await entity.async_select_source("Tuner")
        await entity.async_select_sound_mode("7ch Stereo")
        await entity.async_turn_on()
        await entity.async_turn_off()
        await entity.async_set_volume_level(0.25)
        await entity.async_mute_volume(True)
        await entity.async_volume_up()
        await entity.async_volume_down()
        assert [call.service for call in calls] == [
            "select_option", "select_option", "turn_on", "turn_off",
            "volume_set", "volume_mute", "volume_up", "volume_down",
        ]
        assert calls[0].data == {"entity_id": "select.input", "option": "Tuner"}
        assert calls[1].data == {"entity_id": "select.program", "option": "7ch Stereo"}
        assert calls[4].data["volume_level"] == 0.25
        assert calls[5].data["is_volume_muted"] is True
        assert all(call.context is context for call in calls)
        assert entity.source == "DVD"  # No optimistic state before receiver feedback.
        with pytest.raises(ServiceValidationError):
            await entity.async_select_source("not an input")
    finally:
        await hass.async_stop()


@pytest.mark.asyncio
async def test_availability_and_nonfinite_volume(tmp_path):
    hass = HomeAssistant(str(tmp_path))
    entity = YamahaSerialReceiverMediaPlayer(hass, config(availability_entity="binary_sensor.link"))
    try:
        hass.states.async_set("media_player.transport", "idle", {"volume_level": float("nan")})
        assert entity.volume_level is None
        assert not entity.available
        for state in ("unknown", "unavailable", "off"):
            hass.states.async_set("binary_sensor.link", state)
            assert not entity.available
        hass.states.async_set("binary_sensor.link", "on")
        assert entity.available
        hass.states.async_set("media_player.transport", "off", {"volume_level": 0.25})
        assert entity.available
        assert entity.volume_level == 0.25
        assert entity.state == "off"
        hass.states.async_remove("media_player.transport")
        assert not entity.available
    finally:
        await hass.async_stop()


@pytest.mark.asyncio
async def test_self_reference_rejected(tmp_path):
    hass = HomeAssistant(str(tmp_path))
    entity = YamahaSerialReceiverMediaPlayer(hass, config())
    entity.entity_id = "media_player.transport"
    try:
        with pytest.raises(ServiceValidationError):
            await entity.async_added_to_hass()
    finally:
        await hass.async_stop()


@pytest.mark.asyncio
async def test_yaml_platform_loads_and_publishes_native_attributes(tmp_path):
    integration = Path(__file__).resolve().parents[1] / "custom_components/yamaha_serial_receiver"
    shutil.copytree(integration, tmp_path / "custom_components/yamaha_serial_receiver")
    hass = HomeAssistant(str(tmp_path))
    loader.async_setup(hass)
    hass.config_entries = config_entries.ConfigEntries(hass, {})
    await bootstrap.async_load_base_functionality(hass)
    hass.auth = await auth.auth_manager_from_config(hass, [], [])
    hass.states.async_set("media_player.transport", "idle", {
        "volume_level": 0.3, "is_volume_muted": False,
    })
    hass.states.async_set("select.input", "DVD", {"options": ["DVD", "Tuner"]})
    hass.states.async_set("select.program", "STRAIGHT", {"options": ["STRAIGHT"]})
    try:
        assert await async_setup_component(hass, "media_player", {"media_player": [config()]})
        await hass.async_block_till_done()
        state = hass.states.get("media_player.receiver")
        assert state is not None
        assert state.state == "idle"
        assert state.attributes["source"] == "DVD"
        assert state.attributes["source_list"] == ["DVD", "Tuner"]
        assert state.attributes["sound_mode"] == "STRAIGHT"
        assert state.attributes["volume_level"] == 0.3
        hass.states.async_set("select.input", "Tuner", {"options": ["DVD", "Tuner"]})
        await hass.async_block_till_done()
        assert hass.states.get("media_player.receiver").attributes["source"] == "Tuner"
    finally:
        await hass.async_stop()
