#include "ll.h"

#include <stdint.h>
#include <limits.h>
#include <errno.h>

#define DECLARE_POP_CNT(TYPE, PREFIX, BACKEND) \
    TYPE PREFIX ## _pop_cnt(TYPE x) \
    { \
        return (TYPE) BACKEND(x); \
    }

#define XY1(X, Y) (X), (X) + (Y)
#define XY2(X, Y) XY1((X), (Y)), XY1((X) + (Y), (Y))
#define XY3(X, Y) XY2((X), (Y)), XY2((X) + (Y), (Y))
#define XY4(X, Y) XY3((X), (Y)), XY3((X) + (Y), (Y))
#define XY5(X, Y) XY4((X), (Y)), XY4((X) + (Y), (Y))
#define XY6(X, Y) XY5((X), (Y)), XY5((X) + (Y), (Y))
#define XY7(X, Y) XY6((X), (Y)), XY6((X) + (Y), (Y))
#define XY8(X, Y) XY7((X), (Y)), XY7((X) + (Y), (Y))

unsigned char uchar_bit_scan_reverse(unsigned char x)
{
    static const unsigned char res[] = { UCHAR_MAX, 0, XY1(1, 0), XY2(2, 0), XY3(3, 0), XY4(4, 0), XY5(5, 0), XY6(6, 0), XY7(7, 0) };
    _Static_assert(countof(res) - 1 == UCHAR_MAX, "");
    return res[x];
}

unsigned short ushrt_bit_scan_reverse(unsigned short x) { return (unsigned short) bit_scan_reverse((unsigned) x); }

unsigned char uchar_bit_scan_forward(unsigned char x) { return uchar_bit_scan_reverse((unsigned char) (x & (0 - x))); }
unsigned short ushrt_bit_scan_forward(unsigned short x) { return (unsigned short) bit_scan_forward((unsigned) x); }

unsigned char uchar_pop_cnt(unsigned char x)
{
    static const unsigned char res[] = { XY8(0, 1) };
    _Static_assert(countof(res) - 1 == UCHAR_MAX, "");
    return res[x];
}

#if defined __GNUC__ || defined __clang__

#   define DECLARE_BIT_SCAN_REVERSE(TYPE, PREFIX, BACKEND, MAX) \
        TYPE PREFIX ## _bit_scan_reverse(TYPE x) \
        { \
            return x ? sizeof(TYPE) * CHAR_BIT - (TYPE) BACKEND(x) - 1 : (MAX); \
        }

#   define DECLARE_BIT_SCAN_FORWARD(TYPE, PREFIX, BACKEND, MAX) \
        TYPE PREFIX ## _bit_scan_forward(TYPE x) \
        { \
            return  x ? (TYPE) BACKEND(x) : (MAX); \
        }

#   define DECLARE_INTERLOCKED_COMPARE_EXCHANGE(TYPE, PREFIX) \
        bool PREFIX ## _interlocked_compare_exchange(TYPE volatile *dst, TYPE *p_cmp, TYPE xchg) \
        { \
            return __atomic_compare_exchange_n((TYPE volatile *) dst, p_cmp, xchg, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); \
        }

// This should be used to force compiler to emit 'cmpxchg16b'/'cmpxchg8b' instuctions when:
// 1) gcc is targeted to 'x86_64', even if '-mcx16' flag set (see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=84522);
// 2) clang is targeted to 'i386'.
#   define DECLARE_INTERLOCKED_COMPARE_EXCHANGE_2(TYPE, PREFIX) \
        bool PREFIX ## _interlocked_compare_exchange(TYPE volatile *dst, TYPE *p_cmp, TYPE xchg) \
        { \
            TYPE cmp = *p_cmp; \
            TYPE res = __sync_val_compare_and_swap(dst, cmp, xchg); \
            if (res == cmp) return 1; \
            *p_cmp = res; \
            return 0; \
        }

// Bit scan reverse
DECLARE_BIT_SCAN_REVERSE(unsigned, uint, __builtin_clz, UINT_MAX)
DECLARE_BIT_SCAN_REVERSE(unsigned long, ulong, __builtin_clzl, ULONG_MAX)

// Bit scan forward
DECLARE_BIT_SCAN_FORWARD(unsigned, uint, __builtin_ctz, UINT_MAX)
DECLARE_BIT_SCAN_FORWARD(unsigned long, ulong, __builtin_ctzl, ULONG_MAX)

// Population count
unsigned short ushrt_pop_cnt(unsigned short x) { return (unsigned short) pop_cnt((unsigned) x); }
DECLARE_POP_CNT(unsigned, uint, __builtin_popcount)
DECLARE_POP_CNT(unsigned long, ulong, __builtin_popcountl)

// Atomic compare and swap
DECLARE_INTERLOCKED_COMPARE_EXCHANGE(unsigned char, uchar)
DECLARE_INTERLOCKED_COMPARE_EXCHANGE(unsigned short, ushrt)
DECLARE_INTERLOCKED_COMPARE_EXCHANGE(unsigned, uint)
DECLARE_INTERLOCKED_COMPARE_EXCHANGE(unsigned long, ulong)
#   if defined __clang__ && defined __i386__
DECLARE_INTERLOCKED_COMPARE_EXCHANGE_2(unsigned long long, ullong)
#   else
DECLARE_INTERLOCKED_COMPARE_EXCHANGE(unsigned long long, ullong)
#   endif
DECLARE_INTERLOCKED_COMPARE_EXCHANGE(void *, ptr)

#   ifdef __x86_64__
DECLARE_BIT_SCAN_REVERSE(unsigned long long, ullong, __builtin_clzll, ULLONG_MAX)
DECLARE_BIT_SCAN_FORWARD(unsigned long long, ullong, __builtin_ctzll, ULLONG_MAX)
DECLARE_POP_CNT(unsigned long long, ullong, __builtin_popcountll)
#       ifdef __GNUC__
DECLARE_INTERLOCKED_COMPARE_EXCHANGE_2(Uint128_t, Uint128)
#       else
DECLARE_INTERLOCKED_COMPARE_EXCHANGE(Uint128_t, Uint128)
#       endif
#   endif

#elif defined _MSC_BUILD

#   define DECLARE_BIT_SCAN(TYPE, PREFIX, SUFFIX, BACKEND, MAX) \
        TYPE PREFIX ## _bit_scan_ ## SUFFIX(TYPE x) \
        { \
            unsigned long res; \
            return BACKEND(&res, x) ? (TYPE) res : MAX; \
        }

#   define DECLARE_POP_CNT(TYPE, PREFIX, BACKEND) \
        TYPE PREFIX ## _pop_cnt(TYPE x) \
        { \
            return (TYPE) BACKEND(x); \
        }

#   define DECLARE_INTERLOCKED_COMPARE_EXCHANGE(TYPE, PREFIX, BACKEND_TYPE, BACKEND) \
        bool PREFIX ## _interlocked_compare_exchange(TYPE volatile *dst, TYPE *p_cmp, TYPE xchg) \
        { \
            TYPE cmp = *p_cmp; \
            TYPE res = (TYPE) BACKEND((BACKEND_TYPE volatile *) dst, (BACKEND_TYPE) xchg, (BACKEND_TYPE) cmp); \
            if (res == cmp) return 1; \
            *p_cmp = res; \
            return 0; \
        }

#   define DECLARE_INTERLOCKED_OP(TYPE, PREFIX, SUFFIX, BACKEND_TYPE, BACKEND) \
        TYPE PREFIX ## _interlocked_ ## SUFFIX(TYPE volatile *dst, TYPE arg) \
        { \
            return (TYPE) BACKEND((BACKEND_TYPE volatile *) dst, (BACKEND_TYPE) arg); \
        }

// Bit scan reverse
unsigned uint_bit_scan_reverse(unsigned x) { return (unsigned) bit_scan_reverse((unsigned long) x); }
DECLARE_BIT_SCAN(unsigned long, ulong, reverse, _BitScanReverse, ULONG_MAX)

// Bit scan forward
unsigned uint_bit_scan_forward(unsigned x) { return (unsigned) bit_scan_forward((unsigned long) x); }
DECLARE_BIT_SCAN(unsigned long, ulong, forward, _BitScanForward, ULONG_MAX)

// Population count
DECLARE_POP_CNT(unsigned short, ushrt, __popcnt16)
DECLARE_POP_CNT(unsigned, uint, __popcnt)
unsigned long ulong_pop_cnt(unsigned long x) { return (unsigned long) pop_cnt((unsigned) x); }

// Atomic compare and swap
DECLARE_INTERLOCKED_COMPARE_EXCHANGE(unsigned char, uchar, char, _InterlockedCompareExchange8)
DECLARE_INTERLOCKED_COMPARE_EXCHANGE(unsigned short, ushrt, short, _InterlockedCompareExchange16)
bool uint_interlocked_compare_exchange(volatile unsigned *dst, unsigned *p_cmp, unsigned xchg) { return interlocked_compare_exchange((volatile unsigned long *) dst, (unsigned long *) p_cmp, (unsigned long) xchg); }
DECLARE_INTERLOCKED_COMPARE_EXCHANGE(unsigned long, ulong, long, _InterlockedCompareExchange)
DECLARE_INTERLOCKED_COMPARE_EXCHANGE(unsigned long long, ullong, long long, _InterlockedCompareExchange64)
DECLARE_INTERLOCKED_COMPARE_EXCHANGE(void *, ptr, void *, _InterlockedCompareExchangePointer)

// Atomic AND
DECLARE_INTERLOCKED_OP(unsigned char, uchar, and, char, _InterlockedAnd8)
DECLARE_INTERLOCKED_OP(unsigned short, ushrt, and, short, _InterlockedAnd16)
unsigned uint_interlocked_and(volatile unsigned *dst, unsigned arg) { return (unsigned) interlocked_and((volatile unsigned long *) dst, (unsigned long) arg); }
DECLARE_INTERLOCKED_OP(unsigned long, ulint, and, long, _InterlockedAnd)

// Atomic OR
DECLARE_INTERLOCKED_OP(unsigned char, uchar, or, char, _InterlockedOr8)
DECLARE_INTERLOCKED_OP(unsigned short, ushrt, or , short, _InterlockedOr16)
unsigned uint_interlocked_or(volatile unsigned *dst, unsigned arg) { return (unsigned) interlocked_or((volatile unsigned long *) dst, (unsigned long) arg); }
DECLARE_INTERLOCKED_OP(unsigned long, ulint, or, long, _InterlockedOr)

// Atomic ADD
DECLARE_INTERLOCKED_OP(unsigned char, uchar, add , char, _InterlockedExchangeAdd8)
DECLARE_INTERLOCKED_OP(unsigned short, ushrt, add, short, _InterlockedExchangeAdd16)
unsigned uint_interlocked_add(volatile unsigned *dst, unsigned arg) { return (unsigned) interlocked_add((volatile unsigned long *) dst, (unsigned long) arg); }
DECLARE_INTERLOCKED_OP(unsigned long, ulint, add, long, _InterlockedExchangeAdd)

// Atomic exchange
DECLARE_INTERLOCKED_OP(unsigned char, uchar, exchange, char, _InterlockedExchange8)
DECLARE_INTERLOCKED_OP(unsigned short, ushrt, exchange, short, _InterlockedExchange16)
unsigned uint_interlocked_exchange(volatile unsigned *dst, unsigned arg) { return (unsigned) interlocked_exchange((volatile unsigned long *) dst, (unsigned long) arg); }
DECLARE_INTERLOCKED_OP(unsigned long, ulint, exchange, long, _InterlockedExchange)
DECLARE_INTERLOCKED_OP(void *, ptr, exchange, void *, _InterlockedExchangePointer)

#   ifdef _M_X64
DECLARE_BIT_SCAN(unsigned long long, ullong, reverse, _BitScanReverse64, ULLONG_MAX)
DECLARE_BIT_SCAN(unsigned long long, ullong, forward, _BitScanForward64, ULLONG_MAX)
DECLARE_POP_CNT(unsigned long long, ullong, __popcnt64)
bool Uint128_interlocked_compare_exchange(volatile Uint128_t *dst, Uint128_t *p_cmp, Uint128_t xchg) { return _InterlockedCompareExchange128((volatile long long *) dst->qw, UINT128_HI(xchg), UINT128_LO(xchg), (long long *) p_cmp->qw); }
DECLARE_INTERLOCKED_OP(unsigned long long, ullong, and, long long, _InterlockedAnd64)
DECLARE_INTERLOCKED_OP(unsigned long long, ullong, or, long long, _InterlockedOr64)
DECLARE_INTERLOCKED_OP(unsigned long long, ullong, add, long long, _InterlockedExchangeAdd64)
DECLARE_INTERLOCKED_OP(unsigned long long, ullong, exchange, long long, _InterlockedExchange64)

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
    return _BitScanReverse64(&res, (unsigned long long) x) ? (size_t) res : SIZE_MAX;
}

size_t size_bit_scan_forward(size_t x)
{
    unsigned long res;
    return _BitScanForward64(&res, (unsigned long long) x) ? (size_t) res : SIZE_MAX;
}

#   endif
#endif 

#if defined __GNUC__ || defined __clang__ || defined _M_IX86 || defined __i386__

size_t size_shl(size_t *p_hi, size_t x, uint8_t y)
{
    Dsize_t val = DSIZEC(x, 0) << (y % SIZE_BIT);
    *p_hi = DSIZE_HI(val);
    return DSIZE_LO(val);
}

size_t size_shr(size_t *p_lo, size_t x, uint8_t y)
{
    Dsize_t val = DSIZEC(0, x) >> (y % SIZE_BIT);
    *p_lo = DSIZE_LO(val);
    return DSIZE_HI(val);
}

size_t size_mul(size_t *p_hi, size_t x, size_t y)
{
    Dsize_t val = DSIZEC(x, 0) * DSIZEC(y, 0);
    *p_hi = DSIZE_HI(val);
    return DSIZE_LO(val);
}

size_t size_add(size_t *p_car, size_t x, size_t y)
{
    Dsize_t val = DSIZEC(x, 0) + DSIZEC(y, 0);
    *p_car = DSIZE_HI(val);
    return DSIZE_LO(val);
}

size_t size_sub(size_t *p_bor, size_t x, size_t y)
{
    Dsize_t val = DSIZEC(x, 0) - DSIZEC(y, 0);
    *p_bor = 0 - DSIZE_HI(val);
    return DSIZE_LO(val);
}

#endif

size_t size_interlocked_add_sat(volatile size_t *mem, size_t val)
{
    size_t tmp = load_acquire(mem);
    while (tmp < SIZE_MAX && !interlocked_compare_exchange(mem, &tmp, size_add_sat(tmp, val))) tmp = load_acquire(mem);
    return tmp;
}

size_t size_interlocked_sub_sat(volatile size_t *mem, size_t val)
{
    size_t tmp = load_acquire(mem);
    while (tmp && !interlocked_compare_exchange(mem, &tmp, size_sub_sat(tmp, val))) tmp = load_acquire(mem);
    return tmp;
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

#endif

size_t size_log2(size_t x, bool ceil)
{
    return bit_scan_reverse(x) + (ceil && x && (x & (x - 1)));
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

void spinlock_acquire(spinlock *spinlock)
{
    while (!interlocked_compare_exchange_acquire(spinlock, &(spinlock_base) { 0 }, 1)) while (load_acquire(spinlock)) _mm_pause();
}

GPUSH GWRN(implicit-fallthrough)
void *double_lock_execute(spinlock *spinlock, double_lock_callback init, double_lock_callback comm, void *init_args, void *comm_args)
{
    void *res = NULL;
    switch (load_acquire(spinlock))
    {
    case 0:
        if (interlocked_compare_exchange_acquire(spinlock, &(spinlock_base) { 0 }, 1)) // Strong CAS required here!
        {
            if (init) res = init(init_args);
            store_release(spinlock, 2);
            break;
        }
    case 1: 
        while (load_acquire(spinlock) != 2) _mm_pause();
    case 2:
        if (comm) res = comm(comm_args);
    }
    return res;
}
GPOP

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

// Returns correct sign even for a NaN: 'return x == 0. ? 0 : 1 - 2 * signbit(x)'
int flt64_sign(double x)
{
    int res = _mm_movemask_pd(_mm_castsi128_pd(_mm_cmpeq_epi64(
        _mm_and_si128(_mm_castpd_si128(_mm_loaddup_pd(&x)), _mm_set_epi64x(0x8000000000000000, 0x7fffffffffffffff)), _mm_setzero_si128())));
    return res & 1 ? 0 : res - 1;
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
        return load_acquire((volatile TYPE *) arr + bit / (CHAR_BIT * sizeof(TYPE))) & ((TYPE) 1 << bit % (CHAR_BIT * sizeof(TYPE))); \
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
        TYPE msk = (TYPE) 1 << bit % (CHAR_BIT * sizeof(TYPE)), res = interlocked_or((volatile TYPE *) arr + bit / (CHAR_BIT * sizeof(TYPE)), msk); \
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
        TYPE msk = (TYPE) 1 << bit % (CHAR_BIT * sizeof(TYPE)), res = interlocked_and((volatile TYPE *) arr + bit / (CHAR_BIT * sizeof(TYPE)), ~msk); \
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
        interlocked_or_release((volatile TYPE *) arr + bit / (CHAR_BIT * sizeof(TYPE)), (TYPE) 1 << bit % (CHAR_BIT * sizeof(TYPE))); \
    }

#define DECLARE_BIT_RESET(TYPE, PREFIX) \
    void PREFIX ## _bit_reset(TYPE *arr, size_t bit) \
    { \
        arr[bit / (CHAR_BIT * sizeof(TYPE))] &= ~((TYPE) 1 << bit % (CHAR_BIT * sizeof(TYPE))); \
    }

#define DECLARE_INTERLOCKED_BIT_RESET_RELEASE(TYPE, PREFIX) \
    void PREFIX ## _interlocked_bit_reset_release(volatile void *arr, size_t bit) \
    { \
        interlocked_and_release((volatile TYPE *) arr + bit / (CHAR_BIT * sizeof(TYPE)), ~((TYPE) 1 << bit % (CHAR_BIT * sizeof(TYPE)))); \
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
