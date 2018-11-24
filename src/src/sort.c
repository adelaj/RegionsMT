#include "np.h"
#include "ll.h"
#include "memory.h"
#include "sort.h"

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

struct generic_cmp_thunk {
    stable_cmp_callback cmp;
    void *context;
};

static bool generic_cmp(const void *A, const void *B, void *Thunk)
{
    struct generic_cmp_thunk *thunk = Thunk;
    const void **a = (void *) A, **b = (void *) B;
    int res = thunk->cmp(*a, *b, thunk->context);
    if (res > 0 || (!res && *a > *b)) return 1;
    return 0;
}

uintptr_t *pointers_stable(const void *arr, size_t cnt, size_t sz, stable_cmp_callback cmp, void *context)
{
    uintptr_t *res = NULL;
    if (!array_init(&res, NULL, cnt, sizeof(*res), 0, ARRAY_STRICT)) return NULL;
    for (size_t i = 0; i < cnt; res[i] = (uintptr_t) arr + i * sz, i++);
    quick_sort(res, cnt, sizeof(*res), generic_cmp, &(struct generic_cmp_thunk) { .cmp = cmp, .context = context });
    return res;
}

void orders_from_pointers_inplace(uintptr_t *ptr, uintptr_t base, size_t cnt, size_t sz)
{
    for (size_t i = 0; i < cnt; ptr[i] = (ptr[i] - base) / sz, i++);
}

uintptr_t *orders_stable(const void *arr, size_t cnt, size_t sz, stable_cmp_callback cmp, void *context)
{
    uintptr_t *res = pointers_stable(arr, cnt, sz, cmp, context);
    if (!res) return NULL;
    orders_from_pointers_inplace(res, (uintptr_t) arr, cnt, sz);
    return res;
}

uintptr_t *orders_stable_unique(const void *arr, size_t *p_cnt, size_t sz, stable_cmp_callback cmp, void *context)
{
    size_t cnt = *p_cnt;
    uintptr_t *res = pointers_stable(arr, cnt, sz, cmp, context);
    if (!res) return NULL;

    uintptr_t tmp = 0;
    size_t ucnt = 0;    
    if (cnt) tmp = res[0], res[ucnt++] = (tmp - (uintptr_t) arr) / sz;
    for (size_t i = 1; i < cnt; i++)
        if (cmp((const void *) res[i], (const void *) tmp, context) > 0) tmp = res[i], res[ucnt++] = (tmp - (uintptr_t) arr) / sz;
    if (array_test(&res, p_cnt, sizeof(*res), 0, ARRAY_REDUCE, ucnt)) return res;

    free(res);
    *p_cnt = 0;
    return NULL;
}

void ranks_from_pointers_impl(size_t *rnk, const uintptr_t *ptr, uintptr_t base, size_t cnt, size_t sz)
{
    for (size_t i = 0; i < cnt; rnk[(ptr[i] - base) / sz] = i, i++);
}

size_t *ranks_from_pointers(const uintptr_t *ptr, uintptr_t base, size_t cnt, size_t sz)
{
    size_t *res = NULL;
    if (!array_init(&res, NULL, cnt, sizeof(*res), 0, ARRAY_STRICT)) return NULL;
    ranks_from_pointers_impl(res, ptr, base, cnt, sz);
    return res;
}

size_t *ranks_from_orders(const uintptr_t *ord, size_t cnt)
{
    return ranks_from_pointers(ord, 0, cnt, 1);
}

// Warning! 'ptr' may contain pointers to the 'rnk' array (i. e. 'rnk' = 'base').
void ranks_unique_from_pointers_impl(size_t *rnk, const uintptr_t *ptr, uintptr_t base, size_t *p_cnt, size_t sz, cmp_callback cmp, void *context)
{
    size_t cnt = *p_cnt;
    if (!cnt) return;
    size_t ucnt = 0;
    for (size_t i = 0; i < cnt - 1; i++)
    {
        size_t tmp = ucnt;
        if (cmp((void *) ptr[i + 1], (void *) ptr[i], context)) ucnt++;
        rnk[(ptr[i] - base) / sz] = tmp;
    }
    rnk[(ptr[cnt - 1] - base) / sz] = ucnt;
    *p_cnt = ucnt + 1;
}

size_t *ranks_unique_from_pointers(const uintptr_t *ptr, uintptr_t base, size_t *p_cnt, size_t sz, cmp_callback cmp, void *context)
{
    size_t *res = NULL;
    if (!array_init(&res, NULL, *p_cnt, sizeof(*res), 0, ARRAY_STRICT)) return NULL;
    ranks_unique_from_pointers_impl(res, ptr, base, p_cnt, sz, cmp, context);
    return res;
}

struct cmp_helper_thunk {
    cmp_callback cmp;
    const void *arr;
    size_t sz;
    void *context;
};

static bool cmp_helper(const void *a, const void *b, void *Context)
{
    struct cmp_helper_thunk *context = Context;
    return context->cmp((char *) context->arr + (uintptr_t) a * context->sz, (char *) context->arr + (uintptr_t) b * context->sz, context->context);
}

size_t *ranks_unique_from_orders(const uintptr_t *ord, const void *arr, size_t *p_cnt, size_t sz, cmp_callback cmp, void *context)
{
    return ranks_unique_from_pointers(ord, 0, p_cnt, 1, cmp_helper, &(struct cmp_helper_thunk) { .cmp = cmp, .arr = arr, .sz = sz, .context = context });
}

void ranks_from_pointers_inplace_impl(uintptr_t *restrict ptr, uintptr_t base, size_t cnt, size_t sz, uint8_t *restrict bits)
{
    for (size_t i = 0; i < cnt; i++)
    {
        size_t j = i;
        uintptr_t k = (ptr[i] - base) / sz;
        while (!uint8_bit_test(bits, j))
        {
            uintptr_t l = (ptr[k] - base) / sz;
            uint8_bit_set(bits, j);
            ptr[k] = j;
            j = k;
            k = l;
        }
    }
}

bool ranks_from_pointers_inplace(uintptr_t *restrict ptr, uintptr_t base, size_t cnt, size_t sz)
{
    uint8_t *bits = NULL;
    if (!array_init(&bits, NULL, UINT8_CNT(cnt), sizeof(*bits), 0, ARRAY_STRICT | ARRAY_CLEAR)) return 0;
    ranks_from_pointers_inplace_impl(ptr, base, cnt, sz, bits);
    free(bits);
    return 1;
}

bool ranks_from_orders_inplace(uintptr_t *restrict ord, size_t cnt)
{
    return ranks_from_pointers_inplace(ord, 0, cnt, 1);
}

uintptr_t *ranks_stable(const void *arr, size_t cnt, size_t sz, stable_cmp_callback cmp, void *context)
{
    uintptr_t *res = pointers_stable(arr, cnt, sz, cmp, context);
    if (!res) return NULL;
    if (ranks_from_pointers_inplace(res, (uintptr_t) arr, cnt, sz)) return res;
    free(res);
    return NULL;
}

void orders_apply_impl(uintptr_t *restrict ord, size_t cnt, size_t sz, void *restrict arr, uint8_t *restrict bits, void *restrict swp)
{
    for (size_t i = 0; i < cnt; i++)
    {
        if (uint8_bit_test(bits, i)) continue;
        size_t k = ord[i];
        if (k == i) continue;
        uint8_bit_set(bits, i);
        memcpy(swp, (char *) arr + i * sz, sz);
        for (size_t j = i;;)
        {
            while (k < cnt && uint8_bit_test(bits, k)) k = ord[k];
            memcpy((char *) arr + j * sz, (char *) arr + k * sz, sz);
            if (k >= cnt)
            {
                memcpy((char *) arr + k * sz, swp, sz);
                break;
            }
            j = k;
            k = ord[j];
            uint8_bit_set(bits, j);
            if (k == i)
            {
                memcpy((char *) arr + j * sz, swp, sz);
                break;
            }
        }
    }
}

// This procedure applies orders to the array. Orders are not assumed to be a surjective map. 
bool orders_apply(uintptr_t *restrict ord, size_t cnt, size_t sz, void *restrict arr)
{
    uint8_t *bits = NULL;
    if (!array_init(&bits, NULL, UINT8_CNT(cnt), sizeof(*bits), 0, ARRAY_STRICT | ARRAY_CLEAR)) return 0;
    orders_apply_impl(ord, cnt, sz, arr, bits, Alloca(sz));
    free(bits);
    return 1;
}

static void swap(void *restrict a, void *restrict b, void *restrict swp, size_t sz)
{
    memcpy(swp, a, sz);
    memcpy(a, b, sz);
    memcpy(b, swp, sz);
}

static void lin_sort_stub(void *restrict arr, size_t tot, size_t sz, cmp_callback cmp, void *context, void *restrict swp, size_t cutoff)
{
    (void) arr, (void) tot, (void) sz, (void) cmp, (void) context, (void) swp, (void) cutoff;
}

static void insertion_sort_impl(void *restrict arr, size_t tot, size_t sz, cmp_callback cmp, void *context, void *restrict swp, size_t cutoff)
{
    size_t min = 0;
    for (size_t i = sz; i < cutoff; i += sz) if (cmp((char *) arr + min, (char *) arr + i, context)) min = i;
    if (min) swap((char *) arr + min, (char *) arr, swp, sz);
    for (size_t i = sz + sz; i < tot; i += sz)
    {
        size_t j = i;
        if (cmp((char *) arr + j - sz, (char *) arr + j, context)) // First iteration is unrolled
        {
            j -= sz;
            memcpy(swp, (char *) arr + j, sz);
            for (; j > sz && cmp((char *) arr + j - sz, (char *) arr + i, context); j -= sz) memcpy((char *) arr + j, (char *) arr + j - sz, sz);
            memcpy((char *) arr + j, (char *) arr + i, sz);
            memcpy((char *) arr + i, swp, sz);
        }
    }
}

typedef void (*lin_sort_callback)(void *restrict, size_t, size_t, cmp_callback, void *, void *restrict, size_t);
typedef void (*sort_callback)(void *restrict, size_t, size_t, cmp_callback, void *, void *restrict, size_t, lin_sort_callback);

static size_t comb_sort_gap_impl(size_t tot, size_t sz)
{
    size_t gap = (size_t) (.8017118471377937539 * (tot / sz)); // Magic constant suggested by 'ksort.h'. Generally any positive value < 1.0 is acceptable 
    if (gap == 9 || gap == 10) gap = 11;
    return gap * sz;
}

static void comb_sort_impl(void *restrict arr, size_t tot, size_t sz, cmp_callback cmp, void *context, void *restrict swp, size_t lin_cutoff, lin_sort_callback lin_sort)
{
    size_t gap = comb_sort_gap_impl(tot, sz);
    for (bool flag = 0;; flag = 0, gap = comb_sort_gap_impl(gap, sz))
    {
        for (size_t i = 0, j = gap; i < tot - gap; i += sz, j += sz)
            if (cmp((char *) arr + i, (char *) arr + j, context)) swap((char *) arr + i, (char *) arr + j, swp, sz), flag = 1;
        if (gap == sz) return;
        if (!flag || gap <= lin_cutoff) break;
    }
    lin_sort(arr, tot, sz, cmp, context, swp, gap);
}

static void quick_sort_impl(void *restrict arr, size_t tot, size_t sz, cmp_callback cmp, void *context, void *restrict swp, size_t lin_cutoff, lin_sort_callback lin_sort, size_t log_cutoff, sort_callback log_sort)
{
    struct frm { size_t a, b; } *stk = Alloca(log_cutoff * sizeof(*stk));
    size_t frm = 0, a = 0, b = tot - sz;
    for (;;)
    {
        size_t left = a, right = b, pvt = a + ((b - a) / sz >> 1) * sz;
        if (cmp((char *) arr + left, (char *) arr + pvt, context)) swap((char *) arr + left, (char *) arr + pvt, swp, sz);
        if (cmp((char *) arr + pvt, (char *) arr + right, context))
        {
            swap((char *) arr + pvt, (char *) arr + right, swp, sz);
            if (cmp((char *) arr + left, (char *) arr + pvt, context)) swap((char *) arr + left, (char *) arr + pvt, swp, sz);
        }
        left += sz;
        right -= sz;
        do {
            while (cmp((char *) arr + pvt, (char *) arr + left, context)) left += sz;
            while (cmp((char *) arr + right, (char *) arr + pvt, context)) right -= sz;
            if (left == right)
            {
                left += sz;
                right -= sz;
                break;
            }
            else if (left > right) break;
            swap((char *) arr + left, (char *) arr + right, swp, sz);
            if (left == pvt) pvt = right;
            else if (pvt == right) pvt = left;
            left += sz;
            right -= sz;
        } while (left <= right);
        if (right - a < lin_cutoff)
        {
            size_t tmp = right - a + sz;
            lin_sort((char *) arr + a, tmp, sz, cmp, context, swp, tmp);
            if (b - left < lin_cutoff)
            {
                tmp = b - left + sz;
                lin_sort((char *) arr + left, tmp, sz, cmp, context, swp, tmp);
                if (!frm--) break;
                a = stk[frm].a;
                b = stk[frm].b;
            }
            else a = left;
        }
        else if (b - left < lin_cutoff)
        {
            size_t tmp = b - left + sz;
            lin_sort((char *) arr + left, tmp, sz, cmp, context, swp, tmp);
            b = right;
        }
        else
        {
            if (right - a > b - left)
            {
                if (frm < log_cutoff) stk[frm++] = (struct frm) { .a = a, .b = right };
                else log_sort((char *) arr + a, right - a + sz, sz, cmp, context, swp, lin_cutoff, lin_sort);
                a = left;
            }
            else
            {
                if (frm < log_cutoff) stk[frm++] = (struct frm) { .a = left, .b = b };
                else log_sort((char *) arr + left, b - left + sz, sz, cmp, context, swp, lin_cutoff, lin_sort);
                b = right;
            }
        }
    }
}

void quick_sort(void *restrict arr, size_t cnt, size_t sz, cmp_callback cmp, void *context)
{
    if (cnt < 2) return;
    void *restrict swp = Alloca(sz);
    if (cnt > 2)
    {
        const size_t lin_cutoff = QUICK_SORT_CUTOFF * sz, tot = cnt * sz;
        if (tot > lin_cutoff)
        {
            size_t log_cutoff = size_sub_sat(size_log2_ceiling(cnt), size_bit_scan_reverse(QUICK_SORT_CUTOFF) << 1);
#       ifdef QUICK_SORT_CACHED
            quick_sort_impl(arr, tot, sz, cmp, context, swp, lin_cutoff, insertion_sort_impl, log_cutoff, comb_sort_impl);
#       else
            quick_sort_impl(arr, tot, sz, cmp, context, swp, lin_cutoff, lin_sort_stub, log_cutoff, comb_sort_impl);
            insertion_sort_impl(arr, tot, sz, cmp, context, swp, lin_cutoff);
#       endif 
        }
        else insertion_sort_impl(arr, tot, sz, cmp, context, swp, tot);
    }
    else if (cmp(arr, (char *) arr + sz, context)) swap(arr, (char *) arr + sz, swp, sz);
}

bool binary_search(size_t *p_ind, const void *key, const void *arr, size_t cnt, size_t sz, stable_cmp_callback cmp, void *context, enum binary_search_flags flags)
{
    if (!cnt) return 0;
    bool critical = !!(flags & BINARY_SEARCH_CRITICAL), rightmost = !!(flags & BINARY_SEARCH_RIGHTMOST);
    size_t left = 0, right = cnt - 1;
    int res = 0;
    while (left < right) for (;;)
    {
        size_t mid = left + ((right - left + rightmost) >> 1);
        res = cmp(key, (char *) arr + sz * mid, context);
        if (res > 0) left = mid + 1;
        else if (res < 0) right = mid - 1;
        else
        {
            if (critical)
            {
                if (rightmost) left = mid;
                else right = mid;
                if (left < right) continue;
            }
            *p_ind = mid;
            return 1;
        }
        break;
    }
    size_t ind = rightmost ? left : right;
    if ((flags & BINARY_SEARCH_APPROX) || cmp(key, (char *) arr + sz * ind, context)) return 0;
    *p_ind = ind;
    return 1;
}
