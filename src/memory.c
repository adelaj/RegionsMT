#include "np.h"
#include "ll.h"
#include "memory.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

void array_broadcast(void *arr, size_t cnt, size_t sz, void *val)
{
    size_t tot = cnt * sz;
    for (size_t i = 0; i < tot; i += sz) memcpy((char *) arr + i, val, sz);
}

struct array_init_status {
    size_t tot;
    unsigned status;
};

// 'p_cap' -- pointer to initial capacity
// 'cnt' -- desired capacity
// On error 'p_Src' and 'p_cap' are untouched
unsigned array_init(void *p_Src, size_t *restrict p_cap, size_t cnt, size_t sz, size_t diff, enum array_flags flags)
{
    void **restrict p_src = p_Src, *src = *p_src;
    if (src && p_cap && (flags & ARRAY_REALLOC))
    {
        size_t bor, tmp = size_sub(&bor, cnt, *p_cap);
        if (bor) // cnt < *p_cap
        {
            if (!(flags & ARRAY_REDUCE)) return 1 | ARRAY_UNTOUCHED;
            size_t tot = cnt * sz + diff; // No checks for overflows
            void *res = realloc(src, tot); // Array shrinking
            if (tot && !res) return 0;
            *p_cap = cnt;
            *p_src = res;
            return 1;
        }
        else if (!tmp) return 1 | ARRAY_UNTOUCHED; // cnt == cap            
    }
    if (!(flags & ARRAY_STRICT) && cnt)
    {
        size_t log2 = size_log2(cnt, 1);
        cnt = log2 == SIZE_BIT ? cnt : (size_t) 1 << log2;
    }
    size_t tot;
    if ((flags & ARRAY_STRICT) && (flags & ARRAY_FAILSAFE)) tot = cnt * sz + diff;
    else
    {
        tot = cnt;
        if (!size_mul_add_test(&tot, sz, diff))
        {
            errno = ERANGE;
            return 0;
        }
    }
    void *res;
    if (src && (flags & ARRAY_REALLOC))
    {
        res = realloc(src, tot);
        if (res && p_cap && (flags & ARRAY_CLEAR))
        {
            size_t off = *p_cap * sz + diff;
            memset((char *) res + off, 0, tot - off);
        }
    }
    else res = flags & ARRAY_CLEAR ? calloc(tot, 1) : malloc(tot);
    if (tot && !res) return 0;
    *p_src = res;
    if (p_cap) *p_cap = cnt;
    return 1;
}

unsigned array_test_impl(void *p_Arr, size_t *restrict p_cap, size_t sz, size_t diff, enum array_flags flags, size_t *restrict arg, size_t cnt)
{
    size_t car, sum = size_sum(&car, arg, cnt);
    if (!car) return array_init(p_Arr, p_cap, sum, sz, diff, flags | ARRAY_REALLOC);
    errno = ERANGE;
    return 0;
}

bool queue_init(struct queue *queue, size_t cnt, size_t sz)
{
    size_t cap;
    if (!array_init(&queue->arr, &cap, cnt, sz, 0, 0)) return 0;
    queue->cap = cap;
    queue->begin = queue->cnt = 0;
    return 1;
}

void queue_close(struct queue *queue)
{
    free(queue->arr);
}

unsigned queue_test(struct queue *queue, size_t diff, size_t sz)
{
    size_t cap = queue->cap;
    unsigned res = array_test(&queue->arr, &cap, sz, 0, 0, queue->cnt, diff);
    if (!res || (res & ARRAY_UNTOUCHED)) return res;

    size_t bor, rem = size_sub(&bor, queue->begin, queue->cap - queue->cnt);
    if (!bor && rem) // queue->begin > queue->cap - queue->cnt
    {
        size_t off = size_sub(&bor, rem, cap - queue->cap);
        if (!bor && off) // queue->begin > cap - queue->cnt
        {
            size_t tot = off * sz;
            memcpy((char *) queue->arr + queue->cap * sz, queue->arr, tot);
            memmove(queue->arr, (char *) queue->arr + tot, (rem - off) * sz);
        }
        else memcpy((char *) queue->arr + queue->cap * sz, queue->arr, rem * sz);
    }
    queue->cap = cap;
    return ARRAY_SUCCESS;
}

void *queue_fetch(struct queue *queue, size_t offset, size_t sz)
{
    size_t bor, diff = size_sub(&bor, queue->begin, queue->cap - offset);
    if (!bor) return (char *) queue->arr + diff * sz;
    return (char *) queue->arr + (queue->begin + offset) * sz;
}

// This function should be called ONLY if 'queue_test' succeeds
void queue_enqueue_lo(struct queue *restrict queue, void *restrict arr, size_t cnt, size_t sz)
{
    size_t bor, left = size_sub(&bor, queue->begin, queue->cap - queue->cnt);
    if (bor) left += queue->cap;
    size_t diff = queue->cap - left; // Never overflows
    size_t bor2, diff2 = size_sub(&bor2, cnt, diff);
    if (!bor2 && diff2) // cnt > queue->cap - left
    {
        size_t tot = diff * sz;
        memcpy((char *) queue->arr + left * sz, arr, tot);
        memcpy(queue->arr, (char *) arr + tot, diff2 * sz);
    }
    else memcpy((char *) queue->arr + left * sz, arr, cnt * sz);    
    queue->cnt += cnt;
}

void queue_enqueue_yield_lo(struct queue *restrict queue, generator_callback genetator, void *context, size_t cnt, size_t sz)
{
    size_t bor, left = size_sub(&bor, queue->begin, queue->cap - queue->cnt);
    if (bor) left += queue->cap;
    size_t diff = queue->cap - left; // Never overflows
    size_t bor2, diff2 = size_sub(&bor2, cnt, diff);
    if (!bor2 && diff2) // cnt > queue->cap - left
    {
        size_t ind = 0;
        for (size_t i = left * sz; ind < diff; i += sz, ind++) genetator((char *) queue->arr + i, ind, context);
        for (size_t i = 0; ind < cnt; i += sz, ind++) genetator((char *) queue->arr + i, ind, context);
    }
    else for (size_t i = left * sz, ind = 0; ind < cnt; i += sz, ind++) genetator((char *) queue->arr + i, ind, context);
    queue->cnt += cnt;
}

// This function should be called ONLY if 'queue_test' succeeds
void queue_enqueue_hi(struct queue *restrict queue, void *restrict arr, size_t cnt, size_t sz)
{
    size_t bor, diff = size_sub(&bor, queue->begin, cnt);
    if (bor && diff) // cnt > queue->begin
    {
        diff = 0 - diff;
        size_t tot = diff * sz, left = queue->cap - diff;
        memcpy((char *) queue->arr + left * sz, arr, tot);
        memcpy(queue->arr, (char *) arr + tot, queue->begin * sz);
        queue->begin = left;
    }
    else
    {
        memcpy((char *) queue->arr + diff * sz, arr, cnt * sz);
        queue->begin = diff;
    }
    queue->cnt += cnt;
}

static void queue_enqueue_yield_hi(struct queue *restrict queue, generator_callback genetator, void *context, size_t cnt, size_t sz)
{
    size_t bor, diff = size_sub(&bor, queue->begin, cnt);
    if (bor && diff) // cnt > queue->begin
    {
        diff = 0 - diff;
        size_t left = queue->cap - diff, ind = 0;
        for (size_t i = left; ind < diff; i += sz, ind++) genetator((char *) queue->arr + i, ind, context);
        for (size_t i = 0; ind < cnt; i += sz, ind++) genetator((char *) queue->arr + i, ind, context);
        queue->begin = left;
    }
    else
    {
        for (size_t i = diff, ind = 0; ind < cnt; i += sz, ind++) genetator((char *) queue->arr + i, ind, context);
        queue->begin = diff;
    }
    queue->cnt += cnt;
}

unsigned queue_enqueue(struct queue *restrict queue, bool hi, void *restrict arr, size_t cnt, size_t sz)
{
    unsigned res = queue_test(queue, cnt, sz);
    if (!res) return 0;
    (hi ? queue_enqueue_hi : queue_enqueue_lo)(queue, arr, cnt, sz);
    return res;
}

unsigned queue_enqueue_yield(struct queue *restrict queue, bool hi, generator_callback genetator, void *context, size_t cnt, size_t sz)
{
    unsigned res = queue_test(queue, cnt, sz);
    if (!res) return 0;
    (hi ? queue_enqueue_yield_hi : queue_enqueue_yield_lo)(queue, genetator, context, cnt, sz);
    return res;
}

void queue_dequeue(struct queue *queue, size_t offset, size_t sz)
{
    if (offset)
    {
        size_t bor, ind = size_sub(&bor, queue->begin, queue->cap - offset);
        if (bor) ind += queue->cap;
        memcpy((char *) queue->arr + ind * sz, (char *) queue->arr + queue->begin * sz, sz);
    }
    queue->cnt--;
    queue->begin++;
    if (queue->begin == queue->cap) queue->begin = 0;
}

unsigned persistent_array_init(struct persistent_array *arr, size_t cnt, size_t sz)
{
    if (!cnt)
    {
        memset(arr, 0, sizeof(*arr));
        return 1 | ARRAY_UNTOUCHED;
    }
    size_t off = arr->off = size_log2(cnt, 0);
    if (!array_init(&arr->ptr, &arr->cap, arr->tot = 1, sizeof(*arr->ptr), 0, 0)) return 0;
    if (array_init(arr->ptr, NULL, ((size_t) 2 << off) - 1, sz, 0, ARRAY_STRICT)) return 1;
    free(arr->ptr);
    return 0;
}

void persistent_array_close(struct persistent_array *arr)
{
    for (size_t i = 0; i < arr->tot; free(arr->ptr[i++]));
    free(arr->ptr);
}

unsigned persistent_array_test(struct persistent_array *arr, size_t cnt, size_t sz)
{
    size_t cap = (size_t) 1 << arr->tot << arr->off;
    if (cnt < cap) return 1 | ARRAY_UNTOUCHED;
    if (!arr->tot) return persistent_array_init(arr, cnt, sz);
    size_t tot = size_log2(cnt + 1, 1) - arr->off;
    if (!array_test(&arr->ptr, &arr->cap, sizeof(*arr->ptr), 0, 0, tot)) return 0;
    for (size_t i = arr->tot; i < tot; i++, cap <<= 1)
    {
        if (array_init(arr->ptr + i, NULL, cap, sz, 0, ARRAY_STRICT)) continue;
        arr->tot = i;
        return 0;
    }
    arr->tot = tot;
    return 1;    
}

void *persistent_array_fetch(struct persistent_array *arr, size_t ind, size_t sz)
{
    size_t off = size_sub_sat(size_log2(ind + 1, 0), arr->off);
    return (char *) arr->ptr[off] + (off ? ind + 1 - ((size_t) 1 << arr->off << off) : ind) * sz;
}
