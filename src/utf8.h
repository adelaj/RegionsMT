#pragma once

#include "common.h"

#include <uchar.h>

#define UTF8_COUNT 6
#define UTF8_BOUND 0x110000
#define UTF8_BOM "\xef\xbb\xbf"

#define UTF8_LDQUO "\xe2\x80\x9c"
#define UTF8_RDQUO "\xe2\x80\x9d"
#define UTF8_LSQUO "\xe2\x80\x98"
#define UTF8_RSQUO "\xe2\x80\x99"

uint8_t utf8_len(uint32_t);

bool utf8_is_overlong(uint32_t, uint8_t);
bool utf8_is_valid(uint32_t, uint8_t);

// Before proceeding with these functions, 'utf8_is_valid' should be ensured to return 'true'.
// In many cases length is known explicitly, so the possibility is provided to pass it directly.
bool utf8_is_whitespace(uint32_t);
bool utf8_is_whitespace_len(uint32_t, uint8_t);
bool utf8_is_xml_char(uint32_t);
bool utf8_is_xml_char_len(uint32_t, uint8_t);
bool utf8_is_xml_name_start_char_len(uint32_t, uint8_t);
bool utf8_is_xml_name_char_len(uint32_t, uint8_t);

void utf8_encode(uint32_t, char *restrict, uint8_t *restrict);
bool utf8_decode(char, uint32_t *restrict, char *restrict, uint8_t *restrict, uint8_t *restrict);

bool utf8_decode_len(const char *restrict, size_t, size_t *restrict);

#define UTF16_COUNT 2

uint8_t utf16_len(uint32_t);
void utf16_encode(uint32_t, char16_t *restrict, uint8_t *restrict, bool);
bool utf16_decode(uint16_t, uint32_t *restrict, char16_t *restrict, uint8_t *restrict, uint8_t *restrict, bool);

struct utf8 {
    union {
        uint8_t byte[UTF8_COUNT];
        char chr[UTF8_COUNT];
    };
    uint8_t len, context;
    uint32_t val;
};
