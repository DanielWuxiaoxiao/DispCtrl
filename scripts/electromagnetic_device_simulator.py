"""Local UDP protocol peer for the electromagnetic-device simulator interface."""
import argparse
import functools
import operator
import socket
import struct
import time

HEADER = struct.Struct("<IHHIHH")
DETECTION = struct.Struct("<ffffifffffIIfIii")
TRACK = struct.Struct("<HHIIBfffffifffIii")


def encode(source, destination, sequence, message, payload):
    body = HEADER.pack(0xFA55FA55, source, destination, sequence, 16 + len(payload), message) + payload
    return body + bytes([functools.reduce(operator.xor, body, 0)]) + struct.pack("<I", 0x55FA55FA)


def decode(packet):
    if not 21 <= len(packet) <= 1472:
        raise ValueError("invalid datagram size")
    head, source, destination, sequence, length, message = HEADER.unpack_from(packet)
    if head != 0xFA55FA55 or struct.unpack_from("<I", packet, len(packet) - 4)[0] != 0x55FA55FA:
        raise ValueError("invalid head/tail")
    if length != len(packet) - 5 or functools.reduce(operator.xor, packet[:length], 0) != packet[length]:
        raise ValueError("invalid length/checksum")
    return source, destination, sequence, message, packet[16:length]


def describe(message, payload):
    if message in (5, 6):
        if len(payload) < 2:
            raise ValueError("missing record count")
        count = struct.unpack_from("<H", payload)[0]
        record = DETECTION if message == 5 else TRACK
        if len(payload) != 2 + count * record.size:
            raise ValueError("record count/size mismatch")
        text = f"records={count} record_bytes={record.size}"
        if count:
            first = record.unpack_from(payload, 2)
            altitude = first[4] if message == 5 else first[10]
            text += f" first_lon={first[-2]/1e7:.7f} lat={first[-1]/1e7:.7f} alt={altitude}m"
            if message == 6:
                text += f" batch={first[0]} status={first[4]} CPI={first[1]} UTC={first[2]} subsecond={first[3]}"
        return text
    if message == 7 and len(payload) == 15:
        return f"panel_status={payload.hex(' ')}"
    if message == 9 and len(payload) == 4:
        return f"software_status={tuple(payload)}"
    raise ValueError("unexpected report type/length")


def self_test():
    assert HEADER.size == 16 and DETECTION.size == 64 and TRACK.size == 61
    packet = encode(0xBB08, 0xBB04, 1, 1, b"\x01")
    assert len(packet) == 22
    assert packet[:17].hex() == "55fa55fa08bb04bb010000001100010001"
    assert decode(packet) == (0xBB08, 0xBB04, 1, 1, b"\x01")
    for damaged in (packet[:-1], packet[:16] + b"\x00" + packet[17:]):
        try:
            decode(damaged)
        except ValueError:
            pass
        else:
            raise AssertionError("corrupt packet accepted")
    assert describe(5, b"\x00\x00").startswith("records=0")
    assert len(encode(1, 2, 0, 5, b"\x16\x00" + bytes(22 * 64))) == 1431
    assert len(encode(1, 2, 0, 6, b"\x17\x00" + bytes(23 * 61))) == 1426
    print("PASS: frame fixture, checksum rejection, record layouts, zero records, MTU boundaries")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--bind-ip", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=6022)
    parser.add_argument("--display-ip", default="127.0.0.1")
    parser.add_argument("--display-port", type=int, default=8022)
    parser.add_argument("--device-id", type=lambda x: int(x, 0), default=0xBB08)
    parser.add_argument("--display-id", type=lambda x: int(x, 0), default=0xBB04)
    parser.add_argument("--sequence", type=lambda x: int(x, 0), default=1)
    parser.add_argument("--duration", type=float, default=30)
    parser.add_argument("--command", choices=["power", "receive-transmit", "mode", "scene", "software"])
    parser.add_argument("--values", default="", help="comma-separated bytes, e.g. 1 or 1,0")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return
    if args.duration <= 0 or not (1 <= args.port <= 65535 and 1 <= args.display_port <= 65535):
        parser.error("duration and ports must be positive and in range")
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.bind((args.bind_ip, args.port))
        sock.settimeout(0.5)
        peer = (args.display_ip, args.display_port)
        if args.command:
            try:
                values = tuple(int(value.strip(), 0) for value in args.values.split(","))
                payload = bytes(values)
            except ValueError:
                parser.error("--values must be comma-separated bytes")
            message = {"power": 1, "receive-transmit": 2, "mode": 3, "scene": 4, "software": 8}[args.command]
            expected = 2 if message == 2 else 1
            if len(values) != expected or (message == 4 and not 1 <= values[0] <= 5) or (message != 4 and any(v > 1 for v in values)):
                parser.error("command value/count outside protocol range")
            if not (1 <= args.device_id <= 65535 and 1 <= args.display_id <= 65535 and 0 <= args.sequence <= 0xFFFFFFFF):
                parser.error("device IDs/sequence outside protocol range")
            sock.sendto(encode(args.device_id, args.display_id, args.sequence, message, payload), peer)
            print(f"TX message={message} values={values} sequence={args.sequence} peer={peer}", flush=True)
        deadline = time.monotonic() + args.duration
        print(f"Listening {args.bind_ip}:{args.port} for {args.duration}s", flush=True)
        while time.monotonic() < deadline:
            try:
                packet, sender = sock.recvfrom(65535)
            except socket.timeout:
                continue
            if sender != peer:
                continue
            try:
                source, destination, sequence, message, payload = decode(packet)
                if source != args.display_id or destination != args.device_id:
                    raise ValueError("device ID mismatch")
                print(f"RX sequence={sequence} message={message} {describe(message, payload)}", flush=True)
            except ValueError as error:
                print(f"REJECT {error}", flush=True)


if __name__ == "__main__":
    main()
