#include "np.h"
#include "ll.h"
#include "log.h"
#include "memory.h"
#include "sort.h"
#include "strproc.h"

#include <string.h>
#include <stdlib.h>

int char_cmp(const void *a, const void *b, void *context)
{
    (void) context;
    return (int) *(const char *) a - (int) *(const char *) b;
}

int str_strl_stable_cmp(const void *Str, const void *Entry, void *p_Len)
{
    const struct strl *entry = Entry;
    size_t len = *(size_t *) p_Len;
    int res = strncmp((const char *) Str, entry->str, len);
    return res ? res : len < entry->len ? INT_MIN : 0;
}

bool str_eq(const void *A, const void *B, void *context)
{
    (void) context;
    return !strcmp((const char *) A, (const char *) B);
}

bool str_off_str_eq(const void *A, const void *B, void *Str)
{
    return str_eq((const char *) Str + *(size_t *) A, B, NULL);
}

bool str_off_eq(const void *A, const void *B, void *Str)
{
    return str_eq((const char *) Str + *(size_t *) A, (const char *) Str + *(size_t *) B, NULL);
}

int str_off_stable_cmp(const void *A, const void *B, void *Str)
{
    return strcmp((const char *) Str + *(size_t *) A, (const char *) Str + *(size_t *) B);
}

bool str_off_cmp(const void *A, const void *B, void *Str)
{
    return str_off_stable_cmp(A, B, Str) > 0;
}

bool empty_handler(const char *str, size_t len, void *Ptr, void *Context)
{
    (void) str;
    (void) len;
    struct handler_context *context = Context;
    if (context) uint8_bit_set((uint8_t *) Ptr + context->offset, context->bit_pos);
    return 1;
}

bool p_str_handler(const char *str, size_t len, void *Ptr, void *context)
{
    (void) context;
    (void) len;
    const char **ptr = Ptr;
    *ptr = str;
    return 1;
}

bool str_handler(const char *str, size_t len, void *Ptr, void *context)
{
    (void) context;
    char **ptr = Ptr;
    char *tmp = malloc(len + 1);
    if (len + 1 && !tmp) return 0;
    memcpy(tmp, str, len + 1);
    *ptr = tmp;
    return 1;
}

#define DECLARE_STR_TO_UINT(TYPE, SUFFIX, LIMIT, BACKEND_RETURN, BACKEND, RADIX) \
    unsigned str_to_ ## SUFFIX(const char *str, const char **ptr, TYPE *p_res) \
    { \
        errno = 0; \
        BACKEND_RETURN res = BACKEND(str, (char **) ptr, (RADIX)); \
        Errno_t err = errno; \
        if (res > (LIMIT)) \
        { \
            *p_res = (LIMIT); \
            return err && err != ERANGE ? 0 : CVT_OUT_OF_RANGE; \
        } \
        *p_res = (TYPE) res; \
        return err ? err == ERANGE ? CVT_OUT_OF_RANGE : 0 : 1; \
    }

DECLARE_STR_TO_UINT(uint64_t, uint64, UINT64_MAX, unsigned long long, strtoull, 10)
DECLARE_STR_TO_UINT(uint32_t, uint32, UINT32_MAX, unsigned long, strtoul, 10)
DECLARE_STR_TO_UINT(uint16_t, uint16, UINT16_MAX, unsigned long, strtoul, 10)
DECLARE_STR_TO_UINT(uint8_t, uint8, UINT8_MAX, unsigned long, strtoul, 10)
DECLARE_STR_TO_UINT(uint64_t, uint64_hex, UINT64_MAX, unsigned long long, strtoull, 16)
DECLARE_STR_TO_UINT(uint32_t, uint32_hex, UINT32_MAX, unsigned long, strtoul, 16)
DECLARE_STR_TO_UINT(uint16_t, uint16_hex, UINT16_MAX, unsigned long, strtoul, 16)
DECLARE_STR_TO_UINT(uint8_t, uint8_hex, UINT8_MAX, unsigned long, strtoul, 16)

#if defined _M_X64 || defined __x86_64__
DECLARE_STR_TO_UINT(size_t, size, SIZE_MAX, unsigned long long, strtoull, 10)
DECLARE_STR_TO_UINT(size_t, size_hex, SIZE_MAX, unsigned long long, strtoull, 16)
#elif defined _M_IX86 || defined __i386__
DECLARE_STR_TO_UINT(size_t, size, SIZE_MAX, unsigned long, strtoul, 10)
DECLARE_STR_TO_UINT(size_t, size_hex, SIZE_MAX, unsigned long, strtoul, 16)
#endif

unsigned str_to_flt64(const char *str, const char **ptr, double *p_res)
{
    errno = 0;
    *p_res = strtod(str, (char **) ptr);
    Errno_t err = errno;
    return err ? err == ERANGE ? CVT_OUT_OF_RANGE : 0 : 1;
}

bool bool_handler(const char *str, size_t len, void *Ptr, void *Context)
{
    (void) len;
    uint8_t res;
    const char *test;
    if (!str_to_uint8(str, &test, &res)) return 0;
    if (*test)
    {
        if (!Stricmp(str, "false")) res = 0;
        else if (!Stricmp(str, "true")) res = 1;
        else return 0;
    }
    if (res > 1) return 0;
    struct bool_handler_context *context = Context;
    if (!context) return 1;
    (res ? uint8_bit_set : uint8_bit_reset)((uint8_t *) Ptr, context->bit_pos);
    return empty_handler(str, len, Ptr, context->context);
}

bool bool_handler2(const char *str, size_t len, void *Ptr, void *Context)
{
    return str && len ? bool_handler(str, len, Ptr, Context) : 
        empty_handler(str, len, Ptr, &(struct handler_context) { .bit_pos = ((struct bool_handler_context *) Context)->bit_pos });
}

#define DECLARE_INT_HANDLER(TYPE, PREFIX, CONV) \
    bool PREFIX ## _handler(const char *str, size_t len, void *Ptr, void *Context) \
    { \
        (void) len; \
        TYPE *ptr = Ptr; \
        const char *test; \
        if (!CONV(str, &test, ptr) || *test) return 0; \
        return empty_handler(str, len, Ptr, Context); \
    }

DECLARE_INT_HANDLER(uint64_t, uint64, str_to_uint64)
DECLARE_INT_HANDLER(uint32_t, uint32, str_to_uint32)
DECLARE_INT_HANDLER(uint16_t, uint16, str_to_uint16)
DECLARE_INT_HANDLER(uint8_t, uint8, str_to_uint8)
DECLARE_INT_HANDLER(size_t, size, str_to_size)
DECLARE_INT_HANDLER(double, flt64, str_to_flt64)

bool str_tbl_handler(const char *str, size_t len, void *p_Off, void *Context)
{
    size_t *p_off = p_Off;
    struct str_tbl_handler_context *context = Context;
    if (!len && context->str_cnt) // Last null-terminator is used to store zero-length strings
    {
        *p_off = context->str_cnt - 1;
        return 1;
    }
    if (!array_test(&context->str, &context->str_cap, sizeof(*context->str), 0, 0, context->str_cnt, len, 1)) return 0;
    *p_off = context->str_cnt;
    memcpy(context->str + context->str_cnt, str, (len + 1) * sizeof(*context->str));
    context->str_cnt += len + 1;
    return 1;
}

enum buff_flags {
    BUFFER_INIT = 1, // Zero characharacter at the beginning
    BUFFER_TERM = 2 // Zero character at the ending
};

unsigned buff_append(struct buff *buff, const char *str, size_t len, enum buff_flags flags)
{
    bool init = flags & BUFFER_INIT, term = flags & BUFFER_TERM;
    unsigned res = array_test(&buff->str, &buff->cap, sizeof(*buff->str), 0, 0, len, buff->len, init + term);
    if (!res) return 0;
    memcpy(buff->str + buff->len + init, str, len * sizeof(*buff->str));
    buff->len += len + init;
    if (term) buff->str[buff->len] = '\0';
    return res;
}

struct str_pool {
    struct buff buff;
    struct hash_table tbl;
};

bool str_pool_init(struct str_pool *pool, size_t cnt, size_t len)
{
    if (array_init(&pool->buff.str, &pool->buff.cap, size_add_sat(len, 1), sizeof(*pool->buff.str), 0, 0))
    {
        pool->buff.str[0] = '\0'; // Addind empty srting
        pool->buff.len = 0;
        if (hash_table_init(&pool->tbl, cnt, sizeof(size_t), 0)) return 1;
        free(pool->buff.str);
    }
    return 0;
}

size_t str_off_x33_hash(const void *Off, void *Str)
{
    return str_x33_hash((char *) Str + *(size_t *) Off, NULL);
}

enum {
    STR_POOL_FAILURE = HASH_FAILURE,
    STR_POOL_SUCCESS = HASH_SUCCESS,
    STR_POOL_UNTOUCHED = HASH_UNTOUCHED,
    STR_POOL_PRESENT = HASH_PRESENT,
};

unsigned str_pool_insert(struct str_pool *pool, const char *str, size_t len, size_t *p_off)
{
    if (!len) // Empty strings handled separately
    {
        *p_off = 0;
        return 1 | STR_POOL_PRESENT | STR_POOL_UNTOUCHED;
    }
    size_t h = str_x33_hash(str, NULL);
    unsigned res = hash_table_alloc(&pool->tbl, &h, str, sizeof(size_t), 0, str_off_x33_hash, str_off_str_eq, pool->buff.str);
    if (!res) return 0;
    if (res & HASH_PRESENT)
    {
        *p_off = *(size_t *) hash_table_fetch_key(&pool->tbl, h, sizeof(size_t));
        return res;
    }
    size_t off = pool->buff.len + 1;
    unsigned res2 = buff_append(&pool->buff, str, len, BUFFER_INIT | BUFFER_TERM);
    if (!res2) 
    {
        hash_table_dealloc(&pool->tbl, h);
        return 0;
    }
    *(size_t *) hash_table_fetch_key(&pool->tbl, h, sizeof(size_t)) = *p_off = off;
    return res & res2;
}
