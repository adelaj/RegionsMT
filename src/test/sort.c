#include "../np.h"
#include "../ll.h"
#include "../memory.h"
#include "../sort.h"
#include "../test.h"
#include "../cmp.h"
#include "sort.h"

#include <string.h> 
#include <stdlib.h> 
#include <math.h> 

static void generator_impl(size_t *arr, size_t cnt)
{
    if (cnt & 1) arr[cnt - 1] = cnt, cnt--;
    size_t hcnt = cnt >> 1;
    for (size_t i = 0; i < hcnt; i++)
    {
        arr[i] = i & 1 ? hcnt + i + !!(hcnt & 1) : i + 1;
        arr[hcnt + i] = (i + 1) << 1;
    }
}

// General testing
bool test_sort_generator_a_1(void *p_res, size_t *p_ind, struct log *log)
{
    size_t ind = *p_ind, cnt = (size_t) 1 << ind;
    if (!p_res)
    {
        if (ind < TEST_SORT_EXP) ++*p_ind;
        else *p_ind = 0;
        return 1;
    }
    if (!array_assert(log, CODE_METRIC, array_init(p_res, NULL, fam_countof(struct test_sort_a, arr, cnt), fam_sizeof(struct test_sort_a, arr), fam_diffof(struct test_sort_a, arr, cnt), ARRAY_STRICT))) return 0;
    struct test_sort_a *res = *(struct test_sort_a **) p_res;
    res->cnt = cnt;
    generator_impl(res->arr, cnt);
    return 1;
}

// Cutoff testing
bool test_sort_generator_a_2(void *p_res, size_t *p_ind, struct log *log)
{
    (void) p_ind;
    if (!p_res) return 1;
    size_t cnt = ((QUICK_SORT_CUTOFF + 1) << 1) + 1, hcnt = cnt >> 1;
    if (!array_assert(log, CODE_METRIC, array_init(p_res, NULL, fam_countof(struct test_sort_a, arr, cnt), fam_sizeof(struct test_sort_a , arr), fam_diffof(struct test_sort_a, arr, cnt), ARRAY_STRICT))) return 0;
    struct test_sort_a *res = *(struct test_sort_a **) p_res;
    res->cnt = cnt;
    for (size_t i = 0; i < hcnt; i++) res->arr[i] = (SIZE_MAX >> 1) - i - 1;
    res->arr[hcnt] = 0;
    for (size_t i = 0; i < hcnt; i++) res->arr[i + hcnt + 1] = (SIZE_MAX >> 1) + hcnt - i + 1;
    return 1;
}

struct sort_worst_case_context {
    size_t *arr, cnt, ind, gas, piv;
};

// This algorithm was suggested in www.cs.dartmouth.edu/~doug/mdmspe.pdf
static bool quick_sort_worst_case_cmp(const void *a, const void *b, void *Context)
{
    struct sort_worst_case_context *context = Context;
    size_t x = *(size_t *) a, y = *(size_t *) b;
    context->cnt++;
    if (context->arr[x] == context->gas)
    {
        if (context->arr[y] == context->gas) context->arr[x == context->piv ? x : y] = context->ind++;
        context->piv = x;
    }
    else if (context->arr[y] == context->gas) context->piv = y;
    return size_cmp_asc(a, b, context);
}

// Worst case testing
bool test_sort_generator_a_3(void *p_res, size_t *p_ind, struct log *log)
{
    size_t ind = *p_ind, cnt = (size_t) 1 << ind;
    if (!p_res)
    {
        if (ind < TEST_SORT_EXP)++ *p_ind;
        else *p_ind = 0;
        return 1;
    }
    if (!array_assert(log, CODE_METRIC, array_init(p_res, NULL, fam_countof(struct test_sort_a, arr, cnt), fam_sizeof(struct test_sort_a, arr), fam_diffof(struct test_sort_a, arr, cnt), ARRAY_STRICT))) return 0;
    struct test_sort_a *res = *(struct test_sort_a **) p_res;
    size_t *tmp;
    if (array_assert(log, CODE_METRIC, array_init(&tmp, NULL, cnt, sizeof(*tmp), 0, ARRAY_STRICT)))
    {
        res->cnt = cnt;
        for (size_t i = 0; i < cnt; tmp[i] = i, res->arr[i] = cnt - 1, i++);
        struct sort_worst_case_context context_tmp = { .arr = res->arr, .gas = cnt - 1 };
        size_t swp;
        quick_sort(tmp, cnt, sizeof(*tmp), quick_sort_worst_case_cmp, &context_tmp, &swp, sizeof(*tmp));
        free(tmp);
        return 1;
    }
    free(res);
    return 0;
}

bool test_sort_generator_b_1(void *p_res, size_t *p_ind, struct log *log)
{
    size_t ind = *p_ind, ucnt = ind * ind + ind + 1, cnt = MAX(((size_t) 1 << ind), ucnt);
    if (!p_res)
    {
        if (ind < TEST_SORT_EXP) ++*p_ind;
        else *p_ind = 0;
        return 1;
    }
    if (!array_assert(log, CODE_METRIC, array_init(p_res, NULL, fam_countof(struct test_sort_b, arr, cnt), fam_sizeof(struct test_sort_b, arr), fam_diffof(struct test_sort_b, arr, cnt), ARRAY_STRICT))) return 0;
    struct test_sort_b *res = *(struct test_sort_b **) p_res;
    res->cnt = cnt;
    res->ucnt = ucnt;
    for (size_t i = 0; i < cnt; i++) res->arr[i] = (double) (i % ucnt) + 1. / (double) (i % ucnt + 1);
    for (size_t i = 0; i < cnt - 1; i++)
    {
        size_t n = (size_t) ((double) (cnt * cnt) * sin((double) i)) % cnt;
        n = MAX(n, i + 1);
        double swp = res->arr[i];
        res->arr[i] = res->arr[n];
        res->arr[n] = swp;
    }
    return 1;
}

bool test_sort_generator_c_1(void *p_res, size_t *p_ind, struct log *log)
{
    size_t ind = *p_ind, cnt = ((size_t) 1 << ind) - 1;
    if (!p_res)
    {
        if (ind < TEST_SORT_EXP)++ *p_ind;
        else *p_ind = 0;
        return 1;
    }
    if (!array_assert(log, CODE_METRIC, array_init(p_res, NULL, fam_countof(struct test_sort_c, arr, cnt), fam_sizeof(struct test_sort_c, arr), fam_diffof(struct test_sort_c, arr, cnt), ARRAY_STRICT))) return 0;
    struct test_sort_c *res = *(struct test_sort_c **) p_res;
    res->cnt = cnt;
    for (size_t i = 0, j = 0; i < cnt; i++)
    {
        res->arr[i] = j;
        if (i >= ((size_t) 1 << j)) j++;
    }
    return 1;
}

bool test_sort_generator_d_1(void *p_res, size_t *p_ind, struct log *log)
{
    size_t ind = *p_ind, cnt = ind + 1;
    if (!p_res)
    {
        if (ind < UINT64_BIT) ++*p_ind;
        else *p_ind = 0;
        return 1;
    }
    if (!array_assert(log, CODE_METRIC, array_init(p_res, NULL, fam_countof(struct test_sort_d, arr, cnt), fam_sizeof(struct test_sort_d, arr), fam_diffof(struct test_sort_d, arr, cnt), ARRAY_STRICT))) return 0;
    struct test_sort_d *res = *(struct test_sort_d **) p_res;
    res->cnt = cnt;
    for (size_t i = 0; i < cnt; i++) res->arr[i] = UINT64_C(1) << i;
    return 1;
}

struct size_cmp_asc_test {
    size_t cnt;
    void *a, *b;
    bool succ;
};

static bool size_cmp_asc_test(const void *a, const void *b, void *Context)
{
    struct size_cmp_asc_test *context = Context;
    context->cnt++;
    if (a < context->a || b < context->a || context->b < a || context->b < b) context->succ = 0;
    return size_cmp_asc(a, b, context);
}

unsigned test_sort_a(void *In, void *Context, void *Tls)
{
    (void) Context;
    (void) Tls;
    struct test_sort_a *in = In;
    struct size_cmp_asc_test context = { .a = in->arr, .b = in->arr + in->cnt * sizeof(*in->arr) - sizeof(*in->arr), .succ = 1 };
    size_t swp;
    quick_sort(in->arr, in->cnt, sizeof(*in->arr), size_cmp_asc_test, &context, &swp, sizeof(*in->arr));
    size_t ind = 1;
    for (; ind < in->cnt; ind++) if (in->arr[ind - 1] > in->arr[ind]) break;
    return (!in->cnt || ind == in->cnt) && context.succ;
}

unsigned test_sort_b_1(void *In, void *Context, void *Tls)
{
    (void) Context;
    struct test_sort_b *in = In;
    struct test_tls *tls = Tls;
    size_t ucnt = in->cnt;
    uintptr_t *ord;
    if (!array_assert(&tls->log, CODE_METRIC, orders_stable_unique(&ord, in->arr, &ucnt, sizeof(*in->arr), flt64_stable_cmp_dsc, NULL))) return TEST_RETRY;
    size_t ind = 1;
    for (; ind < ucnt; ind++) if (in->arr[ord[ind - 1]] <= in->arr[ord[ind]]) break;
    free(ord);
    return ucnt == in->ucnt && (!ucnt || ind == ucnt);
}

unsigned test_sort_b_2(void *In, void *Context, void *Tls)
{
    struct test_sort_b *in = In;
    struct test_tls *tls = Tls;
    struct test_context *context = Context;
    unsigned res = TEST_RETRY;
    if (!context->storage)
    {
        size_t ucnt = in->cnt;
        if (!array_assert(&tls->log, CODE_METRIC, orders_stable_unique((uintptr_t **) &context->storage, in->arr, &ucnt, sizeof(*in->arr), flt64_stable_cmp_dsc, NULL))) return TEST_RETRY;
        if (ucnt != in->ucnt) res = 0;
    }
    if (res)
    {
        size_t ucnt = in->ucnt;
        uintptr_t *ord = context->storage;
        double *arr;
        if (array_assert(&tls->log, CODE_METRIC, array_init(&arr, NULL, in->cnt, sizeof(*arr), 0, ARRAY_STRICT)))
        {
            memcpy(arr, in->arr, in->cnt * sizeof(*arr));
            double swp;
            if (array_assert(&tls->log, CODE_METRIC, orders_apply(ord, ucnt, sizeof(*arr), arr, &swp, sizeof(*arr))))
            {
                size_t ind = 1;
                for (; ind < ucnt; ind++) if (in->arr[ord[ind]] != arr[ind]) break;
                res = !ucnt || ind == ucnt;
            }
            free(arr);
        }
    }
    if (context->no_retry || res != TEST_RETRY) free(context->storage);
    return res;
}

unsigned test_sort_c_1(void *In, void *Context, void *Tls)
{
    (void) Context;
    (void) Tls;
    struct test_sort_c *in = In;
    if (!in->cnt) return 1;
    for (size_t i = 1, j = 0; i < in->cnt; i++)
    {
        size_t tmp;
        if (!binary_search(&tmp, in->arr + j, in->arr, in->cnt, sizeof(*in->arr), size_stable_cmp_asc, NULL, BINARY_SEARCH_CRITICAL) || tmp != j) return 0;
        if (!binary_search(&tmp, in->arr + j, in->arr, in->cnt, sizeof(*in->arr), size_stable_cmp_asc, NULL, 0) || in->arr[tmp] != in->arr[j]) return 0;
        if (in->arr[j] != in->arr[i]) j = i;
    }
    return 1;
}

unsigned test_sort_c_2(void *In, void *Context, void *Tls)
{
    (void) Context;
    (void) Tls;
    struct test_sort_c *in = In;
    if (!in->cnt) return 1;
    for (size_t i = in->cnt, j = in->cnt - 1; --i;)
    {
        size_t tmp;
        if (!binary_search(&tmp, in->arr + j, in->arr, in->cnt, sizeof(*in->arr), size_stable_cmp_asc, NULL, BINARY_SEARCH_CRITICAL | BINARY_SEARCH_RIGHTMOST) || tmp != j) return 0;
        if (!binary_search(&tmp, in->arr + j, in->arr, in->cnt, sizeof(*in->arr), size_stable_cmp_asc, NULL, BINARY_SEARCH_RIGHTMOST) || in->arr[tmp] != in->arr[j]) return 0;
        if (in->arr[j] != in->arr[i]) j = i;
    }
    return 1;
}

static int uint64_stable_cmp_asc_test(const void *A, const void *B, void *thunk)
{
    (void) thunk;
    uint64_t a = *(uint64_t *) A, b = *(uint64_t *) B;
    return (a > b) - (a < b);
}

unsigned test_sort_d_1(void *In, void *Context, void *Tls)
{
    (void) Context;
    (void) Tls;
    struct test_sort_d *in = In;
    for (size_t i = 0; i < UINT64_BIT; i++)
    {
        uint64_t x = (UINT64_C(1) << i) + 1, y = x - 2;
        size_t tmp, res_x = MIN(i + 1, in->cnt - 1), res_y = y ? MIN(i - 1, in->cnt - 1) : 0;
        if (!binary_search(&tmp, &x, in->arr, in->cnt, sizeof(*in->arr), uint64_stable_cmp_asc_test, NULL, BINARY_SEARCH_IMPRECISE | BINARY_SEARCH_RIGHTMOST) || tmp != res_x) return 0;
        if (!binary_search(&tmp, &y, in->arr, in->cnt, sizeof(*in->arr), uint64_stable_cmp_asc_test, NULL, BINARY_SEARCH_IMPRECISE) || tmp != res_y) return 0;
    }
    return 1;
}
