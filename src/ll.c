#include "ll.h"

#include <stdint.h>
#include <limits.h>
#include <errno.h>

#define DECL_OP(TYPE, PREFIX, SUFFIX, OP_TYPE, OP) \
    TYPE PREFIX ## _ ## SUFFIX(TYPE *p_hi, TYPE x, TYPE y) \
    { \
        _Static_assert(bitsof(OP_TYPE) == 2 * bitsof(TYPE), ""); \
        OP_TYPE res = (OP_TYPE) x OP (OP_TYPE) y; \
        *p_hi = (TYPE) (res >> bitsof(TYPE)); \
        return (TYPE) res; \
    }

#define DECL_SHL(TYPE, PREFIX, OP_TYPE) \
    TYPE PREFIX ## _shl(TYPE *p_hi, TYPE x, unsigned char y) \
    { \
        _Static_assert(bitsof(OP_TYPE) == 2 * bitsof(TYPE), ""); \
        OP_TYPE val = (OP_TYPE) x << (OP_TYPE) y % bitsof(TYPE); \
        *p_hi = (TYPE) (val >> bitsof(TYPE)); \
        return (TYPE) (val); \
    }

#define DECL_SHR(TYPE, PREFIX, OP_TYPE) \
    TYPE PREFIX ## _shr(TYPE *p_lo, TYPE x, unsigned char y) \
    { \
        _Static_assert(bitsof(OP_TYPE) == 2 * bitsof(TYPE), ""); \
        OP_TYPE val = (OP_TYPE) x >> (OP_TYPE) y % bitsof(TYPE); \
        *p_lo = (TYPE) (val); \
        return (TYPE) (val >> bitsof(TYPE)); \
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

#define DECL_POP_CNT(TYPE, PREFIX, BACKEND) \
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

// Multiply
DECL_OP(unsigned char, uchar, mul, unsigned short, *)
DECL_OP(unsigned short, ushort, mul, unsigned, *)

// Shift left
DECL_SHL(unsigned char, uchar, unsigned short)
DECL_SHL(unsigned short, ushort, unsigned)

// Shift right
DECL_SHR(unsigned char, uchar, unsigned short)
DECL_SHR(unsigned short, ushort, unsigned)

// Bit scan reverse
unsigned char uchar_bit_scan_reverse(unsigned char x)
{
    static const unsigned char res[] = { UCHAR_MAX, 0, XY1(1, 0), XY2(2, 0), XY3(3, 0), XY4(4, 0), XY5(5, 0), XY6(6, 0), XY7(7, 0) };
    _Static_assert(countof(res) - 1 == UCHAR_MAX, "");
    return res[x];
}

unsigned short ushrt_bit_scan_reverse(unsigned short x) { return (unsigned short) bit_scan_reverse((unsigned) x); }

// Bit scan forward
unsigned char uchar_bit_scan_forward(unsigned char x) { return uchar_bit_scan_reverse((unsigned char) (x & (0 - x))); }
unsigned short ushrt_bit_scan_forward(unsigned short x) { return (unsigned short) bit_scan_forward((unsigned) x); }

// Population count
unsigned char uchar_pop_cnt(unsigned char x)
{
    static const unsigned char res[] = { XY8(0, 1) };
    _Static_assert(countof(res) - 1 == UCHAR_MAX, "");
    return res[x];
}

#if ((defined __GNUC__ || defined __clang__) && defined __i386__) || (defined _MSC_BUILD)

// Multiply (MSVC or [GCC, 32 bit])
DECL_OP(unsigned, uint, mul, unsigned long long, *)
DECL_OP(unsigned long, ulong, mul, unsigned long long, *)

// Shift left (MSVC or [GCC, 32 bit])
DECL_SHL(unsigned, uint, unsigned long long)
DECL_SHL(unsigned long, ulong, unsigned long long)

// Shift right (MSVC or [GCC, 32 bit])
DECL_SHR(unsigned, uint, unsigned long long)
DECL_SHR(unsigned long, ulong, unsigned long long)

#endif

#if defined __GNUC__ || defined __clang__

#   define DECL_OP_2(TYPE, PREFIX, SUFFIX, BACKEND) \
        TYPE PREFIX ## _ ## SUFFIX(TYPE *p_hi, TYPE x, TYPE y) \
        { \
            TYPE res; \
            *p_hi = BACKEND(x, y, &res); \
            return res; \
        }

#   define DECL_BIT_SCAN_REVERSE(TYPE, PREFIX, BACKEND, MAX) \
        TYPE PREFIX ## _bit_scan_reverse(TYPE x) \
        { \
            return x ? bitsof(TYPE) - (TYPE) BACKEND(x) - 1 : (MAX); \
        }

#   define DECL_BIT_SCAN_FORWARD(TYPE, PREFIX, BACKEND, MAX) \
        TYPE PREFIX ## _bit_scan_forward(TYPE x) \
        { \
            return  x ? (TYPE) BACKEND(x) : (MAX); \
        }

#   define DECL_ATOMIC_COMPARE_EXCHANGE(TYPE, PREFIX) \
        bool PREFIX ## _atomic_compare_exchange(TYPE volatile *dst, TYPE *p_cmp, TYPE xchg) \
        { \
            return __atomic_compare_exchange_n((TYPE volatile *) dst, p_cmp, xchg, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE); \
        }

// This should be used to force compiler to emit 'cmpxchg16b'/'cmpxchg8b' instuctions when:
// 1) gcc is targeted to 'x86_64', even if '-mcx16' flag set (see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=84522);
// 2) clang is targeted to 'i386'.
#   define DECL_ATOMIC_COMPARE_EXCHANGE_2(TYPE, PREFIX) \
        bool PREFIX ## _atomic_compare_exchange(TYPE volatile *dst, TYPE *p_cmp, TYPE xchg) \
        { \
            TYPE cmp = *p_cmp; \
            TYPE res = __sync_val_compare_and_swap(dst, cmp, xchg); \
            if (res == cmp) return 1; \
            *p_cmp = res; \
            return 0; \
        }

// Add (GCC)
DECL_OP_2(unsigned char, uchar, add, __builtin_add_overflow)
DECL_OP_2(unsigned short, ushrt, add, __builtin_add_overflow)
DECL_OP_2(unsigned, uint, add, __builtin_add_overflow)
DECL_OP_2(unsigned long, ulong, add, __builtin_add_overflow)
DECL_OP_2(unsigned long long, ullong, add, __builtin_add_overflow)

// Subtract (GCC)
DECL_OP_2(unsigned char, uchar, sub, __builtin_sub_overflow)
DECL_OP_2(unsigned short, ushrt, sub, __builtin_sub_overflow)
DECL_OP_2(unsigned, uint, sub, __builtin_sub_overflow)
DECL_OP_2(unsigned long, ulong, sub, __builtin_sub_overflow)
DECL_OP_2(unsigned long long, ullong, sub, __builtin_sub_overflow)

// Bit scan reverse (GCC)
DECL_BIT_SCAN_REVERSE(unsigned, uint, __builtin_clz, UINT_MAX)
DECL_BIT_SCAN_REVERSE(unsigned long, ulong, __builtin_clzl, ULONG_MAX)

// Bit scan forward (GCC)
DECL_BIT_SCAN_FORWARD(unsigned, uint, __builtin_ctz, UINT_MAX)
DECL_BIT_SCAN_FORWARD(unsigned long, ulong, __builtin_ctzl, ULONG_MAX)

// Population count (GCC)
unsigned short ushrt_pop_cnt(unsigned short x) { return (unsigned short) pop_cnt((unsigned) x); }
DECL_POP_CNT(unsigned, uint, __builtin_popcount)
DECL_POP_CNT(unsigned long, ulong, __builtin_popcountl)

// Atomic compare and swap (GCC)
DECL_ATOMIC_COMPARE_EXCHANGE(unsigned char, uchar)
DECL_ATOMIC_COMPARE_EXCHANGE(unsigned short, ushrt)
DECL_ATOMIC_COMPARE_EXCHANGE(unsigned, uint)
DECL_ATOMIC_COMPARE_EXCHANGE(unsigned long, ulong)
DECL_ATOMIC_COMPARE_EXCHANGE(void *, ptr)

#   ifdef __i386__

// Atomic compare and swap (GCC, 32 bit)
#       ifdef __clang__
DECL_ATOMIC_COMPARE_EXCHANGE_2(unsigned long long, ullong)
#       else
DECL_ATOMIC_COMPARE_EXCHANGE(unsigned long long, ullong)
#       endif

#   elif defined __x86_64__

// Multiply (GCC, 64 bit)
DECL_OP(unsigned, uint, mul, unsigned long, *)
DECL_OP(unsigned long, ulong, mul, Uint128_t, *)
DECL_OP(unsigned long long, ullong, mul, Uint128_t, *)

// Shift left (GCC, 64 bit)
DECL_SHL(unsigned, uint, unsigned long)
DECL_SHL(unsigned long, ulong, Uint128_t)
DECL_SHL(unsigned long long, ullong, Uint128_t)

// Shift right (GCC, 64 bit)
DECL_SHR(unsigned, uint, unsigned long)
DECL_SHR(unsigned long, ulong, Uint128_t)
DECL_SHR(unsigned long long, ullong, Uint128_t)

// Bit scan reverse (GCC, 64 bit)
DECL_BIT_SCAN_REVERSE(unsigned long long, ullong, __builtin_clzll, ULLONG_MAX)

// Bit scan forward (GCC, 64 bit)
DECL_BIT_SCAN_FORWARD(unsigned long long, ullong, __builtin_ctzll, ULLONG_MAX)

// Population count (GCC, 64 bit)
DECL_POP_CNT(unsigned long long, ullong, __builtin_popcountll)

// Atomic compare and swap (GCC, 64 bit)
#       ifdef __GNUC__
DECL_ATOMIC_COMPARE_EXCHANGE_2(Uint128_t, Uint128)
#       else
DECL_ATOMIC_COMPARE_EXCHANGE(Uint128_t, Uint128)
#       endif
#   endif

#elif defined _MSC_BUILD

#   define DECL_OP_2(TYPE, PREFIX, SUFFIX, BACKEND) \
        TYPE PREFIX ## _ ## SUFFIX(TYPE *p_hi, TYPE x, TYPE y) \
        { \
            TYPE res; \
            *p_hi = BACKEND(0, x, y, &res); \
            return res; \
        }

#   define DECL_OP_3(TYPE, PREFIX, SUFFIX, BACKEND_TYPE, BACKEND) \
        TYPE PREFIX ## _ ## SUFFIX(TYPE *p_car, TYPE x, TYPE y) \
        { \
            _Static_assert(bitsof(BACKEND_TYPE) == 2 * bitsof(TYPE), ""); \
            BACKEND_TYPE lo, hi; \
            *p_car = (TYPE) BACKEND(BACKEND(0, (TYPE) x, (TYPE) y, &lo), (TYPE) (x >> bitsof(TYPE)), (TYPE) (y >> bitsof(TYPE)), &hi); \
            return (TYPE) lo | ((TYPE) hi << bitsof(BACKEND_TYPE)); \
        }

#   define DECL_BIT_SCAN(TYPE, PREFIX, SUFFIX, BACKEND, MAX) \
        TYPE PREFIX ## _bit_scan_ ## SUFFIX(TYPE x) \
        { \
            unsigned long res; \
            return BACKEND(&res, x) ? (TYPE) res : MAX; \
        }

#   define DECL_POP_CNT(TYPE, PREFIX, BACKEND) \
        TYPE PREFIX ## _pop_cnt(TYPE x) \
        { \
            return (TYPE) BACKEND(x); \
        }

#   define DECL_ATOMIC_COMPARE_EXCHANGE(TYPE, PREFIX, BACKEND_TYPE, BACKEND) \
        bool PREFIX ## _atomic_compare_exchange(TYPE volatile *dst, TYPE *p_cmp, TYPE xchg) \
        { \
            _Static_assert(bitsof(BACKEND_TYPE) == bitsof(TYPE), ""); \
            TYPE cmp = *p_cmp; \
            TYPE res = (TYPE) BACKEND((BACKEND_TYPE volatile *) dst, (BACKEND_TYPE) xchg, (BACKEND_TYPE) cmp); \
            if (res == cmp) return 1; \
            *p_cmp = res; \
            return 0; \
        }

#   define DECL_ATOMIC_OP(TYPE, PREFIX, SUFFIX, BACKEND_TYPE, BACKEND) \
        TYPE PREFIX ## _atomic_ ## SUFFIX(TYPE volatile *dst, TYPE arg) \
        { \
            _Static_assert(bitsof(BACKEND_TYPE) == bitsof(TYPE), ""); \
            return (TYPE) BACKEND((BACKEND_TYPE volatile *) dst, (BACKEND_TYPE) arg); \
        }

#   define DECL_ATOMIC_SUB(TYPE, PREFIX) \
        TYPE PREFIX ## _atomic_sub(TYPE volatile *dst, TYPE arg) \
        { \
            return (TYPE) atomic_add((TYPE volatile *) dst, 0 - arg); \
        }

// Add (MSVC)
DECL_OP_2(unsigned char, uchar, add, _addcarry_u8)
DECL_OP_2(unsigned short, ushort, add, _addcarry_u16)
DECL_OP_2(unsigned, uint, add, _addcarry_u32)

unsigned long ulong_add(unsigned long *p_car, unsigned long x, unsigned long y) 
{ 
    _Static_assert(bitsof(unsigned long) == bitsof(unsigned), "");
    return (unsigned long) add((unsigned *) p_car, (unsigned) x, (unsigned) y); 
}

// Subtract (MSVC)
DECL_OP_2(unsigned char, uchar, sub, _subborrow_u8)
DECL_OP_2(unsigned short, ushort, sub, _subborrow_u16)
DECL_OP_2(unsigned, uint, sub, _subborrow_u32)

unsigned long ulong_sub(unsigned long *p_car, unsigned long x, unsigned long y) 
{ 
    _Static_assert(bitsof(unsigned long) == bitsof(unsigned), "");
    return (unsigned long) sub((unsigned *) p_car, (unsigned) x, (unsigned) y); 
}

// Bit scan reverse (MSVC)
unsigned uint_bit_scan_reverse(unsigned x) 
{ 
    _Static_assert(bitsof(unsigned) == bitsof(unsigned long), "");
    return (unsigned) bit_scan_reverse((unsigned long) x); 
}

DECL_BIT_SCAN(unsigned long, ulong, reverse, _BitScanReverse, ULONG_MAX)

// Bit scan forward (MSVC)
unsigned uint_bit_scan_forward(unsigned x) 
{ 
    _Static_assert(bitsof(unsigned) == bitsof(unsigned long), "");
    return (unsigned) bit_scan_forward((unsigned long) x); 
}

DECL_BIT_SCAN(unsigned long, ulong, forward, _BitScanForward, ULONG_MAX)

// Population count (MSVC)
DECL_POP_CNT(unsigned short, ushrt, __popcnt16)
DECL_POP_CNT(unsigned, uint, __popcnt)
unsigned long ulong_pop_cnt(unsigned long x) { return (unsigned long) pop_cnt((unsigned) x); }

// Atomic compare and swap (MSVC)
DECL_ATOMIC_COMPARE_EXCHANGE(unsigned char, uchar, char, _InterlockedCompareExchange8)
DECL_ATOMIC_COMPARE_EXCHANGE(unsigned short, ushrt, short, _InterlockedCompareExchange16)

bool uint_atomic_compare_exchange(volatile unsigned *dst, unsigned *p_cmp, unsigned xchg) 
{ 
    _Static_assert(bitsof(unsigned) == bitsof(unsigned long), "");
    return atomic_compare_exchange((volatile unsigned long *) dst, (unsigned long *) p_cmp, (unsigned long) xchg);
}

DECL_ATOMIC_COMPARE_EXCHANGE(unsigned long, ulong, long, _InterlockedCompareExchange)
DECL_ATOMIC_COMPARE_EXCHANGE(unsigned long long, ullong, long long, _InterlockedCompareExchange64)
DECL_ATOMIC_COMPARE_EXCHANGE(void *, ptr, void *, _InterlockedCompareExchangePointer)

// Atomic AND (MSVC)
DECL_ATOMIC_OP(unsigned char, uchar, and, char, _InterlockedAnd8)
DECL_ATOMIC_OP(unsigned short, ushrt, and, short, _InterlockedAnd16)

unsigned uint_atomic_and(volatile unsigned *dst, unsigned arg) 
{ 
    _Static_assert(bitsof(unsigned) == bitsof(unsigned long), "");
    return (unsigned) atomic_and((volatile unsigned long *) dst, (unsigned long) arg); 
}

DECL_ATOMIC_OP(unsigned long, ulong, and, long, _InterlockedAnd)

// Atomic OR (MSVC)
DECL_ATOMIC_OP(unsigned char, uchar, or, char, _InterlockedOr8)
DECL_ATOMIC_OP(unsigned short, ushrt, or , short, _InterlockedOr16)

unsigned uint_atomic_or(volatile unsigned *dst, unsigned arg)
{ 
    _Static_assert(bitsof(unsigned) == bitsof(unsigned long), "");
    return (unsigned) atomic_or((volatile unsigned long *) dst, (unsigned long) arg);
}

DECL_ATOMIC_OP(unsigned long, ulong, or, long, _InterlockedOr)

// Atomic add (MSVC)
DECL_ATOMIC_OP(unsigned char, uchar, add, char, _InterlockedExchangeAdd8)
DECL_ATOMIC_OP(unsigned short, ushrt, add, short, _InterlockedExchangeAdd16)

unsigned uint_atomic_add(volatile unsigned *dst, unsigned arg) 
{ 
    _Static_assert(bitsof(unsigned) == bitsof(unsigned long), "");
    return (unsigned) atomic_add((volatile unsigned long *) dst, (unsigned long) arg); 
}

DECL_ATOMIC_OP(unsigned long, ulong, add, long, _InterlockedExchangeAdd)

// Atomic subtract (MSVC)
DECL_ATOMIC_SUB(unsigned char, uchar)
DECL_ATOMIC_SUB(unsigned short, ushrt)
DECL_ATOMIC_SUB(unsigned, uint)
DECL_ATOMIC_SUB(unsigned long, ulong)

// Atomic exchange (MSVC)
DECL_ATOMIC_OP(unsigned char, uchar, exchange, char, _InterlockedExchange8)
DECL_ATOMIC_OP(unsigned short, ushrt, exchange, short, _InterlockedExchange16)

unsigned uint_atomic_exchange(volatile unsigned *dst, unsigned arg) 
{ 
    _Static_assert(bitsof(unsigned) == bitsof(unsigned long), "");
    return (unsigned) atomic_exchange((volatile unsigned long *) dst, (unsigned long) arg);
}

DECL_ATOMIC_OP(unsigned long, ulong, exchange, long, _InterlockedExchange)
DECL_ATOMIC_OP(void *, ptr, exchange, void *, _InterlockedExchangePointer)

#   ifdef _M_IX86

// Add (MSVC, 32 bit)
DECL_OP_3(unsigned long long, ullong, add, unsigned, _addcarry_u32)

// Subtract (MSVC, 32 bit)
DECL_OP_3(unsigned long long, ullong, sub, unsigned, _subborrow_u32)

#   elif defined _M_X64

// Add (MSVC, 64 bit)
DECL_OP_2(unsigned long long, ullong, add, _addcarry_u64)

// Subtract (MSVC, 64 bit)
DECL_OP_2(unsigned long long, ullong, sub, _subborrow_u64)

// Multiply (MSVC, 64 bit)
unsigned long long ullong_mul(unsigned long long *p_hi, unsigned long long x, unsigned long long y) 
{ 
    return _umul128(x, y, p_hi); 
}

// Shift left (MSVC, 64 bit)
unsigned long long ullong_shl(unsigned long long *p_hi, unsigned long long x, unsigned char y)
{
    *p_hi = __shiftleft128(x, 0, y); // Warning! 'y %= 64' is done internally!
    return x << y;
}

// Shift right (MSVC, 64 bit)
unsigned long long ullong_shr(unsigned long long *p_lo, unsigned long long x, unsigned char y)
{
    *p_lo = __shiftright128(0, x, y); // Warning! 'y %= 64' is done internally!
    return x >> y;
}

// Bit scan reverse (MSVC, 64 bit)
DECL_BIT_SCAN(unsigned long long, ullong, reverse, _BitScanReverse64, ULLONG_MAX)

// Bit scan forward (MSVC, 64 bit)
DECL_BIT_SCAN(unsigned long long, ullong, forward, _BitScanForward64, ULLONG_MAX)

// Population count (MSVC, 64 bit)
DECL_POP_CNT(unsigned long long, ullong, __popcnt64)

// Atomic compare and swap (MSVC, 64 bit)
bool Uint128_atomic_compare_exchange(volatile Uint128_t *dst, Uint128_t *p_cmp, Uint128_t xchg)
{ 
    _Static_assert(bitsof(Uint128_t) == 2 * bitsof(long long), "");
    return _InterlockedCompareExchange128((volatile long long *) dst->qw, UINT128_HI(xchg), UINT128_LO(xchg), (long long *) p_cmp->qw);
}

// Atomic AND (MSVC, 64 bit)
DECL_ATOMIC_OP(unsigned long long, ullong, and, long long, _InterlockedAnd64)

// Atomic OR (MSVC, 64 bit)
DECL_ATOMIC_OP(unsigned long long, ullong, or, long long, _InterlockedOr64)

// Atomic add (MSVC, 64 bit)
DECL_ATOMIC_OP(unsigned long long, ullong, add, long long, _InterlockedExchangeAdd64)

// Atomic subtract (MSVC, 64 bit)
DECL_ATOMIC_SUB(unsigned long long, ullong)

// Atomic exchange (MSVC, 64 bit)
DECL_ATOMIC_OP(unsigned long long, ullong, exchange, long long, _InterlockedExchange64)

#   endif
#endif 

size_t size_atomic_add_sat(volatile size_t *mem, size_t val)
{
    size_t tmp = load_acquire(mem);
    while (tmp < SIZE_MAX && !atomic_compare_exchange(mem, &tmp, size_add_sat(tmp, val))) tmp = load_acquire(mem);
    return tmp;
}

size_t size_atomic_sub_sat(volatile size_t *mem, size_t val)
{
    size_t tmp = load_acquire(mem);
    while (tmp && !atomic_compare_exchange(mem, &tmp, size_sub_sat(tmp, val))) tmp = load_acquire(mem);
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

#define DECL_UINT_LOG10(TYPE, PREFIX, MAX, ...) \
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
DECL_UINT_LOG10(uint16_t, uint16, UINT16_MAX, TEN5(u))
DECL_UINT_LOG10(uint32_t, uint32, UINT32_MAX, TEN10(u))
DECL_UINT_LOG10(uint64_t, uint64, UINT64_MAX, TEN20(u))

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

DECL_UINT_LOG10(size_t, size, SIZE_MAX, TEN20(u))

size_t size_hash(size_t x)
{
    return (size_t) uint64_hash((uint64_t) x);
}

size_t size_hash_inv(size_t x)
{
    return (size_t) uint64_hash_inv((uint64_t) x);
}

#elif defined _M_IX86 || defined __i386__

DECL_UINT_LOG10(size_t, size, SIZE_MAX, TEN10(u))

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
    while (!atomic_compare_exchange_acquire(spinlock, &(spinlock_base) { 0 }, 1)) while (load_acquire(spinlock)) _mm_pause();
}

GPUSH GWRN(implicit-fallthrough)
void *double_lock_execute(spinlock *spinlock, double_lock_callback init, double_lock_callback comm, void *init_args, void *comm_args)
{
    void *res = NULL;
    switch (load_acquire(spinlock))
    {
    case 0:
        if (atomic_compare_exchange_acquire(spinlock, &(spinlock_base) { 0 }, 1)) // Strong CAS required here!
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

#define DECL_STABLE_CMP_ASC(PREFIX, SUFFIX) \
    int PREFIX ## _stable_cmp_asc ## SUFFIX (const void *A, const void *B, void *thunk) \
    { \
        return PREFIX ## _stable_cmp_dsc ## SUFFIX (B, A, thunk);\
    }

#define DECL_CMP_ASC(PREFIX, SUFFIX) \
    bool PREFIX ## _cmp_asc ## SUFFIX (const void *A, const void *B, void *thunk) \
    { \
        return PREFIX ## _cmp_dsc ## SUFFIX (B, A, thunk);\
    }

int size_stable_cmp_dsc(const void *A, const void *B, void *thunk)
{
    (void) thunk;
    return size_sign(*(size_t *) B, *(size_t *) A);
}

DECL_STABLE_CMP_ASC(size, )

bool size_cmp_dsc(const void *A, const void *B, void *thunk)
{
    (void) thunk;
    return *(size_t *) B > *(size_t *) A;
}

DECL_CMP_ASC(size, )

int flt64_stable_cmp_dsc(const void *a, const void *b, void *thunk)
{
    (void) thunk;
    __m128d ab = _mm_loadh_pd(_mm_load_sd(a), b);
    __m128i res = _mm_castpd_si128(_mm_cmplt_pd(ab, _mm_permute_pd(ab, 1)));
    return _mm_extract_epi32(res, 2) - _mm_cvtsi128_si32(res);
}

DECL_STABLE_CMP_ASC(flt64, )

int flt64_stable_cmp_dsc_abs(const void *a, const void *b, void *thunk)
{
    (void) thunk;
    __m128d ab = _mm_and_pd(_mm_loadh_pd(_mm_load_sd(a), b), _mm_castsi128_pd(_mm_set1_epi64x(0x7fffffffffffffff)));
    __m128i res = _mm_castpd_si128(_mm_cmplt_pd(ab, _mm_permute_pd(ab, 1)));
    return _mm_extract_epi32(res, 2) - _mm_cvtsi128_si32(res);
}

DECL_STABLE_CMP_ASC(flt64, _abs)

// Warning! The approach from the 'DECL_STABLE_CMP_ASC' marcro doesn't work here
#define DECL_FLT64_STABLE_CMP_NAN(INFIX, DIR) \
    int flt64_stable_cmp ## INFIX ## _nan(const void *a, const void *b, void *thunk) \
    { \
        (void) thunk; \
        __m128d ab = _mm_loadh_pd(_mm_load_sd(a), b); \
        __m128i res = _mm_sub_epi32(_mm_castpd_si128(_mm_cmpunord_pd(ab, ab)), _mm_castpd_si128(_mm_cmp_pd(ab, _mm_permute_pd(ab, 1), (DIR)))); \
        return _mm_extract_epi32(res, 2) - _mm_cvtsi128_si32(res); \
    }

DECL_FLT64_STABLE_CMP_NAN(_dsc, _CMP_NLE_UQ)
DECL_FLT64_STABLE_CMP_NAN(_asc, _CMP_NGE_UQ)

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

#define DECL_BIT_TEST(TYPE, PREFIX) \
    bool PREFIX ## _bit_test(TYPE *arr, size_t bit) \
    { \
        return arr[bit / bitsof(TYPE)] & ((TYPE) 1 << bit % bitsof(TYPE)); \
    }

#define DECL_BIT_TEST_ACQUIRE(TYPE, PREFIX) \
    bool PREFIX ## _bit_test_acquire(volatile void *arr, size_t bit) \
    { \
        return load_acquire((volatile TYPE *) arr + bit / bitsof(TYPE)) & ((TYPE) 1 << bit % bitsof(TYPE)); \
    }

#define DECL_BIT_TEST_SET(TYPE, PREFIX) \
    bool PREFIX ## _bit_test_set(TYPE *arr, size_t bit) \
    { \
        size_t div = bit / bitsof(TYPE); \
        TYPE msk = (TYPE) 1 << bit % bitsof(TYPE); \
        if (arr[div] & msk) return 1; \
        arr[div] |= msk; \
        return 0; \
    }

#define DECL_ATOMIC_BIT_TEST_SET(TYPE, PREFIX) \
    bool PREFIX ## _atomic_bit_test_set(volatile void *arr, size_t bit) \
    { \
        TYPE msk = (TYPE) 1 << bit % bitsof(TYPE), res = atomic_or((volatile TYPE *) arr + bit / bitsof(TYPE), msk); \
        return res & msk; \
    }

#define DECL_BIT_TEST_RESET(TYPE, PREFIX) \
    bool PREFIX ## _bit_test_reset(TYPE *arr, size_t bit) \
    { \
        size_t div = bit / bitsof(TYPE); \
        TYPE msk = (TYPE) 1 << bit % bitsof(TYPE); \
        if (!(arr[div] & msk)) return 0; \
        arr[div] &= ~msk; \
        return 1; \
    }

#define DECL_ATOMIC_BIT_TEST_RESET(TYPE, PREFIX) \
    bool PREFIX ## _atomic_bit_test_reset(volatile void *arr, size_t bit) \
    { \
        TYPE msk = (TYPE) 1 << bit % bitsof(TYPE), res = atomic_and((volatile TYPE *) arr + bit / bitsof(TYPE), ~msk); \
        return res & msk; \
    }

#define DECL_BIT_SET(TYPE, PREFIX) \
    void PREFIX ## _bit_set(TYPE *arr, size_t bit) \
    { \
        arr[bit / bitsof(TYPE)] |= (TYPE) 1 << bit % bitsof(TYPE); \
    }

#define DECL_ATOMIC_BIT_SET_RELEASE(TYPE, PREFIX) \
    void PREFIX ## _atomic_bit_set_release(volatile void *arr, size_t bit) \
    { \
        atomic_or_release((volatile TYPE *) arr + bit / bitsof(TYPE), (TYPE) 1 << bit % bitsof(TYPE)); \
    }

#define DECL_BIT_RESET(TYPE, PREFIX) \
    void PREFIX ## _bit_reset(TYPE *arr, size_t bit) \
    { \
        arr[bit / bitsof(TYPE)] &= ~((TYPE) 1 << bit % bitsof(TYPE)); \
    }

#define DECL_ATOMIC_BIT_RESET_RELEASE(TYPE, PREFIX) \
    void PREFIX ## _atomic_bit_reset_release(volatile void *arr, size_t bit) \
    { \
        atomic_and_release((volatile TYPE *) arr + bit / bitsof(TYPE), ~((TYPE) 1 << bit % bitsof(TYPE))); \
    }

#define DECL_BIT_FETCH_BURST(TYPE, PREFIX, STRIDE) \
    TYPE PREFIX ## _bit_fetch_burst ## STRIDE(TYPE *arr, size_t pos) \
    { \
        return (arr[pos / (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1))] >> (pos % (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1)) << ((STRIDE) - 1))) & (((TYPE) 1 << (STRIDE)) - 1); \
    }

#define DECL_BIT_FETCH_SET_BURST(TYPE, PREFIX, STRIDE) \
    TYPE PREFIX ## _bit_fetch_set_burst ## STRIDE(TYPE *arr, size_t pos, TYPE msk) \
    { \
        size_t div = pos / (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1)), rem = pos % (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1)) << ((STRIDE) - 1); \
        TYPE res = (arr[div] >> rem) & msk; \
        if (res != msk) arr[div] |= msk << rem; \
        return res; \
    }

#define DECL_BIT_FETCH_RESET_BURST(TYPE, PREFIX, STRIDE) \
    TYPE PREFIX ## _bit_fetch_reset_burst ## STRIDE(TYPE *arr, size_t pos, TYPE msk) \
    { \
        size_t div = pos / (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1)), rem = pos % (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1)) << ((STRIDE) - 1); \
        TYPE res = (arr[div] >> rem) & msk; \
        if (res) arr[div] &= ~(msk << rem); \
        return res; \
    }

#define DECL_BIT_SET_BURST(TYPE, PREFIX, STRIDE) \
    void PREFIX ## _bit_set_burst ## STRIDE(TYPE *arr, size_t pos, TYPE msk) \
    { \
        arr[pos / (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1))] |= msk << (pos % (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1)) << ((STRIDE) - 1)); \
    }

#define DECL_BIT_RESET_BURST(TYPE, PREFIX, STRIDE) \
    void PREFIX ## _bit_reset_burst ## STRIDE(TYPE *arr, size_t pos, TYPE msk) \
    { \
        arr[pos / (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1))] &= ~(msk << (pos % (CHAR_BIT * sizeof(TYPE) >> ((STRIDE) - 1)) << ((STRIDE) - 1))); \
    }

#define DECL_ROL(TYPE, PREFIX) \
    TYPE PREFIX ## _rol(TYPE x, TYPE y) \
    { \
        y %= CHAR_BIT * sizeof(TYPE); \
        return (x << y) | (x >> (CHAR_BIT * sizeof(TYPE) - y)); \
    }

#define DECL_ROR(TYPE, PREFIX) \
    TYPE PREFIX ## _ror(TYPE x, TYPE y) \
    { \
        y %= CHAR_BIT * sizeof(TYPE); \
        return (x >> y) | (x << (CHAR_BIT * sizeof(TYPE) - y)); \
    }

DECL_BIT_TEST(uint8_t, uint8)
DECL_BIT_TEST_SET(uint8_t, uint8)
DECL_BIT_TEST_RESET(uint8_t, uint8)
DECL_BIT_SET(uint8_t, uint8)
DECL_BIT_RESET(uint8_t, uint8)

DECL_BIT_FETCH_BURST(uint8_t, uint8, 2)
DECL_BIT_FETCH_SET_BURST(uint8_t, uint8, 2)
DECL_BIT_FETCH_RESET_BURST(uint8_t, uint8, 2)
DECL_BIT_SET_BURST(uint8_t, uint8, 2)
DECL_BIT_RESET_BURST(uint8_t, uint8, 2)

DECL_BIT_TEST_ACQUIRE(uint8_t, uint8)
DECL_ATOMIC_BIT_SET_RELEASE(uint8_t, uint8)
DECL_ATOMIC_BIT_RESET_RELEASE(uint8_t, uint8)
DECL_ATOMIC_BIT_TEST_SET(uint8_t, uint8)
DECL_ATOMIC_BIT_TEST_RESET(uint8_t, uint8)

DECL_BIT_TEST(size_t, size)
DECL_BIT_TEST_SET(size_t, size)
DECL_BIT_TEST_RESET(size_t, size)
DECL_BIT_SET(size_t, size)
DECL_BIT_RESET(size_t, size)

DECL_ROL(uint8_t, uint8)
DECL_ROL(uint16_t, uint16)
DECL_ROL(uint32_t, uint32)
DECL_ROL(uint64_t, uint64)
