# 7bit-ucs Specification

**Version:** 1.0.0  
**Author:** 刘占江 (Liu Zhanjiang)  
**Email:** jny7@163.com  
**GitHub:** https://github.com/uponcn/7bit-ucs  
**Date:** 2026-09-19

---

## 1. Introduction

`7bit-ucs` is a variable-length character encoding scheme for Unicode.
It encodes each Unicode code point (U+0000 – U+10FFFF) into a sequence of
1 to 3 bytes. Each byte carries 7 payload bits. The high bit of each byte
is used as a continuation marker: `1` means "more bytes follow", `0` means
"this is the last byte of the character".

7bit-ucs is ASCII-transparent, byte-order-independent, stateless, and
self-synchronizing. It is offered for limited use in environments where
its space savings and simple self-synchronization properties are
beneficial.

## 2. Terminology

- **Code point**: A value in the Unicode range U+0000 – U+10FFFF.
- **Payload byte**: A byte that carries 7 payload bits in its low 7 bits.
- **Terminating byte**: A byte whose high bit is 0 (`0xxxxxxx`).
- **Non-terminating byte**: A byte whose high bit is 1 (`1xxxxxxx`).
- **Overlong encoding**: An encoding that uses more bytes than the
  minimum required to represent a given code point.

## 3. Code Point Ranges

| Bytes | Payload bits | Code point range    |
|-------|--------------|---------------------|
| 1     | 7            | U+0000 – U+007F     |
| 2     | 14           | U+0080 – U+3FFF     |
| 3     | 21           | U+4000 – U+10FFFF   |

## 4. Encoding Procedure

Given a Unicode code point `cp`:

1. If `cp <= 0x7F`:
   - Emit one byte: `cp`.
2. Else if `cp <= 0x3FFF`:
   - Emit byte `0x80 | (cp >> 7)`.
   - Emit byte `cp & 0x7F`.
3. Else if `cp <= 0x10FFFF`:
   - Emit byte `0x80 | (cp >> 14)`.
   - Emit byte `0x80 | ((cp >> 7) & 0x7F)`.
   - Emit byte `cp & 0x7F`.
4. Else: the code point is invalid and MUST be rejected.

The most significant 7-bit group appears first in the byte stream. The
least significant 7-bit group appears last and always has its high bit
cleared.

### 4.1 Examples

| Character | Code point | UTF-8        | 7bit-ucs     |
|-----------|------------|--------------|--------------|
| `A`       | U+0041     | `41`         | `41`         |
| `é`       | U+00E9     | `C3 A9`      | `81 69`      |
| `क`       | U+0915     | `E0 A4 95`   | `92 15`      |
| `中`      | U+4E2D     | `E4 B8 AD`   | `81 9C 2D`   |
| `😀`      | U+1F600    | `F0 9F 98 80`| `87 EC 00`   |

## 5. Decoding Procedure

Given a byte stream, decode one code point as follows:

1. Initialize `acc = 0` and `i = 0`.
2. Read byte `b = data[i]`.
3. Set `acc = (acc << 7) | (b & 0x7F)`.
4. If `b < 0x80`, the code point is complete; proceed to step 5.
5. Otherwise, increment `i`. If `i > 2`, the sequence is too long and
   MUST be rejected. Return to step 2.
6. The decoded code point is `acc`. Validate it:
   - If `i == 1` and `acc < 0x80`: overlong encoding, MUST be rejected.
   - If `i == 2` and `acc < 0x4000`: overlong encoding, MUST be rejected.
   - If `acc > 0x10FFFF`: invalid code point, MUST be rejected.
7. Return `(acc, i + 1)`.

## 6. Self-Synchronization

A decoder MAY begin decoding at any byte position and recover the
character boundary within at most 2 bytes of look-back.

- If the current byte is `0xxxxxxx`, it is the last byte of a character.
  Look back over at most two preceding `1xxxxxxx` bytes to locate the
  first byte of the character.
- If the current byte is `1xxxxxxx`:
  - If the previous byte is `0xxxxxxx` (or there is no previous byte),
    the current byte is the first byte of a character.
  - If the previous byte is `1xxxxxxx`, the current byte is the second
    byte of a character. A third consecutive `1xxxxxxx` cannot occur.

## 7. Unicode Signature

The Unicode signature sequence for 7bit-ucs is:

```
80 80 80 00
```

This sequence is the 4-byte overlong encoding of U+0000. Since the
maximum sequence length is 3 bytes, this sequence is never produced by a
conforming encoder. A decoder that recognizes this sequence at the start
of a stream MUST skip it and MUST NOT decode it as a character.

## 8. Error Handling

A conforming decoder MUST reject the following:

1. Overlong encodings (as defined in Section 5).
2. Sequences longer than 3 bytes.
3. Code points greater than U+10FFFF.
4. Truncated sequences (a non-terminating byte at the end of input).
5. Isolated non-terminating bytes not followed by a terminating byte
   within 2 subsequent bytes.

## 9. Determinism

7bit-ucs is deterministic: a given code point sequence always produces
exactly one byte sequence. Two byte sequences are equal if and only if
they decode to the same code point sequence.

## 10. Byte Order

7bit-ucs is byte-order independent. No byte order mark is required. The
Unicode signature sequence described in Section 7 may optionally be used
to identify a stream.

## 11. MIME Suitability

7bit-ucs is suitable for use in MIME text objects. It is ASCII-transparent
for U+0000 – U+007F. Non-ASCII characters produce bytes in the range
0x80 – 0xFF, which are not valid ASCII.

## 12. Security Considerations

Decoders MUST enforce all constraints in Section 8. Failure to do so may
allow overlong encodings to bypass security filters, cause buffer
overruns, or enable denial-of-service attacks via malformed input.

Encoders MUST NOT produce overlong encodings, even if the input requests
them. The canonical encoding is the only valid one.

## 13. Test Vectors

See [`../tests/test_vectors.json`](../tests/test_vectors.json) for a
complete set of conformance test vectors.

## 14. References

- [RFC 2978 — IANA Charset Registration Procedures](https://www.rfc-editor.org/rfc/rfc2978)
- [The Unicode Standard](https://www.unicode.org/standard/standard.html)
- [IANA Character Sets Registry](https://www.iana.org/assignments/character-sets/character-sets.xhtml)

---

Copyright (c) 2026 刘占江 (Liu Zhanjiang).  
Licensed under CC BY 4.0. See [LICENSE](LICENSE).
