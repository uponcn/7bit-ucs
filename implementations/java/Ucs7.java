/*
 * Ucs7.java - 7bit-ucs encoder/decoder (single-file implementation)
 *
 * Part of the 7bit-ucs project.
 * Author: 刘占江 (Liu Zhanjiang)
 * Email:  jny7@163.com
 * GitHub: https://github.com/uponcn/7bit-ucs
 *
 * SPDX-License-Identifier: MIT
 *
 * Build and run:
 *     javac Ucs7.java && java Ucs7
 * or (JDK 11+):
 *     java Ucs7.java
 */

public final class Ucs7 {

    public static final int MAX_CODE_POINT = 0x10FFFF;

    private Ucs7() {
    }

    public static byte[] encode(int cp) {
        if (cp < 0 || cp > MAX_CODE_POINT) {
            throw new IllegalArgumentException(
                    String.format("invalid code point: U+%04X", cp));
        }
        if (cp < 0x80) {
            return new byte[] { (byte) cp };
        }
        if (cp < 0x4000) {
            return new byte[] {
                    (byte) (0x80 | (cp >> 7)),
                    (byte) (cp & 0x7F)
            };
        }
        return new byte[] {
                (byte) (0x80 | (cp >> 14)),
                (byte) (0x80 | ((cp >> 7) & 0x7F)),
                (byte) (cp & 0x7F)
        };
    }

    public static int[] decode(byte[] data, int offset) {
        int acc = 0;
        for (int i = 0; i < 3; i++) {
            if (offset + i >= data.length) {
                throw new IllegalArgumentException("truncated sequence");
            }
            int b = data[offset + i] & 0xFF;
            acc = (acc << 7) | (b & 0x7F);
            if (b < 0x80) {
                if (i == 1 && acc < 0x80) {
                    throw new IllegalArgumentException("overlong encoding");
                }
                if (i == 2 && acc < 0x4000) {
                    throw new IllegalArgumentException("overlong encoding");
                }
                if (acc > MAX_CODE_POINT) {
                    throw new IllegalArgumentException("code point out of range");
                }
                return new int[] { acc, i + 1 };
            }
        }
        throw new IllegalArgumentException("sequence longer than 3 bytes");
    }

    public static byte[] encodeString(String s) {
        byte[] buf = new byte[s.length() * 3];
        int pos = 0;
        for (int i = 0; i < s.length(); ) {
            int cp = s.codePointAt(i);
            i += Character.charCount(cp);
            byte[] enc = encode(cp);
            System.arraycopy(enc, 0, buf, pos, enc.length);
            pos += enc.length;
        }
        byte[] out = new byte[pos];
        System.arraycopy(buf, 0, out, 0, pos);
        return out;
    }

    public static String decodeString(byte[] data) {
        StringBuilder sb = new StringBuilder();
        int pos = 0;
        while (pos < data.length) {
            int[] r = decode(data, pos);
            sb.appendCodePoint(r[0]);
            pos += r[1];
        }
        return sb.toString();
    }

    public static int findStart(byte[] data, int pos) {
        if (pos == 0) {
            return 0;
        }
        int cur = data[pos] & 0xFF;
        int prev = data[pos - 1] & 0xFF;
        if (cur < 0x80) {
            if (prev < 0x80) {
                return pos;
            }
            if (pos < 2 || (data[pos - 2] & 0xFF) < 0x80) {
                return pos - 1;
            }
            return pos - 2;
        }
        if (prev < 0x80) {
            return pos;
        }
        return pos - 1;
    }

    public static void main(String[] args) {
        int[][] samples = {
                { 0x0041, 0x41 },
                { 0x00E9, 0x8169 },
                { 0x0915, 0x9215 },
                { 0x4E2D, 0x819C2D },
                { 0x1F600, 0x87EC00 }
        };
        for (int[] s : samples) {
            int cp = s[0];
            byte[] enc = encode(cp);
            int[] dec = decode(enc, 0);
            if (dec[0] != cp || dec[1] != enc.length) {
                throw new AssertionError("roundtrip failed for U+"
                        + Integer.toHexString(cp));
            }
        }

        // Overlong rejection
        try {
            decode(new byte[] { (byte) 0x80, 0x00 }, 0);
            throw new AssertionError("overlong not rejected");
        } catch (IllegalArgumentException ok) {
            // expected
        }

        // Too long rejection
        try {
            decode(new byte[] { (byte) 0x80, (byte) 0x80, (byte) 0x80, 0x00 }, 0);
            throw new AssertionError("too-long not rejected");
        } catch (IllegalArgumentException ok) {
            // expected
        }

        // Truncated rejection
        try {
            decode(new byte[] { (byte) 0x80 }, 0);
            throw new AssertionError("truncated not rejected");
        } catch (IllegalArgumentException ok) {
            // expected
        }

        // Self-sync
        byte[] buf = encodeString("\u4e2d\ud83d\ude00\u0915A");
        for (int pos = 0; pos < buf.length; pos++) {
            int start = findStart(buf, pos);
            int[] r = decode(buf, start);
            if (start + r[1] <= pos) {
                throw new AssertionError("self-sync failed at " + pos);
            }
        }

        // String roundtrip
        String s = "Hello \u4e16\u754c \ud83d\ude00";
        if (!s.equals(decodeString(encodeString(s)))) {
            throw new AssertionError("string roundtrip failed");
        }

        System.out.println("all tests passed");
    }
}
