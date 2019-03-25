#include "ll.h"
#include "lde.h"

#include <float.h>
#include <math.h>

double maf_impl(uint8_t *gen, size_t phen_cnt)
{
    size_t t[3] = { 0 };
    for (size_t i = 0; i < phen_cnt; i++) if (gen[i] < 3) t[gen[i]]++;
    size_t p = t[0] + t[0] + t[1], q = t[1] + t[2] + t[2];
    return (double) MIN(p, q) / (double) (p + q);
}

void maf_all(uint8_t *gen, size_t phen_cnt, double *maf)
{
    for (size_t i = 0; i < phen_cnt; i++) maf[i] = maf_impl(gen + phen_cnt * i, phen_cnt);
}

#define EM_INIT .01
#define EM_MAX_IT 1000
#define TOL DBL_EPSILON

static size_t frequency_estimate_impl(size_t *in, size_t dh, double *out, size_t max_it)
{
    size_t totp = in[0] + in[1] + in[2] + in[3], tot = totp + 2 * dh;
    double rtot = 1. / (double) tot;
    if (!dh)
    {
        for (size_t i = 0; i < 4; i++) out[i] = rtot * (double) in[i];
        return 0;
    }
    double lh = 0., rtot_init = 1. / ((double) totp + 4. * EM_INIT);
    for (size_t i = 0; i < 4; i++)
    {
        out[i] = rtot_init * ((double) in[i] + EM_INIT);
        lh += (double) in[i] * log(out[i]);
    }
    double pr03 = out[0] * out[3], pr12 = out[1] * out[2], spr;
    lh += (double) dh * log(pr03 + pr12);
    double tol = fabs(lh * sqrt(TOL));
    if (tol < TOL) tol = TOL;

    size_t it = 0;
    while (it++ < max_it)
    {
        spr = 1. / (pr03 + pr12);
        double dh0 = (double) dh * pr03 * spr, dh1 = (double) dh * pr12 * spr, diff[4] = { dh0, dh1, dh1, dh0 }, lht = 0.;
        for (size_t i = 0; i < 4; i++)
        {
            out[i] = rtot * ((double) in[i] + diff[i]);
            lht += (double) in[i] * log(out[i]);
        }
        pr03 = out[0] * out[3], pr12 = out[1] * out[2];
        lht += (double) dh * log(pr03 + pr12);

        if (fabs(lh - lht) < tol) break;
        lh = lht;
    }
    return it;
}


size_t lde_hwe(char *gen_pos, char *gen_neg, size_t phen_cnt, double *d_prime, double *r, size_t max_it)
{
    size_t t[9] = { 0 };
    for (size_t i = 0; i < phen_cnt; i++)
    {
        uint8_t pos = (uint8_t) gen_pos[i], neg = (uint8_t) gen_neg[i];
        if (pos < 3 && neg < 3) t[pos + 3 * neg]++;
    }
    double fr[4];
    size_t it = frequency_estimate_impl((size_t[]) { 2 * t[0] + t[1] + t[3], 2 * t[2] + t[1] + t[5], 2 * t[6] + t[3] + t[7], 2 * t[8] + t[5] + t[7] }, t[4], fr, max_it);
    double mar_pos[] = { fr[0] + fr[1], fr[2] + fr[3] }, mar_neg[] = { fr[0] + fr[2], fr[1] + fr[3] }, cov = fr[0] - mar_pos[0] * mar_neg[0], pr0, pr1, div;
    if (cov >= 0.) pr0 = mar_pos[0] * mar_neg[1], pr1 = mar_neg[0] * mar_pos[1], div = MIN(pr0, pr1);
    else pr0 = mar_pos[0] * mar_neg[0], pr1 = mar_pos[1] * mar_neg[1], div = -MIN(pr0, pr1);
    *d_prime = cov / div;
    *r = cov / sqrt(pr0 * pr1);
    return it;
}

double lde_impl(uint8_t *gen_pos, uint8_t *gen_neg, size_t phen_cnt)
{
    size_t t[9] = { 0 };
    for (size_t i = 0; i < phen_cnt; i++) if (gen_pos[i] < 3 && gen_neg[i] < 3) t[gen_pos[i] + 3 * gen_neg[i]]++;
    size_t ts = t[0];
    for (size_t i = 1; i < 9; ts += t[i++]);
    size_t fr[] = { 
        4 * t[0] + 2 * (t[1] + t[3]) + t[4], 
        4 * t[2] + 2 * (t[1] + t[5]) + t[4],
        4 * t[6] + 2 * (t[3] + t[7]) + t[4],
        4 * t[8] + 2 * (t[5] + t[7]) + t[4]
    };
    size_t mar_pos[] = { fr[0] + fr[1], fr[2] + fr[3] }, mar_neg[] = { fr[0] + fr[2], fr[1] + fr[3] };
    size_t bor, cov = size_sub(&bor, 4 * fr[0] * ts, mar_pos[0] * mar_neg[0]); // Actual covariance = 'cov / (16 * ts * ts)'
    size_t pr0, pr1;
    if (bor) pr0 = mar_pos[0] * mar_neg[0], pr1 = mar_pos[1] * mar_neg[1], cov = 0 - cov; 
    else pr0 = mar_pos[0] * mar_neg[1], pr1 = mar_pos[1] * mar_neg[0];
    double dp = (double) cov / (double) MIN(pr0, pr1);
    //return (struct lde) { .d_prime = (double) cov / (double) MIN(pr0, pr1), .r = (double) cov / sqrt(pr0 * pr1) };
    return (double) cov / (double) MIN(pr0, pr1);
}

