#include "np.h"
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

#include <gsl/gsl_multifit.h>
#include <gsl/gsl_cdf.h>

// 'deg == 0' corresponds to categorical covariate
// 'deg > 0' corresponds to numeric/ordinal covariate
// For covariate of numeric type the 'sort' should be set to 'COV_NUM' 
struct lm_term {
    size_t off, deg;
    enum cov_sort sort;
};

struct lm_supp {
    size_t *filter, *ind, *reg_vect_len;
    uint8_t *gen;
    double *obs, *reg, *reg_vect;
    gsl_multifit_linear_workspace *workspace;
};

void lm_close(struct lm_supp *supp)
{
    free(supp->filter);
    free(supp->gen);

}

bool lm_init(struct lm_supp *supp, size_t phen_cnt, size_t reg_vect_cnt, size_t reg_cnt, size_t term_max_len)
{
    if (array_init(&supp->reg_vect_len, NULL, term_max_len, sizeof(*supp->reg_vect_len), 0, ARRAY_STRICT) &&
        array_init(&supp->ind, NULL, term_max_len, sizeof(*supp->ind), 0, ARRAY_STRICT) &&
        array_init(&supp->reg_vect, NULL, reg_vect_cnt, sizeof(*supp->reg_vect), 0, ARRAY_STRICT) &&
        array_init(&supp->reg, NULL, (reg_cnt + 3) * phen_cnt, 2 * sizeof(*supp->reg), 0, ARRAY_STRICT) &&
        array_init(&supp->obs, NULL, phen_cnt, 2 * sizeof(*supp->obs), 0, ARRAY_STRICT) &&
        array_init(&supp->gen, NULL, phen_cnt, 2 * sizeof(*supp->gen), 0, ARRAY_STRICT) &&
        array_init(&supp->filter, NULL, phen_cnt, sizeof(*supp->filter), 0, ARRAY_STRICT | ARRAY_FAILSAFE) &&
        //array_init(&supp->outer, NULL, phen_ucnt, GEN_CNT * sizeof(*supp->outer), 0, ARRAY_STRICT) &&
        //array_init(&supp->tbl, NULL, phen_ucnt, 2 * GEN_CNT * sizeof(*supp->tbl), 0, ARRAY_STRICT)
        1
        ) return 1;

    lm_close(supp);
    return 1;
}

struct lm_expr {
    struct lm_term *term;
    size_t *len, term_cnt, len_cnt;
};

// Warning! 'cnt' should be non-zero and array 'vect_len' should be strictly ascending  
void tensor_prod(size_t cnt, double *restrict vect, size_t *restrict vect_len, double *restrict res, size_t *restrict ind)
{
    ind[0] = 0;
    for (size_t i = 1; i < cnt; i++) ind[i] = vect_len[i - 1];
    for (size_t i = 0;; i++)
    {
        double prod = vect[ind[0]];
        for (size_t j = 1; j < cnt; j++) prod *= vect[ind[j]];
        res[i] = prod;

        size_t j = cnt;
        for (; --j && ++ind[j] == vect_len[j]; ind[j] = vect_len[j - 1]);
        if (!j && ++ind[0] == vect_len[0]) break;
    }
}

/*
bool check_matrix_rank(double *matrix, size_t dimx, size_t dimy)
{
    for (size_t i = 0; i < dimx; i++)
    {

    }
}
*/

// 'phen' is actually a double pointer
struct categorical_res lm_impl(struct lm_supp *supp, uint8_t *gen, void **cov_ord, size_t phen_ind, size_t *cov_level, size_t phen_cnt, size_t reg_cnt, struct lm_term *term, size_t *term_len, size_t term_len_cnt, enum categorical_flags flags)
{
    struct categorical_res res;
    array_broadcast(res.nlpv, countof(res.nlpv), sizeof(*res.nlpv), &(double) { nan(__func__) });
    array_broadcast(res.qas, countof(res.qas), sizeof(*res.qas), &(double) { nan(__func__) });

    // Initializing genotype filter
    size_t cnt = filter_init(supp->filter, gen, phen_cnt);
    if (!cnt) return res;

    //
    // BASELINE MODEL
    //
    double chisq_bs;
    {
        size_t dimx = cnt, dimy = 1 + reg_cnt;
        // Setting up the matrix of regressors:
        // 0) Intercept parameter
        for (size_t j = 0, k = 0; j < dimx; j++, k += dimy)
            supp->reg[k] = 1.;

        // 1) Phenotypes
        size_t off = 1;
        for (size_t j = 0; j < dimx; j++)
            supp->obs[j + cnt] = supp->obs[j] = ((double *) cov_ord[phen_ind])[supp->filter[j]];
        
        // 3) Covariates
        for (size_t z = 0, zz = off, yy = 0; z < cnt; z++, zz += dimy, yy = 0)
        {
            for (size_t j = 0, k = 0, zzz = 0; j < term_len_cnt; j++)
            {
                size_t pos = 0, stride = 1;
                for (size_t l = 0; k < term_len[j]; k++, l++)
                {
                    size_t d = term[k].deg;
                    if (d)
                    {
                        double x0 = ((double *) cov_ord[term[k].off])[supp->filter[z]], x = x0;
                        stride *= d;
                        supp->reg_vect[pos++] = x;
                        for (size_t y = 1; y < d; y++)
                            supp->reg_vect[pos++] = x *= x0;
                    }
                    else
                    {
                        size_t r = ((size_t *) cov_ord[term[k].off])[supp->filter[z]];
                        stride *= cov_level[term[k].off] - 1;
                        for (size_t y = 1; y < cov_level[term[k].off]; y++)
                            supp->reg_vect[pos++] = y == r;
                    }
                    supp->reg_vect_len[l] = pos;
                }
                tensor_prod(term_len[j] - yy, supp->reg_vect, supp->reg_vect_len, supp->reg + zz + zzz, supp->ind);
                zzz += stride;
                yy = term_len[j];
            }
        }

        // 3) Fit BASELINE model
        gsl_vector *P = gsl_vector_alloc(dimy);
        gsl_matrix_view R = gsl_matrix_view_array(supp->reg, dimx, dimy);
        gsl_vector_view O = gsl_vector_view_array(supp->obs, dimx);
        gsl_multifit_linear_workspace *W = gsl_multifit_linear_alloc(dimx, dimy);
        gsl_matrix *C = gsl_matrix_alloc(dimy, dimy);

        size_t rk;
        gsl_multifit_linear_tsvd(&R.matrix, &O.vector, 1e-5, P, C, &chisq_bs, &rk, W);

        gsl_matrix_free(C);
        gsl_vector_free(P);
        gsl_multifit_linear_free(W);
    }

    // Performing computations for each alternative
    for (size_t i = 0; i < ALT_CNT; i++, flags >>= 1) if (flags & 1)
    {
        //
        // MODEL FOR TEST
        //
        double ss_e;
        size_t df_e;
        {
            size_t dimx = cnt << (i == 3), dimy0 = 2 + (i == 0), dimy = dimy0 + reg_cnt;
            // Setting up the matrix of regressors:
            // 0) Intercept parameter
            for (size_t j = 0, k = 0; j < dimx; j++, k += dimy)
                supp->reg[k] = 1.;

            // 1) Genotypes & phenotypes
            size_t off = 1;
            switch (i)
            {
            case 0: // codominant
                for (size_t j = 0, k = off; j < dimx; j++, k += dimy)
                {
                    uint8_t g = gen[supp->filter[j]];
                    supp->reg[k] = (double) (g == 1);
                    supp->reg[k + 1] = (double) (g == 2);
                    //supp->obs[j] = ((double *) cov_ord[phen_ind])[supp->filter[j]];
                }
                off += 2;
                break;
            case 1: // recessive
                for (size_t j = 0, k = off; j < dimx; j++, k += dimy)
                {
                    uint8_t g = gen[supp->filter[j]];
                    supp->reg[k] = (double) (g == 2);
                    //supp->obs[j] = ((double *) cov_ord[phen_ind])[supp->filter[j]];
                }
                off++;
                break;
            case 2: // dominant
                for (size_t j = 0, k = off; j < dimx; j++, k += dimy)
                {
                    uint8_t g = gen[supp->filter[j]];
                    supp->reg[k] = (double) ((g == 1) || (g == 2));
                    //supp->obs[j] = ((double *) cov_ord[phen_ind])[supp->filter[j]];
                }
                off++;
                break;
            default: // allelic
                for (size_t j = 0, k = off; j < cnt; j++, k += dimy)
                {
                    uint8_t g = gen[supp->filter[j]];
                    supp->reg[k] = (double) (g == 2);
                    supp->reg[k + cnt * dimy] = (double) ((g == 1) || (g == 2));
                    //supp->obs[j + cnt] = supp->obs[j] = ((double *) cov_ord[phen_ind])[supp->filter[j]];
                }
                off++;
            }

            // 2) Test that matrix of genotypes has full rank
            //gsl_matrix_view T = gsl_matrix_view_array_with_tda(supp->reg, dimx, dimy0, dimy);

            // 3) Covariates
            for (size_t z = 0, zz = off, yy = 0; z < cnt; z++, zz += dimy, yy = 0)
            {
                for (size_t j = 0, k = 0, zzz = 0; j < term_len_cnt; j++)
                {
                    size_t pos = 0, stride = 1;
                    for (size_t l = 0; k < term_len[j]; k++, l++)
                    {
                        size_t d = term[k].deg;
                        if (d)
                        {                            
                            double x0 = ((double *) cov_ord[term[k].off])[supp->filter[z]], x = x0;
                            stride *= d;
                            supp->reg_vect[pos++] = x;
                            for (size_t y = 1; y < d; y++)
                                supp->reg_vect[pos++] = x *= x0;
                        }
                        else
                        {
                            size_t r = ((size_t *) cov_ord[term[k].off])[supp->filter[z]];
                            stride *= cov_level[term[k].off] - 1;
                            for (size_t y = 1; y < cov_level[term[k].off]; y++)
                                supp->reg_vect[pos++] = y == r;
                        }
                        supp->reg_vect_len[l] = pos;                        
                    }
                    tensor_prod(term_len[j] - yy, supp->reg_vect, supp->reg_vect_len, supp->reg + zz + zzz, supp->ind);
                    zzz += stride;
                    yy = term_len[j];
                }
                if (i == 3)
                {
                    memcpy(supp->reg + zz + dimy * cnt, supp->reg + zz, sizeof(*supp->reg) * (dimy - dimy0)); // Allelic mode
                }
            }

            // 3) Fit model for 'TEST'
            gsl_vector *P = gsl_vector_alloc(dimy);
            gsl_matrix_view R = gsl_matrix_view_array(supp->reg, dimx, dimy);
            gsl_vector_view O = gsl_vector_view_array(supp->obs, dimx);
            gsl_multifit_linear_workspace *W = gsl_multifit_linear_alloc(dimx, dimy);
            gsl_matrix *C = gsl_matrix_alloc(dimy, dimy);

            gsl_multifit_linear_tsvd(&R.matrix, &O.vector, 1e-5, P, C, &ss_e, &df_e, W);
            df_e = dimx - df_e;

            gsl_matrix_free(C);
            gsl_vector_free(P);
            gsl_multifit_linear_free(W);            
        }
        size_t df_h = 1 + (i == 1);
        res.nlpv[i] = gsl_cdf_fdist_Q(((chisq_bs - ss_e) * (double) df_e) / (ss_e * (double) df_h), (double) df_h, (double) df_e);

        //
        // MODEL FOR QAS
        //

        double qas;
        {
            size_t dimx = cnt << (i == 3), dimy0 = 2, dimy = dimy0 + reg_cnt;
            // Setting up the matrix of regressors:
            // 0) Intercept parameter
            for (size_t j = 0, k = 0; j < dimx; j++, k += dimy)
                supp->reg[k] = 1.;

            // 1) Genotypes & phenotypes
            size_t off = 1;
            switch (i)
            {
            case 0: // codominant
                for (size_t j = 0, k = off; j < dimx; j++, k += dimy)
                {
                    uint8_t g = gen[supp->filter[j]];
                    supp->reg[k] = (double) g;
                    supp->obs[j] = ((double *) cov_ord[phen_ind])[supp->filter[j]];
                }
                off++;
                break;
            case 1: // recessive
                for (size_t j = 0, k = off; j < dimx; j++, k += dimy)
                {
                    uint8_t g = gen[supp->filter[j]];
                    supp->reg[k] = (double) (g == 2);
                    supp->obs[j] = ((double *) cov_ord[phen_ind])[supp->filter[j]];
                }
                off++;
                break;
            case 2: // dominant
                for (size_t j = 0, k = off; j < dimx; j++, k += dimy)
                {
                    uint8_t g = gen[supp->filter[j]];
                    supp->reg[k] = (double) ((g == 1) || (g == 2));
                    supp->obs[j] = ((double *) cov_ord[phen_ind])[supp->filter[j]];
                }
                off++;
                break;
            default: // allelic
                for (size_t j = 0, k = off; j < cnt; j++, k += dimy)
                {
                    uint8_t g = gen[supp->filter[j]];
                    supp->reg[k] = (double) (g == 2);
                    supp->reg[k + cnt * dimy] = (double) ((g == 1) || (g == 2));
                    supp->obs[j + cnt] = supp->obs[j] = ((double *) cov_ord[phen_ind])[supp->filter[j]];
                }
                off++;
            }

            // 2) Test that matrix of genotypes has full rank
            //gsl_matrix_view T = gsl_matrix_view_array_with_tda(supp->reg, dimx, dimy0, dimy);

            // 3) Covariates
            for (size_t z = 0, zz = off, yy = 0; z < cnt; z++, zz += dimy, yy = 0)
            {
                for (size_t j = 0, k = 0, zzz = 0; j < term_len_cnt; j++)
                {
                    size_t pos = 0, stride = 1;
                    for (size_t l = 0; k < term_len[j]; k++, l++)
                    {
                        size_t d = term[k].deg;
                        if (d)
                        {
                            double x0 = ((double *) cov_ord[term[k].off])[supp->filter[z]], x = x0;
                            stride *= d;
                            supp->reg_vect[pos++] = x;
                            for (size_t y = 1; y < d; y++)
                                supp->reg_vect[pos++] = x *= x0;
                        }
                        else
                        {
                            size_t r = ((size_t *) cov_ord[term[k].off])[supp->filter[z]];
                            stride *= cov_level[term[k].off] - 1;
                            for (size_t y = 1; y < cov_level[term[k].off]; y++)
                                supp->reg_vect[pos++] = y == r;
                        }
                        supp->reg_vect_len[l] = pos;
                    }
                    tensor_prod(term_len[j] - yy, supp->reg_vect, supp->reg_vect_len, supp->reg + zz + zzz, supp->ind);
                    zzz += stride;
                    yy = term_len[j];
                }
                if (i == 3) memcpy(supp->reg + zz + dimy * cnt, supp->reg + zz, sizeof(*supp->reg) * (dimy - dimy0)); // Allelic mode 
            }

            // 3) Fit model for 'TEST'
            gsl_vector *P = gsl_vector_alloc(dimy);
            gsl_matrix_view R = gsl_matrix_view_array(supp->reg, dimx, dimy);
            gsl_vector_view O = gsl_vector_view_array(supp->obs, dimx);
            gsl_multifit_linear_workspace *W = gsl_multifit_linear_alloc(dimx, dimy);
            gsl_matrix *C = gsl_matrix_alloc(dimy, dimy);

            double chisq;
            size_t rk;
            gsl_multifit_linear_tsvd(&R.matrix, &O.vector, 1e-5, P, C, &chisq, &rk, W);
            qas = gsl_vector_get(P, 1);

            gsl_matrix_free(C);
            gsl_vector_free(P);
            gsl_multifit_linear_free(W);
        }
        res.qas[i] = qas;
    }
    return res;
}

enum cov_sort {
    COV_SORT_NUM = 0,
    COV_SORT_STRCMP_ASC,
    COV_SORT_STRCMP_DSC,
    COV_SORT_STRCMPNAT_ASC,
    COV_SORT_STRCMPNAT_DSC,
    COV_SORT_CNT
};

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
        if (!array_init(&arg->term_len, &arg->term_len_cap, 1, sizeof(*arg->term_len), 0, ARRAY_CLEAR) ||
            !array_init(&arg->term, &arg->term_cap, 1, sizeof(*arg->term), 0, ARRAY_CLEAR) ||
            !str_pool_init(&arg->pool, 0, 0, 0)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
        else
        {
            arg->term_len[arg->term_len_cnt++]++;
            arg->term_cnt++;
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
    case LM_EXPR_TYPE:
        if (!utf8.val) break;
        if (strchr("^*+)", utf8.val)) log_message_error_xml_chr(log, CODE_METRIC, metric, XML_UNEXPECTED_CHAR, utf8.byte, utf8.len);
        else if (utf8.val == '(')
        {
            if (!STREQ(context->buff->str, context->len, "categorical", Strnicmp))
            {
                if (!STREQ(context->buff->str, context->len, "numeric", Strnicmp))
                {
                    if (!STREQ(context->buff->str, context->len, "ordinal", Strnicmp)) 
                        log_message_xml_generic(log, CODE_METRIC, MESSAGE_ERROR, context->metric, "Unexpected covariate type %~s*", context->buff->str, context->len);
                    else
                    {
                        arg->term[arg->term_cnt - 1] = (struct lm_term) { .sort = COV_SORT_STRCMP_ASC, .deg = 1 };
                        context->st++;
                        return 1;                        
                    }
                    break;
                }
                arg->term[arg->term_cnt - 1].deg = 1;
                context->st++;
                return 1;
            }
            arg->term[arg->term_cnt - 1].sort = COV_SORT_STRCMP_ASC;
            context->st++;
            return 1;
            // TODO : Add custom sorting
        }
        else if (!buff_append(context->buff, utf8.chr, utf8.len, 0)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
        else
        {
            if (!utf8_is_whitespace_len(utf8.val, utf8.len)) context->len = context->buff->len;
            return 1;
        }
        break;
    case LM_EXPR_VAR_INIT:
        if (!utf8.val) break;
        if (utf8_is_whitespace_len(utf8.val, utf8.len)) return 1;
        if (strchr("^*+(", utf8.val)) log_message_error_xml_chr(log, CODE_METRIC, metric, XML_UNEXPECTED_CHAR, utf8.byte, utf8.len);
        else if (utf8.val == ')') log_message_xml_generic(log, CODE_METRIC, MESSAGE_ERROR, metric, "Covariate name expected before %~c", ')');
        else if (!buff_append(context->buff, utf8.chr, utf8.len, BUFFER_DISCARD)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
        else
        {
            context->metric = metric;
            context->len = utf8.len;
            context->st++;
            return 1;
        }
        break;
    case LM_EXPR_VAR:
        if (!utf8.val) break;
        if (strchr("^*+(", utf8.val)) log_message_error_xml_chr(log, CODE_METRIC, metric, XML_UNEXPECTED_CHAR, utf8.byte, utf8.len);
        else if (utf8.val == ')')
        {
            context->buff->len = context->len;
            if (!buff_append(context->buff, 0, 0, BUFFER_TERM) ||
                !str_pool_insert(&arg->pool, context->buff->str, context->len, &arg->term[arg->term_cnt - 1].off, 0, NULL)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
            else
            {
                context->st = LM_EXPR_OP;
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
    case LM_EXPR_OP:
        switch (utf8.val)
        {
        case '\0':
            return 1;
        case '^':
            if (!arg->term[arg->term_cnt - 1].deg) log_message_xml_generic(log, CODE_METRIC, MESSAGE_ERROR, metric, "Operation %~c cannot be applied to the covariate of categorical type", '^');
            else
            {
                context->st = LM_EXPR_NUM_INIT;
                return 1;
            }
            break;
        case '*':
            context->st = LM_EXPR_TYPE_INIT;
            arg->term_len[arg->term_len_cnt - 1]++;
            return 1;
        case '+':
            if (!array_test(&arg->term_len, &arg->term_len_cap, sizeof(*arg->term_len), 0, ARRAY_CLEAR, arg->term_len_cnt, 1)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
            else
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
        if (!array_test(&arg->term, &arg->term_cap, sizeof(*arg->term), 0, ARRAY_CLEAR, arg->term_cnt, 1)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
        else
        {
            //arg->len[arg->len_cnt - 1]++;
            arg->term_cnt++;
            if (strchr("^*+()", utf8.val)) log_message_error_xml_chr(log, CODE_METRIC, metric, XML_UNEXPECTED_CHAR, utf8.byte, utf8.len);
            else if (!buff_append(context->buff, utf8.chr, utf8.len, BUFFER_DISCARD)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
            else
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
            arg->term[arg->term_cnt - 1].deg = utf8.val - '0';
            context->st++;
            context->len = 0;
            return 1;
        }
        break;
    case LM_EXPR_NUM:
        switch (utf8.val)
        {
        case '\0':
            return 1;
        case '*':
            context->st = LM_EXPR_TYPE_INIT;
            arg->term_len[arg->term_len_cnt - 1]++;
            return 1;
        case '+':
            if (!array_test(&arg->term_len, &arg->term_len_cap, sizeof(*arg->term_len), 0, ARRAY_CLEAR, arg->term_len_cnt, 1)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
            else
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
            else if (!size_mul_add_test(&arg->term[arg->term_cnt - 1].deg, 10, utf8.val - '0')) log_message_error_xml(log, CODE_METRIC, context->metric, XML_OUT_OF_RANGE);
            else return 1;
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
    size_t *p_ind;
    unsigned r = str_pool_insert(&cov->pool, str, len, NULL, sizeof(size_t), &p_ind);
    if (!r) return 0;
    // if (r & HASH_PRESENT) return 0;
    *p_ind = cov->cnt++; // Just override column index with new one
    return 1;
}

static bool cov_handler(const char *str, size_t len, void *res, void *Context)
{
    struct cov *cov = Context;
    size_t off = cov->buff.len + !!cov->buff.len;
    if (!buff_append(&cov->buff, str, len, BUFFER_INIT | BUFFER_TERM)) return 0;
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
        if (!array_test(&context->cov->off, &context->cap, sizeof(*context->cov->off), 0, 0, context->cnt, 1)) return 0;
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
    
    if (str_pool_init(&cov->pool, 0, 0, sizeof(size_t)))
    {
        if (tbl_read(path, 0, tbl_cov_selector, NULL, &context, NULL, &skip, &cnt, &length, ',', log))
        {            
            cov->dim = cnt - 1;
            if (array_init(&cov->ord, NULL, cov->cnt * COV_SORT_CNT, sizeof(*cov->ord), 0, ARRAY_CLEAR) &&
                array_init(&cov->level, NULL, cov->cnt * COV_SORT_CNT, sizeof(*cov->level), 0, ARRAY_CLEAR)) return 1;
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
    free(cov->ord);
    free(cov->off);
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
            size_t off = ind * COV_SORT_CNT + term[i].sort;
            term[i].off = off;
            void **ptr = cov->ord + off;
            size_t *level = cov->level + off;
            if (!*ptr)
            {
                switch (term[i].sort)
                {
                case COV_SORT_NUM:
                    if (!array_init(ptr, NULL, cov->dim, sizeof(double), 0, ARRAY_STRICT)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
                    else
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
                        break;
                    }
                    return 0;
                case COV_SORT_STRCMP_ASC:
                    *level = cov->dim;
                    if (!ranks_unique(ptr, cov->off + ind * cov->dim, level, sizeof(*cov->off), str_off_cmp, cov->buff.str)) log_message_crt(log, CODE_METRIC, MESSAGE_ERROR, errno);
                    else continue;
                }
            }
            continue;
        }
        return 0;
    }
    return 1;
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
    for (size_t i = 0; i < len; i++)
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
    if (!array_init(&off_trans, NULL, cov.dim * cov.cnt, sizeof(*off_trans), 0, 0)) return 0;
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

    size_t blk = cov.cnt * COV_SORT_CNT;
    
    size_t *data = NULL;
    if (!array_init(&data, NULL, arg.term_len_cnt, sizeof(*data) * blk, 0, ARRAY_STRICT | ARRAY_CLEAR)) return 0;
    
    for (size_t i = 0, j = 0; i < arg.term_len_cnt; i++)
    {
        for (; j < arg.term_len[i]; j++)
        {
            size_t ind = arg.term[j].off, deg = arg.term[j].deg;
            if (deg) data[i * blk + ind] = MAX(deg, data[i * blk + ind]);
            else data[i * blk + ind] = 1;
        }
    }

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

    struct lm_supp supp;
    if (!lm_init(&supp, cov.dim, reg_vect_cnt, reg_cnt, max_len)) return 0;

    struct lm_term phen = { .off = 0, .deg = 1 };
    if (!cov_querry(&cov, &(struct buff) {.str = (char *) phen_name }, &phen, 1, log)) return 0;

    FILE *f = fopen(path_out, "w");
    if (!f)
    {
        log_message_fopen(log, CODE_METRIC, MESSAGE_ERROR, path_out, errno);
        return 0;
    }
    
    for (size_t i = 0; i < snp_cnt; i++)
    {
        struct categorical_res x = lm_impl(&supp, gen + i * cov.dim, cov.ord, phen.off, cov.level, cov.dim, reg_cnt, arg.term, arg.term_len, arg.term_len_cnt, 15);
        fprintf(f,
            "%zu,%.15e,%.15e\n"
            "%zu,%.15e,%.15e\n"
            "%zu,%.15e,%.15e\n"
            "%zu,%.15e,%.15e\n",
            4 * i + 1, x.nlpv[0], x.qas[0],
            4 * i + 2, x.nlpv[1], x.qas[1],
            4 * i + 3, x.nlpv[2], x.qas[2],
            4 * i + 4, x.nlpv[3], x.qas[3]);
        fflush(f);
    }
    fclose(f);

    free(data);
    free(bits);
    lm_expr_arg_close(&arg);    
    return 1;
}
