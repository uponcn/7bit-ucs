"""7bit-ucs encoder/decoder (single-file implementation).

Part of the 7bit-ucs project.
Author: 刘占江 (Liu Zhanjiang)
Email:  jny7@163.com
GitHub: https://github.com/uponcn/7bit-ucs

SPDX-License-Identifier: MIT
"""

from __future__ import annotations

from typing import Tuple

MAX_CODE_POINT = 0x10FFFF


class Ucs7Error(ValueError):
    """Raised on invalid 7bit-ucs input or code point."""


def encode(cp: int) -> bytes:
    """Encode a Unicode code point into 7bit-ucs bytes."""
    if not (0 <= cp <= MAX_CODE_POINT):
        raise Ucs7Error(f"invalid code point: U+{cp:04X}")
    if cp < 0x80:
        return bytes([cp])
    if cp < 0x4000:
        return bytes([0x80 | (cp >> 7), cp & 0x7F])
    return bytes(
        [
            0x80 | (cp >> 14),
            0x80 | ((cp >> 7) & 0x7F),
            cp & 0x7F,
        ]
    )


def decode(data: bytes, offset: int = 0) -> Tuple[int, int]:
    """Decode one 7bit-ucs character.

    Returns (code_point, bytes_consumed).
    Raises Ucs7Error on invalid input.
    """
    acc = 0
    for i in range(3):
        if offset + i >= len(data):
            raise Ucs7Error("truncated sequence")
        b = data[offset + i]
        acc = (acc << 7) | (b & 0x7F)
        if b < 0x80:
            if i == 1 and acc < 0x80:
                raise Ucs7Error("overlong encoding")
            if i == 2 and acc < 0x4000:
                raise Ucs7Error("overlong encoding")
            if acc > MAX_CODE_POINT:
                raise Ucs7Error("code point out of range")
            return acc, i + 1
    raise Ucs7Error("sequence longer than 3 bytes")


def encode_str(s: str) -> bytes:
    """Encode a Python string into 7bit-ucs bytes."""
    out = bytearray()
    for ch in s:
        out.extend(encode(ord(ch)))
    return bytes(out)


def decode_str(data: bytes) -> str:
    """Decode 7bit-ucs bytes into a Python string."""
    out = []
    pos = 0
    while pos < len(data):
        cp, n = decode(data, pos)
        out.append(chr(cp))
        pos += n
    return "".join(out)


def find_start(data: bytes, pos: int) -> int:
    """Return the index of the first byte of the character containing pos."""
    if pos == 0:
        return 0
    if data[pos] < 0x80:
        if data[pos - 1] < 0x80:
            return pos
        if pos < 2 or data[pos - 2] < 0x80:
            return pos - 1
        return pos - 2
    if data[pos - 1] < 0x80:
        return pos
    return pos - 1


__all__ = [
    "MAX_CODE_POINT",
    "Ucs7Error",
    "encode",
    "decode",
    "encode_str",
    "decode_str",
    "find_start",
]


if __name__ == "__main__":
    samples = [
        (0x0041, b"\x41"),
        (0x00E9, b"\x81\x69"),
        (0x0915, b"\x92\x15"),
        (0x4E2D, b"\x81\x9C\x2D"),
        (0x1F600, b"\x87\xEC\x00"),
    ]
    for cp, expected in samples:
        got = encode(cp)
        assert got == expected, (hex(cp), got, expected)
        back, n = decode(got)
        assert back == cp and n == len(got)

    s = "Hello \u4e16\u754c \U0001F600 \u0645\u0631\u062d\u0628\u0627"
    assert decode_str(encode_str(s)) == s

    # self-sync
    buf = encode_str("中😀क")
    for pos in range(len(buf)):
        start = find_start(buf, pos)
        _, used = decode(buf, start)
        assert start + used > pos

    print("all tests passed")
