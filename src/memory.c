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

void array_shift_left(void *arr, size_t cnt, size_t sz, size_t off)
{
    if (!off || !cnt) return;
    size_t div = cnt / off, rem = (cnt % off) * sz, tot = off * sz, left = 0;
    for (size_t i = 0; i < div; i++, left += tot) memcpy((char *) arr + left, (char *) arr + left + tot, tot);
    memcpy((char *) arr + left, (char *) arr + left + rem, rem);
}

// 'p_cap' -- pointer to initial capacity
// 'cnt' -- desired capacity
unsigned array_init(void *p_Src, size_t *restrict p_cap, size_t cnt, size_t sz, size_t diff, enum array_flags flags)
{
    void **restrict p_src = p_Src, *src = *p_src;
    if (src && p_cap && (flags & ARRAY_REALLOC))
    {
        size_t bor, tmp = size_sub(&bor, cnt, *p_cap);
        if (bor) // cnt < cap
        {
            if (!(flags & ARRAY_REDUCE)) return 1 | ARRAY_UNTOUCHED;
            size_t tot = cnt * sz + diff; // No checks for overflows
            void *res = realloc(src, tot);
            *p_src = res;
            *p_cap = cnt;
            return !(tot && !res);
        }
        else if (!tmp) return 1 | ARRAY_UNTOUCHED; // cnt == cap            
    }
    if (!(flags & ARRAY_STRICT))
    {
        size_t log2 = size_bit_scan_reverse(cnt);
        if ((size_t) ~log2)
        {
            size_t cap2 = (size_t) 1 << log2;
            if (cnt != cap2)
            {
                cnt = cap2 << 1;
                if (!cnt)
                {
                    errno = ERANGE;
                    return 0;
                }
            }
        }
    }        
    size_t hi, pr = size_mul(&hi, cnt, sz);
    if (!hi)
    {
        size_t car, tot = size_add(&car, pr, diff);
        if (!car)
        {
            void *res;
            if (src && p_cap && (flags & ARRAY_REALLOC))
            {
                res = realloc(src, tot);
                if (res && (flags & ARRAY_CLEAR))
                {
                    size_t off = *p_cap * sz + diff;
                    memset((char *) res + off, 0, tot - off);
                }                
                *p_cap = cnt;
            }
            else
            {
                res = flags & ARRAY_CLEAR ? calloc(tot, 1) : malloc(tot);
                if (p_cap) *p_cap = cnt;
            }
            *p_src = res;
            return !(tot && !res);
        }
    }
    errno = ERANGE;
    return 0;
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

    size_t bor, left = size_sub(&bor, queue->begin, queue->cap - queue->cnt);
    if (!bor && left) // queue->begin > queue->cap - queue->cnt
    {
        /*size_t left2 = size_sub(&bor, queue->begin, cap - queue->cnt);
        if (!bor && left2) // queue->begin > cap - queue->cnt
        {
            size_t capp_diff = cap - queue->cap, capp_diff_pr = capp_diff * sz;
            memcpy((char *) queue->arr + queue->cap * sz, queue->arr, capp_diff_pr);

            size_t left3 = size_sub(&bor, left2, capp_diff);
            if (!bor && left3) // queue->begin - cap + queue->cnt > cap - queue->cap
            {
                memcpy(queue->arr, (char *) queue->arr + capp_diff_pr, capp_diff_pr);
                memcpy((char *) queue->arr + capp_diff_pr, (char *) queue->arr + (capp_diff_pr << 1), left3 * sz);
            }
            else memcpy(queue->arr, (char *) queue->arr + capp_diff_pr, left2 * sz);
        }
        else memcpy((char *) queue->arr + queue->cap * sz, queue->arr, left * sz);*/
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
static void queue_enqueue_lo(struct queue *restrict queue, void *restrict arr, size_t cnt, size_t sz)
{
    size_t bor, left = size_sub(&bor, queue->begin, queue->cap - queue->cnt);
    if (bor) left += queue->cap;
    size_t diff = queue->cap - left; // Never overflows
    size_t bor2, left2 = size_sub(&bor2, cnt, diff);
    if (!bor2 && left2) // cnt > queue->cap - left
    {
        size_t diff_pr = diff * sz;
        memcpy((char *) queue->arr + left * sz, arr, diff_pr);
        memcpy(queue->arr, (char *) queue->arr + diff_pr, left2 * sz);
    }
    else memcpy((char *) queue->arr + left * sz, arr, cnt * sz);    
    queue->cnt += cnt;
}

// This function should be called ONLY if 'queue_test' succeeds
static void queue_enqueue_hi(struct queue *restrict queue, void *restrict arr, size_t cnt, size_t sz)
{
    size_t bor, diff = size_sub(&bor, cnt, queue->begin);
    if (!bor && diff) // cnt > queue->begin
    {
        size_t diff_pr = diff * sz, left = queue->cap - diff;
        memcpy((char *) queue->arr + left * sz, arr, diff_pr);
        memcpy(queue->arr, (char *) queue->arr + diff_pr, queue->begin * sz);
        queue->begin = left;
    }
    else
    {
        memcpy((char *) queue->arr + diff * sz, arr, cnt * sz);
        queue->begin = 0 - diff;
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

struct persistent_array *persistent_array_create(size_t cnt, size_t sz)
{
    struct persistent_array *res;
    size_t off = size_bit_scan_reverse(cnt);
    if ((size_t) ~off)
    {
        if (array_init(&res, NULL, SIZE_BIT - off, sizeof(*res->ptr), sizeof(*res), ARRAY_STRICT | ARRAY_CLEAR))
        {
            size_t cap = res->cap = ((size_t) 2 << (res->off = off)) - 1;
            if (array_init(res->ptr, NULL, cap, sz, 0, ARRAY_STRICT)) return res;
            free(res);
        }        
    }
    else if (array_init(&res, NULL, SIZE_BIT, sizeof(*res->ptr), sizeof(*res), ARRAY_STRICT | ARRAY_CLEAR)) return res;
    return NULL;
}

void persistent_array_dispose(struct persistent_array *arr)
{
    if (!arr) return;
    size_t cnt = size_bit_scan_reverse(arr->cap) - arr->off + 1;
    for (size_t i = 0; i < cnt; free(arr->ptr[i++]));
    free(arr);
}

unsigned persistent_array_test(struct persistent_array *arr, size_t cnt, size_t sz)
{
    size_t bor = 0, diff = size_sub(&bor, cnt, arr->cap - arr->cnt);
    if (!bor && diff) // cnt + arr->cnt > arr->cap
    {
        size_t off = size_bit_scan_reverse(arr->cap) - arr->off + 1, cap = (size_t) 1 << off;
        for (;;) 
        {
            if (!array_init(arr->ptr + off, NULL, cap, sz, 0, ARRAY_STRICT)) return 0;
            arr->cap += cap;
            diff = size_sub(&bor, diff, cap);
            if (bor || !diff) break;
            cap <<= 1;
            off++;
        }
        return 1;
    }
    return 1 | ARRAY_UNTOUCHED;
}

void *persistent_array_fetch(struct persistent_array *arr, size_t ind, size_t sz)
{
    size_t off = size_bit_scan_reverse(ind) - arr->off + 1;
    return (char *) arr->ptr[off] + (ind - (((size_t) 1 << off) >> 1)) * sz;
}
