#include "ll.h"

#include <stdint.h>
#include <limits.h>
#include <errno.h>

#if defined __GNUC__ || defined __clang__

#   define DECLARE_LOAD_ACQUIRE(TYPE, PREFIX) \
        TYPE PREFIX ## _load_acquire(volatile void *src) \
        { \
            return __atomic_load_n((TYPE volatile *) src, __ATOMIC_ACQUIRE); \
        }
  
#   define DECLARE_STORE_RELEASE(TYPE, PREFIX) \
        void PREFIX ## _store_release(volatile void *dst, TYPE val) \
        { \
            __atomic_store_n((TYPE volatile *) dst, val, __ATOMIC_RELEASE); \
        }

#   define DECLARE_INTERLOCKED_COMPARE_EXCHANGE(TYPE, PREFIX, SUFFIX, MEM_ORD_SUCC, MEM_ORD_FAIL) \
        TYPE PREFIX ## _interlocked_compare_exchange ## SUFFIX(volatile void *dst, TYPE cmp, TYPE xchg) \
        { \
            __atomic_compare_exchange_n((TYPE volatile *) dst, &cmp, xchg, 0, MEM_ORD_SUCC, MEM_ORD_FAIL); \
            return cmp;\
        }

#   define DECLARE_INTERLOCKED_OP(TYPE, PREFIX, SUFFIX, BACKEND, MEM_ORD) \
        TYPE PREFIX ## _interlocked_ ## SUFFIX(volatile void *dst, TYPE arg) \
        { \
            return BACKEND((TYPE volatile *) dst, arg, MEM_ORD); \
        }

static DECLARE_LOAD_ACQUIRE(int, spinlock)
static DECLARE_STORE_RELEASE(int, spinlock)
static DECLARE_INTERLOCKED_COMPARE_EXCHANGE(int, spinlock, _acquire, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)

DECLARE_INTERLOCKED_COMPARE_EXCHANGE(void *, ptr,, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)

DECLARE_INTERLOCKED_OP(uint8_t, uint8, or, __atomic_fetch_or, __ATOMIC_ACQ_REL)
DECLARE_INTERLOCKED_OP(uint8_t, uint8, or_release, __atomic_fetch_or, __ATOMIC_RELEASE)
DECLARE_INTERLOCKED_OP(uint16_t, uint16, or, __atomic_fetch_or, __ATOMIC_ACQ_REL)
DECLARE_INTERLOCKED_OP(uint8_t, uint8, and, __atomic_fetch_and, __ATOMIC_ACQ_REL)
DECLARE_INTERLOCKED_OP(uint8_t, uint8, and_release, __atomic_fetch_and, __ATOMIC_RELEASE)
DECLARE_INTERLOCKED_OP(uint16_t, uint16, and, __atomic_fetch_and, __ATOMIC_ACQ_REL)
DECLARE_INTERLOCKED_OP(void *, ptr, exchange, __atomic_exchange_n, __ATOMIC_ACQ_REL)
DECLARE_INTERLOCKED_OP(size_t, size, add, __atomic_fetch_add, __ATOMIC_ACQ_REL)

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
    return __builtin_popcount((unsigned) x);
}

#   ifdef __x86_64__

size_t size_bit_scan_reverse(size_t x)
{
    return x ? SIZE_BIT - __builtin_clzll((unsigned long long) x) - 1 : SIZE_MAX;
}

size_t size_bit_scan_forward(size_t x)
{
    return x ? __builtin_ctzll((unsigned long long) x) : SIZE_MAX;
}

size_t size_pop_cnt(size_t x)
{
    return __builtin_popcountll((unsigned long long) x);
}

#   endif 
#elif defined _MSC_BUILD
#   include <intrin.h>

// Acquire semantics imposed by volatility. Should be used only with '/volatile:ms' option
#   define DECLARE_LOAD_ACQUIRE(TYPE, PREFIX) \
        TYPE PREFIX ## _load_acquire(volatile void *src) \
        { \
            return *(TYPE volatile *) src; \
        }

// Release semantics imposed by volatility. Should be used only with '/volatile:ms' option
#   define DECLARE_STORE_RELEASE(TYPE, PREFIX) \
        void PREFIX ## _store_release(volatile void *dst, TYPE val) \
        { \
            *(TYPE volatile *) dst = val; \
        }

#   define DECLARE_INTERLOCKED_COMPARE_EXCHANGE(TYPE, PREFIX, BACKEND_CAST, BACKEND, SUFFIX) \
        TYPE PREFIX ## _interlocked_compare_exchange ## SUFFIX(volatile void *dst, TYPE cmp, TYPE xchg) \
        { \
            return (TYPE) BACKEND((BACKEND_CAST volatile *) dst, (BACKEND_CAST) xchg, (BACKEND_CAST) cmp);\
        }

#   define DECLARE_INTERLOCKED_OP(TYPE, PREFIX, SUFFIX, BACKEND_CAST, BACKEND) \
        TYPE PREFIX ## _interlocked_ ## SUFFIX(volatile void *dst, TYPE arg) \
        { \
            return (TYPE) BACKEND((BACKEND_CAST volatile *) dst, (BACKEND_CAST) arg); \
        }

static DECLARE_LOAD_ACQUIRE(long, spinlock)
static DECLARE_STORE_RELEASE(long, spinlock)
static DECLARE_INTERLOCKED_COMPARE_EXCHANGE(long, spinlock, long, _InterlockedCompareExchange, _acquire)

DECLARE_INTERLOCKED_COMPARE_EXCHANGE(void *, ptr, void *, _InterlockedCompareExchangePointer,)

DECLARE_INTERLOCKED_OP(uint8_t, uint8, or, char, _InterlockedOr8)
DECLARE_INTERLOCKED_OP(uint8_t, uint8, or_release, char, _InterlockedOr8)
DECLARE_INTERLOCKED_OP(uint16_t, uint16, or, short, _InterlockedOr16)
DECLARE_INTERLOCKED_OP(uint8_t, uint8, and, char, _InterlockedAnd8)
DECLARE_INTERLOCKED_OP(uint8_t, uint8, and_release, char, _InterlockedAnd8)
DECLARE_INTERLOCKED_OP(uint16_t, uint16, and, short, _InterlockedAnd16)
DECLARE_INTERLOCKED_OP(void *, ptr, exchange, void *, _InterlockedExchangePointer)

uint32_t uint32_bit_scan_reverse(uint32_t x)
{
    unsigned long res;
    return (uint32_t) _BitScanReverse(&res, (unsigned long) x) ? (uint32_t) res : UINT32_MAX;
}

uint32_t uint32_bit_scan_forward(uint32_t x)
{
    unsigned long res;
    return (uint32_t) _BitScanForward(&res, (unsigned long) x) ? (uint32_t) res : UINT32_MAX;
}

uint32_t uint32_pop_cnt(uint32_t x)
{
    return (uint32_t) __popcnt((unsigned) x);
}

#   ifdef _M_X64

DECLARE_INTERLOCKED_OP(size_t, size, add, long long, _InterlockedExchangeAdd64)

// Warning! 'y %= 64' is done internally!
size_t size_shl(size_t *p_hi, size_t x, uint8_t y)
{
    *p_hi = (size_t) __shiftleft128((unsigned long long) x, 0, y);
    return x << y;
}

size_t size_shr(size_t *p_lo, size_t x, uint8_t y)
{
    *p_lo = (size_t) __shiftright128(0, (unsigned long long) x, y);
    return x >> y;
}

size_t size_mul(size_t *p_hi, size_t x, size_t y)
{
    unsigned long long hi;
    size_t res = (size_t) _umul128((unsigned long long) x, (unsigned long long) y, &hi);
    *p_hi = (size_t) hi;
    return res;
}

size_t size_add(size_t *p_car, size_t x, size_t y)
{
    unsigned long long res;
    *p_car = (size_t) _addcarry_u64(0, (unsigned long long) x, (unsigned long long) y, &res);
    return (size_t) res;
}

size_t size_sub(size_t *p_bor, size_t x, size_t y)
{
    unsigned long long res;
    *p_bor = (size_t) _subborrow_u64(0, (unsigned long long) x, (unsigned long long) y, &res);
    return (size_t) res;
}

size_t size_bit_scan_reverse(size_t x)
{
    unsigned long res;
    return (size_t) _BitScanReverse64(&res, (unsigned long long) x) ? (size_t) res : SIZE_MAX;
}

size_t size_bit_scan_forward(size_t x)
{
    unsigned long res;
    return (size_t) _BitScanForward64(&res, (unsigned long long) x) ? (size_t) res : SIZE_MAX;
}

size_t size_pop_cnt(size_t x)
{
    return (size_t) __popcnt64((unsigned long long) x);
}

#   elif defined _M_IX86

DECLARE_INTERLOCKED_OP(size_t, size, add, long long, _InterlockedExchangeAdd)

#   endif
#endif 

// Warning! 'Dsize_t' is not defined for MSVC under x86_64
#if defined __GNUC__ || defined __clang__ || defined _M_IX86 || defined __i386__
#   if defined _M_X64 || defined __x86_64__
typedef unsigned __int128 Dsize_t;
#   elif defined _M_IX86 || defined __i386__
typedef unsigned long long Dsize_t;
#   endif

size_t size_shl(size_t *p_hi, size_t x, uint8_t y)
{
    Dsize_t val = (Dsize_t) x << (y % SIZE_BIT);
    *p_hi = (size_t) (val >> SIZE_BIT);
    return (size_t) val;
}

size_t size_shr(size_t *p_lo, size_t x, uint8_t y)
{
    Dsize_t val = ((Dsize_t) x << SIZE_BIT) >> (y % SIZE_BIT);
    *p_lo = (size_t) val;
    return (size_t) (val >> SIZE_BIT);
}

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

DECLARE_LOAD_ACQUIRE(bool, bool)
DECLARE_LOAD_ACQUIRE(uint8_t, uint8)
DECLARE_LOAD_ACQUIRE(uint16_t, uint16)
DECLARE_LOAD_ACQUIRE(uint32_t, uint32)
DECLARE_LOAD_ACQUIRE(size_t, size)
DECLARE_LOAD_ACQUIRE(void *, ptr)
DECLARE_STORE_RELEASE(bool, bool)
DECLARE_STORE_RELEASE(uint8_t, uint8)
DECLARE_STORE_RELEASE(uint16_t, uint16)
DECLARE_STORE_RELEASE(uint32_t, uint32)
DECLARE_STORE_RELEASE(size_t, size)
DECLARE_STORE_RELEASE(void *, ptr)

size_t size_interlocked_sub(volatile void *mem, size_t val)
{
    return size_interlocked_add(mem, 0 - val);
}

size_t size_interlocked_inc(volatile void *mem)
{
    return size_interlocked_add(mem, 1);
}

size_t size_interlocked_dec(volatile void *mem)
{
    return size_interlocked_sub(mem, 1);
}

size_t size_sum(size_t *p_hi, size_t *argl, size_t cnt)
{
    if (!cnt)
    {
        *p_hi = 0;
        return 0;
    }
    size_t lo = argl[0], hi = 0, car;
    for (size_t i = 1; i < cnt; lo = size_add(&car, lo, argl[i++]), hi += car);
    *p_hi = hi;
    return lo;
}

size_t size_prod_test(size_t *p_res, size_t *argl, size_t cnt)
{
    if (!cnt)
    {
        *p_res = 1;
        return cnt;
    }
    size_t res = argl[0];
    for (size_t i = 1; i < cnt; i++)
    {
        size_t hi;
        res = size_mul(&hi, res, argl[i]);
        if (hi) return i;
    }
    *p_res = res;
    return cnt;
}

#define DECLARE_UINT_LOG10(TYPE, PREFIX, MAX, ...) \
    TYPE PREFIX ## _log10(TYPE x, bool ceil) \
    { \
        const TYPE arr[] = { __VA_ARGS__ }; \
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
        return ceil && left == countof(arr) - 1 && x > arr[left] ? countof(arr) : left; \
    }

#define TEN5(X) 1 ## X, 10 ## X, 100 ## X, 1000 ## X, 10000 ## X
#define TEN10(X) TEN5(X), TEN5(00000 ## X)
#define TEN20(X) TEN10(X), TEN10(0000000000 ## X)
DECLARE_UINT_LOG10(uint16_t, uint16, UINT16_MAX, TEN5(u))
DECLARE_UINT_LOG10(uint32_t, uint32, UINT32_MAX, TEN10(u))
DECLARE_UINT_LOG10(uint64_t, uint64, UINT64_MAX, TEN20(u))

// Copy-pasted from https://github.com/skeeto/hash-prospector
uint32_t uint32_hash(uint32_t x)
{
    x ^= x >> 16;
    x *= UINT32_C(0x7feb352d);
    x ^= x >> 15;
    x *= UINT32_C(0x846ca68b);
    x ^= x >> 16;
    return x;
}

uint32_t uint32_hash_inv(uint32_t x)
{
    x ^= x >> 16;
    x *= UINT32_C(0x43021123);
    x ^= x >> 15 ^ x >> 30;
    x *= UINT32_C(0x1d69e2a5);
    x ^= x >> 16;
    return x;
}

// Copy-pasted from https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
uint64_t uint64_hash(uint64_t x) 
{
    x ^= x >> 30;
    x *= UINT64_C(0xbf58476d1ce4e5b9);
    x ^= x >> 27;
    x *= UINT64_C(0x94d049bb133111eb);
    x ^= x >> 31;
    return x;
}

uint64_t uint64_hash_inv(uint64_t x)
{
    x ^= x >> 31 ^ x >> 62;
    x *= UINT64_C(0x319642b2d24d8ec3);
    x ^= x >> 27 ^ x >> 54;
    x *= UINT64_C(0x96de1b173f119089);
    x ^= x >> 30 ^ x >> 60;
    return x;
}

#if defined _M_X64 || defined __x86_64__

DECLARE_UINT_LOG10(size_t, size, SIZE_MAX, TEN20(u))

size_t size_hash(size_t x)
{
    return (size_t) uint64_hash((uint64_t) x);
}

size_t size_hash_inv(size_t x)
{
    return (size_t) uint64_hash_inv((uint64_t) x);
}

#elif defined _M_IX86 || defined __i386__

DECLARE_UINT_LOG10(size_t, size, SIZE_MAX, TEN10(u))

size_t size_hash(size_t x)
{
    return (size_t) uint32_hash((uint32_t) x);
}

size_t size_hash_inv(size_t x)
{
    return (size_t) uint32_hash_inv((uint32_t) x);
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

size_t m128i_byte_scan_forward(__m128i a)
{
    const __m128i msk[] = {
        _mm_set_epi64x(-1, 0),
        _mm_set_epi32(-1, 0, -1, 0),
        _mm_set_epi16(-1, 0, -1, 0, -1, 0, -1, 0),
        _mm_set_epi8(-1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0)
    };
    size_t res = 0;
    for (size_t i = 0, j = 1 << (countof(msk) - 1); i < countof(msk) - 1; i++, j >>= 1)
    {
        __m128i t = msk[i];
        if (_mm_testz_si128(a, t)) a = _mm_andnot_si128(t, a);
        else res += j, a = _mm_and_si128(t, a);
    }
    if (!_mm_testz_si128(a, msk[countof(msk) - 1])) res++;
    return res;
}

void spinlock_acquire(spinlock_handle *p_spinlock)
{
    while (spinlock_interlocked_compare_exchange_acquire(p_spinlock, 0, 1)) while (spinlock_load_acquire(p_spinlock)) _mm_pause();
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
        if (!spinlock_interlocked_compare_exchange_acquire(p_spinlock, 0, 1))
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

// Returns the sign of the a - b
int size_sign(size_t a, size_t b)
{
    size_t bor;
    return size_sub(&bor, a, b) ? 1 - (int) (bor << 1) : 0;
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
    return size_sign(*(size_t *) B, *(size_t *) A);
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

// Returns correct sign even for a NaN
int flt64_sign(double x)
{
    __m128i res = _mm_cmpeq_epi64(_mm_and_si128(_mm_castpd_si128(_mm_loaddup_pd(&x)), _mm_set_epi64x(0x8000000000000000, 0x7fffffffffffffff)), _mm_set_epi64x(0, 0));
    return _mm_cvtsi128_si32(res) + 1 ? -(_mm_extract_epi32(res, 2) << 1) - 1 : 0;
}

size_t size_fused_mul_add(size_t *p_res, size_t m, size_t a)
{
    size_t hi, lo = size_mul(&hi, *p_res, m), car;
    *p_res = size_add(&car, lo, a);
    return hi + car;
}

// On failure 'result' is untouched
bool size_mul_add_test(size_t *p_res, size_t m, size_t a)
{
    size_t hi, lo = size_mul(&hi, *p_res, m);
    if (hi) return 0;
    lo = size_add(&hi, lo, a);
    if (hi) return 0;
    *p_res = lo;
    return 1;
}

#define DECLARE_BIT_TEST(TYPE, PREFIX) \
    bool PREFIX ## _bit_test(TYPE *arr, size_t bit) \
    { \
        return arr[bit / (CHAR_BIT * sizeof(TYPE))] & ((TYPE) 1 << bit % (CHAR_BIT * sizeof(TYPE))); \
    }

#define DECLARE_BIT_TEST_ACQUIRE(TYPE, PREFIX) \
    bool PREFIX ## _bit_test_acquire(volatile void *arr, size_t bit) \
    { \
        return PREFIX ## _load_acquire((volatile TYPE *) arr + bit / (CHAR_BIT * sizeof(TYPE))) & ((TYPE) 1 << bit % (CHAR_BIT * sizeof(TYPE))); \
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

#define DECLARE_INTERLOCKED_BIT_TEST_SET(TYPE, PREFIX) \
    bool PREFIX ## _interlocked_bit_test_set(volatile void *arr, size_t bit) \
    { \
        TYPE msk = (TYPE) 1 << bit % (CHAR_BIT * sizeof(TYPE)), res = PREFIX ## _interlocked_or((volatile TYPE *) arr + bit / (CHAR_BIT * sizeof(TYPE)), msk); \
        return res & msk; \
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

#define DECLARE_INTERLOCKED_BIT_TEST_RESET(TYPE, PREFIX) \
    bool PREFIX ## _interlocked_bit_test_reset(volatile void *arr, size_t bit) \
    { \
        TYPE msk = (TYPE) 1 << bit % (CHAR_BIT * sizeof(TYPE)), res = PREFIX ## _interlocked_and((volatile TYPE *) arr + bit / (CHAR_BIT * sizeof(TYPE)), ~msk); \
        return res & msk; \
    }

#define DECLARE_BIT_SET(TYPE, PREFIX) \
    void PREFIX ## _bit_set(TYPE *arr, size_t bit) \
    { \
        arr[bit / (CHAR_BIT * sizeof(TYPE))] |= (TYPE) 1 << bit % (CHAR_BIT * sizeof(TYPE)); \
    }

#define DECLARE_INTERLOCKED_BIT_SET_RELEASE(TYPE, PREFIX) \
    void PREFIX ## _interlocked_bit_set_release(volatile void *arr, size_t bit) \
    { \
        PREFIX ## _interlocked_or_release((volatile TYPE *) arr + bit / (CHAR_BIT * sizeof(TYPE)), (TYPE) 1 << bit % (CHAR_BIT * sizeof(TYPE))); \
    }

#define DECLARE_BIT_RESET(TYPE, PREFIX) \
    void PREFIX ## _bit_reset(TYPE *arr, size_t bit) \
    { \
        arr[bit / (CHAR_BIT * sizeof(TYPE))] &= ~((TYPE) 1 << bit % (CHAR_BIT * sizeof(TYPE))); \
    }

#define DECLARE_INTERLOCKED_BIT_RESET_RELEASE(TYPE, PREFIX) \
    void PREFIX ## _interlocked_bit_reset_release(volatile void *arr, size_t bit) \
    { \
        PREFIX ## _interlocked_and_release((volatile TYPE *) arr + bit / (CHAR_BIT * sizeof(TYPE)), ~((TYPE) 1 << bit % (CHAR_BIT * sizeof(TYPE)))); \
    }

#define DECLARE_BIT_FETCH_BURST(TYPE, PREFIX, STRIDE) \
    TYPE PREFIX ## _bit_fetch_burst ## STRIDE(TYPE *arr, size_t pos) \
    { \
        return (arr[pos / (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1))] >> (pos % (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1)) << ((STRIDE) - 1))) & (((TYPE) 1 << (STRIDE)) - 1); \
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

#define DECLARE_ROL(TYPE, PREFIX) \
    TYPE PREFIX ## _rol(TYPE x, TYPE y) \
    { \
        y %= CHAR_BIT * sizeof(TYPE); \
        return (x << y) | (x >> (CHAR_BIT * sizeof(TYPE) - y)); \
    }

#define DECLARE_ROR(TYPE, PREFIX) \
    TYPE PREFIX ## _ror(TYPE x, TYPE y) \
    { \
        y %= CHAR_BIT * sizeof(TYPE); \
        return (x >> y) | (x << (CHAR_BIT * sizeof(TYPE) - y)); \
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

DECLARE_BIT_TEST_ACQUIRE(uint8_t, uint8)
DECLARE_INTERLOCKED_BIT_SET_RELEASE(uint8_t, uint8)
DECLARE_INTERLOCKED_BIT_RESET_RELEASE(uint8_t, uint8)
DECLARE_INTERLOCKED_BIT_TEST_SET(uint8_t, uint8)
DECLARE_INTERLOCKED_BIT_TEST_RESET(uint8_t, uint8)

DECLARE_BIT_TEST(size_t, size)
DECLARE_BIT_TEST_SET(size_t, size)
DECLARE_BIT_TEST_RESET(size_t, size)
DECLARE_BIT_SET(size_t, size)
DECLARE_BIT_RESET(size_t, size)

DECLARE_ROL(uint8_t, uint8)
DECLARE_ROL(uint16_t, uint16)
DECLARE_ROL(uint32_t, uint32)
DECLARE_ROL(uint64_t, uint64)
