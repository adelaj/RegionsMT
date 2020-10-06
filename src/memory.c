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

// Total size = xdim * (ydim * sz + ydiff) + xdiff 
struct array_result matrix_init(void *p_Src, size_t *restrict p_cap, size_t xdim, size_t ydim, size_t sz, size_t xdiff, size_t ydiff, enum array_flags flags)
{
    size_t xtot = ydim;
    if (flags & ARRAY_FAILSAFE) xtot = xtot * sz + ydiff;
    else if (!test_ma(&xtot, sz, ydiff)) return (struct array_result) { .error = ARRAY_OVERFLOW };
    struct array_result res = array_init(p_Src, p_cap, xdim, xtot, xdiff, flags);
    if (p_cap && res.status == ARRAY_SUCCESS) *p_cap *= ydim;
    return res;
}

static void *Aligned_realloc_impl(void *ptr, size_t *restrict p_cap, size_t al, size_t tot)
{
#ifdef HAS_ALIGNED_REALLOC
    (void) p_cap;
    return Aligned_realloc(ptr, al, tot);
#else
    if (!p_cap || !tot) return Aligned_realloc(ptr, al, tot); // This will return 'NULL' and set 'errno' if 'tot' is non-zero, and release memory otherwise
    void *res = Aligned_malloc(al, tot);
    if (!res) return NULL;
    memcpy(res, ptr, *p_cap);
    Aligned_free(ptr);
    return res;
#endif
}

// 'al' -- alignment of array, used only when 'ARRAY_ALIGNED' flag is set
// 'p_cap' -- pointer to initial capacity
// 'cnt' -- desired capacity
// On error 'p_Src' and 'p_cap' are untouched
// Warning! 'ARRAY_ALIGNED | ARRAY_REALLOC' will yield an error on most platforms when 'p_cap' is not specified!
struct array_result array_init_impl(void *p_Src, size_t *restrict p_cap, size_t al, size_t cnt, size_t sz, size_t diff, enum array_flags flags)
{
    void **restrict p_src = p_Src, *src = *p_src;
    if (src && p_cap && (flags & ARRAY_REALLOC))
    {
        unsigned char bor = 0;
        size_t tmp = sub(&bor, cnt, *p_cap);
        if (bor) // cnt < *p_cap
        {
            if (!(flags & ARRAY_REDUCE)) return (struct array_result) { .status = ARRAY_SUCCESS_UNTOUCHED };
            size_t tot = cnt * sz + diff; // No checks for overflows
            void *res = flags & ARRAY_ALIGN ? Aligned_realloc_impl(src, p_cap, al, tot) : Realloc(src, tot); // Array shrinking; when 'tot' is zero memory is released
            if (tot && !res) return (struct array_result) { .error = ARRAY_OUT_OF_MEMORY, .tot = tot };
            *p_cap = cnt;
            *p_src = res;
            return (struct array_result) { .status = ARRAY_SUCCESS, .tot = tot };
        }
        else if (!tmp) return (struct array_result) { .status = ARRAY_SUCCESS_UNTOUCHED }; // cnt == cap            
    }
    if (!(flags & ARRAY_STRICT) && cnt)
    {
        size_t log2 = ulog2(cnt, 1);
        cnt = log2 == SIZE_BIT ? cnt : (size_t) 1 << log2;
    }
    size_t tot = cnt;
    if ((flags & ARRAY_STRICT) && (flags & ARRAY_FAILSAFE)) tot *= sz, tot += diff;
    else if (!test_ma(&tot, sz, diff)) return (struct array_result) { .error = ARRAY_OVERFLOW };
    void *res;
    if (src && (flags & ARRAY_REALLOC))
    {
        res = flags & ARRAY_ALIGN ? Aligned_realloc_impl(src, p_cap, al, tot) : Realloc(src, tot); // When 'tot' is zero memory is released
        if (res && p_cap && (flags & ARRAY_CLEAR))
        {
            size_t off = *p_cap * sz + diff;
            memset((char *) res + off, 0, tot - off);
        }
    }
    else res = flags & ARRAY_ALIGN ? 
        flags & ARRAY_CLEAR ? Aligned_calloc(al, 1, tot) : Aligned_malloc(al, tot) :
        flags & ARRAY_CLEAR ? calloc(1, tot) : malloc(tot);
    if (tot && !res) return (struct array_result) { .error = ARRAY_OUT_OF_MEMORY, .tot = tot };
    *p_src = res;
    if (p_cap) *p_cap = cnt;
    return (struct array_result) { .status = ARRAY_SUCCESS, .tot = tot };
}

struct array_result array_init(void *p_Src, size_t *restrict p_cap, size_t cnt, size_t sz, size_t diff, enum array_flags flags)
{
    return array_init_impl(p_Src, p_cap, 0, cnt, sz, diff, flags & ~(enum array_flags) ARRAY_ALIGN);
}

struct array_result array_test_impl(void *p_Arr, size_t *restrict p_cap, size_t sz, size_t diff, enum array_flags flags, size_t *restrict cntl, size_t cnt)
{
    size_t sum;
    return test_sum(&sum, cntl, cnt) == cnt ? array_init(p_Arr, p_cap, sum, sz, diff, flags | ARRAY_REALLOC) : (struct array_result) { .error = ARRAY_OVERFLOW };
}

struct array_result queue_init(struct queue *queue, size_t cnt, size_t sz)
{
    size_t cap;
    struct array_result res = array_init(&queue->arr, &cap, cnt, sz, 0, 0);
    if (!res.status) return res;
    queue->cap = cap;
    queue->begin = queue->cnt = 0;
    return res;
}

void queue_close(struct queue *queue)
{
    free(queue->arr);
}

struct array_result queue_test(struct queue *queue, size_t diff, size_t sz)
{
    size_t cap = queue->cap;
    struct array_result res = array_test(&queue->arr, &cap, sz, 0, 0, queue->cnt, diff);
    if (!res.status || (res.status & ARRAY_UNTOUCHED)) return res;
    unsigned char bor = 0;
    size_t rem = sub(&bor, queue->begin, queue->cap - queue->cnt);
    if (!bor && rem) // queue->begin > queue->cap - queue->cnt
    {
        size_t off = sub(&bor, rem, cap - queue->cap);
        if (!bor && off) // queue->begin > cap - queue->cnt
        {
            size_t tot = off * sz;
            memcpy((char *) queue->arr + queue->cap * sz, queue->arr, tot);
            memmove(queue->arr, (char *) queue->arr + tot, (rem - off) * sz);
        }
        else memcpy((char *) queue->arr + queue->cap * sz, queue->arr, rem * sz);
    }
    queue->cap = cap;
    return res;
}

void *queue_fetch(struct queue *queue, size_t offset, size_t sz)
{
    size_t diff;
    return (char *) queue->arr + (test_sub(&diff, queue->begin, queue->cap - offset) ? diff : queue->begin + offset) * sz;
}

// This function should be called ONLY if 'queue_test' succeeds
static void queue_enqueue_lo(struct queue *restrict queue, generator_callback generator, void *restrict arrco, size_t cnt, size_t sz)
{
    unsigned char bor = 0;
    size_t left = sub(&bor, queue->begin, queue->cap - queue->cnt);
    if (bor) left += queue->cap;
    size_t diff = queue->cap - left; // Never overflows
    unsigned char bor2 = 0;
    size_t diff2 = sub(&bor2, cnt, diff);
    if (!bor2 && diff2) // cnt > queue->cap - left
    {
        if (generator)
        {
            size_t ind = 0;
            for (size_t i = left * sz; ind < diff; i += sz, ind++) generator((char *) queue->arr + i, ind, arrco);
            for (size_t i = 0; ind < cnt; i += sz, ind++) generator((char *) queue->arr + i, ind, arrco);
        }
        else
        {
            size_t tot = diff * sz;
            memcpy((char *) queue->arr + left * sz, arrco, tot);
            memcpy(queue->arr, (char *) arrco + tot, diff2 * sz);
        }
    }
    else if (generator) for (size_t i = left * sz, ind = 0; ind < cnt; i += sz, ind++) generator((char *) queue->arr + i, ind, arrco);
    else memcpy((char *) queue->arr + left * sz, arrco, cnt * sz);    
    queue->cnt += cnt;
}

// This function should be called ONLY if 'queue_test' succeeds
static void queue_enqueue_hi(struct queue *restrict queue, generator_callback generator, void *restrict arrco, size_t cnt, size_t sz)
{
    unsigned char bor = 0;
    size_t diff = sub(&bor, queue->begin, cnt);
    if (bor && diff) // cnt > queue->begin
    {
        diff = 0 - diff;
        size_t left = queue->cap - diff;
        if (generator)
        {
            size_t ind = 0;
            for (size_t i = left; ind < diff; i += sz, ind++) generator((char *) queue->arr + i, ind, arrco);
            for (size_t i = 0; ind < cnt; i += sz, ind++) generator((char *) queue->arr + i, ind, arrco);
        }
        else
        {
            size_t tot = diff * sz;
            memcpy((char *) queue->arr + left * sz, arrco, tot);
            memcpy(queue->arr, (char *) arrco + tot, queue->begin * sz);
        }
        queue->begin = left;
    }
    else
    {
        if (generator) for (size_t i = diff, ind = 0; ind < cnt; i += sz, ind++) generator((char *) queue->arr + i, ind, arrco);
        else memcpy((char *) queue->arr + diff * sz, arrco, cnt * sz);
        queue->begin = diff;
    }
    queue->cnt += cnt;
}

struct array_result queue_enqueue(struct queue *restrict queue, bool hi, generator_callback generator, void *restrict arr, size_t cnt, size_t sz)
{
    struct array_result res = queue_test(queue, cnt, sz);
    if (!res.status) return res;
    (hi ? queue_enqueue_hi : queue_enqueue_lo)(queue, generator, arr, cnt, sz);
    return res;
}

void queue_dequeue(struct queue *queue, size_t offset, size_t sz)
{
    if (offset)
    {
        unsigned char bor = 0;
        size_t ind = sub(&bor, queue->begin, queue->cap - offset);
        if (bor) ind += queue->cap;
        memcpy((char *) queue->arr + ind * sz, (char *) queue->arr + queue->begin * sz, sz);
    }
    queue->cnt--;
    queue->begin++;
    if (queue->begin == queue->cap) queue->begin = 0;
}

struct array_result persistent_array_init(struct persistent_array *arr, size_t cnt, size_t sz, enum persistent_array_flags flags)
{
    bool weak = flags & PERSISTENT_ARRAY_WEAK;
    if (!cnt)
    {
        *arr = (struct persistent_array) { 0 };
        return weak ?
            (struct array_result) { .status = ARRAY_SUCCESS_UNTOUCHED } : 
            array_init(&arr->ptr, &arr->cap, SIZE_BIT, sizeof(*arr->ptr), 0, 0);
    }
    size_t off = arr->off = ulog2(cnt, 0);
    arr->bck = 1;
    struct array_result res = array_init(&arr->ptr, &arr->cap, weak ? 1 : SIZE_BIT - off, sizeof(*arr->ptr), 0, weak ? 0 : ARRAY_STRICT);
    if (!res.status) return res;
    size_t tot = res.tot;
    res = array_init(arr->ptr, NULL, ((size_t) 2 << off) - 1, sz, 0, ARRAY_STRICT | (flags & PERSISTENT_ARRAY_CLEAR));
    if (res.status) return (struct array_result) { .status = ARRAY_SUCCESS, .tot = add_sat(tot, res.tot) };
    free(arr->ptr);
    return res;
}

void persistent_array_close(struct persistent_array *arr)
{
    for (size_t i = 0; i < arr->bck; free(arr->ptr[i++]));
    free(arr->ptr);
}

struct array_result persistent_array_test(struct persistent_array *arr, size_t cnt, size_t sz, enum persistent_array_flags flags)
{
    if (!arr->bck) return persistent_array_init(arr, cnt, sz, flags);
    size_t cap1 = (size_t) 2 << (arr->bck - 1) << arr->off; // 'cap1' equals to the actual capacity plus one
    if (cnt <= cap1 - 1) return (struct array_result) { .status = ARRAY_SUCCESS_UNTOUCHED }; // Do not replace condition with 'cnt < cap1'!
    size_t cnt1 = add_sat(cnt, 1), bck = ulog2(cnt1, 1) - arr->off, tot = 0;
    if (flags & PERSISTENT_ARRAY_WEAK)
    {
        struct array_result res = array_test(&arr->ptr, &arr->cap, sizeof(*arr->ptr), 0, 0, bck);
        if (!res.status) return res;
        tot = res.tot;
    }
    for (size_t i = arr->bck; i < bck; i++, cap1 <<= 1) // 'cap1' is always non-zero!
    {
        struct array_result res = array_init(arr->ptr + i, NULL, cap1, sz, 0, ARRAY_STRICT | (flags & PERSISTENT_ARRAY_CLEAR));
        if (!res.status)
        {
            arr->bck = i;
            return res;
        }
        tot = add_sat(tot, res.tot);
    }
    arr->bck = bck;
    return (struct array_result) { .status = ARRAY_SUCCESS, .tot = tot };
}

void *persistent_array_fetch(struct persistent_array *arr, size_t ind, size_t sz)
{
    size_t ind1 = ind + 1, log2 = ulog2(ind1, 0), off = sub_sat(log2, arr->off);
    return (char *) arr->ptr[off] + (off ? ind1 - ((size_t) 1 << arr->off << off) : ind) * sz;
}
