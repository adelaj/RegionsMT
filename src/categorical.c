#include "np.h"
#include "common.h"
#include "gslsupp.h"
#include "ll.h"
#include "memory.h"
#include "categorical.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define DECLARE_BITS_INIT(TYPE, PREFIX) \
    size_t PREFIX ## _bits_init(uint8_t *bits, size_t cnt, size_t ucnt, size_t *filter, TYPE *data) \
    { \
        size_t res = 0; \
        for (size_t i = 0; i < cnt; i++) \
        { \
            if (uint8_bit_test_set(bits, data[filter[i]])) continue; \
            if (++res == ucnt) break; \
        } \
        return res; \
    }

static DECLARE_BITS_INIT(uint8_t, gen)
static DECLARE_BITS_INIT(size_t, phen)

static size_t gen_pop_cnt_alt_impl(enum mt_alt alt, uint8_t *bits, size_t pop_cnt)
{
    switch (alt)
    {
    case ALT_CD: // codominant
        return pop_cnt;
    case ALT_R: // recessive
        return pop_cnt == 2 ? bits[0] == (1 | 4) || bits[0] == (2 | 4) ? 2 : 1 : MIN(pop_cnt, 2);
    case ALT_D: // dominant
        return pop_cnt == 2 ? bits[0] == (1 | 2) || bits[0] == (1 | 4) ? 2 : 1 : MIN(pop_cnt, 2);
    case ALT_A: // allelic
        return MIN(pop_cnt, 2);
    default:;
    }
    return 0;
}

static void gen_shuffle_alt_impl(enum mt_alt alt, size_t *dst, size_t *src, uint8_t *bits)
{
    switch (alt)
    {
    case ALT_CD: // codominant
        switch (bits[0])
        {
        case (1 | 2 | 4):
            dst[2] = src[2];
        case (1 | 2):
            dst[0] = src[0];
            dst[1] = src[1];
            break;
        case (1 | 4):
            dst[0] = src[0];
            dst[1] = src[2];
            break;
        case (2 | 4):
            dst[0] = src[1];
            dst[1] = src[2];
            break;
        }
        break;
    case ALT_R: // recessive
        switch (bits[0])
        {
        case (1 | 2 | 4):
            dst[0] = src[0] + src[1];
            dst[1] = src[2];
            break;
        case (2 | 4):
            dst[0] = src[1];
            dst[1] = src[2];
        case (1 | 4):
            dst[0] = src[0];
            dst[1] = src[2];
            break;
        }
        break;
    case ALT_D: // dominant
        switch (bits[0])
        {
        case (1 | 2 | 4):
            dst[0] = src[0];
            dst[1] = src[1] + src[2];
            break;
        case (1 | 4):
            dst[0] = src[0];
            dst[1] = src[2];
            break;
        case (1 | 2):
            dst[0] = src[0];
            dst[1] = src[1];
            break;
        }
        break;
    case ALT_A: // allelic
        switch (bits[0])
        {
        case (1 | 2 | 4):
            dst[0] = 2 * src[0] + src[1];
            dst[1] = src[1] + 2 * src[2];
            break;
        case (2 | 4):
            dst[0] = src[1];
            dst[1] = src[1] + 2 * src[2];
        case (1 | 4):
            dst[0] = src[0];
            dst[1] = src[1];
            break;
        case (1 | 2):
            dst[0] = 2 * src[0] + src[1];
            dst[1] = src[1];
            break;
        }
        break;
    default:;
    }
}

struct categorical_snp_data {
    size_t gen_mar[GEN_CNT * ALT_CNT], mar[ALT_CNT], cnt, gen_pop_cnt_alt[ALT_CNT], flags_pop_cnt;
    uint8_t gen_bits[UINT8_CNT(GEN_CNT)];
};

// It is implicitly assumed that 'phen_ucnt <= phen_cnt'
struct array_result categorical_init(struct categorical_supp *supp, size_t phen_cnt, size_t phen_ucnt)
{
    *supp = (struct categorical_supp) { 0 };
    struct array_result res = { 0 };
    for (size_t i = 0, tot = 0;; i++, tot = size_add_sat(tot, res.tot))
    {
        switch (i)
        {
        case 0:
            // It is assumed below that user was able to allocate memory at least for the phenotype data before function call
            res = array_init(&supp->phen_val, NULL, phen_ucnt, sizeof(*supp->phen_val), 0, ARRAY_STRICT | is_failsafe(*supp->phen_val, size_t));
            break;
        case 1:
            res = array_init(&supp->phen_mar, NULL, phen_ucnt, sizeof(*supp->phen_mar), 0, ARRAY_STRICT | is_failsafe(*supp->phen_mar, size_t));
            break;
        case 2:
            res = array_init(&supp->phen_bits, NULL, UINT8_CNT(phen_ucnt), sizeof(*supp->phen_bits), 0, ARRAY_STRICT | is_failsafe(*supp->phen_bits, size_t));
            break;
        case 3:
            res = array_init(&supp->filter, NULL, phen_cnt, sizeof(*supp->filter), 0, ARRAY_STRICT | is_failsafe(*supp->filter, size_t));
            break;
        case 4:
            res = matrix_init(&supp->outer, NULL, phen_ucnt, GEN_CNT, sizeof(*supp->outer), 0, 0, ARRAY_STRICT);
            break;
        case 5:
            res = matrix_init(&supp->tbl, NULL, phen_ucnt, 2 * GEN_CNT, sizeof(*supp->tbl), 0, 0, ARRAY_STRICT);
            break;
        default:
            res.tot = tot;
            return res;
        }
        if (!res.status) break;
    }
    categorical_close(supp);
    return res;
}

void categorical_close(struct categorical_supp *supp)
{
    free(supp->phen_val);
    free(supp->phen_mar);
    free(supp->phen_bits);
    free(supp->filter);
    free(supp->outer);
    free(supp->tbl);
}

// It is implicitly assumed that 'phen_ucnt <= phen_cnt'
struct array_result categorical_adj_average_init(struct categorical_adj_average_supp *supp, size_t snp_cnt, size_t phen_cnt, size_t phen_ucnt)
{
    *supp = (struct categorical_adj_average_supp) { 0 };
    struct array_result res = { 0 };
    for (size_t i = 0, tot = 0;; i++, tot = size_add_sat(tot, res.tot))
    {
        switch (i)
        {
        case 0:
            res = array_init(&supp->phen_perm, NULL, phen_cnt, sizeof(*supp->phen_perm), 0, ARRAY_STRICT | is_failsafe(*supp->phen_perm, size_t));
            break;
        case 1:
            res = array_init(&supp->phen_filter, NULL, phen_cnt, sizeof(*supp->phen_filter), 0, ARRAY_STRICT | is_failsafe(*supp->phen_filter, size_t));
            break;
        case 2:
            res = array_init(&supp->phen_mar, NULL, phen_cnt, sizeof(*supp->phen_mar), 0, ARRAY_STRICT | is_failsafe(*supp->phen_mar, size_t));
            break;
        case 3:
            res = array_init(&supp->phen_bits, NULL, UINT8_CNT(phen_ucnt), sizeof(*supp->phen_bits), 0, ARRAY_STRICT | is_failsafe(*supp->phen_bits, size_t));
            break;
        case 4:
            res = array_init(&supp->snp_data, NULL, snp_cnt, sizeof(*supp->snp_data), 0, ARRAY_STRICT);
            break;
        case 5:
            res = matrix_init(&supp->filter, NULL, snp_cnt, phen_cnt, sizeof(*supp->filter), 0, 0, ARRAY_STRICT);
            break;
        case 6:
            res = matrix_init(&supp->outer, NULL, phen_ucnt, GEN_CNT, sizeof(*supp->outer), 0, 0, ARRAY_STRICT);
            break;
        case 7:
            res = matrix_init(&supp->tbl, NULL, phen_ucnt, 2 * GEN_CNT, sizeof(*supp->tbl), 0, 0, ARRAY_STRICT);
            break;
        default:
            res.tot = tot;
            return res;
        }
        if (!res.status) break;
    }    
    maver_adj_close(supp);
    return res;
}

void maver_adj_close(struct categorical_adj_average_supp *supp)
{
    free(supp->phen_perm);
    free(supp->phen_filter);
    free(supp->phen_mar);
    free(supp->phen_bits);
    free(supp->snp_data);
    free(supp->filter);
    free(supp->outer);
    free(supp->tbl);
}

static size_t filter_init(size_t *filter, uint8_t *gen, size_t *phen, size_t phen_cnt)
{
    size_t cnt = 0;
    for (size_t i = 0; i < phen_cnt; i++) if ((!gen || gen[i] < GEN_CNT) && phen[i] != SIZE_MAX) filter[cnt++] = i;
    return cnt;
}

static size_t gen_pop_cnt_alt_init(size_t *gen_pop_cnt_alt, uint8_t *gen_bits, size_t gen_pop_cnt, enum mt_flags flags)
{
    size_t res = 0;
    for (enum mt_alt i = 0; i < ALT_CNT; i++, flags >>= 1) if (flags & 1)
    {
        size_t tmp = gen_pop_cnt_alt_impl(i, gen_bits, gen_pop_cnt);
        if (tmp < 2) continue;
        gen_pop_cnt_alt[i] = tmp;
        res++;
    }
    return res;
}

static void contingency_table_init(size_t *tbl, uint8_t *gen, size_t *phen, size_t cnt, size_t *filter)
{
    for (size_t i = 0; i < cnt; i++)
    {
        size_t ind = filter[i];
        tbl[gen[ind] + GEN_CNT * phen[ind]]++;
    }
}

static void contingency_table_shuffle_alt_impl(enum mt_alt alt, size_t *dst, size_t *src, uint8_t *gen_bits, size_t gen_pop_cnt, uint8_t *phen_bits, size_t phen_pop_cnt)
{
    for (size_t i = 0, j = 0, off = 0; i < phen_pop_cnt; i++, j += GEN_CNT)
    {
        if (!uint8_bit_test(phen_bits, i)) continue;
        gen_shuffle_alt_impl(alt, dst + off, src + j, gen_bits);
        off += gen_pop_cnt;
    }
}

void mar_init(size_t *tbl, size_t *xmar, size_t *ymar, size_t *p_mar, size_t dimx, size_t dimy)
{
    size_t mar = 0;
    for (size_t i = 0; i < dimy; mar += ymar[i], i++) for (size_t j = 0; j < dimx; j++)
    {
        size_t el = tbl[j + dimx * i];
        xmar[j] += el;
        ymar[i] += el;
    }
    *p_mar = mar;
}

void ymar_init(size_t *tbl, size_t *ymar, size_t dimx, size_t dimy)
{
    for (size_t i = 0; i < dimy; i++) for (size_t j = 0; j < dimx; ymar[i] += tbl[j + dimx * i], j++);
}

void outer_init(size_t *outer, size_t *xmar, size_t *ymar, size_t dimx, size_t dimy)
{
    for (size_t i = 0; i < dimy; i++) for (size_t j = 0; j < dimx; j++) outer[j + dimx * i] = xmar[j] * ymar[i];
}

bool outer_combined_init(size_t *outer, size_t *xmar, size_t *ymar, size_t mar, size_t dimx, size_t dimy)
{
    if (dimx > 2 || dimy > 2)
    {
        outer_init(outer, xmar, ymar, dimx, dimy);
        return 1;
    }
    size_t lim = 5 * mar;
    for (size_t i = 0; i < dimy; i++) for (size_t j = 0; j < dimx; j++)
    {
        size_t tmp = xmar[j] * ymar[i];
        if (tmp >= lim) outer[j + dimx * i] = tmp;
        else return 0;
    }
    return 1;
}

double stat_exact(size_t *tbl, size_t *xmar, size_t *ymar)
{
    size_t g0 = xmar[0], p0 = ymar[0], p1 = ymar[1], lo = size_sub_sat(g0, p1), hi = MIN(g0, p0);
    double hyp_comp = pdf_hypergeom(tbl[0], p0, p1, g0), a = 0., b = 0.;
    for (size_t i = lo; i <= hi; i++)
    {
        double hyp = pdf_hypergeom(i, p0, p1, g0);
        a += hyp;
        if (hyp <= hyp_comp) b += hyp;
    }
    return b / a; // nlpv = log10(a) - log10(b);
}

double qas_or(size_t *tbl)
{
    return (double) (tbl[0] * tbl[3]) / (double) (tbl[1] * tbl[2]); // lor = log10((double) tbl[0]) + log10((double) tbl[3]) - (log10((double) tbl[1]) + log10((double) tbl[2]))
}

double stat_chisq(size_t *tbl, size_t *outer, size_t mar, size_t dimx, size_t dimy)
{
    double stat = 0.;
    size_t pr = dimx * dimy;
    for (size_t i = 0; i < pr; i++)
    {
        size_t out = outer[i];
        double diff = (double) out - (double) (tbl[i] * mar);
        stat += diff * diff / (double) (out * mar);
    }
    return cdf_chisq_Q(stat, (double) (pr - dimx - dimy + 1)); // nlpv = -log10(...)
}

static void val_init(size_t *val, uint8_t *bits, size_t cnt)
{
    for (size_t i = 0, j = 0; i < cnt; i++) if (uint8_bit_test(bits, i)) val[j++] = i;
}

double qas_fisher(size_t *tbl, size_t *xval, size_t *yval, size_t *xmar, size_t *ymar, size_t mar, size_t dimx, size_t dimy)
{
    size_t s = 0, t = 0, s2 = 0, t2 = 0, st = 0;
    for (size_t i = !yval[0]; i < dimy; i++)
    {
        for (size_t j = !xval[0]; j < dimx; j++) st += yval[i] * xval[j] * tbl[j + dimx * i];
        size_t m = yval[i] * ymar[i];
        t += m, t2 += yval[i] * m;
    }
    for (size_t i = !xval[0]; i < dimx; i++)
    {
        size_t m = xval[i] * xmar[i];
        s += m, s2 += xval[i] * m;
    }
    double a = (double) (st * mar) - (double) (s * t), b = sqrt(((double) (s2 * mar) - (double) (s * s)) * ((double) (t2 * mar) - (double) (t * t)));
    return sqrt((b + a) / (b - a)); // lqas = .5 * (log10(b + a) - log10(b - a))
}

static void perm_init(size_t *perm, size_t *filter, size_t cnt, gsl_rng *rng)
{
    for (size_t i = 0; i + 1 < cnt; i++) // Performing 'Knuth shuffle'
    {
        size_t j = i + (size_t) floor(gsl_rng_uniform(rng) * (cnt - i)), fi = filter[i], fj = filter[j];
        size_t swp = perm[fi];
        perm[fi] = perm[fj];
        perm[fj] = swp;
    }
}

struct mt_result categorical_impl(struct categorical_supp *supp, uint8_t *gen, size_t *phen, size_t phen_cnt, size_t phen_ucnt, enum mt_flags flags)
{    
    struct mt_result res;
    uint8_t gen_bits[UINT8_CNT(GEN_CNT)] = { 0 };
    size_t gen_pop_cnt_alt[ALT_CNT] = { 0 };
    size_t gen_val[GEN_CNT];

    array_broadcast(res.pv, countof(res.pv), sizeof(*res.pv), &(double) { nan(__func__) });
    array_broadcast(res.qas, countof(res.qas), sizeof(*res.qas), &(double) { nan(__func__) });

    size_t table_disp = GEN_CNT * phen_ucnt;
       
    // Initializing genotype/phenotype filter
    size_t cnt = filter_init(supp->filter, gen, phen, phen_cnt);
    if (!cnt) return res;
    
    // Counting unique genotypes
    if (!gen_pop_cnt_alt_init(gen_pop_cnt_alt, gen_bits, gen_bits_init(gen_bits, cnt, GEN_CNT, supp->filter, gen), flags)) return res;
    
    // Counting unique phenotypes
    memset(supp->phen_bits, 0, UINT8_CNT(phen_ucnt));
    size_t phen_pop_cnt = phen_bits_init(supp->phen_bits, cnt, phen_ucnt, supp->filter, phen);
    if (phen_pop_cnt < 2) return res;

    // Building contingency table
    memset(supp->tbl + table_disp, 0, table_disp * sizeof(*supp->tbl));
    contingency_table_init(supp->tbl + table_disp, gen, phen, cnt, supp->filter);

    // Performing computations for each alternative
    for (enum mt_alt i = 0; i < ALT_CNT; i++)
    {
        size_t gen_pop_cnt = gen_pop_cnt_alt[i];
        if (!gen_pop_cnt) continue;

        contingency_table_shuffle_alt_impl(i, supp->tbl, supp->tbl + table_disp, gen_bits, gen_pop_cnt, supp->phen_bits, phen_pop_cnt);

        // Computing sums
        size_t gen_mar[GEN_CNT] = { 0 }, mar = 0;
        memset(supp->phen_mar, 0, phen_pop_cnt * sizeof(*supp->phen_mar));
        mar_init(supp->tbl, gen_mar, supp->phen_mar, &mar, gen_pop_cnt, phen_pop_cnt);

        // Computing p-value
        res.pv[i] = 
            outer_combined_init(supp->outer, gen_mar, supp->phen_mar, mar, gen_pop_cnt, phen_pop_cnt) ? 
            stat_chisq(supp->tbl, supp->outer, mar, gen_pop_cnt, phen_pop_cnt) : 
            stat_exact(supp->tbl, gen_mar, supp->phen_mar);
        
        // Computing qas
        if (i == ALT_CD && phen_ucnt == 2) res.qas[i] = qas_or(supp->tbl);
        else
        {
            val_init(gen_val, gen_bits, GEN_CNT);
            val_init(supp->phen_val, supp->phen_bits, phen_ucnt);
            res.qas[i] = qas_fisher(supp->tbl, gen_val, supp->phen_val, gen_mar, supp->phen_mar, mar, gen_pop_cnt, phen_pop_cnt);
        }
    }
    return res;
}

struct adj_result categorical_adj_average(struct categorical_adj_average_supp *supp, uint8_t *gen, size_t *phen, size_t snp_cnt, size_t phen_cnt, size_t phen_ucnt, size_t rpl, size_t k, gsl_rng *rng, enum mt_flags flags)
{
    size_t table_disp = GEN_CNT * phen_ucnt;
    memset(supp->snp_data, 0, snp_cnt * sizeof(*supp->snp_data));

    //  Initialization
    double density[ALT_CNT] = { 0. };
    size_t density_cnt[ALT_CNT] = { 0 }, phen_filter_cnt = filter_init(supp->phen_filter, NULL, phen, phen_cnt);

    for (size_t i = 0, off = 0; i < snp_cnt; i++, off += phen_cnt)
    {
        // Initializing genotype/phenotype filter
        size_t cnt = filter_init(supp->filter + off, gen + off, phen, phen_cnt); // 'supp->filter' is a subset of 'supp->phen_filter'
        if (!cnt) continue;
        supp->snp_data[i].cnt = cnt;

        // Counting unique genotypes
        size_t flags_pop_cnt = gen_pop_cnt_alt_init(supp->snp_data[i].gen_pop_cnt_alt, supp->snp_data[i].gen_bits, gen_bits_init(supp->snp_data[i].gen_bits, cnt, GEN_CNT, supp->filter + off, gen + off), flags);
        if (!flags_pop_cnt) continue;
        supp->snp_data[i].flags_pop_cnt = flags_pop_cnt;

        // Counting unique phenotypes
        memset(supp->phen_bits, 0, UINT8_CNT(phen_ucnt));
        size_t phen_pop_cnt = phen_bits_init(supp->phen_bits, cnt, phen_ucnt, supp->filter + off, phen);
        if (phen_pop_cnt < 2) continue;

        // Building contingency table
        memset(supp->tbl + table_disp, 0, table_disp * sizeof(*supp->tbl));
        contingency_table_init(supp->tbl + table_disp, gen + off, phen, cnt, supp->filter + off);

        // Performing computations for each alternative
        for (enum mt_alt j = 0; j < ALT_CNT; j++)
        {
            size_t gen_pop_cnt = supp->snp_data[i].gen_pop_cnt_alt[j];
            if (!gen_pop_cnt) continue;

            contingency_table_shuffle_alt_impl(j, supp->tbl, supp->tbl + table_disp, supp->snp_data[i].gen_bits, gen_pop_cnt, supp->phen_bits, phen_pop_cnt);

            // Computing sums
            memset(supp->phen_mar, 0, phen_pop_cnt * sizeof(*supp->phen_mar));
            mar_init(supp->tbl, supp->snp_data[i].gen_mar + (size_t) j * GEN_CNT, supp->phen_mar, supp->snp_data[i].mar + j, gen_pop_cnt, phen_pop_cnt);
            outer_init(supp->outer, supp->snp_data[i].gen_mar + (size_t) j * GEN_CNT, supp->phen_mar, gen_pop_cnt, phen_pop_cnt);
            double st = log(stat_chisq(supp->tbl, supp->outer, supp->snp_data[i].mar[j], gen_pop_cnt, phen_pop_cnt)); // log10(...)
            if (isnan(st)) continue;
            density[j] -= st;
            density_cnt[j]++;            
        }
    }     

    // Naive P-value for density = gsl_sf_gamma_inc_Q((double) density_cnt[i], density[i] /* / M_LOG10E */)
    bool alt[ALT_CNT];
    for (enum mt_alt i = 0; i < ALT_CNT; i++, flags >>= 1) alt[i] = (flags & 1) && isfinite(density[i] /= (double) density_cnt[i]);

    // Simulations
    size_t qc[ALT_CNT] = { 0 }, qt[ALT_CNT] = { 0 };
    for (size_t r = 0; r < rpl; r++)
    {
        bool alt_rpl[ALT_CNT], alt_any = 0;
        for (enum mt_alt i = 0; i < ALT_CNT; i++) alt_any |= (alt_rpl[i] = alt[i] && (!k || qc[i] < k)); // Adaptive mode for positive parameter 'k'
        if (!alt_any) break;

        // Generating random permutation
        if (phen_cnt == phen_filter_cnt) memcpy(supp->phen_perm, phen, phen_cnt * sizeof(*supp->phen_perm));
        else for (size_t i = 0; i < phen_filter_cnt; i++) supp->phen_perm[supp->phen_filter[i]] = phen[supp->phen_filter[i]];
        perm_init(supp->phen_perm, supp->phen_filter, phen_filter_cnt, rng);
                
        // Density computation
        double density_perm[ALT_CNT] = { 0. };
        size_t density_perm_cnt[ALT_CNT] = { 0 };

        for (size_t i = 0, off = 0; i < snp_cnt; i++, off += phen_cnt)
        {
            size_t cnt = supp->snp_data[i].cnt;
            if (!cnt || !supp->snp_data[i].flags_pop_cnt) continue;

            // Counting unique phenotypes
            memset(supp->phen_bits, 0, UINT8_CNT(phen_ucnt));
            size_t phen_pop_cnt = phen_bits_init(supp->phen_bits, cnt, phen_ucnt, supp->filter + off, supp->phen_perm);
            if (phen_pop_cnt < 2) continue;

            // Building contingency table
            memset(supp->tbl + table_disp, 0, table_disp * sizeof(*supp->tbl));
            contingency_table_init(supp->tbl + table_disp, gen + off, supp->phen_perm, cnt, supp->filter + off);

            // Performing computations for each alternative
            for (enum mt_alt j = 0; j < ALT_CNT; j++) if (alt_rpl[j])
            {
                size_t gen_pop_cnt = supp->snp_data[i].gen_pop_cnt_alt[j];
                if (!gen_pop_cnt) continue;

                contingency_table_shuffle_alt_impl(j, supp->tbl, supp->tbl + table_disp, supp->snp_data[i].gen_bits, gen_pop_cnt, supp->phen_bits, phen_pop_cnt);

                // Computing sums
                memset(supp->phen_mar, 0, phen_pop_cnt * sizeof(*supp->phen_mar));
                ymar_init(supp->tbl, supp->phen_mar, gen_pop_cnt, phen_pop_cnt);
                outer_init(supp->outer, supp->snp_data[i].gen_mar + (size_t) j * GEN_CNT, supp->phen_mar, gen_pop_cnt, phen_pop_cnt);
                double st = log(stat_chisq(supp->tbl, supp->outer, supp->snp_data[i].mar[j], gen_pop_cnt, phen_pop_cnt)); // log10(...)
                if (isnan(st)) continue;
                density_perm[j] -= st;
                density_perm_cnt[j]++;
            }
        }

        for (enum mt_alt i = 0; i < ALT_CNT; i++) if (alt_rpl[i])
        {
            if (density_perm[i] >= density[i] * (double) density_perm_cnt[i]) qc[i]++; // >
            qt[i]++;
        }
    }

    struct adj_result res;
    for (enum mt_alt i = 0; i < ALT_CNT; i++) res.pv[i] = alt[i] ? (double) qc[i] / (double) qt[i] : nan(__func__); // nlpv = log10((double) qt[i]) - log10((double) qc[i]);
    return res;
}
