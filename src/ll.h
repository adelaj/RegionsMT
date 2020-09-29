#pragma once

///////////////////////////////////////////////////////////////////////////////
//
//  Functions providing low level facilities
//

#include "common.h"

#include <immintrin.h>

typedef unsigned spinlock_base;
typedef volatile spinlock_base spinlock;

IF_GCC_LLVM(IF_X64(GPUSH GWRN(pedantic) typedef unsigned __int128 Uint128_t; GPOP))
IF_MSVC_X32(typedef struct { uint64_t qw[2]; } Uint128_t;)
IF_X64(typedef Uint128_t Dsize_t;)
IF_X32(typedef uint64_t Dsize_t;)

_Static_assert(sizeof(Dsize_t) == 2 * sizeof(size_t), "");

#define UINT128C(LO, HI) \
    IF_GCC_LLVM((IF_X64((Uint128_t) (LO) | ((Uint128_t) (HI) << (bitsof(Uint128_t) >> 1))))) \
    IF_MSVC_X32(((Uint128_t) { .qw[0] = (LO), .qw[1] = (HI) }))
#define UINT128_LO(X) \
    IF_GCC_LLVM((IF_X64((uint64_t) (X)))) \
    IF_MSVC_X32(((X).qw[0]))
#define UINT128_HI(X) \
    IF_GCC_LLVM(((uint64_t) ((X) >> (bitsof(Uint128_t) >> 1)))) \
    IF_MSVC_X32(((X).qw[1]))
#define DSIZEC(LO, HI) \
    IF_X64((UINT128C((LO), (HI)))) \
    IF_X32(((Dsize_t) (LO) | (((Dsize_t) (HI)) << SIZE_BIT)))
#define DSIZE_LO(X) \
    IF_X64(((size_t) UINT128_LO((X)))) \
    IF_X32(((size_t) (D)))
#define DSIZE_HI(X) \
    IF_X64(((size_t) UINT128_HI((X)))) \
    IF_X32(((size_t) ((D) >> SIZE_BIT)))

// Rounding down/up to the nearest power of 2 for the inline usage 
#define RP2_SUPP(X, Y) ((X) | ((X) >> (Y)))
#define RP2_64(X) RP2_SUPP(RP2_SUPP(RP2_SUPP(RP2_SUPP(RP2_SUPP(RP2_SUPP(0ull | (X), 1), 2), 4), 8), 16), 32)
#define RDP2(X) ((X) && ((X) & ((X) - 1)) ? (RP2_64(X) >> 1) + 1 : (X))
#define RUP2(X) ((X) && ((X) & ((X) - 1)) ? RP2_64(X) + 1 : (X))

#define TYPE_CNT(BIT, TOT) ((BIT) / (TOT) + !!((BIT) % (TOT))) // Not always equals to ((BIT) + (TOT) - 1) / (TOT)
#define NIBBLE_CNT(NIBBLE) TYPE_CNT(NIBBLE, bitsof(uint8_t) >> 1)
#define UINT8_CNT(BIT) TYPE_CNT(BIT, bitsof(uint8_t))

#define SIZE_BIT (bitsof(size_t))
#define SIZE_CNT(BIT) TYPE_CNT(BIT, SIZE_BIT)
#define UINT32_BIT (bitsof(uint32_t))
#define UINT64_BIT (bitsof(uint64_t))

#define umax(X) (_Generic((X), \
    unsigned char: UCHAR_MAX, \
    unsigned short: USHRT_MAX, \
    unsigned: UINT_MAX, \
    unsigned long: ULONG_MAX, \
    unsigned long long: ULLONG_MAX))

// Warning! Acquire/release semantics on MSVC are imposed by volatility 
// '/volatile:ms' compiler option should be enabled!
#define VOLAT_SELECT(PTR, ...) (_Generic((PTR), \
    volatile bool *: (__VA_ARGS__), \
    volatile char *: (__VA_ARGS__), \
    volatile short *: (__VA_ARGS__), \
    volatile int *: (__VA_ARGS__), \
    volatile long *: (__VA_ARGS__), \
    volatile long long *: (__VA_ARGS__), \
    volatile unsigned char *: (__VA_ARGS__), \
    volatile unsigned short *: (__VA_ARGS__), \
    volatile unsigned *: (__VA_ARGS__), \
    volatile unsigned long *: (__VA_ARGS__), \
    volatile unsigned long long *: (__VA_ARGS__), \
    void *volatile *: (__VA_ARGS__)))

// Add
unsigned char uchar_add(unsigned char *, unsigned char, unsigned char);
unsigned short ushrt_add(unsigned char *, unsigned short, unsigned short);
unsigned uint_add(unsigned char *, unsigned, unsigned);
unsigned long ulong_add(unsigned char *, unsigned long, unsigned long);
unsigned long long ullong_add(unsigned char *, unsigned long long, unsigned long long);

#define add(P_CAR, X, Y) (_Generic((X), \
    unsigned char: uchar_add, \
    unsigned short: ushrt_add, \
    unsigned: uint_add, \
    unsigned long: ulong_add, \
    unsigned long long: ullong_add)((P_CAR), (X), (Y)))

// Subtract
unsigned char uchar_sub(unsigned char *, unsigned char, unsigned char);
unsigned short ushrt_sub(unsigned char *, unsigned short, unsigned short);
unsigned uint_sub(unsigned char *, unsigned, unsigned);
unsigned long ulong_sub(unsigned char *, unsigned long, unsigned long);
unsigned long long ullong_sub(unsigned char *, unsigned long long, unsigned long long);

#define sub(P_BOR, X, Y) (_Generic((X), \
    unsigned char: uchar_sub, \
    unsigned short: ushrt_sub, \
    unsigned: uint_sub, \
    unsigned long: ulong_sub, \
    unsigned long long: ullong_sub)((P_BOR), (X), (Y)))

// Multiply
unsigned char uchar_mul(unsigned char *, unsigned char, unsigned char);
unsigned short ushrt_mul(unsigned short *, unsigned short, unsigned short);
unsigned uint_mul(unsigned *, unsigned, unsigned);
unsigned long ulong_mul(unsigned long *, unsigned long, unsigned long);
unsigned long long ullong_mul(unsigned long long *, unsigned long long, unsigned long long);

#define mul(P_HI, X, Y) (_Generic(*(P_HI), \
    unsigned char: uchar_mul, \
    unsigned short: ushrt_mul, \
    unsigned: uint_mul, \
    unsigned long: ulong_mul, \
    unsigned long long: ullong_mul)((P_HI), (X), (Y)))

// Multiply–accumulate
unsigned char uchar_ma(unsigned char *, unsigned char, unsigned char);
unsigned short ushrt_ma(unsigned short *, unsigned short, unsigned short);
unsigned uint_ma(unsigned *, unsigned, unsigned);
unsigned long ulong_ma(unsigned long *, unsigned long, unsigned long);
unsigned long long ullong_ma(unsigned long long *, unsigned long long, unsigned long long);

#define ma(P_RES, X, Y) (_Generic(*(P_RES), \
    unsigned char: uchar_ma, \
    unsigned short: ushrt_ma, \
    unsigned: uint_ma, \
    unsigned long: ulong_ma, \
    unsigned long long: ullong_ma)((P_RES), (X), (Y)))

// Shift left
unsigned char uchar_shl(unsigned char *, unsigned char, unsigned char);
unsigned short ushrt_shl(unsigned short *, unsigned short, unsigned char);
unsigned uint_shl(unsigned *, unsigned, unsigned char);
unsigned long ulong_shl(unsigned long *, unsigned long, unsigned char);
unsigned long long ullong_shl(unsigned long long *, unsigned long long, unsigned char);

#define shl(P_HI, X, Y) (_Generic(*(P_HI), \
    unsigned char: uchar_shl, \
    unsigned short: ushrt_shl, \
    unsigned: uint_shl, \
    unsigned long: ulong_shl, \
    unsigned long long: ullong_shl)((P_HI), (X), (Y)))

// Shift right
unsigned char uchar_shr(unsigned char *, unsigned char, unsigned char);
unsigned short ushrt_shr(unsigned short *, unsigned short, unsigned char);
unsigned uint_shr(unsigned *, unsigned, unsigned char);
unsigned long ulong_shr(unsigned long *, unsigned long, unsigned char);
unsigned long long ullong_shr(unsigned long long *, unsigned long long, unsigned char);

#define shr(P_LO, X, Y) (_Generic(*(P_HI), \
    unsigned char: uchar_shr, \
    unsigned short: ushrt_shr, \
    unsigned: uint_shr, \
    unsigned long: ulong_shr IF64(, \
    unsigned long long: ullong_shr))((P_LO), (X), (Y)))

// Rotate left
unsigned char uchar_rol(unsigned char, unsigned char);
unsigned short ushrt_rol(unsigned short, unsigned char);
unsigned uint_rol(unsigned, unsigned char);
unsigned long ulong_rol(unsigned long, unsigned char);
unsigned long long ullong_rol(unsigned long long, unsigned char);

#define rol(X, Y) (_Generic((X), \
    unsigned char: uchar_rol, \
    unsigned short: ushrt_rol, \
    unsigned: uint_rol, \
    unsigned long: ulong_rol, \
    unsigned long long: ullong_rol)((X), (Y)))

// Rotate right
unsigned char uchar_ror(unsigned char, unsigned char);
unsigned short ushrt_ror(unsigned short, unsigned char);
unsigned uint_ror(unsigned, unsigned char);
unsigned long ulong_ror(unsigned long, unsigned char);
unsigned long long ullong_ror(unsigned long long, unsigned char);

#define ror(X, Y) (_Generic((X), \
    unsigned char: uchar_ror, \
    unsigned short: ushrt_ror, \
    unsigned: uint_ror, \
    unsigned long: ulong_ror, \
    unsigned long long: ullong_ror)((X), (Y)))

// Saturated add
unsigned char uchar_add_sat(unsigned char, unsigned char);
unsigned short ushrt_add_sat(unsigned short, unsigned short);
unsigned uint_add_sat(unsigned, unsigned);
unsigned long ulong_add_sat(unsigned long, unsigned long);
unsigned long long ullong_add_sat(unsigned long long, unsigned long long);

#define add_sat(X, Y) (_Generic((X), \
    unsigned char: uchar_add_sat, \
    unsigned short: ushrt_add_sat, \
    unsigned: uint_add_sat, \
    unsigned long: ulong_add_sat, \
    unsigned long long: ullong_add_sat)((X), (Y)))

// Saturated subtract
unsigned char uchar_sub_sat(unsigned char, unsigned char);
unsigned short ushrt_sub_sat(unsigned short, unsigned short);
unsigned uint_sub_sat(unsigned, unsigned);
unsigned long ulong_sub_sat(unsigned long, unsigned long);
unsigned long long ullong_sub_sat(unsigned long long, unsigned long long);

#define sub_sat(X, Y) (_Generic((X), \
    unsigned char: uchar_sub_sat, \
    unsigned short: ushrt_sub_sat, \
    unsigned: uint_sub_sat, \
    unsigned long: ulong_sub_sat, \
    unsigned long long: ullong_sub_sat)((X), (Y)))

// Sign of the difference of unsigned integers
int uchar_usign(unsigned char, unsigned char);
int ushrt_usign(unsigned short, unsigned short);
int uint_usign(unsigned, unsigned);
int ulong_usign(unsigned long, unsigned long);
int ullong_usign(unsigned long long, unsigned long long);

#define usign(X, Y) (_Generic((X), \
    unsigned char: uchar_usign, \
    unsigned short: ushrt_usign, \
    unsigned: uint_usign, \
    unsigned long: ulong_usign, \
    unsigned long long: ullong_usign)((X), (Y)))

// Absolute difference of unsigned integers
unsigned char uchar_udist(unsigned char, unsigned char);
unsigned short ushrt_udist(unsigned short, unsigned short);
unsigned uint_udist(unsigned, unsigned);
unsigned long ulong_udist(unsigned long, unsigned long);
unsigned long long ullong_udist(unsigned long long, unsigned long long);

#define udist(X, Y) (_Generic((X), \
    unsigned char: uchar_udist, \
    unsigned short: ushrt_udist, \
    unsigned: uint_udist, \
    unsigned long: ulong_udist, \
    unsigned long long: ullong_udist)((X), (Y)))

// Non-destructive add with overflow test
bool uchar_test_add(unsigned char *, unsigned char, unsigned char);
bool ushrt_test_add(unsigned short *, unsigned short, unsigned short);
bool uint_test_add(unsigned *, unsigned, unsigned);
bool ulong_test_add(unsigned long *, unsigned long, unsigned long);
bool ullong_test_add(unsigned long long *, unsigned long long, unsigned long long);

#define test_add(P_RES, X, Y) (_Generic(*(P_RES), \
    unsigned char: uchar_test_add, \
    unsigned short: ushrt_test_add, \
    unsigned: uint_test_add, \
    unsigned long: ulong_test_add, \
    unsigned long long: ullong_test_add)((P_RES), (X), (Y)))

// Non-destructive subtract with overflow test
bool uchar_test_sub(unsigned char *, unsigned char, unsigned char);
bool ushrt_test_sub(unsigned short *, unsigned short, unsigned short);
bool uint_test_sub(unsigned *, unsigned, unsigned);
bool ulong_test_sub(unsigned long *, unsigned long, unsigned long);
bool ullong_test_sub(unsigned long long *, unsigned long long, unsigned long long);

#define test_sub(P_RES, X, Y) (_Generic(*(P_RES), \
    unsigned char: uchar_test_sub, \
    unsigned short: ushrt_test_sub, \
    unsigned: uint_test_sub, \
    unsigned long: ulong_test_sub, \
    unsigned long long: ullong_test_sub)((P_RES), (X), (Y)))

// Non-destructive multiply with overflow test
bool uchar_test_mul(unsigned char *, unsigned char, unsigned char);
bool ushrt_test_mul(unsigned short *, unsigned short, unsigned short);
bool uint_test_mul(unsigned *, unsigned, unsigned);
bool ulong_test_mul(unsigned long *, unsigned long, unsigned long);
bool ullong_test_mul(unsigned long long *, unsigned long long, unsigned long long);

#define test_mul(P_RES, X, Y) (_Generic(*(P_RES), \
    unsigned char: uchar_test_mul, \
    unsigned short: ushrt_test_mul, \
    unsigned: uint_test_mul, \
    unsigned long: ulong_test_mul, \
    unsigned long long: ullong_test_mul)((P_RES), (X), (Y)))

// Non-destructive multiply–accumulate with overflow test
bool uchar_test_ma(unsigned char *, unsigned char, unsigned char);
bool ushrt_test_ma(unsigned short *, unsigned short, unsigned short);
bool uint_test_ma(unsigned *, unsigned, unsigned);
bool ulong_test_ma(unsigned long *, unsigned long, unsigned long);
bool ullong_test_ma(unsigned long long *, unsigned long long, unsigned long long);

#define test_ma(P_RES, X, Y) (_Generic(*(P_RES), \
    unsigned char: uchar_test_ma, \
    unsigned short: ushrt_test_ma, \
    unsigned: uint_test_ma, \
    unsigned long: ulong_test_ma, \
    unsigned long long: ullong_test_ma)((P_RES), (X), (Y)))

// Non-destructive sum with overflow test
size_t uchar_test_sum(unsigned char *, unsigned char *, size_t);
size_t ushrt_test_sum(unsigned short *, unsigned short *, size_t);
size_t uint_test_sum(unsigned *, unsigned *, size_t);
size_t ulong_test_sum(unsigned long *, unsigned long *, size_t);
size_t ullong_test_sum(unsigned long long *, unsigned long long *, size_t);

#define test_sum(P_RES, L, C) (_Generic(*(P_RES), \
    unsigned char: uchar_test_sum, \
    unsigned short: ushrt_test_sum, \
    unsigned: uint_test_sum, \
    unsigned long: ulong_test_sum, \
    unsigned long long: ullong_test_sum)((P_RES), (L), (C)))

// Non-destructive product with overflow test
size_t uchar_test_prod(unsigned char *, unsigned char *, size_t);
size_t ushrt_test_prod(unsigned short *, unsigned short *, size_t);
size_t uint_test_prod(unsigned *, unsigned *, size_t);
size_t ulong_test_prod(unsigned long *, unsigned long *, size_t);
size_t ullong_test_prod(unsigned long long *, unsigned long long *, size_t);

#define test_prod(P_RES, L, C) (_Generic(*(P_RES), \
    unsigned char: uchar_test_prod, \
    unsigned short: ushrt_test_prod, \
    unsigned: uint_test_prod, \
    unsigned long: ulong_test_prod, \
    unsigned long long: ullong_test_prod)((P_RES), (L), (C)))

// Bit scan reverse
unsigned char uchar_bsr(unsigned char);
unsigned short ushrt_bsr(unsigned short);
unsigned uint_bsr(unsigned);
unsigned long ulong_bsr(unsigned long);
unsigned long long ullong_bsr(unsigned long long);

#define bsr(x) (_Generic((x), \
    unsigned char: uchar_bsr, \
    unsigned short: ushrt_bsr, \
    unsigned: uint_bsr, \
    unsigned long: ulong_bsr, \
    unsigned long long: ullong_bsr)(x))

// Bit scan forward
unsigned char uchar_bsf(unsigned char);
unsigned short ushrt_bsf(unsigned short);
unsigned uint_bsf(unsigned);
unsigned long ulong_bsf(unsigned long);
unsigned long long ullong_bsf(unsigned long long);

#define bsf(x) (_Generic((x), \
    unsigned char: uchar_bsf, \
    unsigned short: ushrt_bsf, \
    unsigned: uint_bsf, \
    unsigned long: ulong_bsf, \
    unsigned long long: ullong_bsf)(x))

// Population count
unsigned char uchar_pcnt(unsigned char);
unsigned short ushrt_pcnt(unsigned short);
unsigned uint_pcnt(unsigned);
unsigned long ulong_pcnt(unsigned long);
unsigned long long ullong_pcnt(unsigned long long);

#define pcnt(x) (_Generic((x), \
    unsigned char: uchar_pcnt, \
    unsigned short: ushrt_pcnt, \
    unsigned: uint_pcnt, \
    unsigned long: ulong_pcnt, \
    unsigned long long: ullong_pcnt)(x))

// Load with acquire semantics
#define load_acquire(SRC) \
    IF_GCC_LLVM((__atomic_load_n((SRC), __ATOMIC_ACQUIRE))) \
    IF_MSVC(VOLAT_SELECT((SRC), *(SRC)))

// Store with release semantics
#define store_release(DST, VAL) \
    IF_GCC_LLVM((__atomic_store_n((DST), (VAL), __ATOMIC_RELEASE))) \
    IF_MSVC(VOLAT_SELECT((DST), *(DST) = (VAL)))

// Atomic compare and swap
bool uchar_atomic_compare_exchange(volatile unsigned char *, unsigned char *, unsigned char);
bool ushrt_atomic_compare_exchange(volatile unsigned short*, unsigned short *, unsigned short);
bool uint_atomic_compare_exchange(volatile unsigned *, unsigned *, unsigned);
bool ulong_atomic_compare_exchange(volatile unsigned long *, unsigned long *, unsigned long);
bool ullong_atomic_compare_exchange(volatile unsigned long long *, unsigned long long *, unsigned long long);
bool ptr_atomic_compare_exchange(void *volatile *, void **, void *);
IF_X64(bool Uint128_atomic_compare_exchange(volatile Uint128_t *, Uint128_t *, Uint128_t);)

#define atomic_compare_exchange(DST, P_CMP, XCHG) (_Generic((DST), \
    volatile unsigned char *: uchar_atomic_compare_exchange, \
    volatile unsigned short *: ushrt_atomic_compare_exchange, \
    volatile unsigned *: uint_atomic_compare_exchange, \
    volatile unsigned long *: ulong_atomic_compare_exchange, \
    volatile unsigned long long *: ullong_atomic_compare_exchange,\
    void *volatile *: ptr_atomic_compare_exchange \
    IF_X64(, volatile Uint128_t *: Uint128_atomic_compare_exchange))((DST), (P_CMP), (XCHG)))

// Atomic compare and swap (acquire semantics only)
#define atomic_compare_exchange_acquire(DST, P_CMP, XCHG) \
    IF_GCC_LLVM((__atomic_compare_exchange_n((DST), (P_CMP), (XCHG), 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))) \
    IF_MSVC((atomic_compare_exchange((DST), (P_CMP), (XCHG))))

// Atomic AND
IF_MSVC(
unsigned char uchar_atomic_and(volatile unsigned char *, unsigned char);
unsigned short ushrt_atomic_and(volatile unsigned short *, unsigned short);
unsigned uint_atomic_and(volatile unsigned *, unsigned);
unsigned long ulong_atomic_and(volatile unsigned long *, unsigned long);
IF_X64(unsigned long long ullong_atomic_and(volatile unsigned long long *, unsigned long long);))

#define atomic_and(DST, ARG) \
    IF_GCC_LLVM((__atomic_fetch_and((DST), (ARG), __ATOMIC_ACQ_REL))) \
    IF_MSVC((_Generic((DST), \
    volatile unsigned char *: uchar_atomic_and, \
    volatile unsigned short *: ushrt_atomic_and, \
    volatile unsigned *: uint_atomic_and, \
    volatile unsigned long *: ulong_atomic_and \
    IF_X64(, volatile unsigned long long *: ullong_atomic_and))((DST), (ARG))))

#if defined __GNUC__ || defined __clang__

#   define atomic_and(DST, ARG) 
#   define atomic_and_release(DST, ARG) (__atomic_fetch_and((DST), (ARG), __ATOMIC_RELEASE))
#   define atomic_or(DST, ARG) (__atomic_fetch_or((DST), (ARG), __ATOMIC_ACQ_REL))
#   define atomic_or_release(DST, ARG) (__atomic_fetch_or((DST), (ARG), __ATOMIC_RELEASE))
#   define atomic_add(DST, ARG) (__atomic_fetch_add((DST), (ARG), __ATOMIC_ACQ_REL))
#   define atomic_sub(DST, ARG) (__atomic_fetch_sub((DST), (ARG), __ATOMIC_ACQ_REL))
#   define atomic_exchange(DST, ARG) (__atomic_exchange_n((DST), (ARG), __ATOMIC_ACQ_REL))

#elif defined _MSC_BUILD
#   include <intrin.h>

unsigned char uchar_atomic_or(volatile unsigned char *, unsigned char);
unsigned short ushrt_atomic_or(volatile unsigned short *, unsigned short);
unsigned uint_atomic_or(volatile unsigned *, unsigned);
unsigned long ulong_atomic_or(volatile unsigned long *, unsigned long);
unsigned long long ullong_atomic_or(volatile unsigned long long *, unsigned long long);

unsigned char uchar_atomic_add(volatile unsigned char *, unsigned char);
unsigned short ushrt_atomic_add(volatile unsigned short *, unsigned short);
unsigned uint_atomic_add(volatile unsigned *, unsigned);
unsigned long ulong_atomic_add(volatile unsigned long *, unsigned long);
unsigned long long ullong_atomic_add(volatile unsigned long long *, unsigned long long);

unsigned char uchar_atomic_sub(volatile unsigned char *, unsigned char);
unsigned short ushrt_atomic_sub(volatile unsigned short *, unsigned short);
unsigned uint_atomic_sub(volatile unsigned *, unsigned);
unsigned long ulong_atomic_sub(volatile unsigned long *, unsigned long);
unsigned long long ullong_atomic_sub(volatile unsigned long long *, unsigned long long);

unsigned char uchar_atomic_exchange(volatile unsigned char *, unsigned char);
unsigned short ushrt_atomic_exchange(volatile unsigned short *, unsigned short);
unsigned uint_atomic_exchange(volatile unsigned *, unsigned);
unsigned long ulong_atomic_exchange(volatile unsigned long *, unsigned long);
void *ptr_atomic_exchange(void *volatile *, void *);
unsigned long long ullong_atomic_exchange(volatile unsigned long long *, unsigned long long);




#   define atomic_and_release(DST, ARG) (atomic_and(DST, ARG))

#   define atomic_or(DST, ARG) (_Generic((DST), \
        volatile unsigned char *: uchar_atomic_or, \
        volatile unsigned short *: ushrt_atomic_or, \
        volatile unsigned *: uint_atomic_or, \
        volatile unsigned long *: ulong_atomic_or IF64(, \
        volatile unsigned long long *: ullong_atomic_or))((DST), (ARG)))

#   define atomic_or_release(DST, ARG) (atomic_or(DST, ARG))

#   define atomic_add(DST, ARG) (_Generic((DST), \
        volatile unsigned char *: uchar_atomic_add, \
        volatile unsigned short *: ushrt_atomic_add, \
        volatile unsigned *: uint_atomic_add, \
        volatile unsigned long *: ulong_atomic_add IF64(, \
        volatile unsigned long long *: ullong_atomic_add))((DST), (ARG)))

#   define atomic_sub(DST, ARG) (_Generic((DST), \
        volatile unsigned char *: uchar_atomic_sub, \
        volatile unsigned short *: ushrt_atomic_sub, \
        volatile unsigned *: uint_atomic_sub, \
        volatile unsigned long *: ulong_atomic_sub IF64(, \
        volatile unsigned long long *: ullong_atomic_sub))((DST), (ARG)))

#   define atomic_exchange(DST, ARG) (_Generic((DST), \
        volatile unsigned char *: uchar_atomic_exchange, \
        volatile unsigned short *: ushrt_atomic_exchange, \
        volatile unsigned *: uint_atomic_exchange, \
        volatile unsigned long *: ulong_atomic_exchange IF64(, \
        volatile unsigned long long *: ullong_atomic_exchange), \
        void *volatile *: ptr_atomic_exchange)((DST), (ARG)))

#endif

#define atomic_inc(DST) (atomic_add((DST), 1))
#define atomic_dec(DST) (atomic_sub((DST), 1))
#define ulog2(X, CEIL) (bsr(X) + ((CEIL) && (X) && ((X) & ((X) - 1))))

uint32_t uint32_log10(uint32_t, bool);
uint64_t uint64_log10(uint64_t, bool);

uint32_t uint32_hash(uint32_t);
uint32_t uint32_hash_inv(uint32_t);
uint64_t uint64_hash(uint64_t);
uint64_t uint64_hash_inv(uint64_t);
size_t size_hash(size_t);
size_t size_hash_inv(size_t);

size_t m128i_byte_scan_forward(__m128i a);

#define SPINLOCK_INIT 0
void spinlock_acquire(spinlock *);
#define spinlock_release(SPINLOCK) store_release((SPINLOCK), 0); 

typedef void *(*double_lock_callback)(void *);
void *double_lock_execute(spinlock *, double_lock_callback, double_lock_callback, void *, void *);

int size_stable_cmp_dsc(const void *, const void *, void *);
int size_stable_cmp_asc(const void *, const void *, void *);
bool size_cmp_dsc(const void *, const void *, void *);
bool size_cmp_asc(const void *, const void *, void *);
int flt64_stable_cmp_dsc(const void *, const void *, void *);
int flt64_stable_cmp_asc(const void *, const void *, void *);
int flt64_stable_cmp_dsc_abs(const void *, const void *, void *);
int flt64_stable_cmp_asc_abs(const void *, const void *, void *);
int flt64_stable_cmp_dsc_nan(const void *, const void *, void *);
int flt64_stable_cmp_asc_nan(const void *, const void *, void *);
int flt64_sign(double x);

bool uint8_bit_test(uint8_t *, size_t);
bool uint8_bit_test_set(uint8_t *, size_t);
bool uint8_bit_test_reset(uint8_t *, size_t);
void uint8_bit_set(uint8_t *, size_t);
void uint8_bit_reset(uint8_t *, size_t);

uint8_t uint8_bit_fetch_burst2(uint8_t *, size_t);
uint8_t uint8_bit_fetch_set_burst2(uint8_t *, size_t, uint8_t);
uint8_t uint8_bit_fetch_reset_burst2(uint8_t *, size_t, uint8_t);
void uint8_bit_set_burst2(uint8_t *, size_t, uint8_t);
void uint8_bit_reset_burst2(uint8_t *, size_t, uint8_t);

bool uint8_bit_test_acquire(volatile void *, size_t);
void uint8_atomic_bit_set_release(volatile void *, size_t);
void uint8_atomic_bit_reset_release(volatile void *, size_t);
bool uint8_atomic_bit_test_set(volatile void *, size_t);
bool uint8_atomic_bit_test_reset(volatile void *, size_t);

bool size_bit_test(size_t *, size_t);
bool size_bit_test_set(size_t *, size_t);
bool size_bit_test_reset(size_t *, size_t);
void size_bit_set(size_t *, size_t);
void size_bit_reset(size_t *, size_t);
