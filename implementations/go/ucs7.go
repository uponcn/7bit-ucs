// 7bit-ucs encoder/decoder (single-file implementation).
//
// Part of the 7bit-ucs project.
// Author: 刘占江 (Liu Zhanjiang)
// Email:  jny7@163.com
// GitHub: https://github.com/uponcn/7bit-ucs
//
// SPDX-License-Identifier: MIT
//
// Run:
//     go run ucs7.go
//
// Test (also works):
//     go test ucs7.go
package main

import (
	"errors"
	"fmt"
)

// MaxCodePoint is the largest valid Unicode code point.
const MaxCodePoint rune = 0x10FFFF

var (
	ErrInvalidCodePoint = errors.New("ucs7: invalid code point")
	ErrOverlong         = errors.New("ucs7: overlong encoding")
	ErrTooLong          = errors.New("ucs7: sequence longer than 3 bytes")
	ErrTruncated        = errors.New("ucs7: truncated sequence")
)

// Encode returns the 7bit-ucs encoding of a Unicode code point.
func Encode(cp rune) ([]byte, error) {
	if cp < 0 || cp > MaxCodePoint {
		return nil, fmt.Errorf("%w: U+%04X", ErrInvalidCodePoint, cp)
	}
	if cp < 0x80 {
		return []byte{byte(cp)}, nil
	}
	if cp < 0x4000 {
		return []byte{
			byte(0x80 | (cp >> 7)),
			byte(cp & 0x7F),
		}, nil
	}
	return []byte{
		byte(0x80 | (cp >> 14)),
		byte(0x80 | ((cp >> 7) & 0x7F)),
		byte(cp & 0x7F),
	}, nil
}

// Decode reads one 7bit-ucs character from data.
func Decode(data []byte) (rune, int, error) {
	var acc rune
	limit := 3
	if len(data) < limit {
		limit = len(data)
	}
	for i := 0; i < limit; i++ {
		b := data[i]
		acc = (acc << 7) | rune(b&0x7F)
		if b < 0x80 {
			if i == 1 && acc < 0x80 {
				return 0, 0, ErrOverlong
			}
			if i == 2 && acc < 0x4000 {
				return 0, 0, ErrOverlong
			}
			if acc > MaxCodePoint {
				return 0, 0, ErrInvalidCodePoint
			}
			return acc, i + 1, nil
		}
	}
	if len(data) < 3 {
		return 0, 0, ErrTruncated
	}
	return 0, 0, ErrTooLong
}

// EncodeString encodes a Go string into 7bit-ucs bytes.
func EncodeString(s string) ([]byte, error) {
	buf := make([]byte, 0, len(s)*3)
	for _, r := range s {
		b, err := Encode(r)
		if err != nil {
			return nil, err
		}
		buf = append(buf, b...)
	}
	return buf, nil
}

// DecodeString decodes 7bit-ucs bytes into a Go string.
func DecodeString(data []byte) (string, error) {
	out := make([]rune, 0, len(data))
	pos := 0
	for pos < len(data) {
		cp, n, err := Decode(data[pos:])
		if err != nil {
			return "", err
		}
		out = append(out, cp)
		pos += n
	}
	return string(out), nil
}

// FindStart returns the index of the first byte of the character
// containing pos. Look-back is at most 2 bytes.
func FindStart(data []byte, pos int) int {
	if pos == 0 {
		return 0
	}
	cur := data[pos]
	prev := data[pos-1]
	if cur < 0x80 {
		if prev < 0x80 {
			return pos
		}
		if pos < 2 || data[pos-2] < 0x80 {
			return pos - 1
		}
		return pos - 2
	}
	if prev < 0x80 {
		return pos
	}
	return pos - 1
}

func main() {
	samples := []struct {
		cp  rune
		hex []byte
	}{
		{0x0041, []byte{0x41}},
		{0x00E9, []byte{0x81, 0x69}},
		{0x0915, []byte{0x92, 0x15}},
		{0x4E2D, []byte{0x81, 0x9C, 0x2D}},
		{0x1F600, []byte{0x87, 0xEC, 0x00}},
		{0x10FFFF, []byte{0x84, 0x3F, 0xFF, 0x7F}},
	}
	for _, s := range samples {
		enc, err := Encode(s.cp)
		if err != nil {
			panic(err)
		}
		if len(enc) != len(s.hex) {
			panic(fmt.Sprintf("len mismatch for U+%04X", s.cp))
		}
		for i := range enc {
			if enc[i] != s.hex[i] {
				panic(fmt.Sprintf("byte mismatch U+%04X at %d", s.cp, i))
			}
		}
		got, n, err := Decode(enc)
		if err != nil || got != s.cp || n != len(enc) {
			panic(fmt.Sprintf("decode failed U+%04X", s.cp))
		}
	}

	// Overlong
	if _, _, err := Decode([]byte{0x80, 0x00}); err != ErrOverlong {
		panic("overlong not rejected")
	}
	if _, _, err := Decode([]byte{0x80, 0x80, 0x00}); err != ErrOverlong {
		panic("overlong not rejected")
	}
	// Too long
	if _, _, err := Decode([]byte{0x80, 0x80, 0x80, 0x00}); err != ErrTooLong {
		panic("too-long not rejected")
	}
	// Truncated
	if _, _, err := Decode([]byte{0x80}); err != ErrTruncated {
		panic("truncated not rejected")
	}
	if _, _, err := Decode([]byte{0x80, 0x80}); err != ErrTruncated {
		panic("truncated not rejected")
	}

	// Self-sync
	buf, _ := EncodeString("中😀कA")
	for pos := 0; pos < len(buf); pos++ {
		start := FindStart(buf, pos)
		_, n, err := Decode(buf[start:])
		if err != nil {
			panic(err)
		}
		if start+n <= pos {
			panic(fmt.Sprintf("self-sync failed at %d", pos))
		}
	}

	// String roundtrip
	s := "Hello 世界 😀 مرحبا क"
	enc, _ := EncodeString(s)
	dec, _ := DecodeString(enc)
	if dec != s {
		panic("string roundtrip failed")
	}

	fmt.Println("all tests passed")
}
