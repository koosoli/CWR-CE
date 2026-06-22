#!/usr/bin/env python3
"""Generate a tiny synthetic ODOL v7 fixture with morph animation frames."""

from __future__ import annotations

import math
import struct
from pathlib import Path


OUT = Path(__file__).with_name("animated_morph_odol.p3d")


def p(fmt: str, *values: object) -> bytes:
    return struct.pack("<" + fmt, *values)


def u32_array(values: list[int]) -> bytes:
    return p("I", len(values)) + b"".join(p("I", v) for v in values)


def u16_array(values: list[int]) -> bytes:
    return p("I", len(values)) + b"".join(p("H", v) for v in values)


def vec2_array(values: list[tuple[float, float]]) -> bytes:
    return p("I", len(values)) + b"".join(p("ff", *v) for v in values)


def vec3_array(values: list[tuple[float, float, float]]) -> bytes:
    return p("I", len(values)) + b"".join(p("fff", *v) for v in values)


def list_count(count: int) -> bytes:
    return p("I", count)


def asciiz(text: str) -> bytes:
    return text.encode("ascii") + b"\0"


def bounds(
    points: list[tuple[float, float, float]],
) -> tuple[
    tuple[float, float, float],
    tuple[float, float, float],
    tuple[float, float, float],
    float,
]:
    mn = tuple(min(point[i] for point in points) for i in range(3))
    mx = tuple(max(point[i] for point in points) for i in range(3))
    center = tuple((mn[i] + mx[i]) * 0.5 for i in range(3))
    radius = max(math.dist(center, point) for point in points)
    return mn, mx, center, radius


def main() -> None:
    # A made-up hinged panel. The frame positions lift opposite corners to prove
    # ODOL morph frames are loaded as per-vertex Shape animation phases.
    base_points = [
        (-1.0, 0.0, -0.25),
        (1.0, 0.0, -0.25),
        (1.0, 0.0, 0.25),
        (-1.0, 0.0, 0.25),
    ]
    normals = [(0.0, 1.0, 0.0)] * 4
    uvs = [(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0)]
    frames = [
        (0.0, base_points),
        (
            0.5,
            [
                (-1.0, 0.45, -0.25),
                (1.0, -0.35, -0.25),
                (1.0, -0.35, 0.25),
                (-1.0, 0.45, 0.25),
            ],
        ),
        (1.0, base_points),
    ]

    mn, mx, center, radius = bounds([point for _, frame in frames for point in frame])

    data = bytearray()
    data += b"ODOL"
    data += p("I", 7)
    data += p("I", 1)

    # Vertex table.
    data += u32_array([0, 0, 0, 0])
    data += vec2_array(uvs)
    data += vec3_array(base_points)
    data += vec3_array(normals)

    # LOD bounds.
    data += p("ii", 0, 0)
    data += p("fff", *mn)
    data += p("fff", *mx)
    data += p("fff", *center)
    data += p("f", radius)

    # Textures, edges, faces, sections, selections, and properties.
    data += list_count(0)
    data += u16_array([0, 1, 2, 3])
    data += u16_array([0, 1, 2, 3])
    data += list_count(1)
    data += p("I", 0)
    data += p("IhB", 0, -1, 4)
    data += p("HHHH", 0, 1, 2, 3)
    data += list_count(0)
    data += list_count(1)
    data += asciiz("synthetic_panel")
    data += u16_array([0])
    data += p("I", 1) + p("B", 255)
    data += u32_array([])
    data += p("B", 0)
    data += u32_array([])
    data += u16_array([0, 1, 2, 3])
    data += p("I", 4) + b"\xff" * 4
    data += list_count(0)

    # ODOL frame block.
    data += list_count(len(frames))
    for time, points in frames:
        data += p("f", time)
        data += vec3_array(points)

    data += p("III", 0xFFFFFFFF, 0xFFFFFFFF, 0)
    data += list_count(0)

    # Per-LOD resolutions.
    data += p("f", 1.0)

    # Model metadata.
    data += p("i", 0)
    data += p("f", radius)
    data += p("f", radius)
    data += p("iii", 0, 0, 0)
    data += p("fff", *center)
    data += p("II", 0xFFFFFFFF, 0xFFFFFFFF)
    data += p("f", 1.0)
    data += p("fff", *mn)
    data += p("fff", *mx)
    data += p("fff", *center)
    data += p("fff", *center)
    data += p("fff", 0.0, 0.0, 0.0)
    data += p("fffffffff", 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0)
    data += p("BBBBB", 0, 0, 0, 0, 1)
    data += p("b", 0)
    data += list_count(0)
    data += p("ffff", 0.0, 0.0, 0.0, 0.0)
    data += p("bbbbbbbbbbbb", *([-1] * 12))

    OUT.write_bytes(data)


if __name__ == "__main__":
    main()
