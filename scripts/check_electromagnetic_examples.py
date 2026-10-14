"""Read-only verification of the partner's deterministic outbound-frame examples."""
import argparse
import math
from pathlib import Path
import re
import struct

import electromagnetic_device_simulator as protocol


def rounded(value):
    # Match C++ std::round, not Python's ties-to-even round.
    return math.floor(value + 0.5) if value >= 0 else math.ceil(value - 0.5)


def fixtures():
    horizontal = 1000.0 * math.cos(math.radians(30.0))
    east = horizontal * math.sin(math.radians(90.0))
    north = horizontal * math.cos(math.radians(90.0))
    longitude = rounded((120.0 + east / (6378137.0 * math.cos(math.radians(30.0)) * math.pi / 180.0)) * 1e7)
    latitude = rounded((30.0 + north / (6378137.0 * math.pi / 180.0)) * 1e7)
    height = rounded(100.0 + 1000.0 * math.sin(math.radians(30.0)))
    assert (longitude, latitude, height) == (1200089832, 300000000, 600)
    detection = protocol.DETECTION.pack(
        1000, 15, 90, 30, height, 20, 24, 22, 90, 30, 100, 8, 0.75, 1, longitude, latitude)

    def track(status):
        return protocol.TRACK.pack(101, 19, 1700000000, 500000000, status,
                                   20, 24, 1000, 90, 30, height, 15, 15, 0, 1, longitude, latitude)

    panel = struct.pack("<BBHHH5BH", 0x9F, 1, 450, 400, 9000, 255, 255, 255, 255, 15, 12000)
    return [
        ("detection", 1, 5, b"\x01\x00" + detection, 87, 0x1C),
        ("track", 2, 6, b"\x01\x00" + track(0), 84, 0x91),
        ("panel", 3, 7, panel, 36, 0x11),
        ("software", 4, 9, bytes(4), 25, 0x15),
        ("remove", 5, 6, b"\x01\x00" + track(2), 84, 0x94),
        ("empty_detection", 6, 5, bytes(2), 23, 0x1D),
        ("empty_track", 7, 6, bytes(2), 23, 0x1F),
        ("prediction", 8, 6, b"\x01\x00" + track(1), 84, 0x9A),
    ]


def verify(path):
    source = path.read_text(encoding="utf-8-sig")
    matches = re.findall(r"<!-- frame:(\w+) -->\s*```text\s*([0-9A-Fa-f\s]+)```", source)
    assert len(matches) == 8 and len(dict(matches)) == 8, "expected eight unique full-frame blocks"
    examples = {name: bytes.fromhex(text) for name, text in matches}
    assert protocol.HEADER.size == 16 and protocol.DETECTION.size == 64 and protocol.TRACK.size == 61
    for name, sequence, message, payload, size, checksum in fixtures():
        expected = protocol.encode(0xBB04, 0xBB08, sequence, message, payload)
        actual = examples[name]
        assert actual == expected, f"{name}: full-frame byte mismatch"
        assert len(actual) == size and actual[-5] == checksum, f"{name}: size/XOR mismatch"
        assert struct.unpack_from("<H", actual, 12)[0] == size - 5
        assert protocol.decode(actual) == (0xBB04, 0xBB08, sequence, message, payload)
        detail = protocol.describe(message, payload)
        # A one-byte payload change must be rejected until the checksum is recomputed.
        corrupted = bytearray(actual)
        corrupted[16] ^= 1
        try:
            protocol.decode(corrupted)
        except ValueError:
            pass
        else:
            raise AssertionError(f"{name}: corruption was accepted")
        print(f"PASS {name}: message={message} seq={sequence} bytes={size} XOR={checksum:02X} {detail}")
    print("PASS: all 8 document frames, deterministic fields, geometry, lengths and corruption rejection")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("document", nargs="?", type=Path,
                        default=Path(__file__).resolve().parents[1] / "docs" / "电磁设备上报帧字节流示例.md")
    verify(parser.parse_args().document)
