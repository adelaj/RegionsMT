#include "la.h"
#include "ll.h"

#include <math.h>

// Performs Gaussian elimination with full pivot and returns the number of linearly independent columns
struct array_result gauss_col(double *restrict m, size_t dimx, size_t *restrict p_rky, size_t tda, double tol)
{
    size_t dimy = *p_rky, dim = MIN(dimx, dimy);

    // Finding global maximum
    size_t maxx = 0, maxy = 0;
    double max = 0., amax = 0.; // Preventing compiler complaints
    if (dim)
    {
        amax = fabs(max = m[0]);
        for (size_t j = 0, k = 0; j < dimx; j++, k += tda) for (size_t l = 0; l < dimy; l++)
        {
            double tmp = m[l + k], atmp = fabs(tmp);
            if (atmp <= amax) continue;
            maxx = j;
            maxy = l;
            amax = atmp;
            max = tmp;
        }
        if (max == 0.) dim = 0;
    }
    if (!dim)
    {
        *p_rky = 0;
        return (struct array_result) { .status = ARRAY_SUCCESS_UNTOUCHED };
    }

    uint8_t *pivx;
    struct array_result res = array_init(&pivx, NULL, UINT8_CNT(dimx), sizeof(*pivx), 0, ARRAY_STRICT | ARRAY_CLEAR);
    if (!res.status) return res;

    size_t rky = 1;
    tol *= amax;
    for (size_t i = 0; i < dim; i++, rky++)
    {
        // Finding pivot (except for the first column)
        if (i)
        {
            maxx = 0;
            maxy = i;
            amax = fabs(max = m[i]);
            for (size_t j = 0, k = 0; j < dimx; j++, k += tda) if (!uint8_bit_test(pivx, j)) for (size_t l = i; l < dimy; l++)
            {
                double tmp = m[l + k], atmp = fabs(tmp);
                if (atmp <= amax) continue;
                maxx = j;
                maxy = l;
                amax = atmp;
                max = tmp;
            }
            if (max < tol) break;
        }
        uint8_bit_set(pivx, maxx);
        maxx *= tda;

        // Swapping columns
        if (i != maxy) for (size_t j = 0, k = i, l = maxy; j < dimx; j++, k += tda, l += tda)
        {
            double tmp = m[k];
            m[k] = m[l];
            m[l] = tmp;
        }

        // Performing Gaussian elimination
        double maxinv = 1. / max;
        size_t cnty = i + 1;
        for (size_t l = cnty; l < dimy; l++)
        {
            double c = m[l + maxx] * maxinv;
            size_t cntx = 0;
            for (size_t j = 0, k = 0; j < dimx; j++, k += tda)
                if (fabs(m[l + k] -= c * m[i + k]) < tol) cntx++;
            if (cntx == dimx) cnty++;
        }
        if (cnty == dimy) break;
    }
    free(pivx);
    *p_rky = rky;
    return (struct array_result) { .status = ARRAY_SUCCESS_UNTOUCHED };
}

// Warning! 'cnt' should be non-zero and array 'vect_len' should be of strictly ascending positive integers   
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
