#!/usr/bin/env python3
"""Live UDP viewer for CameraCaptureUdpStream sketch.

Protocol:
- UDP packets are sent to the configured IP:PORT
- Each frame is split into chunks
- Each chunk packet begins with a 16-byte header:
    b'FRAME' + payload_size:u32 + frame_id:u32 + chunk_index:u16 + chunk_count:u16
- The remaining bytes in the packet are the raw RGB565 pixel payload for that chunk

Default frame format in the sketch is 320x240 RGB565.
"""

from __future__ import annotations

import argparse
import socket
import struct
import sys
import time
from collections import defaultdict

import cv2
import numpy as np

MAGIC = b"FRAME"
HEADER_STRUCT = struct.Struct("<4sIIHH")
HEADER_SIZE = HEADER_STRUCT.size


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="View Arduino camera frames over UDP")
    parser.add_argument("--listen-port", type=int, default=5005, help="UDP port to listen on (default: 5005)")
    parser.add_argument("--width", type=int, default=320, help="Frame width (default: 320)")
    parser.add_argument("--height", type=int, default=240, help="Frame height (default: 240)")
    parser.add_argument(
        "--format",
        default="rgb565_le",
        choices=["rgb565_le", "rgb565_be", "bgr565_le", "bgr565_be", "rgb565_byte_swapped"],
        help="Pixel format and endianness (default: rgb565_le)",
    )
    parser.add_argument(
        "--vflip",
        action="store_true",
        help="Flip image vertically (upside down correction)",
    )
    parser.add_argument(
        "--hmirror",
        action="store_true",
        help="Mirror image horizontally (left-right correction)",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=1.5,
        help="Receive timeout in seconds before dropping a partial frame",
    )
    return parser.parse_args()


def convert_frame(frame_bytes: bytes, width: int, height: int, pixel_format: str) -> np.ndarray:
    """Convert packed RGB565 bytes to BGR image for OpenCV display."""
    if pixel_format == "rgb565_le":
        pixels = np.frombuffer(frame_bytes, dtype="<u2").reshape((height, width))
    elif pixel_format == "rgb565_be":
        pixels = np.frombuffer(frame_bytes, dtype=">u2").reshape((height, width))
    elif pixel_format == "bgr565_le":
        pixels = np.frombuffer(frame_bytes, dtype="<u2").reshape((height, width))
    elif pixel_format == "bgr565_be":
        pixels = np.frombuffer(frame_bytes, dtype=">u2").reshape((height, width))
    elif pixel_format == "rgb565_byte_swapped":
        raw = np.frombuffer(frame_bytes, dtype="<u2")
        pixels = ((raw & 0xFF) << 8) | ((raw >> 8) & 0xFF)
        pixels = pixels.reshape((height, width))
    else:
        raise ValueError(f"Unknown format: {pixel_format}")

    if pixel_format.startswith("bgr"):
        b_val = ((pixels >> 11) & 0x1F).astype(np.uint8)
        g_val = ((pixels >> 5) & 0x3F).astype(np.uint8)
        r_val = (pixels & 0x1F).astype(np.uint8)
    else:
        r_val = ((pixels >> 11) & 0x1F).astype(np.uint8)
        g_val = ((pixels >> 5) & 0x3F).astype(np.uint8)
        b_val = (pixels & 0x1F).astype(np.uint8)

    r_val = (r_val << 3) | (r_val >> 2)
    g_val = (g_val << 2) | (g_val >> 4)
    b_val = (b_val << 3) | (b_val >> 2)

    return np.dstack((b_val, g_val, r_val))


def pad_frame(frame_bytes: bytes, size: int) -> bytes:
    if len(frame_bytes) >= size:
        return frame_bytes[:size]
    return frame_bytes + b"\x00" * (size - len(frame_bytes))


def recv_udp_frame(sock: socket.socket, timeout_s: float, expected_size: int):
    """Receive one complete frame from the UDP stream.

    Returns a tuple (frame_bytes, ok) where ok indicates the frame was complete.
    Only valid FRAME packets are accepted; frames are reassembled from chunk index order.
    """
    sock.settimeout(timeout_s)
    frame_buffers: dict[int, dict[int, bytes]] = {}
    frame_sizes: dict[int, int] = {}
    frame_counts: dict[int, int] = {}
    deadline = time.monotonic() + timeout_s

    while True:
        try:
            data, _ = sock.recvfrom(65535)
        except socket.timeout:
            break

        if len(data) < HEADER_SIZE:
            continue

        magic, payload_size, frame_id, chunk_index, chunk_count = HEADER_STRUCT.unpack(data[:HEADER_SIZE])
        if magic != MAGIC:
            print(f"Skipping invalid packet magic: {magic!r}", file=sys.stderr)
            continue

        if payload_size <= 0:
            continue

        if chunk_index >= chunk_count:
            continue

        frame_buffers.setdefault(frame_id, {})[chunk_index] = data[HEADER_SIZE:]
        frame_sizes[frame_id] = payload_size
        frame_counts[frame_id] = chunk_count

        chunks = frame_buffers[frame_id]
        if len(chunks) == chunk_count:
            ordered = b"".join(chunks[i] for i in range(chunk_count))
            if len(ordered) >= payload_size:
                return ordered[:payload_size], True

        if time.monotonic() >= deadline:
            break

    for frame_id, chunks in frame_buffers.items():
        chunk_count = frame_counts.get(frame_id, 0)
        if chunk_count <= 0 or len(chunks) < chunk_count:
            continue

        ordered = b"".join(chunks[i] for i in range(chunk_count) if i in chunks)
        if len(ordered) >= frame_sizes.get(frame_id, 0):
            return ordered[: frame_sizes[frame_id]], True

    return b"", False


def main() -> int:
    args = parse_args()
    expected_size = args.width * args.height * 2

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", args.listen_port))
    sock.settimeout(args.timeout)

    print(f"Listening for UDP camera stream on 0.0.0.0:{args.listen_port} (accepting packets from any sender)")
    print(f"Expecting {args.width}x{args.height} RGB565 frames ({expected_size} bytes)")
    print("Press q in the image window to quit")

    cv2.namedWindow("UDP Camera", cv2.WINDOW_NORMAL)

    try:
        while True:
            try:
                frame_bytes, complete = recv_udp_frame(sock, args.timeout, expected_size)
            except KeyboardInterrupt:
                raise

            if not complete:
                print("Timeout or incomplete frame; waiting for next frame...", file=sys.stderr)
                continue

            frame_bytes = pad_frame(frame_bytes, expected_size)
            frame = convert_frame(frame_bytes, args.width, args.height, args.format)

            if args.vflip:
                frame = cv2.flip(frame, 0)
            if args.hmirror:
                frame = cv2.flip(frame, 1)

            cv2.imshow("UDP Camera", frame)
            key = cv2.waitKey(1) & 0xFF
            if key == ord("q"):
                break

    except KeyboardInterrupt:
        pass
    finally:
        sock.close()
        cv2.destroyAllWindows()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
