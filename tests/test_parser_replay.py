from __future__ import annotations

import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
CPP = ROOT / "components/yamaha_serial/yamaha_serial.cpp"

STX = 0x02
ETX = 0x03
DC1 = 0x11
DC2 = 0x12
DC4 = 0x14
FRAME_STARTS = {STX, DC1, DC2, DC4}


def checksum_8bit(payload: str) -> str:
    return f"{sum(payload.encode('ascii')) & 0xFF:02X}"


def dc4_frame(payload: str) -> bytes:
    return bytes([DC4]) + payload.encode("ascii") + checksum_8bit(payload).encode("ascii") + bytes([ETX])


def replay_chunks(chunks: list[bytes]) -> list[bytes]:
    in_frame = False
    buffer = bytearray()
    frames: list[bytes] = []
    for chunk in chunks:
        for byte in chunk:
            if byte in FRAME_STARTS:
                in_frame = True
                buffer.clear()
                buffer.append(byte)
                continue
            if not in_frame:
                continue
            buffer.append(byte)
            if byte == ETX:
                frames.append(bytes(buffer))
                in_frame = False
                buffer.clear()
    return frames


def test_replay_handles_fragmented_and_coalesced_yamaha_frames() -> None:
    model_payload = "200F0000F08RX-V1600"
    model_frame = dc4_frame(model_payload)
    status_frame = bytes([STX]) + b"000210" + bytes([ETX])

    frames = replay_chunks(
        [
            b"\x00ignored",
            model_frame[:5],
            model_frame[5:] + status_frame,
        ]
    )

    assert frames == [model_frame, status_frame]
    assert model_frame[1:-3].decode("ascii") == model_payload
    assert model_frame[-3:-1].decode("ascii") == checksum_8bit(model_payload)


def test_dc4_checksum_vector_matches_component_rules() -> None:
    payload = "200F0000F08RX-V1600"
    frame = dc4_frame(payload)
    corrupted = bytearray(frame)
    corrupted[-2] = ord("0") if corrupted[-2] != ord("0") else ord("1")

    assert frame[-3:-1].decode("ascii") == checksum_8bit(payload)
    assert bytes(corrupted)[-3:-1].decode("ascii") != checksum_8bit(payload)


def test_component_parser_keeps_expected_frame_guards() -> None:
    text = CPP.read_text(encoding="utf-8")

    assert "is_frame_start_(byte)" in text
    assert "byte == static_cast<uint8_t>(FrameType::ETX)" in text
    assert 'record_parse_error_("UART frame overflow"' in text
    assert 'record_parse_error_("UART frame interrupted by new frame start"' in text
    assert 'record_parse_error_("DC4 checksum mismatch"' in text
