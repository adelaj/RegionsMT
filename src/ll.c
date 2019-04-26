#include "ll.h"

#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <immintrin.h>

#if defined __GNUC__ || defined __clang__

#   define DECLARE_LOAD_ACQUIRE(TYPE, PREFIX) \
        TYPE PREFIX ## _load_acquire(TYPE volatile *src) \
        { \
            return __atomic_load_n(src, __ATOMIC_ACQUIRE); \
        }

#   define DECLARE_STORE_RELEASE(TYPE, PREFIX) \
        void PREFIX ## _store_release(TYPE volatile *dst, TYPE val) \
        { \
            __atomic_store_n(dst, val, __ATOMIC_RELEASE); \
        }

#   define DECLARE_INTERLOCKED_COMPARE_EXCHANGE(TYPE, PREFIX) \
        TYPE PREFIX ## _interlocked_compare_exchange(TYPE volatile *dst, TYPE cmp, TYPE xchg) \
        { \
            __atomic_compare_exchange_n(dst, &cmp, xchg, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED); \
            return cmp;\
        }

#   define DECLARE_INTERLOCKED_OP(TYPE, PREFIX, SUFFIX, BACKEND) \
        TYPE PREFIX ## _interlocked_ ## SUFFIX(TYPE volatile *dst, TYPE msk) \
        { \
            return BACKEND(dst, msk, __ATOMIC_ACQ_REL); \
        }

static DECLARE_LOAD_ACQUIRE(int, spinlock)
static DECLARE_STORE_RELEASE(int, spinlock)
static DECLARE_INTERLOCKED_COMPARE_EXCHANGE(int, spinlock)

DECLARE_INTERLOCKED_COMPARE_EXCHANGE(void *, ptr)

DECLARE_LOAD_ACQUIRE(uint8_t, uint8)
DECLARE_LOAD_ACQUIRE(uint16_t, uint16)
DECLARE_LOAD_ACQUIRE(uint32_t, uint32)

DECLARE_INTERLOCKED_OP(uint8_t, uint8, or, __atomic_fetch_or)
DECLARE_INTERLOCKED_OP(uint16_t, uint16, or, __atomic_fetch_or)
DECLARE_INTERLOCKED_OP(uint8_t, uint8, and, __atomic_fetch_and)
DECLARE_INTERLOCKED_OP(uint16_t, uint16, and, __atomic_fetch_and)

void size_inc_interlocked(volatile size_t *mem)
{
    __atomic_add_fetch(mem, 1, __ATOMIC_ACQ_REL);
}

void size_dec_interlocked(volatile size_t *mem)
{
    __atomic_sub_fetch(mem, 1, __ATOMIC_ACQ_REL);
}

uint32_t uint32_bit_scan_reverse(uint32_t x)
{
    return x ? ((sizeof(unsigned) * CHAR_BIT) - __builtin_clz((unsigned) x) - 1) : UINT_MAX;
}

uint32_t uint32_bit_scan_forward(uint32_t x)
{
    return x ? __builtin_ctz((unsigned) x) : UINT_MAX;
}

uint32_t uint32_pop_cnt(uint32_t x)
{
    return __builtin_popcount(x);
}

#   ifdef __x86_64__

DECLARE_LOAD_ACQUIRE(uint64_t, uint64)
DECLARE_LOAD_ACQUIRE(size_t, size)

size_t size_bit_scan_reverse(size_t x)
{
    return x ? SIZE_BIT - __builtin_clzll(x) - 1 : SIZE_MAX;
}

size_t size_bit_scan_forward(size_t x)
{
    return x ? __builtin_ctzll(x) : SIZE_MAX;
}

size_t size_pop_cnt(size_t x)
{
    return __builtin_popcountll(x);
}

#   endif 
#elif defined _MSC_BUILD
#   include <intrin.h>

#   define DECLARE_LOAD_ACQUIRE(TYPE, PREFIX, BACKEND_CAST, BACKEND) \
        TYPE PREFIX ## _load_acquire(TYPE volatile *src) \
        { \
            return (TYPE) BACKEND((BACKEND_CAST volatile *) src, 0); \
        }

#   define DECLARE_STORE_RELEASE(TYPE, PREFIX, BACKEND_CAST, BACKEND) \
        void PREFIX ## _store_release(TYPE volatile *dst, TYPE val) \
        { \
            BACKEND((BACKEND_CAST volatile *) dst, val); \
        }

#   define DECLARE_INTERLOCKED_COMPARE_EXCHANGE(TYPE, PREFIX, BACKEND_CAST, BACKEND) \
        TYPE PREFIX ## _interlocked_compare_exchange(TYPE volatile *dst, TYPE cmp, TYPE xchg) \
        { \
            return (TYPE) BACKEND((BACKEND_CAST volatile *) dst, xchg, cmp);\
        }

#   define DECLARE_INTERLOCKED_OP(TYPE, PREFIX, SUFFIX, BACKEND_CAST, BACKEND) \
        TYPE PREFIX ## _interlocked_ ## SUFFIX(TYPE volatile *dst, TYPE msk) \
        { \
            return (TYPE) BACKEND((BACKEND_CAST volatile *) dst, msk); \
        }

static DECLARE_LOAD_ACQUIRE(long, spinlock, long, _InterlockedOr)
static DECLARE_STORE_RELEASE(long, spinlock, long, _InterlockedExchange)
static DECLARE_INTERLOCKED_COMPARE_EXCHANGE(long, spinlock, long, _InterlockedCompareExchange)

DECLARE_INTERLOCKED_COMPARE_EXCHANGE(void *, ptr, void *, _InterlockedCompareExchangePointer)

DECLARE_LOAD_ACQUIRE(uint8_t, uint8, char, _InterlockedOr8)
DECLARE_LOAD_ACQUIRE(uint16_t, uint16, short, _InterlockedOr16)
DECLARE_LOAD_ACQUIRE(uint32_t, uint32, long, _InterlockedOr)

DECLARE_INTERLOCKED_OP(uint8_t, uint8, or, char, _InterlockedOr8)
DECLARE_INTERLOCKED_OP(uint16_t, uint16, or, short, _InterlockedOr16)
DECLARE_INTERLOCKED_OP(uint8_t, uint8, and, char, _InterlockedAnd8)
DECLARE_INTERLOCKED_OP(uint16_t, uint16, and, short, _InterlockedAnd16)

uint32_t uint32_bit_scan_reverse(uint32_t x)
{
    unsigned long res;
    return _BitScanReverse(&res, (unsigned long) x) ? (uint32_t) res : UINT32_MAX;
}

uint32_t uint32_bit_scan_forward(uint32_t x)
{
    unsigned long res;
    return _BitScanForward(&res, (unsigned long) x) ? (uint32_t) res : UINT32_MAX;
}

uint32_t uint32_pop_cnt(uint32_t x)
{
    return __popcnt((unsigned) x);
}

#   ifdef _M_X64

DECLARE_LOAD_ACQUIRE(uint64_t, uint64, long long, _InterlockedOr64)
DECLARE_LOAD_ACQUIRE(size_t, size, long long, _InterlockedOr64)

void size_inc_interlocked(volatile size_t *mem)
{
    _InterlockedIncrement64((volatile __int64 *) mem);
}

void size_dec_interlocked(volatile size_t *mem)
{
    _InterlockedDecrement64((volatile __int64 *) mem);
}

size_t size_mul(size_t *p_hi, size_t a, size_t b)
{
    unsigned __int64 hi;
    size_t res = (size_t) _umul128((unsigned __int64) a, (unsigned __int64) b, &hi);
    *p_hi = (size_t) hi;
    return res;
}

size_t size_add(size_t *p_car, size_t x, size_t y)
{
    unsigned __int64 res;
    *p_car = _addcarry_u64(0, x, y, &res);
    return (size_t) res;
}

size_t size_sub(size_t *p_bor, size_t x, size_t y)
{
    unsigned __int64 res;
    *p_bor = _subborrow_u64(0, x, y, &res);
    return (size_t) res;
}

size_t size_bit_scan_reverse(size_t x)
{
    unsigned long res;
    return _BitScanReverse64(&res, (unsigned __int64) x) ? res : SIZE_MAX;
}

size_t size_bit_scan_forward(size_t x)
{
    unsigned long res;
    return _BitScanForward64(&res, (unsigned __int64) x) ? res : SIZE_MAX;
}

size_t size_pop_cnt(size_t x)
{
    return __popcnt64((__int64) x);
}

#   elif defined _M_IX86

void size_inc_interlocked(volatile size_t *mem)
{
    _InterlockedIncrement((volatile long *) mem);
}

void size_dec_interlocked(volatile size_t *mem)
{
    _InterlockedDecrement((volatile long *) mem);
}

#   endif
#endif 

// Warning! 'Dsize_t' is not defined for MSVC under x86_64
#if defined __GNUC__ || defined __clang__ || defined _M_IX86 || defined __i386__
#   if defined _M_X64 || defined __x86_64__
typedef unsigned __int128 Dsize_t;
#   elif defined _M_IX86 || defined __i386__
typedef unsigned long long Dsize_t;
#   endif

size_t size_mul(size_t *p_hi, size_t x, size_t y)
{
    Dsize_t val = (Dsize_t) x * (Dsize_t) y;
    *p_hi = (size_t) (val >> SIZE_BIT);
    return (size_t) val;
}

size_t size_add(size_t *p_car, size_t x, size_t y)
{
    Dsize_t val = (Dsize_t) x + (Dsize_t) y;
    *p_car = (size_t) (val >> SIZE_BIT);
    return (size_t) val;
}

size_t size_sub(size_t *p_bor, size_t x, size_t y)
{
    Dsize_t val = (Dsize_t) x - (Dsize_t) y;
    *p_bor = 0 - (size_t) (val >> SIZE_BIT);
    return (size_t) val;
}

#endif

size_t size_sum(size_t *p_hi, size_t *args, size_t args_cnt)
{
    if (!args_cnt)
    {
        *p_hi = 0;
        return 0;
    }
    size_t lo = args[0], hi = 0, car;
    for (size_t i = 1; i < args_cnt; lo = size_add(&car, lo, args[i++]), hi += car);
    *p_hi = hi;
    return lo;
}

#define DECLARE_UINT_LOG10(TYPE, PREFIX, MAX, ...) \
    TYPE PREFIX ## _log10(TYPE x, bool ceil) \
    { \
        static const TYPE arr[] = { __VA_ARGS__ }; \
        if (!x) return (MAX); \
        uint8_t left = 0, right = countof(arr) - 1; \
        while (left < right) \
        { \
            uint8_t mid = (left + right + !ceil) >> 1; \
            TYPE t = arr[mid]; \
            if (x > t) left = mid + ceil; \
            else if (x < t) right = mid - !ceil; \
            else return mid; \
        } \
        return ceil && left == countof(arr) - 1 ? countof(arr) : left; \
    }

#define TEN5(X) 1 ## X, 10 ## X, 100 ## X, 1000 ## X, 10000 ## X
#define TEN10(X) TEN5(X), TEN5(00000 ## X)
#define TEN20(X) TEN10(X), TEN10(0000000000 ## X)
DECLARE_UINT_LOG10(uint32_t, uint32, UINT32_MAX, TEN10(u))
DECLARE_UINT_LOG10(uint64_t, uint64, UINT64_MAX, TEN20(u))

#if defined _M_X64 || defined __x86_64__

DECLARE_UINT_LOG10(size_t, size, SIZE_MAX, TEN20(u))

#elif defined _M_IX86 || defined __i386__

DECLARE_UINT_LOG10(size_t, size, SIZE_MAX, TEN10(u))

size_t size_load_acquire(volatile size_t *src)
{
    return (size_t) uint32_load_acquire((volatile uint32_t *) src);
}

size_t size_bit_scan_reverse(size_t x)
{
    return (size_t) uint32_bit_scan_reverse((uint32_t) x);
}

size_t size_bit_scan_forward(size_t x)
{
    return (size_t) uint32_bit_scan_forward((uint32_t) x);
}

size_t size_pop_cnt(size_t x)
{
    return (size_t) uint32_pop_cnt((uint32_t) x);
}

#endif

size_t size_log2(size_t x, bool ceil)
{
    return size_bit_scan_reverse(x) + (ceil && x && (x & (x - 1)));
}

void spinlock_acquire(spinlock_handle *p_spinlock)
{
    while (spinlock_interlocked_compare_exchange(p_spinlock, 0, 1)) while (spinlock_load_acquire(p_spinlock)) _mm_pause();
}

void spinlock_release(spinlock_handle *p_spinlock)
{
    spinlock_store_release(p_spinlock, 0);
}

void *double_lock_execute(spinlock_handle *p_spinlock, double_lock_callback init, double_lock_callback comm, void *init_args, void *comm_args)
{
    void *res = NULL;
    switch (spinlock_load_acquire(p_spinlock))
    {
    case 0:
        if (!spinlock_interlocked_compare_exchange(p_spinlock, 0, 1))
        {
            if (init) res = init(init_args);
            spinlock_store_release(p_spinlock, 2);
            break;
        }
    case 1:
        while (spinlock_load_acquire(p_spinlock) != 2) _mm_pause();
    case 2:
        if (comm) res = comm(comm_args);
    }
    return res;
}

void bit_set_interlocked(volatile uint8_t *arr, size_t bit)
{
    uint8_interlocked_or(arr + bit / CHAR_BIT, 1 << bit % CHAR_BIT);
}

void bit_reset_interlocked(volatile uint8_t *arr, size_t bit)
{
    uint8_interlocked_and(arr + bit / CHAR_BIT, ~(1 << bit % CHAR_BIT));
}

void bit_set2_interlocked(volatile uint8_t *arr, size_t bit)
{
    uint8_t pos = bit % CHAR_BIT;
    if (pos < CHAR_BIT - 1) uint8_interlocked_or(arr + bit / CHAR_BIT, 3u << pos);
    else uint16_interlocked_or((volatile uint16_t *) (arr + bit / CHAR_BIT), 3u << (CHAR_BIT - 1));
}

uint8_t bit_get2_acquire(volatile uint8_t *arr, size_t bit)
{
    uint8_t pos = bit % CHAR_BIT;
    if (pos < CHAR_BIT - 1) return (uint8_load_acquire(arr + bit / CHAR_BIT) >> pos) & 3;
    else return (uint16_load_acquire((uint16_t *) (arr + bit / CHAR_BIT)) >> (CHAR_BIT - 1)) & 3;
}

bool bit_test2_acquire(volatile uint8_t *arr, size_t bit)
{
    return bit_get2_acquire(arr, bit) == 3;
}

bool bit_test_range_acquire(volatile uint8_t *arr, size_t cnt)
{
    size_t div = cnt / CHAR_BIT, rem = cnt % CHAR_BIT;
    for (size_t i = 0; i < div; i++) if (uint8_load_acquire(arr + i) != UINT8_MAX) return 0;
    if (!rem) return 1;
    uint8_t msk = (1u << rem) - 1;
    return (uint8_load_acquire(arr + div) & msk) == msk;    
}

bool bit_test2_range01_acquire(volatile uint8_t *arr, size_t cnt)
{
    size_t div = cnt / CHAR_BIT, rem = cnt % CHAR_BIT;
    for (size_t i = 0; i < div; i++) if ((uint8_load_acquire(arr + i) & BIT_TEST2_MASK01) != BIT_TEST2_MASK01) return 0;
    if (!rem) return 1;
    uint8_t msk = ((1u << rem) - 1) & BIT_TEST2_MASK01;
    return (uint8_load_acquire(arr + div) & msk) == msk;
}

bool bit_test2_range_acquire(volatile uint8_t *arr, size_t cnt)
{
    size_t div = cnt / CHAR_BIT, rem = cnt % CHAR_BIT;
    for (size_t i = 0; i < div; i++)
    {
        uint8_t res = uint8_load_acquire(arr + i);
        if (((res | res >> 1) & BIT_TEST2_MASK01) != BIT_TEST2_MASK01) return 0;
    }
    if (!rem) return 1;
    uint8_t res = uint8_load_acquire(arr + div), msk = ((1u << rem) - 1) & BIT_TEST2_MASK01;
    return ((res | (res >> 1)) & msk) == msk;
}

bool size_test_acquire(volatile size_t *mem)
{
    return size_load_acquire(mem);
}

uint8_t uint8_bit_scan_reverse(uint8_t x)
{
    return (uint8_t) uint32_bit_scan_reverse((uint8_t) x);
}

uint8_t uint8_bit_scan_forward(uint8_t x)
{
    return (uint8_t) uint32_bit_scan_forward((uint8_t) x);
}

size_t size_add_sat(size_t a, size_t b)
{
    size_t car, res = size_add(&car, a, b);
    return car ? SIZE_MAX : res;
}

size_t size_sub_sat(size_t a, size_t b)
{
    size_t bor, res = size_sub(&bor, a, b);
    return bor ? 0 : res;
}

#define DECLARE_STABLE_CMP_ASC(PREFIX, SUFFIX) \
    int PREFIX ## _stable_cmp_asc ## SUFFIX (const void *A, const void *B, void *thunk) \
    { \
        return PREFIX ## _stable_cmp_dsc ## SUFFIX (B, A, thunk);\
    }

#define DECLARE_CMP_ASC(PREFIX, SUFFIX) \
    bool PREFIX ## _cmp_asc ## SUFFIX (const void *A, const void *B, void *thunk) \
    { \
        return PREFIX ## _cmp_dsc ## SUFFIX (B, A, thunk);\
    }

int size_stable_cmp_dsc(const void *A, const void *B, void *thunk)
{
    (void) thunk;
    size_t bor, diff = size_sub(&bor, *(size_t *) B, *(size_t *) A);
    return diff ? 1 - (int) (bor << 1) : 0;
}

DECLARE_STABLE_CMP_ASC(size, )

bool size_cmp_dsc(const void *A, const void *B, void *thunk)
{
    (void) thunk;
    return *(size_t *) B > *(size_t *) A;
}

DECLARE_CMP_ASC(size, )

int flt64_stable_cmp_dsc(const void *a, const void *b, void *thunk)
{
    (void) thunk;
    __m128d ab = _mm_loadh_pd(_mm_load_sd(a), b);
    __m128i res = _mm_castpd_si128(_mm_cmplt_pd(ab, _mm_permute_pd(ab, 1)));
    return _mm_extract_epi32(res, 2) - _mm_cvtsi128_si32(res);
}

DECLARE_STABLE_CMP_ASC(flt64, )

int flt64_stable_cmp_dsc_abs(const void *a, const void *b, void *thunk)
{
    (void) thunk;
    __m128d ab = _mm_and_pd(_mm_loadh_pd(_mm_load_sd(a), b), _mm_castsi128_pd(_mm_set1_epi64x(0x7fffffffffffffff)));
    __m128i res = _mm_castpd_si128(_mm_cmplt_pd(ab, _mm_permute_pd(ab, 1)));
    return _mm_extract_epi32(res, 2) - _mm_cvtsi128_si32(res);
}

DECLARE_STABLE_CMP_ASC(flt64, _abs)

// Warning! The approach from the 'DECLARE_STABLE_CMP_ASC' marcro doesn't work here
#define DECLARE_FLT64_STABLE_CMP_NAN(INFIX, DIR) \
    int flt64_stable_cmp ## INFIX ## _nan(const void *a, const void *b, void *thunk) \
    { \
        (void) thunk; \
        __m128d ab = _mm_loadh_pd(_mm_load_sd(a), b); \
        __m128i res = _mm_sub_epi32(_mm_castpd_si128(_mm_cmpunord_pd(ab, ab)), _mm_castpd_si128(_mm_cmp_pd(ab, _mm_permute_pd(ab, 1), (DIR)))); \
        return _mm_extract_epi32(res, 2) - _mm_cvtsi128_si32(res); \
    }

DECLARE_FLT64_STABLE_CMP_NAN(_dsc, _CMP_NLE_UQ)
DECLARE_FLT64_STABLE_CMP_NAN(_asc, _CMP_NGE_UQ)

int flt64_sign(double x)
{
    __m128i res = _mm_castpd_si128(_mm_cmpgt_pd(_mm_set_sd(x), _mm_set_pd(x, 0)));
    return _mm_extract_epi32(res, 2) - _mm_cvtsi128_si32(res);
}

uint32_t uint32_fused_mul_add(uint32_t *p_res, uint32_t m, uint32_t a)
{
    uint64_t val = (uint64_t) *p_res * (uint64_t) m + (uint64_t) a;
    *p_res = (uint32_t) val;
    return (uint32_t) (val >> UINT32_BIT);
}

size_t size_fused_mul_add(size_t *p_res, size_t m, size_t a)
{
    size_t hi, lo = size_mul(&hi, *p_res, m), car;
    *p_res = size_add(&car, lo, a);
    return hi + car;
}

#define DECLARE_BIT_TEST(TYPE, PREFIX) \
    bool PREFIX ## _bit_test(TYPE *arr, size_t bit) \
    { \
        return arr[bit / (CHAR_BIT * sizeof(TYPE))] & ((TYPE) 1 << bit % (CHAR_BIT * sizeof(TYPE))); \
    }

#define DECLARE_BIT_TEST_SET(TYPE, PREFIX) \
    bool PREFIX ## _bit_test_set(TYPE *arr, size_t bit) \
    { \
        size_t div = bit / (CHAR_BIT * sizeof(TYPE)); \
        TYPE msk = (TYPE) 1 << bit % (CHAR_BIT * sizeof(TYPE)); \
        if (arr[div] & msk) return 1; \
        arr[div] |= msk; \
        return 0; \
    }

#define DECLARE_BIT_TEST_RESET(TYPE, PREFIX) \
    bool PREFIX ## _bit_test_reset(TYPE *arr, size_t bit) \
    { \
        size_t div = bit / (CHAR_BIT * sizeof(TYPE)); \
        TYPE msk = (TYPE) 1 << bit % (CHAR_BIT * sizeof(TYPE)); \
        if (!(arr[div] & msk)) return 0; \
        arr[div] &= ~msk; \
        return 1; \
    }

#define DECLARE_BIT_SET(TYPE, PREFIX) \
    void PREFIX ## _bit_set(TYPE *arr, size_t bit) \
    { \
        arr[bit / (CHAR_BIT * sizeof(TYPE))] |= (TYPE) 1 << bit % (CHAR_BIT * sizeof(TYPE)); \
    }

#define DECLARE_BIT_RESET(TYPE, PREFIX) \
    void PREFIX ## _bit_reset(TYPE *arr, size_t bit) \
    { \
        arr[bit / (CHAR_BIT * sizeof(TYPE))] &= ~((TYPE) 1 << bit % (CHAR_BIT * sizeof(TYPE))); \
    }

#define DECLARE_BIT_FETCH_BURST(TYPE, PREFIX, STRIDE) \
    TYPE PREFIX ## _bit_fetch_burst ## STRIDE(TYPE *arr, size_t pos) \
    { \
        return (arr[pos / (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1))] >> (pos % (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1)) << ((STRIDE) - 1))) & ((TYPE) 1 << ((STRIDE) - 1)); \
    }

#define DECLARE_BIT_FETCH_SET_BURST(TYPE, PREFIX, STRIDE) \
    TYPE PREFIX ## _bit_fetch_set_burst ## STRIDE(TYPE *arr, size_t pos, TYPE msk) \
    { \
        size_t div = pos / (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1)), rem = pos % (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1)) << ((STRIDE) - 1); \
        TYPE res = (arr[div] >> rem) & msk; \
        if (res != msk) arr[div] |= msk << rem; \
        return res; \
    }

#define DECLARE_BIT_FETCH_RESET_BURST(TYPE, PREFIX, STRIDE) \
    TYPE PREFIX ## _bit_fetch_reset_burst ## STRIDE(TYPE *arr, size_t pos, TYPE msk) \
    { \
        size_t div = pos / (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1)), rem = pos % (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1)) << ((STRIDE) - 1); \
        TYPE res = (arr[div] >> rem) & msk; \
        if (res) arr[div] &= ~(msk << rem); \
        return res; \
    }

#define DECLARE_BIT_SET_BURST(TYPE, PREFIX, STRIDE) \
    void PREFIX ## _bit_set_burst ## STRIDE(TYPE *arr, size_t pos, TYPE msk) \
    { \
        arr[pos / (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1))] |= msk << (pos % (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1)) << ((STRIDE) - 1)); \
    }

#define DECLARE_BIT_RESET_BURST(TYPE, PREFIX, STRIDE) \
    void PREFIX ## _bit_reset_burst ## STRIDE(TYPE *arr, size_t pos, TYPE msk) \
    { \
        arr[pos / (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1))] &= ~(msk << (pos % (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1)) << ((STRIDE) - 1))); \
    }

DECLARE_BIT_TEST(uint8_t, uint8)
DECLARE_BIT_TEST_SET(uint8_t, uint8)
DECLARE_BIT_TEST_RESET(uint8_t, uint8)
DECLARE_BIT_SET(uint8_t, uint8)
DECLARE_BIT_RESET(uint8_t, uint8)

DECLARE_BIT_FETCH_BURST(uint8_t, uint8, 2)
DECLARE_BIT_FETCH_SET_BURST(uint8_t, uint8, 2)
DECLARE_BIT_FETCH_RESET_BURST(uint8_t, uint8, 2)
DECLARE_BIT_SET_BURST(uint8_t, uint8, 2)
DECLARE_BIT_RESET_BURST(uint8_t, uint8, 2)

DECLARE_BIT_TEST(size_t, size)
DECLARE_BIT_TEST_SET(size_t, size)
DECLARE_BIT_TEST_RESET(size_t, size)
DECLARE_BIT_SET(size_t, size)
DECLARE_BIT_RESET(size_t, size)
