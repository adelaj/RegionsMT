#include "np.h"
#include "la.h"
#include "ll.h"
#include "lm.h"
#include "memory.h"
#include "object.h"
#include "sort.h"
#include "strproc.h"
#include "utf8.h"
#include "intdom.h"
#include "categorical.h"
#include "tblproc.h"

#include "module/categorical.h"

#include <string.h>

#include "gslsupp.h"
#include <gsl/gsl_cdf.h>
#include <gsl/gsl_blas.h>

enum cov_sort {
    COV_SORT_FLAG_STR = 1, // Covariate is not numeric, if not set other flags are ignored
    COV_SORT_FLAG_NAT = 2, // Use natural order instead of lexicographical
    COV_SORT_FLAG_DSC = 4, // Use descending order
    COV_SORT_FLAG_CAT = 8 // Covariate is categorical, otherwise ordinal
};

#define COV_SORT_CNT 5
#define COV_SORT_IND(FLAG) ((FLAG) & COV_SORT_FLAG_STR ? 1 + (((FLAG) & (COV_SORT_FLAG_NAT | COV_SORT_FLAG_DSC)) >> 1) : 0)

#define LM_TOL 1e-9

struct lm_term {
    size_t off, deg;
    enum cov_sort sort;
};

struct lm_supp {
    size_t *filter;
    double *obs, *reg, *par;
    gsl_multifit_linear_workspace *workspace;
};

void lm_close(struct lm_supp *supp)
{
    free(supp->filter);
    free(supp->obs);
    free(supp->reg);
    free(supp->par);
    //free(supp->cov);
    gsl_multifit_linear_free(supp->workspace);
}

bool lm_init(struct lm_supp *supp, size_t phen_cnt, size_t reg_cnt, enum mt_flags flags)
{
    if (!flags)
    {
        *supp = (struct lm_supp) { NULL };
        return 1;
    }

    size_t hi, dphen_cnt = size_shl(&hi, phen_cnt, flags & TEST_A);
    if (hi) return 0;
    reg_cnt = size_add(&hi, reg_cnt, (size_t) (flags & TEST_CD) + 2);
    if (hi) return 0;

    supp->workspace = gsl_multifit_linear_alloc(dphen_cnt, reg_cnt);

    if (supp->workspace &&
        array_init(&supp->par, NULL, reg_cnt, sizeof(*supp->par), 0, ARRAY_STRICT).status &&
        matrix_init(&supp->reg, NULL, reg_cnt, dphen_cnt, sizeof(*supp->reg), 0, 0, ARRAY_STRICT).status &&
        array_init(&supp->obs, NULL, dphen_cnt, sizeof(*supp->obs), 0, ARRAY_STRICT).status &&
        array_init(&supp->filter, NULL, phen_cnt, sizeof(*supp->filter), 0, ARRAY_STRICT | ARRAY_FAILSAFE).status) return 1;

    lm_close(supp);
    return 0;
}

struct lm_expr {
    struct lm_term *term;
    size_t *len, term_cnt, len_cnt;
};

static size_t filter_init(size_t *filter, uint8_t *gen, double *phen, size_t phen_cnt)
{
    size_t cnt = 0;
    for (size_t i = 0; i < phen_cnt; i++) if (gen[i] < GEN_CNT && !isnan(phen[i])) filter[cnt++] = i;
    return cnt;
}

struct mt_result lm_impl(struct lm_supp *supp, uint8_t *gen, double *reg, double *phen, size_t phen_cnt, size_t reg_cnt, size_t reg_tda, enum mt_flags flags)
{
    struct mt_result res;
    array_broadcast(res.pv, countof(res.pv), sizeof(*res.pv), &(double) { nan(__func__) });
    array_broadcast(res.qas, countof(res.qas), sizeof(*res.qas), &(double) { nan(__func__) });

    if (!flags) return res;

    // Initializing genotype/phenotype filter
    size_t cnt = filter_init(supp->filter, gen, phen, phen_cnt);
    if (!cnt) return res;

    // If codominant test is not scheduled, 'off' is set to 1.
    bool test_cd = flags & TEST_CD;
    size_t off = (size_t) test_cd + 1, tda = reg_cnt + off + 1;

    //
    // BASELINE MODEL
    //
    size_t rk;
    double s_h, tol = LM_TOL, chisq;

    // TODO: Remove braces
    {
        size_t dimx = cnt, dimy = 1 + reg_cnt;
        
        // Phenotypes and covariates initialization
        for (size_t j = 0, k = off; j < dimx; j++, k += tda)
        {
            supp->obs[j] = phen[supp->filter[j]];
            supp->reg[k] = 1.;
            memcpy(supp->reg + k + 1, reg + reg_tda * supp->filter[j], reg_cnt * sizeof(*supp->reg));
        }
        if (flags & TEST_A)
        {
            memcpy(supp->obs + dimx, supp->obs, dimx * sizeof(*supp->obs));
            memcpy(supp->reg + dimx * tda + off, supp->reg + off, (dimx * tda - off) * sizeof(*supp->reg));
        }

        // Fit 'BASELINE' model
        gsl_matrix_view R = gsl_matrix_view_array_with_tda(supp->reg + off, dimx, dimy, tda); // C = gsl_matrix_view_array(supp->cov, dimy, dimy);
        gsl_vector_view O = gsl_vector_view_array(supp->obs, dimx), P = gsl_vector_view_array(supp->par, dimy);
                
        multifit_linear_tsvd(&R.matrix, &O.vector, tol, &P.vector, &s_h, &rk, supp->workspace);
    }

    // Performing computations for each alternative
    for (enum mt_alt i = 0; i < ALT_CNT; i++, flags >>= 1) if (flags & 1)
    {
        //
        // MODEL FOR TEST
        //
        double ss_e;
        size_t df_e;
        uint8_t bits = 0;
        {
            size_t dimx = cnt << (i == ALT_A), dimy = reg_cnt + (i == ALT_CD) + 2;
            if (dimy > dimx) continue;
            
            // Genotypes
            switch (i)
            {
            case ALT_CD: // codominant
                for (size_t j = 0, k = 0; j < dimx; j++, k += tda)
                {
                    uint8_t g = gen[supp->filter[j]];
                    bits |= 1 << g;
                    supp->reg[k] = (double) (g == 1);
                    supp->reg[k + 1] = (double) (g == 2);
                }
                switch (dimx) // Checking that genotype (with intercept) submatrix has the full rank
                {
                case 0:
                    continue;
                case 1:
                    break;
                case 2:
                    if (bits == (1 | 2) || bits == (1 | 4) || bits == (2 | 4)) break;
                    continue;
                default:
                    if (bits == (1 | 2 | 4)) break;
                    continue;
                }
                break;
            case ALT_R: // recessive
                for (size_t j = 0, k = test_cd; j < dimx; j++, k += tda)
                {
                    uint8_t g = gen[supp->filter[j]];
                    bits |= 1 << g;
                    supp->reg[k] = (double) (g == 2);
                }
                switch (dimx)
                {
                case 0:
                    continue;
                case 1:
                    break;
                default: // 101 or 111
                    if (bits == (1 | 4) || bits == (1 | 2 | 4)) break;
                    continue;
                }
                break;
            case ALT_D: // dominant
                for (size_t j = 0, k = test_cd; j < dimx; j++, k += tda)
                {
                    uint8_t g = gen[supp->filter[j]];
                    bits |= 1 << g;
                    supp->reg[k] = (double) ((g == 1) || (g == 2));
                }
                switch (dimx)
                {
                case 0:
                    continue;
                case 1:
                    break;
                default: // 011 or 101 or 111
                    if (bits == (1 | 2) || bits == (1 | 4) || bits == (1 | 2 | 4)) break;
                    continue;
                }
                break;
            case ALT_A: // allelic
                for (size_t j = 0, k = test_cd; j < cnt; j++, k += tda)
                {
                    uint8_t g = gen[supp->filter[j]];
                    bits |= 1 << g;
                    supp->reg[k] = (double) (g == 2);
                    supp->reg[k + cnt * tda] = (double) ((g == 1) || (g == 2));
                }
                switch (dimx)
                {
                case 0:
                    continue;
                default: // 010 or 110 or 011 or 111 or 101
                    if ((bits & 2) || bits == (1 | 4)) break;
                    continue;
                }
                break;
            default:
                continue;
            }
            
            // Fit model for 'TEST'
            
            gsl_matrix_view R = gsl_matrix_view_array_with_tda(supp->reg + test_cd - (i == ALT_CD), dimx, dimy, tda); // C = gsl_matrix_view_array(supp->cov, dimy, dimy);
            gsl_vector_view O = gsl_vector_view_array(supp->obs, dimx), P = gsl_vector_view_array(supp->par, dimy);

            multifit_linear_tsvd(&R.matrix, &O.vector, tol, &P.vector, &ss_e, &df_e, supp->workspace);
            df_e = dimx - df_e;
        }

        size_t df_h = (size_t) (i == ALT_CD) + 1;
        res.pv[i] = gsl_cdf_fdist_Q((((i == ALT_A ? s_h + s_h : s_h) - ss_e) * (double) df_e) / (ss_e * (double) df_h), (double) df_h, (double) df_e);

        //
        // MODEL FOR QAS
        //
        if (i == ALT_CD)
        {
            size_t dimx = cnt, dimy = 2 + reg_cnt;
            if (dimy > dimx) continue;

            for (size_t j = 0, k = test_cd; j < dimx; j++, k += tda)
                supp->reg[k] = (double) gen[supp->filter[j]];
            // Warning! No bit check is required

            // Fit model for 'QAS'
            
            gsl_matrix_view R = gsl_matrix_view_array_with_tda(supp->reg + test_cd, dimx, dimy, tda); // C = gsl_matrix_view_array(supp->cov, dimy, dimy);
            gsl_vector_view O = gsl_vector_view_array(supp->obs, dimx), P = gsl_vector_view_array(supp->par, dimy);

            multifit_linear_tsvd(&R.matrix, &O.vector, tol, &P.vector, &chisq, &rk, supp->workspace);
        }
        res.qas[i] = supp->par[0]; // Parameter corresponding to genotype
    }
    return res;
}

// 'cov_cnt' is the number of used covariates wrt their types 
struct lm_expr_arg {
    struct str_pool pool;
    struct lm_term *term;
    size_t *term_len, term_cnt, term_cap, term_len_cnt, term_len_cap;
};

enum {
    LM_EXPR_INIT = 0,
    LM_EXPR_TYPE,
    LM_EXPR_VAR_INIT,
    LM_EXPR_VAR,
    LM_EXPR_OP,
    LM_EXPR_TYPE_INIT,
    LM_EXPR_NUM_INIT,
    LM_EXPR_NUM,
    LM_EXPR_FLAG_INIT,
    LM_EXPR_FLAG,
    LM_EXPR_LP
};

void lm_expr_arg_close(void *Arg)
{
    struct lm_expr_arg *arg = Arg;
    free(arg->term_len);
    free(arg->term);
    str_pool_close(&arg->pool);
}

// Example: 'numeric(a) ^ 2 * categorical(b) + categorical(c) * categorical(b)'
bool lm_expr_impl(void *Arg, void *Context, struct utf8 utf8, struct text_metric metric, struct log *log)
{
    struct lm_expr_arg *restrict arg = Arg;
    struct base_context *restrict context = Context;
    switch (context->st)
    {
    case LM_EXPR_INIT:
        if (!utf8.val) return 1;
        if (utf8_is_whitespace_len(utf8.val, utf8.len)) return 1; // Removing leading whitespaces        
        if (array_assert(log, CODE_METRIC, array_init(&arg->term_len, &arg->term_len_cap, 1, sizeof(*arg->term_len), 0, ARRAY_CLEAR)) &&
            array_assert(log, CODE_METRIC, array_init(&arg->term, &arg->term_cap, 1, sizeof(*arg->term), 0, ARRAY_CLEAR)) &&
            array_assert(log, CODE_METRIC, str_pool_init(&arg->pool, 0, 0, 0)))
        {
            arg->term_len[arg->term_len_cnt++]++;
            arg->term_cnt++;
            //if (strchr("^:+()", utf8.val)) log_message_error_xml_chr(log, CODE_METRIC, metric, XML_UNEXPECTED_CHAR, utf8.byte, utf8.len);
            //else 
            if (array_assert(log, CODE_METRIC, buff_append(context->buff, utf8.chr, utf8.len, BUFFER_DISCARD)))
            {
                context->metric = metric;
                context->len = utf8.len;
                context->st++;
                return 1;
            }
        }
        break;
    case LM_EXPR_TYPE:
        if (!utf8.val) break;
        //if (strchr("^:+)", utf8.val)) log_message_error_xml_chr(log, CODE_METRIC, metric, XML_UNEXPECTED_CHAR, utf8.byte, utf8.len);
        //else 
        if (utf8.val == '(' || utf8.val == '<')
        {
            size_t ind = 0;
            static const struct strl type[] = { STRI("categorical"), STRI("ordinal"), STRI("numeric") };
            for (; ind < countof(type) && (type[ind].len != context->len || Strnicmp(type[ind].str, context->buff->str, type[ind].len)); ind++);
            if (ind == countof(type)) log_message_xml_generic(log, CODE_METRIC, MESSAGE_ERROR, context->metric, "Unexpected covariate type %~s*", context->buff->str, context->len);
            else
            {
                arg->term[arg->term_cnt - 1].sort = (enum cov_sort []) { COV_SORT_FLAG_STR | COV_SORT_FLAG_CAT, COV_SORT_FLAG_STR, 0 }[ind];
                if (utf8.val == '(') context->st++; 
                else context->st = LM_EXPR_FLAG;
                return 1;
            }
        }
        else if (array_assert(log, CODE_METRIC, buff_append(context->buff, utf8.chr, utf8.len, 0)))
        {
            if (!utf8_is_whitespace_len(utf8.val, utf8.len)) context->len = context->buff->len;
            return 1;
        }
        break;
    case LM_EXPR_VAR_INIT:
        if (!utf8.val) break;
        if (utf8_is_whitespace_len(utf8.val, utf8.len)) return 1;
        //if (strchr("^:+(", utf8.val)) log_message_error_xml_chr(log, CODE_METRIC, metric, XML_UNEXPECTED_CHAR, utf8.byte, utf8.len);
        //else 
        if (utf8.val == ')') log_message_xml_generic(log, CODE_METRIC, MESSAGE_ERROR, metric, "Covariate name expected before %~c", ')');
        else if (array_assert(log, CODE_METRIC, buff_append(context->buff, utf8.chr, utf8.len, BUFFER_DISCARD)))
        {
            context->metric = metric;
            context->len = utf8.len;
            context->st++;
            return 1;
        }
        break;
    case LM_EXPR_VAR:
        if (!utf8.val) break;
        //if (strchr("^:+(", utf8.val)) log_message_error_xml_chr(log, CODE_METRIC, metric, XML_UNEXPECTED_CHAR, utf8.byte, utf8.len);
        //else 
        if (utf8.val == ')')
        {
            context->buff->len = context->len;
            if (array_assert(log, CODE_METRIC, buff_append(context->buff, 0, 0, BUFFER_TERM)) &&
                array_assert(log, CODE_METRIC, str_pool_insert(&arg->pool, context->buff->str, context->len, &arg->term[arg->term_cnt - 1].off, 0, NULL, NULL)))
            {
                context->st = LM_EXPR_OP;
                return 1;
            }
        }
        else if (array_assert(log, CODE_METRIC, buff_append(context->buff, utf8.chr, utf8.len, 0)))
        {
            if (!utf8_is_whitespace_len(utf8.val, utf8.len)) context->len = context->buff->len;
            return 1;
        }
        break;
    case LM_EXPR_OP:
        switch (utf8.val)
        {
        case '\0':
            arg->term[arg->term_cnt - 1].deg = 1;
            return 1;
        case '^':
            //if (!arg->term[arg->term_cnt - 1].deg) log_message_xml_generic(log, CODE_METRIC, MESSAGE_ERROR, metric, "Operation %~c cannot be applied to the covariate of categorical type", '^');
            context->st = LM_EXPR_NUM_INIT;
            return 1;            
        case '*':
            context->st = LM_EXPR_TYPE_INIT;
            arg->term[arg->term_cnt - 1].deg = 1;
            arg->term_len[arg->term_len_cnt - 1]++;
            return 1;
        case '+':
            arg->term[arg->term_cnt - 1].deg = 1;
            if (array_assert(log, CODE_METRIC, array_test(&arg->term_len, &arg->term_len_cap, sizeof(*arg->term_len), 0, ARRAY_CLEAR, arg->term_len_cnt, 1)))
            {
                context->st = LM_EXPR_TYPE_INIT;
                arg->term_len[arg->term_len_cnt] = arg->term_len[arg->term_len_cnt - 1] + 1;
                arg->term_len_cnt++;
                return 1;
            }
            break;
        default:
            if (utf8_is_whitespace_len(utf8.val, utf8.len)) return 1;
            log_message_error_xml_chr(log, CODE_METRIC, metric, XML_UNEXPECTED_CHAR, utf8.byte, utf8.len);
            break;
        }
        break;
    case LM_EXPR_TYPE_INIT:
        if (!utf8.val) break;
        if (utf8_is_whitespace_len(utf8.val, utf8.len)) return 1; // Removing leading whitespaces
        if (array_assert(log, CODE_METRIC, array_test(&arg->term, &arg->term_cap, sizeof(*arg->term), 0, ARRAY_CLEAR, arg->term_cnt, 1)))
        {
            //arg->len[arg->len_cnt - 1]++;
            arg->term_cnt++;
            //if (strchr("^*+()", utf8.val)) log_message_error_xml_chr(log, CODE_METRIC, metric, XML_UNEXPECTED_CHAR, utf8.byte, utf8.len);
            //else 
            if (array_assert(log, CODE_METRIC, buff_append(context->buff, utf8.chr, utf8.len, BUFFER_DISCARD)))
            {
                context->metric = metric;
                context->len = utf8.len;
                context->st = LM_EXPR_TYPE;
                return 1;
            }
        }
        break;
    case LM_EXPR_NUM_INIT:
        if (!utf8.val) break;
        if (utf8_is_whitespace_len(utf8.val, utf8.len)) return 1;
        if (utf8.val < '0' || utf8.val > '9') log_message_error_xml_chr(log, CODE_METRIC, metric, XML_UNEXPECTED_CHAR, utf8.byte, utf8.len);
        else
        {
            context->metric = metric;
            context->val.uz = (size_t) (utf8.val - '0');
            context->st++;
            context->len = 0;
            return 1;
        }
        break;
    case LM_EXPR_NUM:
        switch (utf8.val)
        {
        case '\0':
            arg->term[arg->term_cnt - 1].deg = context->val.uz;
            return 1;
        case '*':
            context->st = LM_EXPR_TYPE_INIT;
            arg->term[arg->term_cnt - 1].deg = context->val.uz;
            arg->term_len[arg->term_len_cnt - 1]++;
            return 1;
        case '+':
            arg->term[arg->term_cnt - 1].deg = context->val.uz;
            if (array_assert(log, CODE_METRIC, array_test(&arg->term_len, &arg->term_len_cap, sizeof(*arg->term_len), 0, ARRAY_CLEAR, arg->term_len_cnt, 1)))
            {
                context->st = LM_EXPR_TYPE_INIT;
                arg->term_len[arg->term_len_cnt] = arg->term_len[arg->term_len_cnt - 1] + 1;
                arg->term_len_cnt++;
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
            else if (!size_mul_add_test(&context->val.uz, 10, (size_t) (utf8.val - '0'))) log_message_error_xml(log, CODE_METRIC, context->metric, XML_OUT_OF_RANGE);
            else return 1;
        }
        break;
    case LM_EXPR_FLAG_INIT:
        if (!utf8.val) break;
        if (utf8_is_whitespace_len(utf8.val, utf8.len)) return 1;
        if (array_assert(log, CODE_METRIC, buff_append(context->buff, utf8.chr, utf8.len, BUFFER_DISCARD)))
        {
            context->metric = metric;
            context->len = utf8.len;
            context->st++;
            return 1;
        }
        break;
    case LM_EXPR_FLAG:
        if (!utf8.val) break;
        if (utf8.val == '>' || utf8.val == '|')
        {
            size_t ind = 0;
            static const struct strl type[] = { STRI("descending"), STRI("natural") };
            for (; ind < countof(type) && (type[ind].len != context->len || Strnicmp(type[ind].str, context->buff->str, type[ind].len)); ind++);
            if (ind == countof(type)) log_message_xml_generic(log, CODE_METRIC, MESSAGE_ERROR, context->metric, "Unexpected covariate flag %~s*", context->buff->str, context->len);
            else
            {
                arg->term[arg->term_cnt - 1].sort |= (enum cov_sort[]) { COV_SORT_FLAG_DSC, COV_SORT_FLAG_NAT }[ind];
                if (utf8.val == '>') context->st++;
                else context->st = LM_EXPR_FLAG_INIT;
                return 1;
            }
            break;
        }
        else if (array_assert(log, CODE_METRIC, buff_append(context->buff, utf8.chr, utf8.len, 0)))
        {
            if (!utf8_is_whitespace_len(utf8.val, utf8.len)) context->len = context->buff->len;
            return 1;
        }
        break;
    case LM_EXPR_LP:
        if (!utf8.val) break;
        if (utf8_is_whitespace_len(utf8.val, utf8.len)) return 1;
        if (utf8.val == '(')
        {
            context->st = LM_EXPR_VAR_INIT;
            return 1;
        }
        break;
    }
    lm_expr_arg_close(arg);
    return 0;
}

struct cov {
    struct str_pool pool;
    struct buff buff;
    size_t *off, cnt, dim;
    void **ord; // (covariate index) * COV_SORT_CNT + sort_index -> array
    size_t *level; // (covariate index) * COV_SORT_CNT + sort_index -> number
};

struct tbl_cov_context {
    struct cov *cov;
    size_t cnt, cap;
};

static bool cov_name_handler(const char *str, size_t len, void *res, void *Context)
{
    (void) res;
    struct cov *cov = Context;
    /*if (!len)
    {
        //cov->cnt++;
        //return 1;
    }*/   
    size_t *p_ind, swp;
    unsigned r = str_pool_insert(&cov->pool, str, len, NULL, sizeof(size_t), &p_ind, &swp).status;
    if (!r) return 0;
    // if (r & HASH_PRESENT) return 0;
    *p_ind = cov->cnt++; // Just override column index with new one
    return 1;
}

static bool cov_handler(const char *str, size_t len, void *res, void *Context)
{
    struct cov *cov = Context;
    size_t off = cov->buff.len + !!cov->buff.len;
    if (!buff_append(&cov->buff, str, len, BUFFER_INIT | BUFFER_TERM).status) return 0;
    *(size_t *) res = off;
    return 1;
}

static bool tbl_cov_selector(struct tbl_col *cl, size_t row, size_t col, void *tbl, void *Context)
{
    (void) row;
    (void) col;
    (void) tbl;
    struct tbl_cov_context *context = Context;
    if (!row) 
        *cl = (struct tbl_col) { .handler = { .read = cov_name_handler }, .context = context->cov };
    else
    {
        if (!array_test(&context->cov->off, &context->cap, sizeof(*context->cov->off), 0, 0, context->cnt, 1).status) return 0;
        *cl = (struct tbl_col){ .handler = {.read = cov_handler }, .ptr = context->cov->off + context->cnt, .context = context->cov };
        context->cnt++;
    }
    return 1;
}

bool cov_init(struct cov *cov, const char *path, struct log *log)
{
    cov->cnt = 0;
    cov->off = NULL;
    cov->level = NULL;
    cov->buff = (struct buff) { 0 };
        
    // Reading the head of the file
    size_t skip = 0, cnt = 0, length = 0;
    struct tbl_cov_context context = { .cov = cov, .cap = 0 };
    
    if (str_pool_init(&cov->pool, 200, 0, sizeof(size_t)).status)
    {
        if (tbl_read(path, 0, tbl_cov_selector, NULL, &context, NULL, &skip, &cnt, &length, ',', log))
        {            
            cov->dim = cnt - 1;
            if (array_init(&cov->ord, NULL, cov->cnt, COV_SORT_CNT * sizeof(*cov->ord), 0, ARRAY_CLEAR).status &&
                array_init(&cov->level, NULL, cov->cnt, COV_SORT_CNT * sizeof(*cov->level), 0, 0).status) return 1;
            free(cov->level);
            free(cov->ord);
        }
        str_pool_close(&cov->pool);
    }
    return 0;
}

void cov_close(struct cov *cov)
{
    str_pool_close(&cov->pool);
    free(cov->buff.str);
    free(cov->level);
    for (size_t i = 0; i < COV_SORT_CNT * cov->cnt; free(cov->ord[i++]));
    free(cov->ord);
    free(cov->off);
}

struct term_cmp_thunk {
    size_t blk0, blk;
};

static bool term_cmp(const void *A, const void *B, void *Thunk)
{
    struct term_cmp_thunk *thunk = Thunk;
    const size_t *a = A, *b = B;
    int cmp = 0;
    size_t i = 0;
    for (; i < thunk->blk0 && !cmp; cmp = size_sign(a[i], b[i]), i++);
    for (; i < thunk->blk && !cmp; cmp = size_sign(size_bit_scan_forward(~a[i] & b[i]), size_bit_scan_forward(a[i] & ~b[i])), i++);
    return cmp > 0;
}

bool cov_querry(struct cov *cov, struct buff *buff, struct lm_term *term, size_t cnt, struct log *log)
{
    for (size_t i = 0; i < cnt; i++)
    {
        size_t *p_ind;
        const char *name = buff->str + term[i].off;
        if (!str_pool_fetch(&cov->pool, name, sizeof(size_t), &p_ind)) log_message_fmt(log, CODE_METRIC, MESSAGE_ERROR, "Covariate %~s is unavailable!\n", name);
        else
        {
            size_t ind = *p_ind;
            size_t off = ind * COV_SORT_CNT + COV_SORT_IND(term[i].sort);
            term[i].off = off;
            void **ptr = cov->ord + off;
            size_t *level = cov->level + off;
            if (*ptr) continue;
            if (term[i].sort & COV_SORT_FLAG_STR)
            {
                *level = cov->dim;
                if (array_assert(log, CODE_METRIC, ranks_unique((size_t **) ptr, cov->off + ind * cov->dim, level, sizeof(*cov->off), str_off_cmp, cov->buff.str))) continue;
            }
            else if (array_assert(log, CODE_METRIC, array_init(ptr, NULL, cov->dim, sizeof(double), 0, ARRAY_STRICT)))
            {
                for (size_t j = 0; j < cov->dim; j++)
                {
                    double res;
                    const char *str = cov->buff.str + cov->off[j + cov->dim * ind];
                    if (!flt64_handler(str, 0, &res, NULL)) log_message_fmt(log, CODE_METRIC, MESSAGE_ERROR, "Unable to interpret string %~s as decimal number!\n", str);
                    else
                    {
                        (*(double **) ptr)[j] = res;
                        continue;
                    }
                    free(*ptr);
                    *ptr = NULL;
                    return 0;
                }
                *level = 0;
                continue;
            }                    
        }
        return 0;
    }
    return 1;
}

static bool lm_term_cnt(size_t *data, size_t blk0, size_t blk, size_t *p_cnt)
{
    size_t cnt = 1, i = 1;
    if (blk0)
    {
        if (data[0]) cnt = data[0];
        for (; i < blk0; i++)
        {
            if (data[i] == SIZE_MAX) return 0;
            if (!data[i]) continue;
            size_t hi;
            cnt = size_mul(&hi, cnt, data[i] + 1);
            if (hi) return 0;
        }
    }
    else if (blk)
    {
        if (data[0] == SIZE_MAX) return 0;
        size_t m = size_pop_cnt(data[0]);
        if (m) cnt = (size_t) 1 << m;
    }
    for (; i < blk; i++)
    {
        if (data[i] == SIZE_MAX) return 0;
        size_t m = size_pop_cnt(data[i]);
        if (!m) continue;
        size_t hi;
        cnt = size_mul(&hi, cnt, (size_t) 1 << m);
        if (hi) return 0;
    }
    *p_cnt = cnt - 1;
    return 1;
}

static bool lm_cnt(size_t *data, size_t blk0, size_t blk, size_t *p_cnt)
{
    size_t cnt = *p_cnt;
    if (!cnt) return 1;
    size_t ecnt;
    if (!lm_term_cnt(data, blk0, blk, &ecnt)) return 0;
    for (size_t i = 1, j = blk; i < cnt; i++, j += blk)
    {
        size_t car, tmp = ecnt;
        if (!lm_term_cnt(data + j, blk0, blk, &ecnt)) return 0;
        ecnt = size_add(&car, ecnt, tmp);
        if (car) return 0;
    }
    *p_cnt = ecnt;
    return 1;
}

static void lm_term_expand(size_t *data, size_t blk0, size_t blk, size_t *edata, size_t *ind, size_t *p_off)
{
    size_t off = *p_off;
    for (;;)
    {
        size_t i = 0;
        for (; i < blk0 && ind[i] == data[i]; i++) if (ind[i]) ind[i] = 0;
        if (i < blk0) ind[i]++;
        else
        {
            for (; i < blk && ind[i] == data[i]; i++) if (ind[i]) ind[i] = 0;
            if (i == blk) break;
            size_t k = 1;
            for (; k && (ind[i] & k) == (data[i] & k); k <<= 1) if (ind[i] & k) ind[i] &= (size_t) ~k;
            ind[i] |= k;
        } 
        memcpy(edata + off, ind, blk * sizeof(*edata));
        off += blk;
    }
    *p_off = off;
}

bool lm_expand(size_t **p_data, size_t *p_cnt, size_t blk0, size_t blk, struct log *log)
{
    size_t cnt = *p_cnt, ecnt = cnt, *data = *p_data;
    if (!lm_cnt(data, blk0, blk, &ecnt)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, ERANGE);
    else
    {
        size_t *ind = NULL, *edata = NULL;
        if (array_assert(log, CODE_METRIC, matrix_init(&edata, NULL, ecnt, blk, sizeof(*edata), 0, 0, ARRAY_STRICT)))
        {
            if (array_assert(log, CODE_METRIC, array_init(&ind, NULL, blk, sizeof(*ind), 0, ARRAY_STRICT | ARRAY_CLEAR)))
            {
                for (size_t i = 0, off = 0; i < cnt * blk; i += blk) lm_term_expand(data + i, blk0, blk, edata, ind, &off);
                free(data);
                size_t sz = blk * sizeof(*edata), ucnt = ecnt;
                sort_unique(edata, &ucnt, sz, term_cmp, &(struct term_cmp_thunk) { .blk0 = blk0, .blk = blk }, ind, sz);
                free(ind);
                array_test(&edata, &ecnt, sz, 0, ARRAY_REDUCE, ucnt);
                *p_data = edata;
                *p_cnt = ecnt;
                return 1;
            }
            free(edata);
        }
    }
    return 0;
}

bool lm_expr_test(const char *phen_name, const char *expr, const char *path_phen, const char *path_gen, const char *path_out, struct log *log)
{
    
    struct cov cov;
    if (!cov_init(&cov, path_phen, log)) return 0;
    
    struct buff buff = { 0 };
    struct text_metric metric = { .path = STRI(__FILE__) };
    struct base_context context = { .buff = &buff };
    struct lm_expr_arg arg = { 0 };
    size_t len = strlen(expr);
    for (size_t i = 0; i <= len; i++)
    {
        if (lm_expr_impl(&arg, &context, (struct utf8) { .len = 1, .val = expr[i], .chr = { expr[i] } }, metric, log)) continue;
        log_message_fmt(log, CODE_METRIC, MESSAGE_NOTE, "Malformed expression!\n");
        cov_close(&cov);
        free(buff.str);
        return 0;
    }
    free(buff.str);

    //size_t *p_ind;
    //str_pool_fetch(&cov.pool, "", sizeof(size_t), &p_ind);
    //for (size_t i = 0; i < cov.cnt * cov.dim; i++) cov.off[i] = i;

    // Warning! Table transposition is required
    size_t *off_trans;
    if (!array_init(&off_trans, NULL, cov.dim * cov.cnt, sizeof(*off_trans), 0, 0).status) return 0;
    for (size_t i = 0; i < cov.cnt; i++)
        for (size_t j = 0; j < cov.dim; j++)
            off_trans[i * cov.dim + j] = cov.off[i + cov.cnt * j];
    
    free(cov.off);
    cov.off = off_trans;
    
    if (!cov_querry(&cov, &arg.pool.buff, arg.term, arg.term_cnt, log)) return 0;

    uint8_t *gen = NULL;
    struct gen_context gen_context = { .phen_cnt = cov.dim };
    size_t gen_skip = 0, snp_cnt = 0, gen_length = 0;
    if (!tbl_read(path_gen, 0, tbl_gen_selector2, NULL, &gen_context, &gen, &gen_skip, &snp_cnt, &gen_length, ',', log)) return 0;

    size_t blk0 = cov.cnt * COV_SORT_CNT, blk = blk0 + SIZE_CNT(blk0);
    
    size_t *data = NULL;
    if (!array_init(&data, NULL, arg.term_len_cnt, sizeof(*data) * blk, 0, ARRAY_STRICT | ARRAY_CLEAR).status) return 0;
    
    for (size_t i = 0, j = 0; i < arg.term_len_cnt; i++)
    {
        for (; j < arg.term_len[i]; j++)
        {
            size_t deg = arg.term[j].deg;
            if (!deg) continue;
            //if (!par_term[i]) par_term[i] = 1;
            if (arg.term[j].sort & COV_SORT_FLAG_CAT)
            {
                if (cov.level[arg.term[j].off] > 1)
                {
                    size_bit_set(data + i * blk + blk0, arg.term[j].off);
                    //par_term[i] *= cov.level[arg.term[j].off] - 1;
                }
            }
            else data[i * blk + arg.term[j].off] += deg;
        }        
    }

    size_t cnt = arg.term_len_cnt;
    if (!lm_expand(&data, &cnt, blk0, blk, log)) return 0;

    size_t *par_term = NULL;
    if (!array_init(&par_term, NULL, cnt, sizeof(*data), 0, ARRAY_STRICT | ARRAY_CLEAR).status) return 0;    

    for (size_t i = 0; i < cnt; i++)
    {
        for (size_t j = 0; j < blk0; j++)
        {
            size_t deg = data[i * blk + j];
            if (!deg) continue;
            if (!par_term[i]) par_term[i] = 1;
        }
        for (size_t k = 0; k < blk0; k++)
        {
            if (!size_bit_test(data + i * blk + blk0, k)) continue;
            if (!par_term[i]) par_term[i] = cov.level[k] - 1;
            else par_term[i] *= cov.level[k] - 1;
        }
    }

    /*
    uint8_t *bits = NULL;
    if (!array_init(&bits, NULL, UINT8_CNT(arg.term_len_cnt), sizeof(*bits), 0, ARRAY_STRICT | ARRAY_CLEAR)) return 0;

    for (size_t i = 0; i < arg.term_len_cnt; i++)
    {
        if (uint8_bit_test(bits, i)) continue;
        for (size_t j = i + 1; j < arg.term_len_cnt; j++)
        {
            if (uint8_bit_test(bits, j)) continue;
            size_t k = 0, gr = 0, le = 0, eq = 0;
            for (; k < blk; k++) 
            {
                size_t a = data[i * blk + k], b = data[j * blk + k];
                bool g = (a >= b), l = (a <= b);
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
    
    size_t pos = 0, new_cnt = 0;
    for (size_t i = 0; i < arg.term_len_cnt; i++) if (!uint8_bit_test(bits, i))
    {
        if (i && pos < arg.term_len[i - 1])
            memmove(arg.term + pos, arg.term + arg.term_len[i - 1], sizeof(*arg.term) * arg.term_len[i]);
        pos += arg.term_len[i];
        if (new_cnt < i) arg.term_len[new_cnt] = arg.term_len[i];
        new_cnt++;
    }
    arg.term_len_cnt = new_cnt;
    
    size_t reg_cnt = 0, reg_vect_cnt = 0;
    for (size_t i = 0, j = 0; i < arg.term_len_cnt; i++)
    {
        size_t term_cnt = 1, zzz = 0;
        for (; j < arg.term_len[i]; j++)
        {
            size_t l = arg.term[j].deg ? arg.term[j].deg : cov.level[arg.term[j].off] - 1; // TODO: Handle the case of the empty table
            zzz += l;
            term_cnt *= l;
        }
        if (zzz > reg_vect_cnt) reg_vect_cnt = zzz;
        reg_cnt += term_cnt;
    }

    size_t max_len = arg.term_len[0];
    for (size_t i = 1; i < arg.term_len_cnt; i++)
    {
        size_t l = arg.term_len[i] - arg.term_len[i - 1];
        if (l > max_len) max_len = l;
    }
    */

    //uintptr_t *ord;
    //size_t ucnt = cnt;
    //if (!orders_stable_unique(&ord, data, &ucnt, blk * sizeof(*data), term_cmp, &(struct term_cmp_thunk) { .blk = blk, .blk0 = blk0 }).status) return 0;
        
    size_t dimy = 0;
    for (size_t i = 0; i < cnt; i++)
        dimy += par_term[i];
    
    double *reg;
    if (!array_init(&reg, NULL, dimy * cov.dim, sizeof(*reg), 0, ARRAY_STRICT).status) return 0;
    
    for (size_t x = 0, pos = 0; x < cov.dim; x++)
    {
        for (size_t i = 0; i < cnt; i++)
        {
            size_t off = 0, mul = 1, j = 0;
            for (; j < blk0; j++) if (size_bit_test(data + blk * i + blk0, j))
            {
                size_t val = ((size_t *) cov.ord[j])[x];
                if (!val) break;
                off += (val - 1) * mul;
                mul *= cov.level[j] - 1;
            }
            if (j < blk0)
            {
                array_broadcast(reg + pos, par_term[i], sizeof(*reg), &(double) { 0. });
                pos += par_term[i];
                continue;
            }
            double pr = 1.;
            for (j = 0; j < blk0; j++) if (data[j + blk * i])
            {
                double val = j % COV_SORT_CNT ? (double) ((size_t *) cov.ord[j])[x] : ((double *) cov.ord[j])[x];
                pr *= pow(val, (double) data[j + blk * i]);
            }
            array_broadcast(reg + pos, off, sizeof(*reg), &(double) { 0. });
            reg[pos + off] = pr;
            array_broadcast(reg + pos + off + 1, size_sub_sat(par_term[i], off + 1), sizeof(*reg), &(double) { 0. });
            pos += par_term[i];
        }
    }

    size_t rky = dimy;
    gauss_col(reg, cov.dim, &rky, dimy, LM_TOL);

    struct lm_supp supp;
    if (!lm_init(&supp, cov.dim, rky, 15)) return 0;

    struct lm_term phen = { .off = 0, .deg = 1, .sort = 0 };
    if (!cov_querry(&cov, &(struct buff) { .str = (char *) phen_name }, &phen, 1, log)) return 0;

    FILE *f = fopen(path_out, "w");
    if (!f)
    {
        log_message_fopen(log, CODE_METRIC, MESSAGE_ERROR, path_out, errno);
        return 0;
    }
    
    for (size_t i = 0; i < snp_cnt; i++)
    {
        uint64_t t = get_time();
        struct mt_result x = lm_impl(&supp, gen + i * cov.dim, reg, cov.ord[phen.off], cov.dim, rky, dimy, 15);
        print_cat(f, x);
        //fflush(f);
        log_message_fmt(log, CODE_METRIC, MESSAGE_INFO, "Computation of linear model for snp no. %~uz took %~T.\n", i + 1, t, get_time());
    }
    fclose(f);
    lm_close(&supp);

    free(reg);
    free(gen);
    free(data);
    free(par_term);
    cov_close(&cov);
    lm_expr_arg_close(&arg);    
    return 1;
}
