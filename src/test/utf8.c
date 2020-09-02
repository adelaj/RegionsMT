#include "../memory.h"
#include "utf8.h"

#include <string.h>
#include <stdlib.h>

#define UTF8I(...) INIT(uint8_t, __VA_ARGS__)
#define UTF16I(...) INIT(uint16_t, __VA_ARGS__)

bool test_utf8_generator(void *p_res, size_t *p_ind, struct log *log)
{
    (void) log;
    static const struct test_utf8 data[] = {
        { 0x0, UTF8I('\0'), UTF16I(0) },
        { 0x3F, UTF8I('?'), UTF16I(0x3F) },
        { 0x40, UTF8I('@'), UTF16I(0x40) },
        { 0x7F, UTF8I(0x7F), UTF16I(0x7F)},
        { 0x80, UTF8I(0xC2, 0x80), UTF16I(0x80)},
        { 0x3FF, UTF8I(0xCF, 0xBF), UTF16I(0x3FF) },
        { 0x400, UTF8I(0xD0, 0x80), UTF16I(0x400) },
        { 0x7FF, UTF8I(0xDF, 0xBF), UTF16I(0x7FF)},
        { 0x800, UTF8I(0xE0, 0xA0, 0x80), UTF16I(0x800) },
        { 0x3FFF, UTF8I(0xE3, 0xBF, 0xBF), UTF16I(0x3FFF) },
        { 0x4000, UTF8I(0xE4, 0x80, 0x80), UTF16I(0x4000) },
        { 0xFFFF, UTF8I(0xEF, 0xBF, 0xBF), UTF16I(0xFFFF) },
        { 0x10000, UTF8I(0xF0, 0x90, 0x80, 0x80), UTF16I(0xD800, 0xDC00) },
        { 0x10FFFF, UTF8I(0xF4, 0x8F, 0xBF, 0xBF), UTF16I(0xDBFF, 0xDFFF) }, // Last valid UTF-16 code
        { 0x110000, UTF8I(0xF4, 0x90, 0x80, 0x80) },
        { 0x1FFFFF, UTF8I(0xF7, 0xBF, 0xBF, 0xBF) },
        { 0x200000, UTF8I(0xF8, 0x88, 0x80, 0x80, 0x80) },
        { 0x3FFFFFF, UTF8I(0xFB, 0xBF, 0xBF, 0xBF, 0xBF) },
        { 0x4000000, UTF8I(0xFC, 0x84, 0x80, 0x80, 0x80, 0x80) },
        { 0x7FFFFFFF, UTF8I(0xFD, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF) }
    };
    if (p_res) *(const struct test_utf8 **) p_res = data + *p_ind;
    else if (++*p_ind >= countof(data)) *p_ind = 0;
    return 1;
}

unsigned test_utf8_len(void *In, void *Context, void *Tls)
{
    (void) Context;
    (void) Tls;
    const struct test_utf8 *restrict in = In;
    uint8_t len = utf8_len(in->val);
    return len == in->utf8_len;
}

unsigned test_utf8_encode(void *In, void *Context, void *Tls)
{
    (void) Context;
    (void) Tls;
    const struct test_utf8 *restrict in = In;
    uint8_t byte[UTF8_COUNT], len;
    utf8_encode(in->val, byte, &len);
    return !memcmp(in->utf8, byte, len * sizeof(*byte)) && len == in->utf8_len;
}

unsigned test_utf8_decode(void *In, void *Context, void *Tls)
{
    (void) Context;
    (void) Tls;
    const struct test_utf8 *restrict in = In;
    uint8_t byte[UTF8_COUNT], context = 0, len = 0, ind = 0;
    uint32_t val = 0;
    for (; ind < in->utf8_len; ind++)
        if (!utf8_decode(in->utf8[ind], &val, byte, &len, &context)) break;
    return ind == in->utf8_len && !memcmp(in->utf8, byte, len * sizeof(*byte)) && in->val == val && len == in->utf8_len;
}

unsigned test_utf16_encode(void *In, void *Context, void *Tls)
{
    (void) Context;
    (void) Tls;
    const struct test_utf8 *restrict in = In;
    if (in->val >= UTF8_BOUND) return 1;
    uint16_t word[UTF16_COUNT];
    uint8_t len;
    utf16_encode(in->val, word, &len, 0);
    if (len != in->utf16_len) return 0;
    uint8_t ind = 0;
    for (ind = 0; ind < len; ind++)
        if (in->utf16[ind] != word[ind]) break;
    return ind == len;
}

unsigned test_utf16_decode(void *In, void *Context, void *Tls)
{
    (void) Context;
    (void) Tls;
    const struct test_utf8 *restrict in = In;
    if (in->val >= UTF8_BOUND) return 1;
    uint16_t word[UTF16_COUNT];
    uint8_t context = 0, len = 0, ind = 0;
    uint32_t val = 0;
    for (; ind < in->utf16_len; ind++)
        if (!utf16_decode(in->utf16[ind], &val, word, &len, &context, 0)) break;
    if (ind < in->utf16_len || len != in->utf16_len || in->val != val) return 0;
    for (ind = 0; ind < len; ind++)
        if (in->utf16[ind] != word[ind]) break;
    return ind == len;
}
