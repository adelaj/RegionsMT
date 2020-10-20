#include "../cmp.h"
#include "cmp.h"

#include <float.h>
#include <math.h>

bool test_cmp_generator_a(void *p_res, size_t *p_ind, struct log *log)
{
    (void) log;
    static const struct test_cmp_a data[] = {
        { 1. , -1, -1, 0, -1 },
        { -1., 1, 1, 0, 1 },
        { 1., 0., -1, -1, -1 },
        { 0., 1., 1, 1, 1 },
        { 0., 0., 0, 0, 0 },
        { -1., 0., 1, -1, 1 },
        { 1., 1., 0, 0, 0 },
        { 0., -1., -1, 1, -1 },
        { -1., -1., 0, 0, 0 },
        { NAN, 0., 0, 0, 1 },
        { 0., NAN, 0, 0, -1 },
        { NAN, NAN, 0, 0, 0 },
        { DBL_MIN, DBL_MAX, 1, 1, 1 },
        { DBL_MAX, DBL_MIN, -1, -1, -1 },
        { 0, DBL_EPSILON, 1, 1, 1 },
        { DBL_EPSILON, 0, -1, -1, -1 }
    };
    if (!p_res)
    {
        if (++ * p_ind >= countof(data)) *p_ind = 0;
        return 1;
    }
    if (!array_assert(log, CODE_METRIC, array_init(p_res, NULL, 1, sizeof(struct test_cmp_a), 0, ARRAY_STRICT))) return 0;
    **(struct test_cmp_a **) p_res = data[*p_ind];
    return 1;
}

static int flt64_stable_cmp_dsc_test(double *p_a, double *p_b, void *context)
{
    (void) context;
    double a = *p_a, b = *p_b;
    return a < b ? 1 : b < a ? -1 : 0;
}

static int flt64_stable_cmp_dsc_abs_test(double *p_a, double *p_b, void *context)
{
    return flt64_stable_cmp_dsc_test(&(double) { fabs(*p_a) }, &(double) { fabs(*p_b) }, context);
}

static int flt64_stable_cmp_dsc_nan_test(double *p_a, double *p_b, void *context)
{
    double a = *p_a, b = *p_b;
    return isnan(a) ? (isnan(b) ? 0 : 1) : isnan(b) ? -1 : flt64_stable_cmp_dsc_test(&a, &b, context);
}

unsigned test_cmp_a_1(void *In, void *Context, void *Tls)
{
    (void) Context;
    (void) Tls;
    struct test_cmp_a *in = In;
    int res = flt64_stable_cmp_dsc(&in->a, &in->b, NULL);
    return res == in->res_dsc && flt64_stable_cmp_dsc_test(&in->a, &in->b, NULL) == res;
}

unsigned test_cmp_a_2(void *In, void *Context, void *Tls)
{
    (void) Context;
    (void) Tls;
    struct test_cmp_a *in = In;
    int res = flt64_stable_cmp_dsc_abs(&in->a, &in->b, NULL);
    return res == in->res_dsc_abs && flt64_stable_cmp_dsc_abs_test(&in->a, &in->b, NULL) == res;
}

unsigned test_cmp_a_3(void *In, void *Context, void *Tls)
{
    (void) Context;
    (void) Tls;
    struct test_cmp_a *in = In;
    int res = flt64_stable_cmp_dsc_nan(&in->a, &in->b, NULL);
    return res == in->res_dsc_nan && flt64_stable_cmp_dsc_nan_test(&in->a, &in->b, NULL) == res;
}
