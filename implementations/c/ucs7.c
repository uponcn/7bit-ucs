/*
 * ucs7.c - 7bit-ucs encoder/decoder (single-file implementation)
 *
 * Part of the 7bit-ucs project.
 * Author: 刘占江 (Liu Zhanjiang)
 * Email:  jny7@163.com
 * GitHub: https://github.com/uponcn/7bit-ucs
 *
 * SPDX-License-Identifier: MIT
 *
 * Build as library:
 *     cc -O2 -Wall -Wextra -std=c99 -c ucs7.c
 *
 * Build and run self-test:
 *     cc -O2 -Wall -Wextra -std=c99 -DUCS7_SELFTEST ucs7.c -o ucs7_test
 *     ./ucs7_test
 */

#include <stddef.h>
#include <stdint.h>

#define UCS7_OK              0
#define UCS7_ERR_INVALID_CP  (-1)
#define UCS7_ERR_OVERLONG    (-2)
#define UCS7_ERR_TOO_LONG    (-3)
#define UCS7_ERR_TRUNCATED   (-4)

#define UCS7_MAX_CODE_POINT  0x10FFFFu

/*
 * Encode a Unicode code point into 7bit-ucs.
 * out must have room for at least 3 bytes.
 * Returns bytes written (1..3), or a negative error code.
 */
int ucs7_encode(uint32_t cp, uint8_t *out)
{
    if (cp > UCS7_MAX_CODE_POINT) {
        return UCS7_ERR_INVALID_CP;
    }
    if (cp < 0x80u) {
        out[0] = (uint8_t)cp;
        return 1;
    }
    if (cp < 0x4000u) {
        out[0] = (uint8_t)(0x80u | (cp >> 7));
        out[1] = (uint8_t)(cp & 0x7Fu);
        return 2;
    }
    out[0] = (uint8_t)(0x80u | (cp >> 14));
    out[1] = (uint8_t)(0x80u | ((cp >> 7) & 0x7Fu));
    out[2] = (uint8_t)(cp & 0x7Fu);
    return 3;
}

/*
 * Decode one 7bit-ucs character from in[0..len-1].
 * On success stores code point in *cp and returns bytes consumed (1..3).
 * On failure returns a negative error code.
 */
int ucs7_decode(const uint8_t *in, size_t len, uint32_t *cp)
{
    uint32_t acc = 0;
    size_t i;

    for (i = 0; i < 3 && i < len; i++) {
        uint8_t b = in[i];
        acc = (acc << 7) | (uint32_t)(b & 0x7Fu);
        if (b < 0x80u) {
            if (i == 1 && acc < 0x80u)   return UCS7_ERR_OVERLONG;
            if (i == 2 && acc < 0x4000u) return UCS7_ERR_OVERLONG;
            if (acc > UCS7_MAX_CODE_POINT) return UCS7_ERR_INVALID_CP;
            *cp = acc;
            return (int)(i + 1);
        }
    }
    if (i >= len) {
        return UCS7_ERR_TRUNCATED;
    }
    return UCS7_ERR_TOO_LONG;
}

/*
 * Self-synchronization helper.
 * Given a buffer and a position pos within it, return the index of the
 * first byte of the character containing pos. Look-back at most 2 bytes.
 */
size_t ucs7_find_start(const uint8_t *in, size_t pos)
{
    if (pos == 0) {
        return 0;
    }
    if (in[pos] < 0x80u) {
        if (in[pos - 1] < 0x80u) {
            return pos;
        }
        if (pos < 2 || in[pos - 2] < 0x80u) {
            return pos - 1;
        }
        return pos - 2;
    }
    if (in[pos - 1] < 0x80u) {
        return pos;
    }
    return pos - 1;
}

#ifdef UCS7_SELFTEST

#include <stdio.h>

static int check_encode(uint32_t cp, const uint8_t *expected, int exp_len)
{
    uint8_t buf[3];
    int n = ucs7_encode(cp, buf);
    if (n != exp_len) {
        printf("FAIL encode U+%04X: got %d bytes, expected %d\n",
               cp, n, exp_len);
        return 1;
    }
    for (int i = 0; i < n; i++) {
        if (buf[i] != expected[i]) {
            printf("FAIL encode U+%04X: byte %d = %02X, expected %02X\n",
                   cp, i, buf[i], expected[i]);
            return 1;
        }
    }
    return 0;
}

static int check_decode(const uint8_t *in, size_t len,
                        uint32_t exp_cp, int exp_len)
{
    uint32_t cp = 0;
    int n = ucs7_decode(in, len, &cp);
    if (n != exp_len || cp != exp_cp) {
        printf("FAIL decode: got cp=U+%04X len=%d, "
               "expected U+%04X len=%d\n",
               cp, n, exp_cp, exp_len);
        return 1;
    }
    return 0;
}

static int check_invalid(const uint8_t *in, size_t len, int exp_err)
{
    uint32_t cp = 0;
    int n = ucs7_decode(in, len, &cp);
    if (n != exp_err) {
        printf("FAIL invalid: got %d, expected %d\n", n, exp_err);
        return 1;
    }
    return 0;
}

int main(void)
{
    int fails = 0;

    /* Encoding boundaries */
    fails += check_encode(0x0000, (const uint8_t[]){0x00}, 1);
    fails += check_encode(0x0041, (const uint8_t[]){0x41}, 1);
    fails += check_encode(0x007F, (const uint8_t[]){0x7F}, 1);
    fails += check_encode(0x0080, (const uint8_t[]){0x81, 0x00}, 2);
    fails += check_encode(0x00E9, (const uint8_t[]){0x81, 0x69}, 2);
    fails += check_encode(0x0915, (const uint8_t[]){0x92, 0x15}, 2);
    fails += check_encode(0x3FFF, (const uint8_t[]){0xFF, 0x7F}, 2);
    fails += check_encode(0x4000, (const uint8_t[]){0x81, 0x80, 0x00}, 3);
    fails += check_encode(0x4E2D, (const uint8_t[]){0x81, 0x9C, 0x2D}, 3);
    fails += check_encode(0x1F600, (const uint8_t[]){0x87, 0xEC, 0x00}, 3);
    fails += check_encode(0x10FFFF, (const uint8_t[]){0x84, 0x3F, 0xFF, 0x7F}, 3);

    /* Decoding valid */
    fails += check_decode((const uint8_t[]){0x41}, 1, 0x0041, 1);
    fails += check_decode((const uint8_t[]){0x81, 0x69}, 2, 0x00E9, 2);
    fails += check_decode((const uint8_t[]){0x81, 0x9C, 0x2D}, 3, 0x4E2D, 3);

    /* Overlong detection */
    fails += check_invalid((const uint8_t[]){0x80, 0x00}, 2, UCS7_ERR_OVERLONG);
    fails += check_invalid((const uint8_t[]){0x80, 0x80, 0x00}, 3, UCS7_ERR_OVERLONG);

    /* Too long */
    fails += check_invalid((const uint8_t[]){0x80, 0x80, 0x80, 0x00}, 4,
                           UCS7_ERR_TOO_LONG);

    /* Truncated */
    fails += check_invalid((const uint8_t[]){0x80}, 1, UCS7_ERR_TRUNCATED);
    fails += check_invalid((const uint8_t[]){0x80, 0x80}, 2, UCS7_ERR_TRUNCATED);

    /* Self-synchronization */
    {
        const uint8_t buf[] = {0x81, 0x9C, 0x2D, 0x87, 0xEC, 0x00};
        size_t starts[] = {0, 0, 0, 3, 3, 3};
        for (size_t pos = 0; pos < sizeof(buf); pos++) {
            size_t s = ucs7_find_start(buf, pos);
            if (s != starts[pos]) {
                printf("FAIL self-sync at %zu: got %zu, expected %zu\n",
                       pos, s, starts[pos]);
                fails++;
            }
        }
    }

    if (fails == 0) {
        printf("all tests passed\n");
        return 0;
    }
    printf("%d test(s) failed\n", fails);
    return 1;
}

#endif /* UCS7_SELFTEST */
