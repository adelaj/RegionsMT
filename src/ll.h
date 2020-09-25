#pragma once

///////////////////////////////////////////////////////////////////////////////
//
//  Functions providing low level facilities
//

#include "common.h"

#include <immintrin.h>

typedef unsigned spinlock_base;
typedef volatile spinlock_base spinlock;

#if defined __GNUC__ || defined __clang__ || defined _M_IX86 || defined __i386__
#   if defined _M_X64 || defined __x86_64__
GPUSH GWRN(pedantic) 
typedef unsigned __int128 Uint128_t, Dsize_t;
GPOP
#   define UINT128C(LO, HI) ((Uint128_t) (LO) | ((Uint128_t) (HI) << (bitsof(Uint128_t) >> 1)))
#   define UINT128_LO(X) ((uint64_t) (X))
#   define UINT128_HI(X) ((uint64_t) ((X) >> (bitsof(Uint128_t) >> 1)))
#   elif defined _M_IX86 || defined __i386__
typedef uint64_t Dsize_t;
#   endif
#elif defined _MSC_BUILD
typedef struct { uint64_t qw[2]; } Uint128_t, Dsize_t;
#   define UINT128C(LO, HI) ((Uint128_t) { .qw[0] = (LO), .qw[1] = (HI) })
#   define UINT128_LO(X) ((X).qw[0])
#   define UINT128_HI(X) ((X).qw[1])
#endif

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

#define SIZE_PROD_TEST(PROD, ARG, CNT) (size_prod_test((PROD), (ARG), (CNT)) == (CNT))
#define SIZE_PROD_TEST_VA(PROD, ...) (size_prod_test((PROD), ARG(size_t, __VA_ARGS__)) == countof(((size_t []) { __VA_ARGS__ })))

unsigned char uchar_add(unsigned char *, unsigned char, unsigned char);
unsigned short ushrt_add(unsigned short *, unsigned short, unsigned short);
unsigned uint_add(unsigned *, unsigned, unsigned);
unsigned long ulong_add(unsigned long *, unsigned long, unsigned long);
unsigned long long ullong_add(unsigned long long *, unsigned long long, unsigned long long);

unsigned char uchar_sub(unsigned char *, unsigned char, unsigned char);
unsigned short ushrt_sub(unsigned short *, unsigned short, unsigned short);
unsigned uint_sub(unsigned *, unsigned, unsigned);
unsigned long ulong_sub(unsigned long *, unsigned long, unsigned long);
unsigned long long ullong_sub(unsigned long long *, unsigned long long, unsigned long long);

unsigned char uchar_mul(unsigned char *, unsigned char, unsigned char);
unsigned short ushrt_mul(unsigned short *, unsigned short, unsigned short);
unsigned uint_mul(unsigned *, unsigned, unsigned);
unsigned long ulong_mul(unsigned long *, unsigned long, unsigned long);

unsigned char uchar_shl(unsigned char *, unsigned char, unsigned char);
unsigned short ushrt_shl(unsigned short *, unsigned short, unsigned char);
unsigned uint_shl(unsigned *, unsigned, unsigned char);
unsigned long ulong_shl(unsigned long *, unsigned long, unsigned char);

unsigned char uchar_shr(unsigned char *, unsigned char, unsigned char);
unsigned short ushrt_shr(unsigned short *, unsigned short, unsigned char);
unsigned uint_shr(unsigned *, unsigned, unsigned char);
unsigned long ulong_shr(unsigned long *, unsigned long, unsigned char);

unsigned char uchar_bit_scan_reverse(unsigned char);
unsigned short ushrt_bit_scan_reverse(unsigned short);
unsigned uint_bit_scan_reverse(unsigned);
unsigned long ulong_bit_scan_reverse(unsigned long);

unsigned char uchar_bit_scan_forward(unsigned char);
unsigned short ushrt_bit_scan_forward(unsigned short);
unsigned uint_bit_scan_forward(unsigned);
unsigned long ulong_bit_scan_forward(unsigned long);

unsigned char uchar_pop_cnt(unsigned char);
unsigned short ushrt_pop_cnt(unsigned short);
unsigned uint_pop_cnt(unsigned);
unsigned long ulong_pop_cnt(unsigned long);

bool uchar_atomic_compare_exchange(volatile unsigned char *, unsigned char *, unsigned char);
bool ushrt_atomic_compare_exchange(volatile unsigned short*, unsigned short *, unsigned short);
bool uint_atomic_compare_exchange(volatile unsigned *, unsigned *, unsigned);
bool ulong_atomic_compare_exchange(volatile unsigned long *, unsigned long *, unsigned long);
bool ullong_atomic_compare_exchange(volatile unsigned long long *, unsigned long long *, unsigned long long);
bool ptr_atomic_compare_exchange(void *volatile *, void **, void *);

#if defined __x86_64__ || defined _M_X64
#   define DSIZEC(LO, HI) UINT128C((LO), (HI))
#   define DSIZE_LO(X) ((size_t) UINT128_LO((X)))
#   define DSIZE_HI(X) ((size_t) UINT128_HI((X)))
unsigned long long ullong_mul(unsigned long long *, unsigned long long, unsigned long long);
unsigned long long ullong_shl(unsigned long long *, unsigned long long, unsigned char);
unsigned long long ullong_shr(unsigned long long *, unsigned long long, unsigned char);
unsigned long long ullong_bit_scan_reverse(unsigned long long);
unsigned long long ullong_bit_scan_forward(unsigned long long);
unsigned long long ullong_pop_cnt(unsigned long long);
bool Uint128_atomic_compare_exchange(volatile Uint128_t *, Uint128_t *, Uint128_t);
#   define IF64(...) __VA_ARGS__
#else
#   define DSIZEC(LO, HI) ((Dsize_t) (LO) | (((Dsize_t) (HI)) << SIZE_BIT))
#   define DSIZE_LO(D) ((size_t) (D))
#   define DSIZE_HI(D) ((size_t) ((D) >> SIZE_BIT))
#   define IF64(...)
#endif

#define add(P_HI, X, Y) (_Generic(*(P_HI), \
    unsigned char: uchar_add, \
    unsigned short: ushrt_add, \
    unsigned: uint_add, \
    unsigned long: ulong_add, \
    unsigned long long: ullong_add)((P_HI), (X), (Y)))

#define sub(P_HI, X, Y) (_Generic(*(P_HI), \
    unsigned char: uchar_sub, \
    unsigned short: ushrt_sub, \
    unsigned: uint_sub, \
    unsigned long: ulong_sub, \
    unsigned long long: ullong_sub)((P_HI), (X), (Y)))

#define mul(P_HI, X, Y) (_Generic(*(P_HI), \
    unsigned char: uchar_mul, \
    unsigned short: ushrt_mul, \
    unsigned: uint_mul, \
    unsigned long: ulong_mul IF64(, \
    unsigned long long: ullong_mul))((P_HI), (X), (Y)))

#define shl(P_HI, X, Y) (_Generic(*(P_HI), \
    unsigned char: uchar_shl, \
    unsigned short: ushrt_shl, \
    unsigned: uint_shl, \
    unsigned long: ulong_shl IF64(, \
    unsigned long long: ullong_shl))((P_HI), (X), (Y)))

#define shr(P_LO, X, Y) (_Generic(*(P_HI), \
    unsigned char: uchar_shr, \
    unsigned short: ushrt_shr, \
    unsigned: uint_shr, \
    unsigned long: ulong_shr IF64(, \
    unsigned long long: ullong_shr))((P_LO), (X), (Y)))

#define bit_scan_reverse(x) (_Generic((x), \
    unsigned char: uchar_bit_scan_reverse, \
    unsigned short: ushrt_bit_scan_reverse, \
    unsigned: uint_bit_scan_reverse, \
    unsigned long: ulong_bit_scan_reverse IF64(, \
    unsigned long long: ullong_bit_scan_reverse))(x))

#define bit_scan_forward(x) (_Generic((x), \
    unsigned char: uchar_bit_scan_forward, \
    unsigned short: ushrt_bit_scan_forward, \
    unsigned: uint_bit_scan_forward, \
    unsigned long: ulong_bit_scan_forward IF64(, \
    unsigned long long: ullong_bit_scan_forward))(x))

#define pop_cnt(x) (_Generic((x), \
    unsigned char: uchar_pop_cnt, \
    unsigned short: ushrt_pop_cnt, \
    unsigned: uint_pop_cnt, \
    unsigned long: ulong_pop_cnt IF64(, \
    unsigned long long: ullong_pop_cnt))(x))

#define atomic_compare_exchange(DST, P_CMP, XCHG) (_Generic((DST), \
    volatile unsigned char *: uchar_atomic_compare_exchange, \
    volatile unsigned short *: ushrt_atomic_compare_exchange, \
    volatile unsigned *: uint_atomic_compare_exchange, \
    volatile unsigned long *: ulong_atomic_compare_exchange, \
    volatile unsigned long long *: ullong_atomic_compare_exchange,\
    void *volatile *: ptr_atomic_compare_exchange IF64(, \
    volatile Uint128_t *: Uint128_atomic_compare_exchange))((DST), (P_CMP), (XCHG)))

#if defined __GNUC__ || defined __clang__

#   define load_acquire(SRC) (__atomic_load_n((SRC), __ATOMIC_ACQUIRE))
#   define store_release(DST, VAL) (__atomic_store_n((DST), (VAL), __ATOMIC_RELEASE))
#   define atomic_compare_exchange_acquire(DST, P_CMP, XCHG) (__atomic_compare_exchange_n((DST), (P_CMP), (XCHG), 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
#   define atomic_and(DST, ARG) (__atomic_fetch_and((DST), (ARG), __ATOMIC_ACQ_REL))
#   define atomic_and_release(DST, ARG) (__atomic_fetch_and((DST), (ARG), __ATOMIC_RELEASE))
#   define atomic_or(DST, ARG) (__atomic_fetch_or((DST), (ARG), __ATOMIC_ACQ_REL))
#   define atomic_or_release(DST, ARG) (__atomic_fetch_or((DST), (ARG), __ATOMIC_RELEASE))
#   define atomic_add(DST, ARG) (__atomic_fetch_add((DST), (ARG), __ATOMIC_ACQ_REL))
#   define atomic_sub(DST, ARG) (__atomic_fetch_sub((DST), (ARG), __ATOMIC_ACQ_REL))
#   define atomic_exchange(DST, ARG) (__atomic_exchange_n((DST), (ARG), __ATOMIC_ACQ_REL))

#elif defined _MSC_BUILD
#   include <intrin.h>

unsigned char uchar_atomic_and(volatile unsigned char *, unsigned char);
unsigned short ushrt_atomic_and(volatile unsigned short *, unsigned short);
unsigned uint_atomic_and(volatile unsigned *, unsigned);
unsigned long ulong_atomic_and(volatile unsigned long *, unsigned long);

unsigned char uchar_atomic_or(volatile unsigned char *, unsigned char);
unsigned short ushrt_atomic_or(volatile unsigned short *, unsigned short);
unsigned uint_atomic_or(volatile unsigned *, unsigned);
unsigned long ulong_atomic_or(volatile unsigned long *, unsigned long);

unsigned char uchar_atomic_add(volatile unsigned char *, unsigned char);
unsigned short ushrt_atomic_add(volatile unsigned short *, unsigned short);
unsigned uint_atomic_add(volatile unsigned *, unsigned);
unsigned long ulong_atomic_add(volatile unsigned long *, unsigned long);

unsigned char uchar_atomic_sub(volatile unsigned char *, unsigned char);
unsigned short ushrt_atomic_sub(volatile unsigned short *, unsigned short);
unsigned uint_atomic_sub(volatile unsigned *, unsigned);
unsigned long ulong_atomic_sub(volatile unsigned long *, unsigned long);

unsigned char uchar_atomic_exchange(volatile unsigned char *, unsigned char);
unsigned short ushrt_atomic_exchange(volatile unsigned short *, unsigned short);
unsigned uint_atomic_exchange(volatile unsigned *, unsigned);
unsigned long ulong_atomic_exchange(volatile unsigned long *, unsigned long);
void *ptr_atomic_exchange(void *volatile *, void *);

#   ifdef _M_X64
unsigned long long ullong_atomic_and(volatile unsigned long long *, unsigned long long);
unsigned long long ullong_atomic_or(volatile unsigned long long *, unsigned long long);
unsigned long long ullong_atomic_add(volatile unsigned long long *, unsigned long long);
unsigned long long ullong_atomic_sub(volatile unsigned long long *, unsigned long long);
unsigned long long ullong_atomic_exchange(volatile unsigned long long *, unsigned long long);
#   endif

#   define VOLATILE_FIXED(PTR, ...) (_Generic((PTR), \
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

// Acquire/release semantics imposed by volatility
// Should be used only with '/volatile:ms' compiler option
#   define load_acquire(SRC) VOLATILE_FIXED((SRC), *(SRC))
#   define store_release(DST, VAL) VOLATILE_FIXED((DST), *(DST) = (VAL))
#   define atomic_compare_exchange_acquire(DST, P_CMP, XCHG) (atomic_compare_exchange((DST), (P_CMP), (XCHG)))
#   define atomic_and(DST, ARG) (_Generic((DST), \
        volatile unsigned char *: uchar_atomic_and, \
        volatile unsigned short *: ushrt_atomic_and, \
        volatile unsigned *: uint_atomic_and, \
        volatile unsigned long *: ulong_atomic_and IF64(, \
        volatile unsigned long long *: ullong_atomic_and))((DST), (ARG)))
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

size_t size_shl(size_t *, size_t, uint8_t);
size_t size_shr(size_t *, size_t, uint8_t);
size_t size_add(size_t *, size_t, size_t);
size_t size_sub(size_t *, size_t, size_t);

uint32_t uint32_bit_scan_reverse(uint32_t);
uint32_t uint32_bit_scan_forward(uint32_t);
uint32_t uint32_pop_cnt(uint32_t);
size_t size_bit_scan_reverse(size_t);
size_t size_bit_scan_forward(size_t);
size_t size_pop_cnt(size_t);

size_t size_sum(size_t *, size_t *, size_t);
size_t size_prod_test(size_t *, size_t *, size_t);
size_t size_mul(size_t *, size_t, size_t);

uint32_t uint32_log10(uint32_t, bool);
uint64_t uint64_log10(uint64_t, bool);
size_t size_log10(size_t, bool);
size_t size_log2(size_t, bool);

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

uint8_t uint8_bit_scan_reverse(uint8_t);
uint8_t uint8_bit_scan_forward(uint8_t);
size_t size_add_sat(size_t, size_t);
size_t size_sub_sat(size_t, size_t);
int size_sign(size_t, size_t);

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

uint32_t uint32_fused_mul_add(uint32_t *, uint32_t, uint32_t);
size_t size_fused_mul_add(size_t *, size_t, size_t);
bool size_mul_add_test(size_t *, size_t m, size_t a);

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

uint8_t uint8_rol(uint8_t, uint8_t);
uint16_t uint16_rol(uint16_t, uint16_t);
uint32_t uint32_rol(uint32_t, uint32_t);
uint64_t uint64_rol(uint64_t, uint64_t);
