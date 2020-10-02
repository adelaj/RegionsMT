#pragma once

///////////////////////////////////////////////////////////////////////////////
//
//  Functions providing low level facilities
//

#include "common.h"

#include <immintrin.h>

enum atomic_mo {
    ATOMIC_RELAXED IF_GCC_LLVM(= __ATOMIC_RELAXED),
    ATOMIC_ACQUIRE IF_GCC_LLVM(= __ATOMIC_ACQUIRE),
    ATOMIC_RELEASE IF_GCC_LLVM(= __ATOMIC_RELEASE),
    ATOMIC_ACQ_REL IF_GCC_LLVM(= __ATOMIC_ACQ_REL),
};

typedef unsigned spinlock_base;
typedef volatile spinlock_base spinlock;

GPUSH GWRN(pedantic)
IF_GCC_LLVM(IF_X64(typedef unsigned __int128 Uint128_t;))
GPOP
IF_MSVC_X32(typedef struct { uint64_t qw[2]; } Uint128_t;)
IF_X64(typedef Uint128_t Dsize_t;)
IF_X32(typedef uint64_t Dsize_t;)

_Static_assert(sizeof(Uint128_t) == 2 * sizeof(uint64_t), "");
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

// Atomic load
bool bool_atomic_load_mo(volatile bool *, enum atomic_mo);
unsigned char uchar_atomic_load_mo(volatile unsigned char *, enum atomic_mo);
unsigned short ushrt_atomic_load_mo(volatile unsigned short *, enum atomic_mo);
unsigned uint_atomic_load_mo(volatile unsigned *, enum atomic_mo);
unsigned long ulong_atomic_load_mo(volatile unsigned long *, enum atomic_mo);
unsigned long long ullong_atomic_load_mo(volatile unsigned long long *, enum atomic_mo);

#define atomic_load_mo(SRC, MO) (_Generic((SRC), \
    volatile bool *: bool_atomic_load_mo, \
    volatile unsigned char *: uchar_atomic_load_mo, \
    volatile unsigned short *: ushrt_atomic_load_mo, \
    volatile unsigned *: uint_atomic_load_mo, \
    volatile unsigned long *: ulong_atomic_load_mo, \
    volatile unsigned long long *: ullong_atomic_load_mo)((SRC), (MO)))

#define atomic_load(SRC) \
    (atomic_load_mo((SRC), ATOMIC_ACQUIRE))

// Atomic store
void bool_atomic_store_mo(volatile bool *, bool, enum atomic_mo);
void uchar_atomic_store_mo(volatile unsigned char *, unsigned char, enum atomic_mo);
void ushrt_atomic_store_mo(volatile unsigned short *, unsigned short, enum atomic_mo);
void uint_atomic_store_mo(volatile unsigned *, unsigned, enum atomic_mo);
void ulong_atomic_store_mo(volatile unsigned long *, unsigned long, enum atomic_mo);
void ullong_atomic_store_mo(volatile unsigned long long *, unsigned long long, enum atomic_mo);

#define atomic_store_mo(DST, VAL, MO) (_Generic((DST), \
    volatile bool *: bool_atomic_store_mo, \
    volatile unsigned char *: uchar_atomic_store_mo, \
    volatile unsigned short *: ushrt_atomic_store_mo, \
    volatile unsigned *: uint_atomic_store_mo, \
    volatile unsigned long *: ulong_atomic_store_mo, \
    volatile unsigned long long *: ullong_atomic_store_mo)((DST), (VAL), (MO)))

#define atomic_store(DST, VAL) \
    (atomic_store_mo((DST), (VAL), ATOMIC_RELEASE))

// Atomic compare and swap
bool uchar_atomic_cmp_xchg_mo(volatile unsigned char *, unsigned char *, unsigned char, bool, enum atomic_mo, enum atomic_mo);
bool ushrt_atomic_cmp_xchg_mo(volatile unsigned short*, unsigned short *, unsigned short, bool, enum atomic_mo, enum atomic_mo);
bool uint_atomic_cmp_xchg_mo(volatile unsigned *, unsigned *, unsigned, bool, enum atomic_mo, enum atomic_mo);
bool ulong_atomic_cmp_xchg_mo(volatile unsigned long *, unsigned long *, unsigned long, bool, enum atomic_mo, enum atomic_mo);
bool ullong_atomic_cmp_xchg_mo(volatile unsigned long long *, unsigned long long *, unsigned long long, bool, enum atomic_mo, enum atomic_mo);
bool ptr_atomic_cmp_xchg_mo(void *volatile *, void **, void *, bool, enum atomic_mo, enum atomic_mo);
IF_X64(bool Uint128_atomic_cmp_xchg_mo(volatile Uint128_t *, Uint128_t *, Uint128_t, bool, enum atomic_mo, enum atomic_mo);)

#define atomic_cmp_xchg_mo(DST, P_CMP, XCHG, WEAK, MO_SUCC, MO_FAIL) (_Generic((DST), \
    volatile unsigned char *: uchar_atomic_cmp_xchg_mo, \
    volatile unsigned short *: ushrt_atomic_cmp_xchg_mo, \
    volatile unsigned *: uint_atomic_cmp_xchg_mo, \
    volatile unsigned long *: ulong_atomic_cmp_xchg_mo, \
    volatile unsigned long long *: ullong_atomic_cmp_xchg_mo,\
    void *volatile *: ptr_atomic_cmp_xchg_mo \
    IF_X64(, volatile Uint128_t *: Uint128_atomic_cmp_xchg_mo))((DST), (P_CMP), (XCHG), (WEAK), (MO_SUCC), (MO_FAIL)))

#define atomic_cmp_xchg(DST, P_CMP, XCHG) \
    (atomic_cmp_xchg_mo((DST), (P_CMP), (XCHG), 0, ATOMIC_ACQ_REL, ATOMIC_ACQUIRE))

// Atomic fetch-AND
unsigned char uchar_atomic_fetch_and_mo(volatile unsigned char *, unsigned char, enum atomic_mo);
unsigned short ushrt_atomic_fetch_and_mo(volatile unsigned short *, unsigned short, enum atomic_mo);
unsigned uint_atomic_fetch_and_mo(volatile unsigned *, unsigned, enum atomic_mo);
unsigned long ulong_atomic_fetch_and_mo(volatile unsigned long *, unsigned long, enum atomic_mo);
unsigned long long ullong_atomic_fetch_and_mo(volatile unsigned long long *, unsigned long long, enum atomic_mo);

#define atomic_fetch_and_mo(DST, ARG, MO) (_Generic((DST), \
    volatile unsigned char *: uchar_atomic_fetch_and_mo, \
    volatile unsigned short *: ushrt_atomic_fetch_and_mo, \
    volatile unsigned *: uint_atomic_fetch_and_mo, \
    volatile unsigned long *: ulong_atomic_fetch_and_mo, \
    volatile unsigned long long *: ullong_atomic_fetch_and_mo)((DST), (ARG), (MO)))

#define atomic_fetch_and(DST, ARG) \
    (atomic_fetch_and_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Atomic fetch-OR
unsigned char uchar_atomic_fetch_or_mo(volatile unsigned char *, unsigned char, enum atomic_mo);
unsigned short ushrt_atomic_fetch_or_mo(volatile unsigned short *, unsigned short, enum atomic_mo);
unsigned uint_atomic_fetch_or_mo(volatile unsigned *, unsigned, enum atomic_mo);
unsigned long ulong_atomic_fetch_or_mo(volatile unsigned long *, unsigned long, enum atomic_mo);
unsigned long long ullong_atomic_fetch_or_mo(volatile unsigned long long *, unsigned long long, enum atomic_mo);

#define atomic_fetch_or_mo(DST, ARG, MO) (_Generic((DST), \
    volatile unsigned char *: uchar_atomic_fetch_or_mo, \
    volatile unsigned short *: ushrt_atomic_fetch_or_mo, \
    volatile unsigned *: uint_atomic_fetch_or_mo, \
    volatile unsigned long *: ulong_atomic_fetch_or_mo, \
    volatile unsigned long long *: ullong_atomic_fetch_or_mo)((DST), (ARG), (MO)))

#define atomic_fetch_or(DST, ARG) \
    (atomic_fetch_or_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Atomic fetch-add
unsigned char uchar_atomic_fetch_add_mo(volatile unsigned char *, unsigned char, enum atomic_mo);
unsigned short ushrt_atomic_fetch_add_mo(volatile unsigned short *, unsigned short, enum atomic_mo);
unsigned uint_atomic_fetch_add_mo(volatile unsigned *, unsigned, enum atomic_mo);
unsigned long ulong_atomic_fetch_add_mo(volatile unsigned long *, unsigned long, enum atomic_mo);
unsigned long long ullong_atomic_fetch_add_mo(volatile unsigned long long *, unsigned long long, enum atomic_mo);

#define atomic_fetch_add_mo(DST, ARG, MO) (_Generic((DST), \
    volatile unsigned char *: uchar_atomic_fetch_add_mo, \
    volatile unsigned short *: ushrt_atomic_fetch_add_mo, \
    volatile unsigned *: uint_atomic_fetch_add_mo, \
    volatile unsigned long *: ulong_atomic_fetch_add_mo, \
    volatile unsigned long long *: ullong_atomic_fetch_add_mo)((DST), (ARG), (MO)))

#define atomic_fetch_add(DST, ARG) \
    (atomic_fetch_add_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Atomic fetch-subtract
unsigned char uchar_atomic_fetch_sub_mo(volatile unsigned char *, unsigned char, enum atomic_mo);
unsigned short ushrt_atomic_fetch_sub_mo(volatile unsigned short *, unsigned short, enum atomic_mo);
unsigned uint_atomic_fetch_sub_mo(volatile unsigned *, unsigned, enum atomic_mo);
unsigned long ulong_atomic_fetch_sub_mo(volatile unsigned long *, unsigned long, enum atomic_mo);
unsigned long long ullong_atomic_fetch_sub_mo(volatile unsigned long long *, unsigned long long, enum atomic_mo);

#define atomic_fetch_sub_mo(DST, ARG, MO) (_Generic((DST), \
    volatile unsigned char *: uchar_atomic_fetch_sub_mo, \
    volatile unsigned short *: ushrt_atomic_fetch_sub_mo, \
    volatile unsigned *: uint_atomic_fetch_sub_mo, \
    volatile unsigned long *: ulong_atomic_fetch_sub_mo, \
    volatile unsigned long long *: ullong_atomic_fetch_sub_mo)((DST), (ARG), (MO)))

#define atomic_fetch_sub(DST, ARG) \
    (atomic_fetch_sub_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Atomic exchange
unsigned char uchar_atomic_xchg_mo(volatile unsigned char *, unsigned char, enum atomic_mo);
unsigned short ushrt_atomic_xchg_mo(volatile unsigned short *, unsigned short, enum atomic_mo);
unsigned uint_atomic_xchg_mo(volatile unsigned *, unsigned, enum atomic_mo);
unsigned long ulong_atomic_xchg_mo(volatile unsigned long *, unsigned long, enum atomic_mo);
unsigned long long ullong_atomic_xchg_mo(volatile unsigned long long *, unsigned long long, enum atomic_mo);
void *ptr_atomic_xchg_mo(void *volatile *, void *, enum atomic_mo);

#define atomic_xchg_mo(DST, ARG, MO) (_Generic((DST), \
    volatile unsigned char *: uchar_atomic_xchg_mo, \
    volatile unsigned short *: ushrt_atomic_xchg_mo, \
    volatile unsigned *: uint_atomic_xchg_mo, \
    volatile unsigned long *: ulong_atomic_xchg_mo, \
    volatile unsigned long long *: ullong_atomic_xchg_mo, \
    void *volatile *: ptr_atomic_xchg_mo)((DST), (ARG), (MO)))

#define atomic_xchg(DST, ARG) \
    (atomic_xchg_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Atomic fetch-add saturated
unsigned char uchar_atomic_fetch_add_sat_mo(volatile unsigned char *, unsigned char, enum atomic_mo);
unsigned short ushrt_atomic_fetch_add_sat_mo(volatile unsigned short *, unsigned short, enum atomic_mo);
unsigned uint_atomic_fetch_add_sat_mo(volatile unsigned *, unsigned, enum atomic_mo);
unsigned long ulong_atomic_fetch_add_sat_mo(volatile unsigned long *, unsigned long, enum atomic_mo);
unsigned long long ullong_atomic_fetch_add_sat_mo(volatile unsigned long long *, unsigned long long, enum atomic_mo);

#define atomic_fetch_add_sat_mo(DST, ARG, MO) (_Generic((DST), \
    volatile unsigned char *: uchar_atomic_fetch_add_sat_mo, \
    volatile unsigned short *: ushrt_atomic_fetch_add_sat_mo, \
    volatile unsigned *: uint_atomic_fetch_add_sat_mo, \
    volatile unsigned long *: ulong_atomic_fetch_add_sat_mo, \
    volatile unsigned long long *: ullong_atomic_fetch_add_sat_mo)((DST), (ARG), (MO)))

#define atomic_fetch_add_sat(DST, ARG) \
    (atomic_fetch_add_sat_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Atomic fetch-add subtract
unsigned char uchar_atomic_fetch_sub_sat_mo(volatile unsigned char *, unsigned char, enum atomic_mo);
unsigned short ushrt_atomic_fetch_sub_sat_mo(volatile unsigned short *, unsigned short, enum atomic_mo);
unsigned uint_atomic_fetch_sub_sat_mo(volatile unsigned *, unsigned, enum atomic_mo);
unsigned long ulong_atomic_fetch_sub_sat_mo(volatile unsigned long *, unsigned long, enum atomic_mo);
unsigned long long ullong_atomic_fetch_sub_sat_mo(volatile unsigned long long *, unsigned long long, enum atomic_mo);

#define atomic_fetch_sub_sat_mo(DST, ARG, MO) (_Generic((DST), \
    volatile unsigned char *: uchar_atomic_fetch_sub_sat_mo, \
    volatile unsigned short *: ushrt_atomic_fetch_sub_sat_mo, \
    volatile unsigned *: uint_atomic_fetch_sub_sat_mo, \
    volatile unsigned long *: ulong_atomic_fetch_sub_sat_mo, \
    volatile unsigned long long *: ullong_atomic_fetch_sub_sat_mo)((DST), (ARG), (MO)))

#define atomic_fetch_sub_sat(DST, ARG) \
    (atomic_fetch_sub_sat_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Hash functions
unsigned uint_hash(unsigned);
unsigned long ulong_hash(unsigned long);
unsigned long long ullong_hash(unsigned long long);

#define hash(X) (_Generic((X), \
    unsigned: uint_hash, \
    unsigned long: ulong_hash, \
    unsigned long long: ullong_hash)(X))

unsigned uint_hash_inv(unsigned);
unsigned long ulong_hash_inv(unsigned long);
unsigned long long ullong_hash_inv(unsigned long long);

#define hash_inv(X) (_Generic((X), \
    unsigned: uint_hash_inv, \
    unsigned long: ulong_hash_inv, \
    unsigned long long: ullong_hash_inv)(X))

// Various macros
#define atomic_inc(DST) (atomic_add((DST), 1))
#define atomic_dec(DST) (atomic_sub((DST), 1))
#define ulog2(X, CEIL) (bsr(X) + ((CEIL) && (X) && ((X) & ((X) - 1))))

uint32_t uint32_log10(uint32_t, bool);
uint64_t uint64_log10(uint64_t, bool);


size_t size_hash(size_t);
size_t size_hash_inv(size_t);

size_t m128i_byte_scan_forward(__m128i a);

#define SPINLOCK_INIT 0
void spinlock_acquire(spinlock *);
#define spinlock_release(SPINLOCK) atomic_store((SPINLOCK), 0); 

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
