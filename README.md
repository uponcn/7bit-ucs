# 7bit-ucs

**An ASCII-transparent, byte-order-independent variable-length Unicode encoding.**

**Author:** [Your Name]  
**Contact:** [your@email.com]

[![IANA Registration](https://img.shields.io/badge/IANA-registration%20pending-yellow)](https://www.iana.org/assignments/character-sets/character-sets.xhtml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Spec](https://img.shields.io/badge/spec-7bit--ucs-green)](docs/specification.md)

---

## Overview

`7bit-ucs` is a variable-length Unicode encoding scheme that packs **7 payload bits per byte**. It covers the entire Unicode code point range (U+0000 – U+10FFFF) using **at most 3 bytes per character**, compared to UTF-8's maximum of 4 bytes.

It is designed for environments where space savings and simple self-synchronization matter: Indic and Southeast Asian text, Emoji-dense data, embedded devices, and deterministic database keys.

The name `7bit-ucs` stands for "7-bit payload, Unicode Character Set". The alias `ucs-7` is also registered.

## Author & Contact

This project is created and maintained by:

- **Author:** [刘占江]
- **Email:** [jny7@163.com]
- **GitHub:** [@uponcn](https://github.com/uponcn)

For questions, suggestions, or collaboration, please open an issue or contact the author directly.

## Key Properties

| Property | Value |
|----------|-------|
| ASCII transparent | Yes — U+0000 – U+007F encoded identically to ASCII |
| Byte-order independent | Yes — no BOM required |
| Self-synchronizing | Yes — at most 2 bytes look-back |
| Stateful | No — each character is independently decodable |
| Maximum bytes per character | 3 |
| Unicode coverage | Full (U+0000 – U+10FFFF) |
| Deterministic | Yes — one code point sequence has exactly one encoding |
| Overlong encoding | Rejected by all conforming decoders |
| MIME text suitable | Yes |

## Encoding Structure

A Unicode code point is split into 7-bit groups, starting from the least significant bit. Each group occupies one byte. The byte carrying the least significant 7 bits has its high bit set to `0`; all preceding bytes have their high bit set to `1`. The most significant group appears first in the byte stream.

| Bytes | Payload bits | Code point range | Byte pattern |
|-------|--------------|------------------|--------------|
| 1 | 7 | U+0000 – U+007F | `0xxxxxxx` |
| 2 | 14 | U+0080 – U+3FFF | `1xxxxxxx 0xxxxxxx` |
| 3 | 21 | U+4000 – U+10FFFF | `1xxxxxxx 1xxxxxxx 0xxxxxxx` |

### Example

| Character | Code point | UTF-8 | 7bit-ucs |
|-----------|-----------|-------|----------|
| `A` | U+0041 | `41` | `41` |
| `é` | U+00E9 | `C3 A9` | `81 69` |
| `中` | U+4E2D | `E4 B8 AD` | `81 38 AD` |
| `क` (Devanagari KA) | U+0915 | `E0 A4 95` | `82 15` |
| `😀` | U+1F600 | `F0 9F 98 80` | `87 98 80` |

## Space Efficiency vs. UTF-8

| Code point range | Typical content | UTF-8 | 7bit-ucs | Saving |
|------------------|-----------------|-------|----------|--------|
| U+0000 – U+007F | ASCII | 1 | 1 | — |
| U+0080 – U+07FF | Latin ext., Greek, Cyrillic, Arabic, Hebrew | 2 | 2 | — |
| **U+0800 – U+3FFF** | **Indic, Thai, Lao, extended symbols** | **3** | **2** | **33%** |
| U+4000 – U+FFFF | CJK, Hangul, most symbols | 3 | 3 | — |
| **U+10000 – U+10FFFF** | **Emoji, supplementary plane** | **4** | **3** | **25%** |

## Self-Synchronization

Recovering a character boundary from any byte position requires at most **2 bytes of look-back**.

- If the current byte is `1xxxxxxx`: check the previous byte. If it is `0xxxxxxx`, the current byte starts a character. If it is `1xxxxxxx`, the current byte is the second byte of a character (a third `1xxxxxxx` cannot occur).
- If the current byte is `0xxxxxxx`: it is the **last** byte of a character. Look back over at most two `1xxxxxxx` bytes to find the start.

For comparison, UTF-8 requires up to 3 bytes of look-back.

## Overlong Encoding

Overlong encodings are rejected. Conforming decoders MUST verify:

- A 2-byte sequence decodes to a code point ≥ U+0080.
- A 3-byte sequence decodes to a code point ≥ U+4000.
- No sequence exceeds 3 bytes.
- No code point exceeds U+10FFFF.

## Unicode Signature

The Unicode signature sequence for 7bit-ucs is:

```text
80 80 80 00
```

This is the 3-byte overlong encoding of U+0000. Decoders that recognize it MUST skip it and MUST NOT decode it as a character.

## Implementations

Reference implementations are provided in five languages:

| Language | Directory | Status |
|----------|-----------|--------|
| C | [`implementations/c/`](implementations/c/) | Complete |
| Rust | [`implementations/rust/`](implementations/rust/) | Complete |
| Python | [`implementations/python/`](implementations/python/) | Complete |
| Java | [`implementations/java/`](implementations/java/) | Complete |
| Go | [`implementations/go/`](implementations/go/) | Complete |

All implementations pass the shared conformance test suite in [`tests/`](tests/).

## Testing

```bash
# Python
python3 tests/run_tests.py

# Rust
cd implementations/rust && cargo test

# Go
cd implementations/go && go test ./...

# C
cd implementations/c && make test

# Java
cd implementations/java && ./gradlew test
```

## IANA Registration

Registration of `7bit-ucs` (alias `ucs-7`) in the IANA Character Sets registry is in progress. The registration follows [RFC 2978](https://www.rfc-editor.org/rfc/rfc2978).

## Specification

The complete specification is in [`docs/specification.md`](docs/specification.md).

## Design Rationale

7bit-ucs was designed to explore the space between UTF-8 and fixed-width encodings:

- **7-bit payload** maximizes bits per byte while retaining a simple high-bit marker for sequence termination.
- **3-byte maximum** keeps decoder state minimal and reduces worst-case look-back to 2 bytes.
- **ASCII transparency** preserves compatibility with ASCII-only tooling.
- **Statelessness** avoids the complexity and security pitfalls of stateful encodings like UTF-7.

It is **not** proposed as a replacement for UTF-8 as the preferred Internet Unicode encoding. It is offered for limited use in environments where its specific trade-offs are beneficial.

## License

MIT License. See [LICENSE](LICENSE).

## Contact

- **Author:** [刘占江]
- **Email:** [jny7@163.com]

## References

- [RFC 2978 — IANA Charset Registration Procedures](https://www.rfc-editor.org/rfc/rfc2978)
- [IANA Character Sets Registry](https://www.iana.org/assignments/character-sets/character-sets.xhtml)
- [The Unicode Standard](https://www.unicode.org/standard/standard.html)
