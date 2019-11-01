#include "np.h"
#include "ll.h"
#include "lm.h"
#include "log.h"
#include "memory.h"
#include "object.h"
#include "sort.h"
#include "strproc.h"
#include "utf8.h"
#include "intdom.h"

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

struct lmf_name_context {
    char *buff;
    size_t cnt, cap;
};

/*
unsigned lmf_name_impl(void *Context, struct utf8 *utf8, struct text_metric metric, struct log log)
{

}

unsigned lmf_name_finalize(void *Context, struct text_metric metric, struct log log)
{

}
*/

// 'deg' = 0 corresponds to the categorical (factor) type
// 'deg' > 0 corresponds to the numeric/ordinal type
struct lmf_entry {
    size_t off, deg;
};

struct lmf_expr_arg {
    struct str_pool pool;
    struct lmf_entry *ent;
    size_t *len, ent_cnt, ent_cap, len_cnt, len_cap;
};

enum {
    LMF_EXPR_INIT = 0,
    LMF_EXPR_TYPE,
    LMF_EXPR_VAR_INIT,
    LMF_EXPR_VAR,
    LMF_EXPR_OP,
    LMF_EXPR_TYPE_INIT,
    LMF_EXPR_NUM_INIT,
    LMF_EXPR_NUM,
};

void lmf_expr_arg_close(void *Arg)
{
    struct lmf_expr_arg *arg = Arg;
    free(arg->len);
    free(arg->ent);
    str_pool_close(&arg->pool);
}

// Example: 'numeric(a) ^ 2 * categorical(b) + categorical(c) * categorical(b)'
bool lmf_expr_impl(void *Arg, void *Context, struct utf8 utf8, struct text_metric metric, struct log *log)
{
    struct lmf_expr_arg *restrict arg = Arg;
    struct base_context *restrict context = Context;
    switch (context->st)
    {
    case LMF_EXPR_INIT:
        if (!utf8.val) return 1;
        if (utf8_is_whitespace_len(utf8.val, utf8.len)) return 1; // Removing leading whitespaces        
        if (!array_init(&arg->len, &arg->len_cap, 1, sizeof(*arg->len), 0, ARRAY_CLEAR) ||
            !array_init(&arg->ent, &arg->ent_cap, 1, sizeof(*arg->ent), 0, ARRAY_CLEAR) ||
            !str_pool_init(&arg->pool, 0, 0)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
        else
        {
            arg->len[arg->len_cnt++]++;
            arg->ent_cnt++;
            if (strchr("^*+()", utf8.val)) log_message_error_xml_chr(log, CODE_METRIC, metric, XML_UNEXPECTED_CHAR, utf8.byte, utf8.len);
            else if (!buff_append(context->buff, utf8.chr, utf8.len, BUFFER_DISCARD)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
            else
            {
                context->metric = metric;
                context->len = utf8.len;
                context->st++;
                return 1;
            }
        }
        break;
    case LMF_EXPR_TYPE:
        if (!utf8.val) break;
        if (strchr("^*+)", utf8.val)) log_message_error_xml_chr(log, CODE_METRIC, metric, XML_UNEXPECTED_CHAR, utf8.byte, utf8.len);
        else if (utf8.val == '(')
        {
            if (!STREQ(context->buff->str, context->len, "categorical", Strnicmp))
            {
                if (!STREQ(context->buff->str, context->len, "numeric", Strnicmp) &&
                    !STREQ(context->buff->str, context->len, "ordinal", Strnicmp))
                {
                    log_message_error_xml_generic(log, CODE_METRIC, context->metric, "Unexpected covariate type %~s*", context->buff->str, context->len);
                    break;
                }
                arg->ent[arg->ent_cnt - 1].deg = 1;               
            }
            context->st++;
            return 1;
        }
        else if (!buff_append(context->buff, utf8.chr, utf8.len, 0)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
        else
        {
            if (!utf8_is_whitespace_len(utf8.val, utf8.len)) context->len = context->buff->len;
            return 1;
        }
        break;
    case LMF_EXPR_VAR_INIT:
        if (!utf8.val) break;
        if (utf8_is_whitespace_len(utf8.val, utf8.len)) return 1;
        if (strchr("^*+(", utf8.val)) log_message_error_xml_chr(log, CODE_METRIC, metric, XML_UNEXPECTED_CHAR, utf8.byte, utf8.len);
        else if (utf8.val == ')') log_message_error_xml_generic(log, CODE_METRIC, metric, "Covariate name expected before %~c", ')');
        else if (!buff_append(context->buff, utf8.chr, utf8.len, BUFFER_DISCARD)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
        else
        {
            context->metric = metric;
            context->len = utf8.len;
            context->st++;
            return 1;
        }
        break;
    case LMF_EXPR_VAR:
        if (!utf8.val) break;
        if (strchr("^*+(", utf8.val)) log_message_error_xml_chr(log, CODE_METRIC, metric, XML_UNEXPECTED_CHAR, utf8.byte, utf8.len);
        else if (utf8.val == ')')
        {
            if (!buff_append(context->buff, 0, 0, BUFFER_TERM) ||
                !str_pool_insert(&arg->pool, context->buff->str, context->len, &arg->ent[arg->ent_cnt - 1].off)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
            else
            {
                context->st = LMF_EXPR_OP;
                return 1;
            }
        }
        else if (!buff_append(context->buff, utf8.chr, utf8.len, 0)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
        else
        {
            if (!utf8_is_whitespace_len(utf8.val, utf8.len)) context->len = context->buff->len;
            return 1;
        }
        break;
    case LMF_EXPR_OP:
        switch (utf8.val)
        {
        case '\0':
            return 1;
        case '^':
            if (!arg->ent[arg->ent_cnt - 1].deg) log_message_error_xml_generic(log, CODE_METRIC, metric, "Operation %~c cannot be applied to the covariate of the categorical type", '^');
            else
            {
                context->st = LMF_EXPR_NUM_INIT;
                return 1;
            }
            break;
        case '*':
            context->st = LMF_EXPR_TYPE_INIT;
            arg->len[arg->len_cnt - 1]++;
            return 1;
        case '+':
            if (!array_test(&arg->len, &arg->len_cap, sizeof(*arg->len), 0, ARRAY_CLEAR, arg->len_cnt, 1)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
            else
            {
                context->st = LMF_EXPR_TYPE_INIT;
                arg->len[arg->len_cnt++]++;
                return 1;
            }
            break;
        default:
            if (utf8_is_whitespace_len(utf8.val, utf8.len)) return 1;
            log_message_error_xml_chr(log, CODE_METRIC, metric, XML_UNEXPECTED_CHAR, utf8.byte, utf8.len);
            break;
        }
        break;
    case LMF_EXPR_TYPE_INIT:
        if (!utf8.val) break;
        if (utf8_is_whitespace_len(utf8.val, utf8.len)) return 1; // Removing leading whitespaces
        if (!array_test(&arg->ent, &arg->ent_cap, sizeof(*arg->ent), 0, ARRAY_CLEAR, arg->ent_cnt, 1)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
        else
        {
            //arg->len[arg->len_cnt - 1]++;
            arg->ent_cnt++;
            if (strchr("^*+()", utf8.val)) log_message_error_xml_chr(log, CODE_METRIC, metric, XML_UNEXPECTED_CHAR, utf8.byte, utf8.len);
            else if (!buff_append(context->buff, utf8.chr, utf8.len, BUFFER_DISCARD)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
            else
            {
                context->metric = metric;
                context->len = utf8.len;
                context->st = LMF_EXPR_TYPE;
                return 1;
            }
        }
        break;
    case LMF_EXPR_NUM_INIT:
        if (!utf8.val) break;
        if (utf8_is_whitespace_len(utf8.val, utf8.len)) return 1;
        if (utf8.val < '0' && utf8.val > '9') log_message_error_xml_chr(log, CODE_METRIC, metric, XML_UNEXPECTED_CHAR, utf8.byte, utf8.len);
        else
        {
            context->metric = metric;
            arg->ent[arg->ent_cnt - 1].deg = utf8.val - '0';
            context->st++;
            context->len = 0;
            return 1;
        }
        break;
    case LMF_EXPR_NUM:
        switch (utf8.val)
        {
        case '\0':
            return 1;
        case '*':
            context->st = LMF_EXPR_TYPE_INIT;
            arg->len[arg->len_cnt - 1]++;
            return 1;
        case '+':
            if (!array_test(&arg->len, &arg->len_cap, sizeof(*arg->len), 0, ARRAY_CLEAR, arg->len_cnt, 1)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
            else
            {
                context->st = LMF_EXPR_TYPE_INIT;
                arg->len[arg->len_cnt++]++;
                return 1;
            }
            break;
        default:
            if (utf8_is_whitespace_len(utf8.val, utf8.len))
            {
                context->len = 1;
                return 1;
            }
            else if (context->len || utf8.val < '0' || utf8.val > '9') log_message_error_xml_chr(log, CODE_METRIC, metric, XML_UNEXPECTED_CHAR, utf8.byte, utf8.len);
            else if (!size_mul_add_test(&arg->ent[arg->ent_cnt - 1].deg, 10, utf8.val - '0')) log_message_error_xml(log, CODE_METRIC, context->metric, XML_OUT_OF_RANGE);
            else return 1;
        }
        break;
    }
    lmf_expr_arg_close(arg);
    return 0;
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

bool lmf_entry_cmp(const void *A, const void *B, void *Context)
{
    struct lmf_reg_context *context = Context;
    struct lmf_reg_entry *a = A, *b = B;
    size_t i = 0, j;
    for (; i < context->deg_cnt && a->deg[i] > b->deg[i]; i++);
    for (j = i; j < context->deg_cnt && a->deg[i] >= b->deg[i]; j++);
    if (j == context->deg_cnt) b->mark = 1;
    return i > 0;
}

struct lmf_entry_ask {
    size_t *deg;
    uint8_t *cat;
};

bool tensor_prod(size_t cnt, double **vect, size_t *len, double *res, size_t *ind)
{
    if (!cnt) return 0;
    memset(ind, 0, cnt * sizeof(*ind));
    for (size_t i = 0;; i++)
    {
        double prod = vect[0][ind[0]];
        for (size_t j = 1; j < cnt; j++) prod *= vect[j][ind[j]];
        res[i] = prod;
        
        size_t j = cnt;
        while (j-- && ++ind[j] == len[j]) ind[j] = 0;
        if (!j) break;
    }
}

bool lmf_expr_test(const char *expr, struct log *log)
{
    struct buff buff = { 0 };
    struct text_metric metric = { .path = STRI(__FILE__) };
    struct base_context context = { .buff = &buff };
    struct lmf_expr_arg arg = { 0 };
    size_t len = strlen(expr);
    for (size_t i = 0; i < len; i++)
    {
        if (lmf_expr_impl(&arg, &context, (struct utf8) { .len = 1, .val = expr[i], .chr = { expr[i] } }, metric, log)) continue;
        log_message_fmt(log, CODE_METRIC, MESSAGE_INFO, "Malformed expression!\n");
        free(buff.str);
        return 0;
    }
    
    size_t blk0 = arg.pool.tbl.cnt, blk = blk0 + SIZE_CNT(blk0);
    
    size_t *data = NULL;
    array_init(&data, NULL, arg.len_cnt, sizeof(*data) * blk, 0, ARRAY_STRICT | ARRAY_CLEAR);

    for (size_t i = 0, j = 0, k = 0; i < arg.len_cnt; k += arg.len[i++])
    {
        for (; j < k + arg.len[i]; j++)
        {
            size_t ind;
            str_pool_ord(&arg.pool, arg.ent[j].off, &ind);
            size_t deg = arg.ent[j].deg;
            if (deg) data[i * blk + ind - 1] = deg;
            else size_bit_set(data + i * blk + blk0, ind - 1);
        }
    }

    uint8_t *bits = NULL;
    array_init(&bits, NULL, UINT8_CNT(arg.len_cnt), sizeof(*bits), 0, ARRAY_STRICT | ARRAY_CLEAR);

    for (size_t i = 0; i < arg.len_cnt; i++)
    {
        if (uint8_bit_test(bits, i)) continue;
        for (size_t j = i + 1; j < arg.len_cnt; j++)
        {
            if (uint8_bit_test(bits, j)) continue;
            size_t k = 0, gr = 0, le = 0, eq = 0;
            for (; k < blk0; k++) 
            {
                size_t a = data[i * blk + k], b = data[j * blk + k];
                bool g = (a >= b), l = (a <= b);
                gr += g;
                le += l;
                eq += g && l;
                if (gr - eq && le - eq) break;
            }
            if (k < blk0) continue;
            for (; k < blk; k++)
            {
                size_t a = data[i * blk + k], b = data[j * blk + k];
                bool g = (size_t) (a | ~b) == SIZE_MAX, l = (size_t) (~a | b) == SIZE_MAX;
                if (!(g || l)) break;
                gr += g;
                le += l;
                eq += g && l;
                if (gr - eq && le - eq) break;
            }
            if (k < blk) continue;
            if (le == blk || eq == blk)
            {
                uint8_bit_set(bits, i);
                break;
            }
            if (gr == blk) uint8_bit_set(bits, j);
        }
    }

    size_t phen_cnt = 7;
    double phen[] = { 10, 2, 4, 5, 8, 4, 1 };
    double *cov[] = {
        (double[]) { 1, 4, 7, 9, 1, 5, 6 },
        (double[]) { 8, 3, 1, 2, 6, 9, 5 },
        (double[]) { 1, 4, 7, 1, 1, 5, 6 },
        (double[]) { 1, 9, 7, 6, 1, 5, 6 }
    };

    size_t snp_cnt = 10;
    uint8_t *gen[] = {
        "\x0\x1\x0\x0\x1\x0\x1",
        "\x1\x1\x3\x0\x1\x1\x0",
        "\x1\x1\x0\x0\x0\x3\x2",
        "\x0\x0\x2\x2\x1\x1\x0",
        "\x0\x1\x1\x0\x0\x2\x2",
        "\x3\x0\x3\x0\x1\x2\x1",
        "\x2\x1\x0\x3\x2\x1\x0",
        "\x0\x3\x0\x0\x3\x1\x3",
        "\x2\x1\x0\x2\x0\x2\x0",
        "\x0\x0\x3\x0\x1\x1\x0"
    };

    for (size_t i = 0; i < arg.len_cnt; i++) if (uint8_bit_test(bits, i))
    {

    }



    free(data);
    free(bits);
    lmf_expr_arg_close(&arg);    
    return 1;
}