#!/usr/bin/env python3
"""Conformance test runner for 7bit-ucs.

Part of the 7bit-ucs project.
Author: 刘占江 (Liu Zhanjiang)
Email:  jny7@163.com
GitHub: https://github.com/uponcn/7bit-ucs

SPDX-License-Identifier: MIT
"""

from __future__ import annotations

import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, os.path.join(ROOT, "implementations", "python"))

import ucs7  # noqa: E402


def load_vectors():
    path = os.path.join(HERE, "test_vectors.json")
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def hex_to_bytes(s: str) -> bytes:
    s = s.strip().replace(" ", "")
    return bytes.fromhex(s)


def run():
    vectors = load_vectors()
    passed = 0
    failed = 0

    for case in vectors["encode"]:
        cp = int(case["code_point"], 16)
        expected = hex_to_bytes(case["hex"])
        try:
            got = ucs7.encode(cp)
        except Exception as e:  # noqa: BLE001
            print(f"FAIL encode U+{cp:04X}: {e}")
            failed += 1
            continue
        if got == expected:
            passed += 1
        else:
            print(f"FAIL encode U+{cp:04X}: got {got.hex(' ')}, "
                  f"expected {expected.hex(' ')}")
            failed += 1

    for case in vectors["decode_valid"]:
        data = hex_to_bytes(case["hex"])
        expected_cp = int(case["code_point"], 16)
        expected_len = len(data)
        try:
            cp, n = ucs7.decode(data)
        except Exception as e:  # noqa: BLE001
            print(f"FAIL decode {case['hex']}: {e}")
            failed += 1
            continue
        if cp == expected_cp and n == expected_len:
            passed += 1
        else:
            print(f"FAIL decode {case['hex']}: got U+{cp:04X} "
                  f"({n} bytes), expected U+{expected_cp:04X} "
                  f"({expected_len} bytes)")
            failed += 1

    for case in vectors["decode_invalid"]:
        data = hex_to_bytes(case["hex"])
        reason = case["reason"]
        try:
            ucs7.decode(data)
        except ucs7.Ucs7Error:
            passed += 1
            continue
        print(f"FAIL decode_invalid {case['hex']} ({reason}): "
              "decoder accepted invalid input")
        failed += 1

    print(f"\n{passed} passed, {failed} failed")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(run())
