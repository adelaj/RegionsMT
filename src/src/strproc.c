#include "np.h"
#include "ll.h"
#include "memory.h"
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

int str_off_stable_cmp(const void *A, const void *B, void *Str)
{
    return strcmp((char *) Str + *(size_t *) A, (char *) Str + *(size_t *) B);
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

#define DECLARE_STR_TO_UINT(TYPE, SUFFIX, LIMIT, BACKEND_RETURN, BACKEND) \
    unsigned str_to_ ## SUFFIX(const char *str, const char **ptr, TYPE *p_res) \
    { \
        errno = 0; \
        BACKEND_RETURN res = BACKEND(str, (char **) ptr, 10); \
        Errno_t err = errno; \
        if (res > (LIMIT)) \
        { \
            *p_res = (LIMIT); \
            return err && err != ERANGE ? 0 : CVT_OUT_OF_RANGE; \
        } \
        *p_res = (TYPE) res; \
        return err ? err == ERANGE ? CVT_OUT_OF_RANGE : 0 : 1; \
    }

DECLARE_STR_TO_UINT(uint64_t, uint64, UINT64_MAX, unsigned long long, strtoull)
DECLARE_STR_TO_UINT(uint32_t, uint32, UINT32_MAX, unsigned long, strtoul)
DECLARE_STR_TO_UINT(uint16_t, uint16, UINT16_MAX, unsigned long, strtoul)
DECLARE_STR_TO_UINT(uint8_t, uint8, UINT8_MAX, unsigned long, strtoul)

#if defined _M_X64 || defined __x86_64__
DECLARE_STR_TO_UINT(size_t, size, SIZE_MAX, unsigned long long, strtoull)
#elif defined _M_IX86 || defined __i386__
DECLARE_STR_TO_UINT(size_t, size, SIZE_MAX, unsigned long, strtoul)
#endif

unsigned str_to_flt64(const char *str, const char **ptr, double *p_res)
{
    errno = 0;
    *p_res = strtod(str, (char **) ptr);
    Errno_t err = errno;
    return err ? err == ERANGE ? CVT_OUT_OF_RANGE : 0 : 1;
}

struct bool_handler_context {
    size_t bit_pos;
    struct handler_context *context;
};

bool bool_handler(const char *str, size_t len, void *Ptr, void *Context)
{
    (void) len;
    char *test;
    uint8_t res;
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

#define DECLARE_INTEGER_HANDLER(TYPE, PREFIX, CONV) \
    bool PREFIX ## _handler(const char *str, size_t len, void *Ptr, void *Context) \
    { \
        (void) len; \
        TYPE *ptr = Ptr; \
        char *test; \
        if (CONV(str, &test, ptr) || *test) return 0; \
        return empty_handler(str, len, Ptr, Context); \
    }

DECLARE_INTEGER_HANDLER(uint64_t, uint64, str_to_uint64)
DECLARE_INTEGER_HANDLER(uint32_t, uint32, str_to_uint32)
DECLARE_INTEGER_HANDLER(uint16_t, uint16, str_to_uint16)
DECLARE_INTEGER_HANDLER(uint8_t, uint8, str_to_uint8)
DECLARE_INTEGER_HANDLER(size_t, size, str_to_size)
DECLARE_INTEGER_HANDLER(double, flt64, str_to_flt64)

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
    memcpy(context->str + context->str_cnt, str, len + 1);
    context->str_cnt += len + 1;
    return 1;
}
