"""Export all configured GUI test tracks as deterministic outbound electromagnetic frames.

No sockets, GUI launch, wall-clock waiting or project build. Sampling uses the nominal
configured interval, not a recording of live QTimer jitter. Existing examples are untouched.
"""
import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import math
from pathlib import Path
import re
import struct
import tomllib
from zipfile import ZipFile, ZIP_DEFLATED

import electromagnetic_device_simulator as wire

ROOT = Path(__file__).resolve().parents[1]
F32 = struct.Struct("<f")


def f32(value):
    return F32.unpack(F32.pack(value))[0]


def rounded(value):
    return math.floor(value + 0.5) if value >= 0 else math.ceil(value - 0.5)


def profiles_from_source():
    source = (ROOT / "Controller/testtrackgenerator.cpp").read_text(encoding="utf-8-sig")
    block = re.search(r"constexpr TestTrackProfile kTrackProfiles\[\]\s*=\s*\{(.*?)\n\};", source, re.S)
    if not block:
        raise ValueError("test-track profile table not found")
    rows = re.findall(r"\{([^{}]+)\}", block[1])
    profiles = []
    for row in rows:
        fields = [field.strip() for field in row.split(",")]
        if len(fields) != 6 or fields[5] not in ("true", "false"):
            raise ValueError("test-track profile schema changed")
        profiles.append(tuple(float(field) for field in fields[:5]) + (fields[5] == "true",))
    if len(profiles) != 10:
        raise ValueError("expected current generator's 10 profiles")
    return profiles


def position(profile, speed, elapsed):
    initial_range, azimuth, elevation, course, climb, recognized = profile
    az, el, heading, tilt = (degrees * math.pi / 180.0 for degrees in (azimuth, elevation, course, climb))
    horizontal = initial_range * math.cos(el)
    east = horizontal * math.sin(az) + speed * math.cos(tilt) * math.sin(heading) * elapsed
    north = horizontal * math.cos(az) + speed * math.cos(tilt) * math.cos(heading) * elapsed
    up = initial_range * math.sin(el) + speed * math.sin(tilt) * elapsed
    distance = math.hypot(math.hypot(east, north), up)
    azimuth = math.fmod(math.atan2(east, north) * 180.0 / math.pi, 360.0)
    if azimuth < 0:
        azimuth += 360.0
    elevation = math.atan2(up, math.hypot(east, north)) * 180.0 / math.pi
    # Match PointInfo's float fields before the actual outbound geographic conversion.
    return f32(distance), f32(azimuth), f32(elevation), int(recognized)


def coordinates(distance, azimuth, elevation, origin):
    longitude, latitude, altitude = origin
    factor = math.pi / 180.0
    az = math.fmod(azimuth, 360.0) * factor
    el = elevation * factor
    horizontal = distance * math.cos(el)
    east, north, up = horizontal * math.sin(az), horizontal * math.cos(az), distance * math.sin(el)
    lon = rounded((longitude + east / (6378137.0 * math.cos(latitude * factor) * factor)) * 1e7)
    lat = rounded((latitude + north / (6378137.0 * factor)) * 1e7)
    height = rounded(altitude + up)
    if not (-1800000000 <= lon <= 1800000000 and -900000000 <= lat <= 900000000):
        raise ValueError("target geographic coordinate out of range")
    return lon, lat, height


def frames_from_config(config, start_ms):
    settings = config["test_track"]
    count = max(2, min(10, int(settings.get("track_count", 10))))
    samples = max(1, min(10000, int(settings.get("point_count", 1200))))
    interval = max(100, min(2000, int(settings.get("interval_ms", 500))))
    speed = float(settings.get("speed_mps", 15.0))
    if not math.isfinite(speed) or not 0 < speed <= 300:
        speed = 15.0
    first = int(settings.get("first_batch", 101))
    if first <= 0 or first > 0xffffffff - count:
        first = 101
    if first + count - 1 > 65535:
        raise ValueError("electromagnetic uint16 batch range exceeded")
    radar = config["radar"]
    origin = tuple(float(radar[key]) for key in ("longitude", "latitude", "altitude"))
    if not all(math.isfinite(value) for value in origin) or not (-180 <= origin[0] <= 180 and -90 < origin[1] < 90):
        raise ValueError("invalid configured radar origin")
    peer = config["electromagnetic_device"]
    identifier = lambda value: int(value, 0) if isinstance(value, str) else int(value)
    source, destination = (identifier(peer.get(key, fallback)) for key, fallback in
                           (("local_device_id", "0xBB04"), ("peer_device_id", "0xBB08")))
    if not (0 <= source <= 65535 and 0 <= destination <= 65535):
        raise ValueError("invalid device IDs")
    profiles = profiles_from_source()[:count]
    packets, last_records = [], []
    for sample in range(samples):
        elapsed_ms = sample * interval
        utc_seconds, remainder_ms = divmod(start_ms + elapsed_ms, 1000)
        if not 0 <= utc_seconds <= 0xffffffff:
            raise ValueError("UTC seconds exceed protocol uint32")
        records = []
        for index, profile in enumerate(profiles):
            distance, azimuth, elevation, recognized = position(profile, speed, elapsed_ms / 1000.0)
            lon, lat, height = coordinates(distance, azimuth, elevation, origin)
            # Match prepareTestPoint + encodeTrack: CPI/acceleration=0; vel and spaceVel both PointInfo.speed.
            records.append(wire.TRACK.pack(first + index, 0, utc_seconds, remainder_ms * 1000000, 0,
                                           1.0, 24.0, distance, azimuth, elevation, height,
                                           f32(speed), f32(speed), 0.0, recognized, lon, lat))
        payload = struct.pack("<H", count) + b"".join(records)
        packets.append(wire.encode(source, destination, sample + 1, 6, payload))
        last_records = records
    # Runtime deletion reuses the last encoded record, changing only status at record offset 12.
    removed = [record[:12] + b"\x02" + record[13:] for record in last_records]
    packets.append(wire.encode(source, destination, samples + 1, 6, struct.pack("<H", count) + b"".join(removed)))
    return packets, (count, samples, interval, first, speed, origin, source, destination)


def verify(blob, info):
    count, samples, interval, first, speed, origin, source, destination = info
    offset, number = 0, 0
    normal, removed = Counter(), Counter()
    previous_records = None
    while offset < len(blob):
        if offset + 16 > len(blob):
            raise AssertionError("truncated header")
        length = struct.unpack_from("<H", blob, offset + 12)[0] + 5
        packet = blob[offset:offset + length]
        src, dst, sequence, message, payload = wire.decode(packet)
        assert (src, dst, sequence, message) == (source, destination, number + 1, 6)
        assert length <= 1472 and length == 23 + count * 61
        assert struct.unpack_from("<H", payload)[0] == count
        records = [payload[2 + i * 61:2 + (i + 1) * 61] for i in range(count)]
        for index, record in enumerate(records):
            values = wire.TRACK.unpack(record)
            batch, cpi, utc, subsecond, status, amp, snr, distance, azimuth, elevation, height, vel, space_vel, acc, rec, lon, lat = values
            assert batch == first + index and cpi == 0
            assert all(math.isfinite(value) for value in (amp, snr, distance, azimuth, elevation, vel, space_vel, acc))
            assert distance >= 0 and 0 <= azimuth <= 360 and -90 <= elevation <= 90
            assert (lon, lat, height) == coordinates(distance, azimuth, elevation, origin)
            assert (amp, snr, vel, space_vel, acc) == (1, 24, f32(speed), f32(speed), 0)
            if number < samples:
                assert status == 0
                normal[batch] += 1
                if previous_records:
                    previous = wire.TRACK.unpack(previous_records[index])
                    assert (utc - previous[2]) * 1000000000 + subsecond - previous[3] == interval * 1000000
            else:
                assert status == 2
                assert record[:12] + b"\x00" + record[13:] == previous_records[index]
                removed[batch] += 1
        previous_records = records
        number += 1
        offset += length
    assert offset == len(blob) and number == samples + 1
    assert normal == Counter({first + i: samples for i in range(count)})
    assert removed == Counter({first + i: 1 for i in range(count)})
    # Independently reject a changed payload without fixing XOR.
    corrupted = bytearray(blob[:23 + count * 61]); corrupted[25] ^= 1
    try:
        wire.decode(corrupted)
    except ValueError:
        pass
    else:
        raise AssertionError("corrupted frame accepted")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", type=Path, default=ROOT / "config.toml")
    parser.add_argument("--output-dir", type=Path, default=ROOT / "docs/electromagnetic_frames")
    args = parser.parse_args()
    config = tomllib.loads(args.config.read_text(encoding="utf-8-sig"))
    start_ms = int(datetime(2026, 10, 20, tzinfo=timezone.utc).timestamp()) * 1000
    packets, info = frames_from_config(config, start_ms)
    count, samples, interval, first, speed, origin, source, destination = info
    stem = f"06_test_tracks_all_{count}x{samples}"
    binary = args.output_dir / (stem + ".bin")
    description = args.output_dir / (stem + "_说明.txt")
    archive = args.output_dir / (stem + ".zip")
    if any(path.exists() for path in (binary, description, archive)):
        raise FileExistsError("output already exists; choose another output directory")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    blob = b"".join(packets)
    verify(blob, info)
    text = (
        f"{binary.name}：{count}条航迹（{first}～{first + count - 1}），各{samples}点；{samples + 1}帧，"
        f"共{count * (samples + 1)}条记录（末帧含{count}条消批）。\n"
        f"编号6，每帧{count}条，{len(packets[0])}字节；小端，帧长=偏移12的uint16+5；UDP逐帧发送，勿整文件一包。\n"
        f"前{samples}帧每{interval}ms发送，末帧紧随上一帧；起时2026-10-20 00:00:00 UTC。\n"
        f"源0x{source:04X} → 目的0x{destination:04X}；原点经纬高={origin[0]:.9f}°/{origin[1]:.9f}°/{origin[2]}m。\n"
        f"按当前测试发生器全部预设、{speed:g}m/s及标称间隔生成，含无人机与其它目标；非实机抓包，不做距离/高度筛选。\n"
    )
    binary.write_bytes(blob)
    description.write_text(text, encoding="utf-8")
    verify(binary.read_bytes(), info)
    with ZipFile(archive, "w", compression=ZIP_DEFLATED) as output:
        output.writestr(binary.name, blob)
        output.writestr("说明.txt", text.encode("utf-8"))
    with ZipFile(archive) as check:
        assert check.testzip() is None and check.read(binary.name) == blob
    print(f"PASS {binary}: frames={samples + 1} records={count * (samples + 1)} bytes={len(blob)}")
    print(f"SHA256 {hashlib.sha256(blob).hexdigest()}")
    print(f"ZIP {archive}")


if __name__ == "__main__":
    main()
