#include "lm.h"
#include "log.h"
#include "memory.h"
#include "sort.h"
#include "strproc.h"
#include "utf8.h"

#include <string.h>

size_t str_off_x33_hash(const void *Off, void *Str)
{
    return str_x33_hash((char *) Str + *(size_t *) Off, NULL);
}

void lm_test()
{
    char str[] = "ABC\0EFD\0ABCD";
    size_t off[] = { 0, 4, 8 };
    int val[] = { 1, 2, 3 };
    size_t tmp;
    struct hash_table tbl = { .cnt = 0 };
    unsigned res;
    hash_table_init(&tbl, 0, sizeof(*off), sizeof(*val));
    res = hash_table_insert(&tbl, off + 0, sizeof(*off), val + 0, sizeof(*val), str_off_x33_hash, str_off_eq, str);
    res = hash_table_insert(&tbl, off + 0, sizeof(*off), val + 0, sizeof(*val), str_off_x33_hash, str_off_eq, str);
    res = hash_table_insert(&tbl, off + 1, sizeof(*off), val + 1, sizeof(*val), str_off_x33_hash, str_off_eq, str);
    //hash_table_remove(&tbl, )
    res = hash_table_insert(&tbl, off + 1, sizeof(*off), val + 1, sizeof(*val), str_off_x33_hash, str_off_eq, str);
    res = hash_table_search(&tbl, &tmp, off + 1, sizeof(*off), str_off_x33_hash, str_off_eq, str);
    res = hash_table_search(&tbl, &tmp, off + 2, sizeof(*off), str_off_x33_hash, str_off_eq, str);
}



struct entry {
    size_t *deg;
    uint8_t *bits;
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

unsigned lmf_name_finalize(void *Context, struct text_metric metric, struct log log)
{

}


struct lmf_expr_context {
    unsigned st;
    struct buff *buff;
    struct hash_table tbl;
};

enum {
    LMF_EXPR_BEGIN = 0,
    LMF_EXPR_NAME,

};

bool lmf_expr_init(void *Context, struct text_metric metric, struct log log)
{
    struct lmf_expr_context *context = Context;
    //if (!hash_table_init(context, 0, sizeof(char *), sizeof(size_t)))
}

bool lmf_expr_impl(void *Arg, void *Context, struct utf8 utf8, struct text_metric metric, struct log log)
{
    struct lmf_expr_context *context = Context;
    if (utf8_is_whitespace_len(utf8.val, utf8.len))
    {

    }
    switch (utf8.val)
    {
    case '^':

        break;

    case '*':
        break;

    case '+':
        break;

    default:

        context->st = LMF_EXPR_NAME;

    }
}

bool lmf_expr_close(void *Context, struct text_metric metric, struct log log)
{

}

bool lmf_compile(void *Context, struct utf8 utf8, struct text_metric metric, struct log log)
{

}