//! 7bit-ucs encoder/decoder (single-file implementation).
//!
//! Part of the 7bit-ucs project.
//! Author: 刘占江 (Liu Zhanjiang)
//! Email:  jny7@163.com
//! GitHub: https://github.com/uponcn/7bit-ucs
//!
//! SPDX-License-Identifier: MIT
//!
//! Build and run self-test:
//!     rustc --test ucs7.rs -o ucs7_test
//!     ./ucs7_test
//!
//! Build as a standalone binary (includes a small demo main):
//!     rustc --edition 2021 ucs7.rs -o ucs7_demo
//!     ./ucs7_demo

#![allow(dead_code)]

/// Maximum valid Unicode code point.
pub const MAX_CODE_POINT: u32 = 0x10FFFF;

/// Errors returned by decoding functions.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Error {
    InvalidCodePoint,
    Overlong,
    TooLong,
    Truncated,
}

/// Encode a Unicode code point into 7bit-ucs.
///
/// Returns `(buffer, length)` where only the first `length` bytes of
/// `buffer` are meaningful.
pub fn encode(cp: u32) -> Result<([u8; 3], usize), Error> {
    if cp > MAX_CODE_POINT {
        return Err(Error::InvalidCodePoint);
    }
    if cp < 0x80 {
        Ok(([cp as u8, 0, 0], 1))
    } else if cp < 0x4000 {
        Ok(([(0x80 | (cp >> 7)) as u8, (cp & 0x7F) as u8, 0], 2))
    } else {
        Ok((
            [
                (0x80 | (cp >> 14)) as u8,
                (0x80 | ((cp >> 7) & 0x7F)) as u8,
                (cp & 0x7F) as u8,
            ],
            3,
        ))
    }
}

/// Decode one 7bit-ucs character from `data`.
///
/// Returns `(code_point, bytes_consumed)` on success.
pub fn decode(data: &[u8]) -> Result<(u32, usize), Error> {
    let mut acc: u32 = 0;
    let limit = core::cmp::min(3, data.len());
    for i in 0..limit {
        let b = data[i];
        acc = (acc << 7) | (b & 0x7F) as u32;
        if b < 0x80 {
            if i == 1 && acc < 0x80 {
                return Err(Error::Overlong);
            }
            if i == 2 && acc < 0x4000 {
                return Err(Error::Overlong);
            }
            if acc > MAX_CODE_POINT {
                return Err(Error::InvalidCodePoint);
            }
            return Ok((acc, i + 1));
        }
    }
    if data.len() < 3 {
        Err(Error::Truncated)
    } else {
        Err(Error::TooLong)
    }
}

/// Self-synchronization helper: return the index of the first byte of
/// the character containing `pos`. Look-back is at most 2 bytes.
pub fn find_start(data: &[u8], pos: usize) -> usize {
    if pos == 0 {
        return 0;
    }
    if data[pos] < 0x80 {
        if data[pos - 1] < 0x80 {
            pos
        } else if pos < 2 || data[pos - 2] < 0x80 {
            pos - 1
        } else {
            pos - 2
        }
    } else if data[pos - 1] < 0x80 {
        pos
    } else {
        pos - 1
    }
}

/// Encode a string (UTF-8 in, 7bit-ucs out). Not no_std-friendly, kept
/// here for convenience of the demo.
pub fn encode_str(s: &str) -> Vec<u8> {
    let mut out = Vec::with_capacity(s.len() * 3);
    for c in s.chars() {
        let (buf, len) = encode(c as u32).unwrap();
        out.extend_from_slice(&buf[..len]);
    }
    out
}

/// Decode 7bit-ucs bytes into a `String`.
pub fn decode_str(data: &[u8]) -> Result<String, Error> {
    let mut out = String::with_capacity(data.len());
    let mut pos = 0;
    while pos < data.len() {
        let (cp, n) = decode(&data[pos..])?;
        match char::from_u32(cp) {
            Some(c) => out.push(c),
            None => return Err(Error::InvalidCodePoint),
        }
        pos += n;
    }
    Ok(out)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn encode_boundaries() {
        let cases: &[(u32, &[u8])] = &[
            (0x0000, &[0x00]),
            (0x0041, &[0x41]),
            (0x007F, &[0x7F]),
            (0x0080, &[0x81, 0x00]),
            (0x00E9, &[0x81, 0x69]),
            (0x0915, &[0x92, 0x15]),
            (0x3FFF, &[0xFF, 0x7F]),
            (0x4000, &[0x81, 0x80, 0x00]),
            (0x4E2D, &[0x81, 0x9C, 0x2D]),
            (0x1F600, &[0x87, 0xEC, 0x00]),
            (0x10FFFF, &[0x84, 0x3F, 0xFF, 0x7F]),
        ];
        for &(cp, exp) in cases {
            let (buf, len) = encode(cp).unwrap();
            assert_eq!(&buf[..len], exp, "encode U+{:04X}", cp);
            let (got, used) = decode(exp).unwrap();
            assert_eq!(got, cp);
            assert_eq!(used, exp.len());
        }
    }

    #[test]
    fn overlong_rejected() {
        assert_eq!(decode(&[0x80, 0x00]), Err(Error::Overlong));
        assert_eq!(decode(&[0x80, 0x80, 0x00]), Err(Error::Overlong));
    }

    #[test]
    fn too_long_rejected() {
        assert_eq!(decode(&[0x80, 0x80, 0x80, 0x00]), Err(Error::TooLong));
    }

    #[test]
    fn truncated_rejected() {
        assert_eq!(decode(&[0x80]), Err(Error::Truncated));
        assert_eq!(decode(&[0x80, 0x80]), Err(Error::Truncated));
    }

    #[test]
    fn self_sync() {
        let mut buf = Vec::new();
        for cp in [0x4E2Du32, 0x1F600, 0x0915, 0x41] {
            let (b, l) = encode(cp).unwrap();
            buf.extend_from_slice(&b[..l]);
        }
        for pos in 0..buf.len() {
            let start = find_start(&buf, pos);
            let (_, used) = decode(&buf[start..]).unwrap();
            assert!(start + used > pos, "pos {} start {} used {}", pos, start, used);
        }
    }

    #[test]
    fn string_roundtrip() {
        let s = "Hello 世界 😀 مرحبا क";
        let enc = encode_str(s);
        let dec = decode_str(&enc).unwrap();
        assert_eq!(dec, s);
    }
}

// Optional demo binary. When compiled without --test, this main runs.
#[cfg(not(test))]
fn main() {
    let samples: &[(u32, &[u8])] = &[
        (0x0041, &[0x41]),
        (0x00E9, &[0x81, 0x69]),
        (0x0915, &[0x92, 0x15]),
        (0x4E2D, &[0x81, 0x9C, 0x2D]),
        (0x1F600, &[0x87, 0xEC, 0x00]),
    ];
    for &(cp, exp) in samples {
        let (buf, len) = encode(cp).unwrap();
        assert_eq!(&buf[..len], exp);
        let (got, used) = decode(exp).unwrap();
        assert_eq!(got, cp);
        assert_eq!(used, exp.len());
    }
    let s = "Hello 世界 😀";
    let enc = encode_str(s);
    let dec = decode_str(&enc).unwrap();
    assert_eq!(dec, s);
    println!("all tests passed");
}
