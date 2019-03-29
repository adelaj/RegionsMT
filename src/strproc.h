#pragma once

///////////////////////////////////////////////////////////////////////////////
//
//  Functions for a string processing
//

#include "common.h"

struct strl {
    char *str;
    size_t len;
};

#define STRL(STRL) (STRL).str, (STRL).len

struct text_metric {
    const char *path;
    uint64_t byte;
    size_t col, row;
};

// Functions to be used as 'stable_cmp_callback' and 'cmp_callback' (see sort.h)
int char_cmp(const void *, const void *, void *);
int str_strl_stable_cmp(const void *, const void *, void *);
int str_off_stable_cmp(const void *, const void *, void *);
bool str_off_cmp(const void *, const void *, void *);

typedef bool (*read_callback)(const char *, size_t, void *, void *); // Functional type for read callbacks
typedef bool (*write_callback)(char *, size_t *, void *, void *); // Functional type for write callbacks
typedef union { read_callback read; write_callback write; } rw_callback;

struct handler_context {
    ptrdiff_t offset;
    size_t bit_pos;
};

enum {
    CVT_ERROR = 0,
    CVT_SUCCESS,
    CVT_OUT_OF_RANGE
};

unsigned str_to_uint64(const char *, const char **, uint64_t *);
unsigned str_to_uint32(const char *, const char **, uint32_t *);
unsigned str_to_uint16(const char *, const char **, uint16_t *);
unsigned str_to_uint8(const char *, const char **, uint8_t *);
unsigned str_to_size(const char *, const char **, size_t *);
unsigned str_to_uint64_hex(const char *, const char **, uint64_t *);
unsigned str_to_uint32_hex(const char *, const char **, uint32_t *);
unsigned str_to_uint16_hex(const char *, const char **, uint16_t *);
unsigned str_to_uint8_hex(const char *, const char **, uint8_t *);
unsigned str_to_size_hex(const char *, const char **, size_t *);
unsigned str_to_flt64(const char *, const char **, double *);

struct bool_handler_context {
    size_t bit_pos;
    struct handler_context *context;
};

// Functions to be used as 'read_callback'
bool empty_handler(const char *, size_t, void *, void *);
bool p_str_handler(const char *, size_t, void *, void *);
bool str_handler(const char *, size_t, void *, void *);
bool bool_handler(const char *, size_t, void *, void *);
bool bool_handler2(const char *, size_t, void *, void *);
bool uint64_handler(const char *, size_t, void *, void *);
bool uint32_handler(const char *, size_t, void *, void *);
bool uint16_handler(const char *, size_t, void *, void *);
bool uint8_handler(const char *, size_t, void *, void *);
bool size_handler(const char *, size_t, void *, void *);
bool flt64_handler(const char *, size_t, void *, void *);

struct str_tbl_handler_context {
    char *str;
    size_t str_cap, str_cnt;
};

// Here the second argument should be a definite number 
bool str_tbl_handler(const char *, size_t, void *, void *);
