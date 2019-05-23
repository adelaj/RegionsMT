#include "ll.h"
#include "lm.h"
#include "log.h"
#include "memory.h"
#include "object.h"
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

struct lmf_reg_context {
    size_t deg_cnt;
};

struct lmf_reg_entry {
    size_t *deg;
    bool mark;
};

bool lmf_reg_entry_cmp(const void *A, const void *B, void *Context)
{
    struct lmf_reg_context *context = Context;
    struct lmf_reg_entry *a = A, *b = B;
    size_t i = 0, j;
    for (; i < context->deg_cnt && a->deg[i] > b->deg[i]; i++);
    for (j = i; j < context->deg_cnt && a->deg[i] >= b->deg[i]; j++);
    if (j == context->deg_cnt) b->mark = 1;
    return i > 0;
}

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
    size_t *len, ent_cnt, ent_cap, len_cnt, len_cap;
};

struct base_context {
    struct buff *buff;
    struct hash_table *tbl;
    struct text_metric metric;
    unsigned st;
    size_t reg;
    void *context;
};

enum {
    LMF_EXPR_ST_INIT = 0,
    LMF_EXPR_ST_VAR_INIT,
    LMF_EXPR_ST_WHITESPACE_VAR,
    
    LMF_EXPR_ST_VAR,
    LMF_EXPR_ST_OP,
    LMF_EXPR_ST_WHITESPACE_NUM,
    LMF_EXPR_ST_NUM,
};

void lmf_expr_arg_close(void *Arg)
{
    struct lmf_expr_arg *arg = Arg;
    free(arg->len);
    free(arg->ent);
}

bool lmf_expr_impl(void *Arg, void *Context, struct utf8 utf8, struct text_metric metric, struct log *log)
{
    struct lmf_expr_arg *arg = Arg;
    struct base_context *context = Context;
    for (;;) switch (context->st)
    {
    case LMF_EXPR_ST_INIT:
        if (!utf8_is_whitespace_len(utf8.val, utf8.len)) return 1; // Removing leading whitespaces        
        if (!utf8.val) return 1;
        if (!array_init(&arg->len, &arg->len_cap, sizeof(*arg->len), 0, ARRAY_CLEAR, 1)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
        else
        {
            arg->len[arg->len_cnt++]++;
            context->st++;
            continue;
        }
        return 0;
    case LMF_EXPR_ST_VAR_INIT:
        if (utf8.val == '^' || utf8.val == '*' || utf8.val == '+') log_message_error_str_xml(log, CODE_METRIC, metric, utf8.byte, utf8.len, XML_ERROR_CHAR_UNEXPECTED_CHAR);
        else if (!buff_append(context->buff, utf8.byte, utf8.len, 0)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
        else
        {
            context->reg = utf8.len;
            context->st++;
            return 1;
        }
        lmf_expr_arg_close(arg);
        return 0;
    case LMF_EXPR_ST_VAR:
        switch (utf8.val)
        {
        case '\0':
            break;
        case '^':
            context->st = LMF_EXPR_ST_WHITESPACE_NUM;
            break;
        case '*':
            context->st = LMF_EXPR_ST_WHITESPACE_VAR;
            arg->len[arg->len_cnt - 1]++;
            break;
        case '+':
            if (!array_test(&arg->len, &arg->len_cap, sizeof(*arg->len), 0, ARRAY_CLEAR, arg->len_cnt, 1)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
            else
            {
                arg->len[arg->len_cnt++]++;
                break;
            }
            lmf_expr_arg_close(arg);
            return 0;
        default:
            if (!buff_append(context->buff, utf8.byte, utf8.len, 0)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
            else
            {
                if (!utf8_is_whitespace_len(utf8.val, utf8.len)) context->reg = context->buff->len;
                return 1;
            }
            lmf_expr_arg_close(arg);
            return 0;
        }
        if (!array_test(&arg->ent, &arg->ent_cap, sizeof(*arg->ent), 0, 0, arg->ent_cnt, 1)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
        else if (!str_pool_insert(context->tbl, context->buff, context->reg, &arg->ent[arg->ent_cnt++].off)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
        else return 1;
        lmf_expr_arg_close(arg);
        return 0;
    case LMF_EXPR_ST_WHITESPACE_VAR:
        if (!utf8_is_whitespace_len(utf8.val, utf8.len)) return 1;
        if (utf8.val)
        {
            context->st++;
            continue;
        }
        lmf_expr_arg_close(arg);
        return 0; // Termination error
    case LMF_EXPR_ST_WHITESPACE_NUM:
        if (!utf8_is_whitespace_len(utf8.val, utf8.len)) return 1;
        if (utf8.val < '0' && utf8.val > '9') log_message_error_str_xml(log, CODE_METRIC, metric, utf8.byte, utf8.len, XML_ERROR_CHAR_UNEXPECTED_CHAR);
        else
        {
            context->reg = utf8.val - '0';
            context->st++;
            return 1;
        }
        lmf_expr_arg_close(arg);
        return 0;
    case LMF_EXPR_ST_NUM:
        switch (utf8.val)
        {
        case '\0':
            break;
        case '*':
            break;
        case '+':
            break;
        default:
            if (utf8.val < '0' && utf8.val > '9') log_message_error_str_xml(log, CODE_METRIC, metric, utf8.byte, utf8.len, XML_ERROR_CHAR_UNEXPECTED_CHAR);
            else if (!size_mul_add_test(&context->reg, 10, utf8.val - '0')) log_message_error_val_xml(log, CODE_METRIC, metric, context->reg, XML_ERROR_VAL_RANGE);
            else return 1;
        }
        arg->ent[arg->ent_cnt - 1].deg = context->reg;
        return 1;
    }
}

struct entry {
    size_t *deg;
    uint8_t *bits;
};

bool lmf_matrix(struct lmf_expr_arg *arg)
{
    for (size_t i = 0; i < arg->len_cnt; i++)
    {

    }
}

#include <gsl/gsl_multifit.h>

bool lmf_impl(double *Reg, double *Obs, size_t dimx, size_t dimy)
{
    gsl_matrix_view reg = gsl_matrix_view_array(Reg, dimx, dimy);
    gsl_vector_view obs = gsl_vector_view_array(Obs, dimx);


}