#include "../np.h"
#include "../ll.h"
#include "../cmp.h"
#include "ll.h"

#include <float.h>
#include <math.h>
#include <string.h>

bool test_ll_generator_a(void *p_res, size_t *p_ind, struct log *log)
{
    (void) log;
    static const struct test_ll_a data[] = {
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
        if (++*p_ind >= countof(data)) *p_ind = 0;
        return 1;
    }
    if (!array_assert(log, CODE_METRIC, array_init(p_res, NULL, 1, sizeof(struct test_ll_a), 0, ARRAY_STRICT))) return 0;
    **(struct test_ll_a **) p_res = data[*p_ind];
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

unsigned test_ll_a_1(void *In, void *Context, void *Tls)
{
    (void) Context;
    (void) Tls;
    struct test_ll_a *in = In;
    int res = flt64_stable_cmp_dsc(&in->a, &in->b, NULL);
    return res == in->res_dsc && flt64_stable_cmp_dsc_test(&in->a, &in->b, NULL) == res;
}

unsigned test_ll_a_2(void *In, void *Context, void *Tls)
{
    (void) Context;
    (void) Tls;
    struct test_ll_a *in = In;
    int res = flt64_stable_cmp_dsc_abs(&in->a, &in->b, NULL);
    return res == in->res_dsc_abs && flt64_stable_cmp_dsc_abs_test(&in->a, &in->b, NULL) == res;
}

unsigned test_ll_a_3(void *In, void *Context, void *Tls)
{
    (void) Context;
    (void) Tls;
    struct test_ll_a *in = In;
    int res = flt64_stable_cmp_dsc_nan(&in->a, &in->b, NULL);
    return res == in->res_dsc_nan && flt64_stable_cmp_dsc_nan_test(&in->a, &in->b, NULL) == res;
}

bool test_ll_generator_b(void *p_res, size_t *p_ind, struct log *log)
{
    (void) log;
    static const struct test_ll_b data[] = {
        { 0b0, UINT32_MAX, UINT32_MAX },
        { 0b1, 0, 0 },
        { 0b10, 1, 1 },
        { 0b100, 2, 2 },
        { 0b1001, 0, 3 },
        { 0b10010, 1, 4 },
        { 0b100100, 2, 5 },
        { 0b1001000, 3, 6 },
        { 0b10010000, 4, 7 },
        { 0b100100000, 5, 8 },
        { 0b1001000000, 6, 9 },
        { 0b10010000000, 7, 10 },
        { 0b100100000000, 8, 11 },
        { 0b1001000000000, 9, 12 },
        { 0b10010000000000, 10, 13 },
        { 0b100100000000000, 11, 14 },
        { 0b1001000000000000, 12, 15 },
        { 0b10010000000000000, 13, 16 },
        { 0b100100000000000000, 14, 17 },
        { 0b1001000000000000000, 15, 18 },
    };
    if (p_res) *(const struct test_ll_b **) p_res = data + *p_ind;
    else if (++*p_ind >= countof(data)) *p_ind = 0;
    return 1;
}

unsigned test_ll_b(void *In, void *Context, void *Tls)
{
    (void) Context;
    (void) Tls;
    const struct test_ll_b *in = In;
    uint32_t res_bsr = bsr(in->a), res_bsf = bsf(in->a);
    if (res_bsr != in->res_bsr || res_bsf != in->res_bsf) return 0;
    return 1;
}

static int flt64_sign_test(double x) 
{
    return (0. < x) - (x < 0.);
}

void test_ll_perf(struct log *log)
{
    struct { double a, b; int res; } in[] = {
        { 1. , -1, -1 },
        { -1., 1, 1 },
        { 1., 0., -1 },
        { 0., 1., 1 },
        { -1., 0., 1 },
        { 0., -1., -1 },
        { 0., 0., 0 },
        { 1., 1., 0 },
        { -1., -1., 0 },
        { nan(__func__), 0., 1 },
        { 0., nan(__func__), -1 },
        { nan(__func__), nan(__func__), 0 },
        { nan(__func__), nan("zzz"), 0 },
        { nan(__func__), DBL_MAX, 1 },
        { nan("zzz"), nan(__func__), 0 },
        { DBL_MAX, nan(__func__), -1 },
        { nan("zzz"), nan("zzz"), 0 },
        { DBL_MIN, DBL_MAX, 1 },
        { DBL_MAX, DBL_MIN, -1 },
        { 0, DBL_EPSILON, 1 },
        { DBL_EPSILON, 0, -1 },
    };

    volatile int cnt = 0;
    uint64_t start = get_time();
    for (size_t j = 0; j < 4096 * (USHRT_MAX + 1); j++)
    {
        for (size_t i = 0; i < countof(in); i++)
        {
            cnt += flt64_sign_test(in[i].a);
            //cnt += flt64_stable_cmp_dsc_nan_test(&in[i].a, &in[i].b, NULL);
        }
    }
    printf("Sum: %d\n", cnt);
    log_message_fmt(log, CODE_METRIC, MESSAGE_INFO, "Branched comparison took %~T.\n", start, get_time());

    cnt = 0;
    start = get_time();
    for (size_t j = 0; j < 4096 * (USHRT_MAX + 1); j++)
    {
        for (size_t i = 0; i < countof(in); i++)
        {
            cnt += flt64_sign(in[i].a);
            //cnt += flt64_stable_cmp_dsc_nan(&in[i].a, &in[i].b, NULL);
        }
    }
    printf("Sum: %d\n", cnt);
    log_message_fmt(log, CODE_METRIC, MESSAGE_INFO, "SIMD comparison took %~T.\n", start, get_time());
}

#if 0

struct memory_chunk {
    void *ptr;
    size_t sz;
};

void *a(struct memory_chunk *a)
{
    a->ptr = malloc(a->sz);
    printf("Inside %s\n", __func__);
    return a->ptr;
}

void *b(struct memory_chunk *a)
{
    (void) a;
    (void) b;
    printf("Inside %s\n", __func__);
    return a->ptr;
}

bool test_ll_3()
{
    spinlock sp = SPINLOCK_INIT;
    struct memory_chunk chunk = { .sz = 100 };
    void *res;
    for (size_t i = 0; i < 10; i++)
    {
        res = double_lock_execute(&sp, (double_lock_callback) a, (double_lock_callback) b, &chunk, &chunk);
    }
    free(res);
    return 1;
}
#endif
