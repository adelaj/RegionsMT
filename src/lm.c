#include "lm.h"
#include "log.h"
#include "sort.h"
#include "strproc.h"
#include "utf8.h"

#include <string.h>

size_t str_off_x33_hash(const void *Off, void *Str)
{
    return str_x33_hash((char *) Str + *(size_t *) Off, NULL);
}

bool str_off_eq(const void *A, const void *B, void *Str)
{
    return strcmp((char *) Str + *(size_t *) A, (char *) Str + *(size_t *) B);
}

void lm_test()
{
    char str[] = "ABC\0EFD\0ABCD";
    size_t off[] = { 0, 4, 8 };
    int val[] = { 1, 2, 3 }, *tmp;
    struct hash_table tbl = { .cnt = 0 };
    unsigned res;
    hash_table_init(&tbl, 0, sizeof(*off), sizeof(*val));
    res = hash_table_insert(&tbl, off + 0, sizeof(*off), val + 0, sizeof(*val), str_off_x33_hash, str_off_eq, str);
    res = hash_table_insert(&tbl, off + 0, sizeof(*off), val + 0, sizeof(*val), str_off_x33_hash, str_off_eq, str);
    res = hash_table_insert(&tbl, off + 1, sizeof(*off), val + 1, sizeof(*val), str_off_x33_hash, str_off_eq, str);
    //hash_table_remove(&tbl, )
    res = hash_table_insert(&tbl, off + 1, sizeof(*off), val + 1, sizeof(*val), str_off_x33_hash, str_off_eq, str);
    res = hash_table_search(&tbl, off + 1, sizeof(*off), &tmp, sizeof(tmp), str_off_x33_hash, str_off_eq, str);
    res = hash_table_search(&tbl, off + 2, sizeof(*off), &tmp, sizeof(tmp), str_off_x33_hash, str_off_eq, str);
}



struct entry {
    size_t *deg;
    uint8_t *bits;
};

struct utf8 {
    uint8_t byte[UTF8_COUNT], len, context;
    uint32_t val;
};

enum {
    STATUS_FAILURE = 0,
    STATUS_SUCCESS,
    STATUS_REPEAT,
};

struct lmf_name_context {
    char *buff;
    size_t cnt, cap;
};

unsigned lmf_name_impl(void *Context, struct utf8 *utf8, struct text_metric metric, struct log log)
{

}

unsigned lmf_name_finalize_impl(void *Context, struct text_metric metric, struct log log)
{

}

unsigned lmf_compile(void *Context, struct utf8 *utf8, struct text_metric metric, struct log log)
{
    switch (utf8->val)
    {
    case '^':


    case '*':
    case '+':
    default:
        if (utf8_is_whitespace_len(utf8->val, utf8->len)) return 1;
        else
    }
}