#include "np.h"
#include "ll.h"
#include "memory.h"
#include "sort.h"

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

struct generic_cmp_thunk {
    cmp_callback cmp;
    void *context;
};

static bool generic_cmp(const void *A, const void *B, void *Thunk)
{
    struct generic_cmp_thunk *thunk = Thunk;
    return thunk->cmp(*(const void **) A, *(const void **) B, thunk->context);
}

struct array_result pointers(uintptr_t **p_ptr, const void *arr, size_t cnt, size_t stride, cmp_callback cmp, void *context)
{
    uintptr_t *ptr, swp;
    struct array_result res = array_init(&ptr, NULL, cnt, sizeof(*ptr), 0, ARRAY_STRICT);
    if (!res.status) return res;
    for (size_t i = 0, j = 0; i < cnt; ptr[i] = (uintptr_t) arr + j, i++, j += stride);
    quick_sort(ptr, cnt, sizeof(*ptr), generic_cmp, &(struct generic_cmp_thunk) { .cmp = cmp, .context = context }, &swp, sizeof(*ptr));
    *p_ptr = ptr;
    return res;
}

struct generic_cmp_stable_thunk {
    stable_cmp_callback cmp;
    void *context;
};

static bool generic_cmp_stable(const void *A, const void *B, void *Thunk)
{
    struct generic_cmp_stable_thunk *thunk = Thunk;
    const void **a = (void *) A, **b = (void *) B;
    int res = thunk->cmp(*a, *b, thunk->context);
    if (res > 0 || (!res && *a > *b)) return 1;
    return 0;
}

struct array_result pointers_stable(uintptr_t **p_ptr, const void *arr, size_t cnt, size_t stride, stable_cmp_callback cmp, void *context)
{
    uintptr_t *ptr, swp;
    struct array_result res = array_init(&ptr, NULL, cnt, sizeof(*ptr), 0, ARRAY_STRICT);
    if (!res.status) return res;
    for (size_t i = 0, j = 0; i < cnt; ptr[i] = (uintptr_t) arr + j, i++, j += stride);
    quick_sort(ptr, cnt, sizeof(*ptr), generic_cmp_stable, &(struct generic_cmp_stable_thunk) { .cmp = cmp, .context = context }, &swp, sizeof(*ptr));
    *p_ptr = ptr;
    return res;
}

void orders_from_pointers_inplace(uintptr_t *ptr, uintptr_t base, size_t cnt, size_t stride)
{
    for (size_t i = 0; i < cnt; ptr[i] = (ptr[i] - base) / stride, i++);
}

struct array_result orders_stable(uintptr_t **p_ord, const void *arr, size_t cnt, size_t stride, stable_cmp_callback cmp, void *context)
{
    uintptr_t *ord;
    struct array_result res = pointers_stable(&ord, arr, cnt, stride, cmp, context);
    if (!res.status) return res;
    orders_from_pointers_inplace(ord, (uintptr_t) arr, cnt, stride);
    *p_ord = ord;
    return res;
}

struct array_result orders_stable_unique(uintptr_t **p_ord, const void *arr, size_t *p_cnt, size_t stride, stable_cmp_callback cmp, void *context)
{
    size_t cnt = *p_cnt;
    uintptr_t *ord;
    struct array_result res = pointers_stable(&ord, arr, cnt, stride, cmp, context);
    if (!res.status) return res;

    uintptr_t tmp = 0;
    size_t ucnt = 0;    
    if (cnt) tmp = ord[0], ord[ucnt++] = (tmp - (uintptr_t) arr) / stride;
    for (size_t i = 1; i < cnt; i++) if (cmp((const void *) ord[i], (const void *) tmp, context) > 0)
    {
        tmp = ord[i];
        ord[ucnt++] = (tmp - (uintptr_t) arr) / stride;
    }
    
    // Even if the operation fails, the result is still valid
    array_test(&ord, &ucnt, sizeof(*ord), 0, ARRAY_REDUCE, ucnt);
    *p_ord = ord;
    if (*p_cnt) *p_cnt = ucnt;
    return res;
}

void ranks_from_pointers_impl(size_t *rnk, const uintptr_t *ptr, uintptr_t base, size_t cnt, size_t stride)
{
    for (size_t i = 0; i < cnt; rnk[(ptr[i] - base) / stride] = i, i++);
}

struct array_result ranks_from_pointers(size_t **p_rnk, const uintptr_t *ptr, uintptr_t base, size_t cnt, size_t stride)
{
    size_t *rnk;
    struct array_result res = array_init(&rnk, NULL, cnt, sizeof(*rnk), 0, ARRAY_STRICT);
    if (!res.status) return res;
    ranks_from_pointers_impl(rnk, ptr, base, cnt, stride);
    *p_rnk = rnk;
    return res;
}

struct array_result ranks_from_orders(size_t **p_rnk, const uintptr_t *ord, size_t cnt)
{
    return ranks_from_pointers(p_rnk, ord, 0, cnt, 1);
}

struct array_result ranks_unique(size_t **p_rnk, const void *arr, size_t *p_cnt, size_t stride, cmp_callback cmp, void *context)
{
    bool succ = 0;
    size_t *rnk;
    uintptr_t *ptr = NULL;
    struct array_result res = pointers(&ptr, arr, *p_cnt, stride, cmp, context);
    if (!res.status) return res;
    res = ranks_unique_from_pointers(&rnk, ptr, (uintptr_t) arr, p_cnt, stride, cmp, context);
    if (res.status)
    {
        *p_rnk = rnk;
        succ = 1;
    }
    free(ptr);
    return res;
}

// Warning! 'ptr' may contain pointers to the 'rnk' array (i. e. 'rnk' = 'base').
void ranks_unique_from_pointers_impl(size_t *rnk, const uintptr_t *ptr, uintptr_t base, size_t *p_cnt, size_t stride, cmp_callback cmp, void *context)
{
    size_t cnt = *p_cnt;
    if (!cnt) return;
    size_t ucnt = 0;
    for (size_t i = 0; i < cnt - 1; i++)
    {
        size_t tmp = ucnt;
        if (cmp((void *) ptr[i + 1], (void *) ptr[i], context)) ucnt++;
        rnk[(ptr[i] - base) / stride] = tmp;
    }
    rnk[(ptr[cnt - 1] - base) / stride] = ucnt;
    *p_cnt = ucnt + 1;
}

struct array_result ranks_unique_from_pointers(size_t **p_rnk, const uintptr_t *ptr, uintptr_t base, size_t *p_cnt, size_t stride, cmp_callback cmp, void *context)
{
    size_t *rnk;
    struct array_result res = array_init(&rnk, NULL, *p_cnt, sizeof(*rnk), 0, ARRAY_STRICT);
    if (!res.status) return res;
    ranks_unique_from_pointers_impl(rnk, ptr, base, p_cnt, stride, cmp, context);
    *p_rnk = rnk;
    return res;
}

struct cmp_helper_thunk {
    cmp_callback cmp;
    const void *arr;
    size_t stride;
    void *context;
};

static bool cmp_helper(const void *a, const void *b, void *Context)
{
    struct cmp_helper_thunk *context = Context;
    return context->cmp((char *) context->arr + (uintptr_t) a * context->stride, (char *) context->arr + (uintptr_t) b * context->stride, context->context);
}

struct array_result ranks_unique_from_orders(size_t **p_rnk, const uintptr_t *ord, const void *arr, size_t *p_cnt, size_t stride, cmp_callback cmp, void *context)
{
    return ranks_unique_from_pointers(p_rnk, ord, 0, p_cnt, 1, cmp_helper, &(struct cmp_helper_thunk) { .cmp = cmp, .arr = arr, .stride = stride, .context = context });
}

void ranks_from_pointers_inplace_impl(uintptr_t *restrict ptr, uintptr_t base, size_t cnt, size_t stride, uint8_t *restrict bits)
{
    for (size_t i = 0; i < cnt; i++)
    {
        size_t j = i;
        uintptr_t k = (ptr[i] - base) / stride;
        while (!uint8_bit_test(bits, j))
        {
            uintptr_t l = (ptr[k] - base) / stride;
            uint8_bit_set(bits, j);
            ptr[k] = j;
            j = k;
            k = l;
        }
    }
}

struct array_result ranks_from_pointers_inplace(uintptr_t *restrict ptr, uintptr_t base, size_t cnt, size_t stride)
{
    uint8_t *bits = NULL;
    struct array_result res = array_init(&bits, NULL, UINT8_CNT(cnt), sizeof(*bits), 0, ARRAY_STRICT | ARRAY_CLEAR);
    if (!res.status) return res;
    ranks_from_pointers_inplace_impl(ptr, base, cnt, stride, bits);
    free(bits);
    return (struct array_result) { .status = ARRAY_SUCCESS_UNTOUCHED };
}

struct array_result ranks_from_orders_inplace(uintptr_t *restrict ord, size_t cnt)
{
    return ranks_from_pointers_inplace(ord, 0, cnt, 1);
}

struct array_result ranks_stable(uintptr_t **p_rnk, const void *arr, size_t cnt, size_t stride, stable_cmp_callback cmp, void *context)
{
    uintptr_t *rnk;
    struct array_result res0 = pointers_stable(&rnk, arr, cnt, stride, cmp, context);
    if (!res0.status) return res0;
    struct array_result res1 = ranks_from_pointers_inplace(rnk, (uintptr_t) arr, cnt, stride);
    if (res1.status)
    {
        *p_rnk = rnk;
        return res0;
    }
    free(rnk);
    return res1;
}

void orders_apply_impl(uintptr_t *restrict ord, size_t cnt, size_t sz, void *restrict arr, uint8_t *restrict bits, void *restrict swp, size_t stride)
{
    for (size_t i = 0; i < cnt; i++)
    {
        if (uint8_bit_test(bits, i)) continue;
        size_t k = ord[i];
        if (k == i) continue;
        uint8_bit_set(bits, i);
        memcpy(swp, (char *) arr + i * stride, sz);
        for (size_t j = i;;)
        {
            while (k < cnt && uint8_bit_test(bits, k)) k = ord[k];
            memcpy((char *) arr + j * stride, (char *) arr + k * stride, sz);
            if (k >= cnt)
            {
                memcpy((char *) arr + k * stride, swp, sz);
                break;
            }
            j = k;
            k = ord[j];
            uint8_bit_set(bits, j);
            if (k == i)
            {
                memcpy((char *) arr + j * stride, swp, sz);
                break;
            }
        }
    }
}

// This procedure applies orders to the array. Orders are not assumed to be a surjective map. 
struct array_result orders_apply(uintptr_t *restrict ord, size_t cnt, size_t sz, void *restrict arr, void *restrict swp, size_t stride)
{
    uint8_t *bits = NULL;
    struct array_result res = array_init(&bits, NULL, UINT8_CNT(cnt), sizeof(*bits), 0, ARRAY_STRICT | ARRAY_CLEAR);
    if (!res.status) return res;
    orders_apply_impl(ord, cnt, sz, arr, bits, swp, stride);
    free(bits);
    return (struct array_result) { .status = ARRAY_SUCCESS_UNTOUCHED };
}

static void swap(void *restrict a, void *restrict b, void *restrict swp, size_t sz)
{
    memcpy(swp, a, sz);
    memcpy(a, b, sz);
    memcpy(b, swp, sz);
}

#ifndef QUICK_SORT_CACHED
static void lin_sort_stub(void *restrict arr, size_t tot, size_t sz, cmp_callback cmp, void *context, void *restrict swp, size_t stride, size_t cutoff)
{
    (void) arr, (void) tot, (void) sz, (void) cmp, (void) context, (void) swp, (void) stride, (void) cutoff;
}
#endif

static void insertion_sort_impl(void *restrict arr, size_t tot, size_t sz, cmp_callback cmp, void *context, void *restrict swp, size_t stride, size_t cutoff)
{
    size_t min = 0;
    for (size_t i = stride; i < cutoff; i += stride) if (cmp((char *) arr + min, (char *) arr + i, context)) min = i;
    if (min) swap((char *) arr + min, (char *) arr, swp, sz);
    for (size_t i = stride + stride; i < tot; i += stride)
    {
        size_t j = i;
        if (cmp((char *) arr + j - stride, (char *) arr + j, context)) // First iteration is unrolled
        {
            j -= stride;
            memcpy(swp, (char *) arr + j, sz);
            for (; j > stride && cmp((char *) arr + j - stride, (char *) arr + i, context); j -= stride) memcpy((char *) arr + j, (char *) arr + j - stride, sz);
            memcpy((char *) arr + j, (char *) arr + i, sz);
            memcpy((char *) arr + i, swp, sz);
        }
    }
}

typedef void (*lin_sort_callback)(void *restrict, size_t, size_t, cmp_callback, void *, void *restrict, size_t, size_t);
typedef void (*sort_callback)(void *restrict, size_t, size_t, cmp_callback, void *, void *restrict, size_t, size_t, lin_sort_callback);

static size_t comb_sort_gap_impl(size_t tot, size_t stride)
{
    size_t gap = (size_t) (.8017118471377937539 * (tot / stride)); // Magic constant suggested by 'ksort.h'. Any positive value < 1.0 is acceptable 
    if (gap == 9 || gap == 10) gap = 11;
    return gap * stride;
}

static void comb_sort_impl(void *restrict arr, size_t tot, size_t sz, cmp_callback cmp, void *context, void *restrict swp, size_t stride, size_t lin_cutoff, lin_sort_callback lin_sort)
{
    size_t gap = comb_sort_gap_impl(tot, stride);
    for (bool flag = 0;; flag = 0, gap = comb_sort_gap_impl(gap, stride))
    {
        for (size_t i = 0, j = gap; i < tot - gap; i += stride, j += stride)
            if (cmp((char *) arr + i, (char *) arr + j, context)) swap((char *) arr + i, (char *) arr + j, swp, sz), flag = 1;
        if (gap == stride) return;
        if (!flag || gap <= lin_cutoff) break;
    }
    lin_sort(arr, tot, sz, cmp, context, swp, stride, gap);
}

static void quick_sort_impl(void *restrict arr, size_t tot, size_t sz, cmp_callback cmp, void *context, void *restrict swp, size_t stride, size_t lin_cutoff, lin_sort_callback lin_sort, size_t log_cutoff, sort_callback log_sort)
{
    struct frm { size_t a, b; } *stk = Alloca(log_cutoff * sizeof(*stk));
    size_t frm = 0, a = 0, b = tot - stride;
    for (;;)
    {
        size_t left = a, right = b, pvt = a + ((b - a) / stride >> 1) * stride;
        if (cmp((char *) arr + left, (char *) arr + pvt, context)) swap((char *) arr + left, (char *) arr + pvt, swp, sz);
        if (cmp((char *) arr + pvt, (char *) arr + right, context))
        {
            swap((char *) arr + pvt, (char *) arr + right, swp, sz);
            if (cmp((char *) arr + left, (char *) arr + pvt, context)) swap((char *) arr + left, (char *) arr + pvt, swp, sz);
        }
        left += stride;
        right -= stride;
        do {
            while (cmp((char *) arr + pvt, (char *) arr + left, context)) left += stride;
            while (cmp((char *) arr + right, (char *) arr + pvt, context)) right -= stride;
            if (left == right)
            {
                left += stride;
                right -= stride;
                break;
            }
            else if (left > right) break;
            swap((char *) arr + left, (char *) arr + right, swp, sz);
            if (left == pvt) pvt = right;
            else if (pvt == right) pvt = left;
            left += stride;
            right -= stride;
        } while (left <= right);
        if (right - a < lin_cutoff)
        {
            size_t tmp = right - a + stride;
            lin_sort((char *) arr + a, tmp, sz, cmp, context, swp, stride, tmp);
            if (b - left < lin_cutoff)
            {
                tmp = b - left + stride;
                lin_sort((char *) arr + left, tmp, sz, cmp, context, swp, stride, tmp);
                if (!frm--) break;
                a = stk[frm].a;
                b = stk[frm].b;
            }
            else a = left;
        }
        else if (b - left < lin_cutoff)
        {
            size_t tmp = b - left + stride;
            lin_sort((char *) arr + left, tmp, sz, cmp, context, swp, stride, tmp);
            b = right;
        }
        else
        {
            if (right - a > b - left)
            {
                if (frm < log_cutoff) stk[frm++] = (struct frm) { .a = a, .b = right };
                else log_sort((char *) arr + a, right - a + stride, sz, cmp, context, swp, stride, lin_cutoff, lin_sort);
                a = left;
            }
            else
            {
                if (frm < log_cutoff) stk[frm++] = (struct frm) { .a = left, .b = b };
                else log_sort((char *) arr + left, b - left + stride, sz, cmp, context, swp, stride, lin_cutoff, lin_sort);
                b = right;
            }
        }
    }
}

void quick_sort(void *restrict arr, size_t cnt, size_t sz, cmp_callback cmp, void *context, void *restrict swp, size_t stride)
{
    if (cnt < 2) return;
    if (cnt > 2)
    {
        size_t tot = cnt * stride;
        if (cnt > QUICK_SORT_CUTOFF)
        {
            size_t lin_cutoff = QUICK_SORT_CUTOFF * stride, log_cutoff = size_sub_sat(size_log2(cnt, 1), size_bit_scan_reverse(QUICK_SORT_CUTOFF) << 1);
#       ifdef QUICK_SORT_CACHED
            quick_sort_impl(arr, tot, sz, cmp, context, swp, stride, lin_cutoff, insertion_sort_impl, log_cutoff, comb_sort_impl);
#       else
            quick_sort_impl(arr, tot, sz, cmp, context, swp, stride, lin_cutoff, lin_sort_stub, log_cutoff, comb_sort_impl);
            insertion_sort_impl(arr, tot, sz, cmp, context, swp, stride, lin_cutoff);
#       endif 
        }
        else insertion_sort_impl(arr, tot, sz, cmp, context, swp, stride, tot);
    }
    else if (cmp(arr, (char *) arr + stride, context)) swap(arr, (char *) arr + stride, swp, sz);
}

void sort_unique(void *restrict arr, size_t *p_cnt, size_t sz, cmp_callback cmp, void *context, void *restrict swp, size_t stride)
{
    size_t cnt = *p_cnt;
    quick_sort(arr, cnt, sz, cmp, context, swp, stride);
    if (!cnt) return;
    size_t ucnt = 1, tot = cnt * stride;
    for (size_t i = stride, j = i; i < tot; i += stride)
    {
        if (!cmp((char *) arr + i, (char *) arr + i - stride, context)) continue;
        if (i > j) memcpy((char *) arr + j, (char *) arr + i, sz);
        j += stride;
        ucnt++;
    }
    *p_cnt = ucnt;
}

bool binary_search(size_t *p_ind, const void *key, const void *arr, size_t cnt, size_t sz, stable_cmp_callback cmp, void *context, enum binary_search_flags flags)
{
    if (!cnt) return 0;
    bool rightmost = flags & BINARY_SEARCH_RIGHTMOST;
    size_t left = 0, right = cnt - 1;
    int res = 0;
    while (left < right)
    {
        size_t mid = left + ((right - left + !rightmost) >> 1);
        res = cmp(key, (char *) arr + sz * mid, context);
        if (res > 0) left = mid + rightmost;
        else if (res < 0) right = mid - !rightmost;
        else if (flags & BINARY_SEARCH_CRITICAL)
        {
            if (rightmost) left = mid;
            else right = mid;
            while (left < right)
            {
                mid = left + ((right - left + rightmost) >> 1);
                res = cmp(key, (char *) arr + sz * mid, context);
                if (res > 0) left = mid + 1;
                else if (res < 0) right = mid - 1;
                else if (rightmost) left = mid;
                else right = mid;
            }
            *p_ind = left;
            return 1;
        }
        else
        {
            *p_ind = mid;
            return 1;            
        }
    }
    if (!(flags & BINARY_SEARCH_IMPRECISE) && cmp(key, (char *) arr + sz * left, context)) return 0;
    *p_ind = left;
    return 1;
}

enum {
    FLAG_REMOVED = 1,
    FLAG_NOT_EMPTY = 2
};

struct array_result hash_table_init(struct hash_table *tbl, size_t cnt, size_t szk, size_t szv)
{
    size_t lcnt = size_log2(size_add_sat(cnt, 3), 1);
    if (lcnt == SIZE_BIT) return (struct array_result) { .error = ARRAY_OVERFLOW };
    cnt = (size_t) 1 << lcnt;
    struct array_result res = array_init(&tbl->key, NULL, cnt, szk, 0, ARRAY_STRICT);
    if (res.status)
    {
        size_t tot = res.tot;
        res = array_init(&tbl->val, NULL, cnt, szv, 0, ARRAY_STRICT);
        if (res.status)
        {
            tot = size_add_sat(tot, res.tot);
            res = array_init(&tbl->flags, NULL, NIBBLE_CNT(cnt), sizeof(*tbl->flags), 0, ARRAY_CLEAR | ARRAY_STRICT);
            if (res.status)
            {
                tbl->cnt = tbl->tot = tbl->lim = 0;
                tbl->lcap = lcnt;
                return (struct array_result) { .status = ARRAY_SUCCESS, .tot = size_add_sat(tot, res.tot) };
            }
            free(tbl->val);
        }
        free(tbl->key);
    }
    return res;
}

void hash_table_close(struct hash_table *tbl)
{
    free(tbl->flags);
    free(tbl->val);
    free(tbl->key);
}

bool hash_table_search(struct hash_table *tbl, size_t *p_h, const void *key, size_t szk, cmp_callback eq, void *context)
{
    if (!tbl->cnt) return 0;
    size_t msk = ((size_t) 1 << tbl->lcap) - 1, h = *p_h & msk;
    for (size_t i = 0, j = h;;)
    {
        uint8_t flags = uint8_bit_fetch_burst2(tbl->flags, h);
        if (!(flags & FLAG_NOT_EMPTY)) return 0;
        if (!(flags & FLAG_REMOVED) && eq((char *) tbl->key + h * szk, key, context)) break;
        h = (h + ++i) & msk;
        if (h == j) return 0;
    }
    *p_h = h;
    return 1;
}

void hash_table_dealloc(struct hash_table *tbl, size_t h)
{
    uint8_bit_set_burst2(tbl->flags, h, FLAG_REMOVED);
    tbl->cnt--;
}

bool hash_table_remove(struct hash_table *tbl, size_t h, const void *key, size_t szk, cmp_callback eq, void *context)
{
    if (!(hash_table_search(tbl, &h, key, szk, eq, context))) return 0;
    hash_table_dealloc(tbl, h);
    return 1;
}

void *hash_table_fetch_key(struct hash_table *tbl, size_t h, size_t szk)
{
    return (char *) tbl->key + h * szk;
}

void *hash_table_fetch_val(struct hash_table *tbl, size_t h, size_t szv)
{
    return (char *) tbl->val + h * szv;
}

static struct array_result hash_table_rehash(struct hash_table *tbl, size_t lcnt, size_t szk, size_t szv, hash_callback hash, void *context, void *restrict swpk, void *restrict swpv)
{
    size_t cnt = (size_t) 1 << lcnt, msk = cnt - 1, cap = (size_t) 1 << tbl->lcap;
    uint8_t *flags;
    struct array_result res = array_init(&flags, NULL, NIBBLE_CNT(cnt), sizeof(*flags), 0, ARRAY_CLEAR | ARRAY_STRICT);
    if (!res.status) return res;
    for (size_t i = 0; i < cap; i++)
    {
        if (uint8_bit_fetch_burst2(tbl->flags, i) != FLAG_NOT_EMPTY) continue;
        uint8_bit_set_burst2(tbl->flags, i, FLAG_REMOVED);
        for (;;)
        {
            size_t h = hash((char *) tbl->key + i * szk, context) & msk;
            for (size_t j = 0; uint8_bit_fetch_set_burst2(flags, h, FLAG_NOT_EMPTY); h = (h + ++j) & msk);
            if (i == h) break;
            if (h >= cap || uint8_bit_fetch_burst2(tbl->flags, h) != FLAG_NOT_EMPTY)
            {
                memcpy((char *) tbl->key + h * szk, (char *) tbl->key + i * szk, szk);
                memcpy((char *) tbl->val + h * szv, (char *) tbl->val + i * szv, szv);
                break;
            }            
            uint8_bit_set_burst2(tbl->flags, h, FLAG_REMOVED);
            swap((char *) tbl->key + i * szk, (char *) tbl->key + h * szk, swpk, szk);
            swap((char *) tbl->val + i * szv, (char *) tbl->val + h * szv, swpv, szv);
        }
    }
    free(tbl->flags);
    tbl->flags = flags;
    tbl->lcap = lcnt;
    tbl->tot = tbl->cnt;
    tbl->lim = (size_t) ((double) cnt * .77 + .5); // Magic constants from the 'khash.h'
    return res;
}

struct array_result hash_table_alloc(struct hash_table *tbl, size_t *p_h, const void *key, size_t szk, size_t szv, hash_callback hash, cmp_callback eq, void *context, void *restrict swpk, void *restrict swpv)
{
    struct array_result res = { .status = 1 };
    size_t cap = (size_t) 1 << tbl->lcap;
    if (tbl->tot >= tbl->lim)
    {
        size_t lcnt;
        if ((cap >> 1) <= tbl->cnt)
        {
            lcnt = size_log2(size_add_sat(cap, 1), 1);
            if (lcnt == SIZE_BIT) return (struct array_result) { .error = ARRAY_OVERFLOW };
        }
        else lcnt = cap > 5 ? size_log2(cap - 1, 1) : 2;
        size_t cnt = (size_t) 1 << lcnt;
        if (tbl->cnt >= (size_t) ((double) cnt * .77 + .5) || cap == cnt) res.status |= HASH_UNTOUCHED; // Magic constants from the 'khash.h'
        else
        {
            if (cap < cnt)
            {
                res = array_init(&tbl->key, NULL, cnt, szk, 0, ARRAY_STRICT | ARRAY_REALLOC);
                if (!res.status) return res;

                size_t tot = res.tot;
                res = array_init(&tbl->val, NULL, cnt, szv, 0, ARRAY_STRICT | ARRAY_REALLOC);
                res.tot = size_add_sat(tot, res.tot);
                if (!res.status) return res;
                
                tot = res.tot;
                res = hash_table_rehash(tbl, lcnt, szk, szv, hash, context, swpk, swpv);
                res.tot = size_add_sat(tot, res.tot);
                if (!res.status) return res;                
            }
            else
            {
                res = hash_table_rehash(tbl, lcnt, szk, szv, hash, context, swpk, swpv);
                if (!res.status) return res;
                
                size_t tot = res.tot;
                res = array_init(&tbl->key, NULL, cnt, szk, 0, ARRAY_STRICT | ARRAY_REALLOC | ARRAY_FAILSAFE);
                res.tot = size_add_sat(tot, res.tot);
                if (!res.status) return res;
                
                tot = res.tot;
                res = array_init(&tbl->val, NULL, cnt, szv, 0, ARRAY_STRICT | ARRAY_REALLOC | ARRAY_FAILSAFE);
                res.tot = size_add_sat(tot, res.tot);
                if (!res.status) return res;
            }
            cap = cnt; // Update capacity after rehashing
        }
    }
    else res.status |= HASH_UNTOUCHED;
    size_t msk = cap - 1, h = *p_h & msk;
    for (size_t i = 0, j = h, tmp = cap;;)
    {
        uint8_t flags = uint8_bit_fetch_burst2(tbl->flags, h);
        if (!(flags & FLAG_NOT_EMPTY)) break;
        if (flags & FLAG_REMOVED) tmp = h;
        else if (eq((char *) tbl->key + h * szk, key, context))
        {
            *p_h = h;
            res.status |= HASH_PRESENT;
            return res;
        }
        h = (h + ++i) & msk;
        if (h != j) continue;
        if (tmp != cap) h = tmp;        
        break;
    }
    if (!uint8_bit_fetch_reset_burst2(tbl->flags, h, FLAG_REMOVED)) tbl->tot++;
    uint8_bit_set_burst2(tbl->flags, h, FLAG_NOT_EMPTY);
    tbl->cnt++;
    *p_h = h;
    return res;
}
