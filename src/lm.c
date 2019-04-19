#include "lm.h"
#include "log.h"
#include "memory.h"
#include "sort.h"
#include "strproc.h"
#include "utf8.h"

#include <string.h>

void lm_test()
{
    char str[] = "ABC\0EFD\0ABCD";
    size_t off[] = { 0, 4, 8 };
    int val[] = { 1, 2, 3 };
    struct hash_table tbl = { .cnt = 0 };
    unsigned res;
    hash_table_init(&tbl, 0, sizeof(*off), sizeof(*val));
    size_t h = str_x33_hash("ABC", NULL);
    res = hash_table_alloc(&tbl, &h, "ABC", sizeof(*off), sizeof(*val), str_off_x33_hash, str_off_str_eq, str);
    *(size_t *) hash_table_fetch_key(&tbl, h, sizeof(*off)) = off[0];
    *(int *) hash_table_fetch_val(&tbl, h, sizeof(*val)) = val[0];
    h = str_x33_hash("ABC", NULL);
    res = hash_table_alloc(&tbl, &h, "ABC", sizeof(*off), sizeof(*val), str_off_x33_hash, str_off_str_eq, str);
    h = str_x33_hash("EFD", NULL);
    res = hash_table_alloc(&tbl, &h, "EFD", sizeof(*off), sizeof(*val), str_off_x33_hash, str_off_str_eq, str);
    *(size_t *) hash_table_fetch_key(&tbl, h, sizeof(*off)) = off[1];
    *(int *) hash_table_fetch_val(&tbl, h, sizeof(*val)) = val[1];
    //hash_table_remove(&tbl, )
    h = str_x33_hash("EFD", NULL);
    res = hash_table_alloc(&tbl, &h, "EFD", sizeof(*off), sizeof(*val), str_off_x33_hash, str_off_str_eq, str);
    res = hash_table_search(&tbl, &h, "EFD", sizeof(*off), str_off_str_eq, str);
    h = str_x33_hash("ABCD", NULL);
    res = hash_table_search(&tbl, &h, "ABCD",  sizeof(*off), str_off_str_eq, str);
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

struct lmf_entry {
    size_t off, deg;
};

struct lmf_expr_arg {
    struct lmf_entry *ent;
    size_t *ent_len, cnt;
};

struct base_context {
    struct buff *buff;
    struct hash_table tbl;
    unsigned st;
    size_t len;
    void *context;
};

enum {
    LMF_EXPR_ST_INIT = 0,

    LMF_EXPR_ST_VAR,
    LMF_EXPR_ST_MID,
    LMF_EXPR_ST_NUM,
};

bool lmf_expr_init(void *Context, struct text_metric metric, struct log log)
{
    struct lmf_expr_context *context = Context;
    //if (!hash_table_init(context, 0, sizeof(char *), sizeof(size_t)))
}

bool lmf_expr_impl(void *Arg, void *Context, struct utf8 utf8, struct text_metric metric, struct log log)
{
    struct lmf_expr_arg *arg = arg;
    struct base_context *context = Context;

    for (;;) switch (context->st)
    {
    case LMF_EXPR_ST_INIT:
        if (!utf8.val) return 1;
        // Removing leading whitespaces
        if (utf8_is_whitespace_len(utf8.val, utf8.len)) return 1;
        context->st = LMF_EXPR_ST_VAR;
        continue;
    case LMF_EXPR_ST_VAR:
        switch (utf8.val)
        {
        case '\0':
            // Add to the list
            return 1;
        case '^':
            context->st = LMF_EXPR_ST_NUM;
            return 1;
        case '*':
            if (array_init())
                break;
        case '+':
            break;
        default:
            if (!utf8_is_whitespace_len(utf8.val, utf8.len)) context->len = context->buff->len;

        }
    case LMF_EXPR_ST_NUM:
        switch (utf8.val)
        {

        }

    }
}


bool lmf_compile(void *Context, struct utf8 utf8, struct text_metric metric, struct log log)
{

}