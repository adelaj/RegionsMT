#pragma once

///////////////////////////////////////////////////////////////////////////////
//
//  Functions providing low level facilities
//

#include "common.h"

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <immintrin.h>

enum atomic_mo {
    ATOMIC_RELAXED IF_GCC_LLVM(= __ATOMIC_RELAXED),
    ATOMIC_ACQUIRE IF_GCC_LLVM(= __ATOMIC_ACQUIRE),
    ATOMIC_RELEASE IF_GCC_LLVM(= __ATOMIC_RELEASE),
    ATOMIC_ACQ_REL IF_GCC_LLVM(= __ATOMIC_ACQ_REL),
};

GPUSH GWRN(pedantic)
typedef IF_GCC_LLVM(IF_X64(unsigned __int128)) IF_MSVC_X32(struct { unsigned long long v[2]; }) Uint128_t;
GPOP

#define assert_wide(PREFIX) _Static_assert(2 * sizeof(H ## PREFIX ## _t) == sizeof(PREFIX ## _t), "")
#define assert_narr(PREFIX) _Static_assert(sizeof(D ## PREFIX ## _t) == 2 * sizeof(PREFIX ## _t), "")

typedef bool Bool_t;

typedef unsigned char Uchar_t;
typedef unsigned short DUchar_t;
assert_narr(Uchar);

typedef unsigned char HUshrt_t;
typedef unsigned short Ushrt_t;
typedef unsigned int DUshrt_t;
assert_narr(Ushrt);
assert_wide(Ushrt);

typedef unsigned short HUint_t;
typedef unsigned Uint_t;
typedef IF_GCC_LLVM(IF_X64(unsigned long)) IF_MSVC_X32(unsigned long long) DUint_t;
assert_narr(Uint);
assert_wide(Uint);

typedef IF_GCC_LLVM(IF_X64(unsigned)) IF_MSVC_X32(unsigned short) HUlong_t;
typedef unsigned long Ulong_t;
typedef IF_GCC_LLVM(IF_X64(Uint128_t)) IF_MSVC_X32(unsigned long long) DUlong_t;
typedef IF_GCC_LLVM(IF_X64(unsigned long long)) IF_MSVC_X32(unsigned) EUlong_t; // Equivalent type for 'unsigned long' 
assert_narr(Ulong);
assert_wide(Ulong);
_Static_assert(sizeof(Ulong_t) == sizeof(EUlong_t), "");

typedef unsigned HUllong_t;
typedef unsigned long long Ullong_t;
typedef Uint128_t DUllong_t;
assert_narr(Ullong);
assert_wide(Ullong);

typedef unsigned long long HUint128_t;
assert_wide(Uint128);

typedef IF_X32(unsigned short) IF_X64(unsigned) Hsize_t;
typedef IF_X32(unsigned long long) IF_X64(Uint128_t) Dsize_t;
assert_narr(size);
assert_wide(size);

typedef void *Ptr_t;

// Rounding down/up to the nearest power of 2 for the inline usage 
#define RP2_1(X, Y) ((X) | ((X) >> (Y)))
#define RP2_64(X) RP2_1(RP2_1(RP2_1(RP2_1(RP2_1(RP2_1(0ull | (X), 1), 2), 4), 8), 16), 32)
#define RP2(X, CEIL) ((X) && ((X) & ((X) - 1)) ? (CEIL) ? RP2_64(X) + 1 : (RP2_64(X) >> 1) + 1 : (X))

// Base 2 integer logarithm for inline usage
#define LOG2_1(X, C, Z) ((X) C (1ull << (Z)))
#define LOG2_4(X, C, Z) (LOG2_1(X, C, (Z)) + LOG2_1(X, C, (Z) + 1) + LOG2_1(X, C, (Z) + 2) + LOG2_1(X, C, (Z) + 3))
#define LOG2_16(X, C, Z) (LOG2_4(X, C, (Z)) + LOG2_4(X, C, (Z) + 4) + LOG2_4(X, C, (Z) + 8) + LOG2_4(X, C, (Z) + 12))
#define LOG2_64(X, C) (LOG2_16(X, C, 0) + LOG2_16(X, C, 16) + LOG2_16(X, C, 32) + LOG2_16(X, C, 48))
#define LOG2(X, CEIL) (((CEIL) ? (((X) > 0) + LOG2_64((X), >)) : (LOG2_64((X), >=))) - 1)

// Base 10 integer logarithm for inline usage
#define LOG10_1(X, C, Z) ((X) C 1 ## Z ## u)
#define LOG10_5(X, C, Z) (LOG10_1(X, C, Z) + LOG10_1(X, C, Z ## 0) + LOG10_1(X, C, Z ## 00) + LOG10_1(X, C, Z ## 000) + LOG10_1(X, C, Z ## 0000))
#define LOG10_10(X, C, Z) (LOG10_5(X, C, Z) + LOG10_5(X, C, Z ## 00000))
#define LOG10_20(X, C) (LOG10_10(X, C, ) + LOG10_10(X, C, 0000000000))
#define LOG10(X, CEIL) (((CEIL) ? (((X) > 0) + LOG10_20((X), >)) : (LOG10_20((X), >=))) - 1)

#define TYPE_CNT(BIT, TOT) ((BIT) / (TOT) + !!((BIT) % (TOT))) // Not always equals to ((BIT) + (TOT) - 1) / (TOT)
#define NIBBLE_CNT(NIBBLE) TYPE_CNT(NIBBLE, bitsof(uint8_t) >> 1)
#define UINT8_CNT(BIT) TYPE_CNT(BIT, bitsof(uint8_t))

#define SIZE_BIT (bitsof(size_t))
#define SIZE_CNT(BIT) TYPE_CNT(BIT, SIZE_BIT)
#define UINT32_BIT (bitsof(uint32_t))
#define UINT64_BIT (bitsof(uint64_t))

#define bitsof(X) (CHAR_BIT * sizeof(X))

#define umax(X) (_Generic((X), \
    Uchar_t: UCHAR_MAX, \
    Ushrt_t: USHRT_MAX, \
    Uint_t: UINT_MAX, \
    Ulong_t: ULONG_MAX, \
    Ullong_t: ULLONG_MAX))

// Type of double width
DUchar_t Uchar_wide(Uchar_t, Uchar_t);
DUshrt_t Ushrt_wide(Ushrt_t, Ushrt_t);
DUint_t Uint_wide(Uint_t, Uint_t);
DUlong_t Ulong_wide(Ulong_t, Ulong_t);
DUllong_t Ullong_wide(Ullong_t, Ullong_t);

#define wide(LO, HI) (_Generic((LO), \
    Uchar_t: Uchar_wide, \
    Ushrt_t: Ushrt_wide, \
    Uint_t: Uint_wide, \
    Ulong_t: Ulong_wide, \
    Ullong_t: Ullong_wide)((LO), (HI)))

// Lower part
HUshrt_t Ushrt_lo(Ushrt_t);
HUint_t Uint_lo(Uint_t);
HUlong_t Ulong_lo(Ulong_t);
HUllong_t Ullong_lo(Ullong_t);
HUint128_t Uint128_lo(Uint128_t);

#define lo(X) (_Generic((X), \
    Ushrt_t: Ushrt_lo, \
    Uint_t: Uint_lo, \
    Ulong_t: Ulong_lo, \
    Ullong_t: Ullong_lo, \
    Uint128_t: Uint128_lo)((X)))

// Higher part
HUshrt_t Ushrt_hi(Ushrt_t);
HUint_t Uint_hi(Uint_t);
HUlong_t Ulong_hi(Ulong_t);
HUllong_t Ullong_hi(Ullong_t);
HUint128_t Uint128_hi(Uint128_t);

#define hi(X) (_Generic((X), \
    Ushrt_t: Ushrt_hi, \
    Uint_t: Uint_hi, \
    Ulong_t: Ulong_hi, \
    Ullong_t: Ullong_hi, \
    Uint128_t: Uint128_hi)((X)))

// Add with input/output carry
Uchar_t Uchar_add(Uchar_t *, Uchar_t, Uchar_t);
Ushrt_t Ushrt_add(Uchar_t *, Ushrt_t, Ushrt_t);
Uint_t Uint_add(Uchar_t *, Uint_t, Uint_t);
Ulong_t Ulong_add(Uchar_t *, Ulong_t, Ulong_t);
Ullong_t Ullong_add(Uchar_t *, Ullong_t, Ullong_t);

#define add(P_CAR, X, Y) (_Generic((X), \
    Uchar_t: Uchar_add, \
    Ushrt_t: Ushrt_add, \
    Uint_t: Uint_add, \
    Ulong_t: Ulong_add, \
    Ullong_t: Ullong_add)((P_CAR), (X), (Y)))

// Subtract with input/output borrow
Uchar_t Uchar_sub(Uchar_t *, Uchar_t, Uchar_t);
Ushrt_t Ushrt_sub(Uchar_t *, Ushrt_t, Ushrt_t);
Uint_t Uint_sub(Uchar_t *, Uint_t, Uint_t);
Ulong_t Ulong_sub(Uchar_t *, Ulong_t, Ulong_t);
Ullong_t Ullong_sub(Uchar_t *, Ullong_t, Ullong_t);

#define sub(P_BOR, X, Y) (_Generic((X), \
    Uchar_t: Uchar_sub, \
    Ushrt_t: Ushrt_sub, \
    Uint_t: Uint_sub, \
    Ulong_t: Ulong_sub, \
    Ullong_t: Ullong_sub)((P_BOR), (X), (Y)))

// Multiply
Uchar_t Uchar_mul(Uchar_t *, Uchar_t, Uchar_t);
Ushrt_t Ushrt_mul(Ushrt_t *, Ushrt_t, Ushrt_t);
Uint_t Uint_mul(Uint_t *, Uint_t, Uint_t);
Ulong_t Ulong_mul(Ulong_t *, Ulong_t, Ulong_t);
Ullong_t Ullong_mul(Ullong_t *, Ullong_t, Ullong_t);
IF_X64(Uint128_t Uint128_mul(Uint128_t *, Uint128_t, Uint128_t);)

#define mul(P_HI, X, Y) (_Generic(*(P_HI), \
    Uchar_t: Uchar_mul, \
    Ushrt_t: Ushrt_mul, \
    Uint_t: Uint_mul, \
    Ulong_t: Ulong_mul, \
    Ullong_t: Ullong_mul \
    IF_X64(, Uint128_t: Uint128_mul))((P_HI), (X), (Y)))

// Multiply–accumulate
Uchar_t Uchar_ma(Uchar_t *, Uchar_t, Uchar_t, Uchar_t);
Ushrt_t Ushrt_ma(Ushrt_t *, Ushrt_t, Ushrt_t, Ushrt_t);
Uint_t Uint_ma(Uint_t *, Uint_t, Uint_t, Uint_t);
Ulong_t Ulong_ma(Ulong_t *, Ulong_t, Ulong_t, Ulong_t);
Ullong_t Ullong_ma(Ullong_t *, Ullong_t, Ullong_t, Ullong_t);

#define ma(P_RES, X, Y, Z) (_Generic(*(P_RES), \
    Uchar_t: Uchar_ma, \
    Ushrt_t: Ushrt_ma, \
    Uint_t: Uint_ma, \
    Ulong_t: Ulong_ma, \
    Ullong_t: Ullong_ma)((P_RES), (X), (Y), (Z)))

// Shift left
Uchar_t Uchar_shl(Uchar_t *, Uchar_t, Uchar_t);
Ushrt_t Ushrt_shl(Ushrt_t *, Ushrt_t, Uchar_t);
Uint_t Uint_shl(Uint_t *, Uint_t, Uchar_t);
Ulong_t Ulong_shl(Ulong_t *, Ulong_t, Uchar_t);
Ullong_t Ullong_shl(Ullong_t *, Ullong_t, Uchar_t);

#define shl(P_HI, X, Y) (_Generic(*(P_HI), \
    Uchar_t: Uchar_shl, \
    Ushrt_t: Ushrt_shl, \
    Uint_t: Uint_shl, \
    Ulong_t: Ulong_shl, \
    Ullong_t: Ullong_shl)((P_HI), (X), (Y)))

// Shift right
Uchar_t Uchar_shr(Uchar_t *, Uchar_t, Uchar_t);
Ushrt_t Ushrt_shr(Ushrt_t *, Ushrt_t, Uchar_t);
Uint_t Uint_shr(Uint_t *, Uint_t, Uchar_t);
Ulong_t Ulong_shr(Ulong_t *, Ulong_t, Uchar_t);
Ullong_t Ullong_shr(Ullong_t *, Ullong_t, Uchar_t);

#define shr(P_LO, X, Y) (_Generic(*(P_HI), \
    Uchar_t: Uchar_shr, \
    Ushrt_t: Ushrt_shr, \
    Uint_t: Uint_shr, \
    Ulong_t: Ulong_shr IF64(, \
    Ullong_t: Ullong_shr))((P_LO), (X), (Y)))

// Rotate left
Uchar_t Uchar_rol(Uchar_t, Uchar_t);
Ushrt_t Ushrt_rol(Ushrt_t, Uchar_t);
Uint_t Uint_rol(Uint_t, Uchar_t);
Ulong_t Ulong_rol(Ulong_t, Uchar_t);
Ullong_t Ullong_rol(Ullong_t, Uchar_t);

#define rol(X, Y) (_Generic((X), \
    Uchar_t: Uchar_rol, \
    Ushrt_t: Ushrt_rol, \
    Uint_t: Uint_rol, \
    Ulong_t: Ulong_rol, \
    Ullong_t: Ullong_rol)((X), (Y)))

// Rotate right
Uchar_t Uchar_ror(Uchar_t, Uchar_t);
Ushrt_t Ushrt_ror(Ushrt_t, Uchar_t);
Uint_t Uint_ror(Uint_t, Uchar_t);
Ulong_t Ulong_ror(Ulong_t, Uchar_t);
Ullong_t Ullong_ror(Ullong_t, Uchar_t);

#define ror(X, Y) (_Generic((X), \
    Uchar_t: Uchar_ror, \
    Ushrt_t: Ushrt_ror, \
    Uint_t: Uint_ror, \
    Ulong_t: Ulong_ror, \
    Ullong_t: Ullong_ror)((X), (Y)))

// Saturated add
Uchar_t Uchar_add_sat(Uchar_t, Uchar_t);
Ushrt_t Ushrt_add_sat(Ushrt_t, Ushrt_t);
Uint_t Uint_add_sat(Uint_t, Uint_t);
Ulong_t Ulong_add_sat(Ulong_t, Ulong_t);
Ullong_t Ullong_add_sat(Ullong_t, Ullong_t);

#define add_sat(X, Y) (_Generic((X), \
    Uchar_t: Uchar_add_sat, \
    Ushrt_t: Ushrt_add_sat, \
    Uint_t: Uint_add_sat, \
    Ulong_t: Ulong_add_sat, \
    Ullong_t: Ullong_add_sat)((X), (Y)))

// Saturated subtract
Uchar_t Uchar_sub_sat(Uchar_t, Uchar_t);
Ushrt_t Ushrt_sub_sat(Ushrt_t, Ushrt_t);
Uint_t Uint_sub_sat(Uint_t, Uint_t);
Ulong_t Ulong_sub_sat(Ulong_t, Ulong_t);
Ullong_t Ullong_sub_sat(Ullong_t, Ullong_t);

#define sub_sat(X, Y) (_Generic((X), \
    Uchar_t: Uchar_sub_sat, \
    Ushrt_t: Ushrt_sub_sat, \
    Uint_t: Uint_sub_sat, \
    Ulong_t: Ulong_sub_sat, \
    Ullong_t: Ullong_sub_sat)((X), (Y)))

// Sign of the difference of Uint_t integers
int Uchar_usign(Uchar_t, Uchar_t);
int Ushrt_usign(Ushrt_t, Ushrt_t);
int Uint_usign(Uint_t, Uint_t);
int Ulong_usign(Ulong_t, Ulong_t);
int Ullong_usign(Ullong_t, Ullong_t);

#define usign(X, Y) (_Generic((X), \
    Uchar_t: Uchar_usign, \
    Ushrt_t: Ushrt_usign, \
    Uint_t: Uint_usign, \
    Ulong_t: Ulong_usign, \
    Ullong_t: Ullong_usign)((X), (Y)))

// Absolute difference of Uint_t integers
Uchar_t Uchar_udist(Uchar_t, Uchar_t);
Ushrt_t Ushrt_udist(Ushrt_t, Ushrt_t);
Uint_t Uint_udist(Uint_t, Uint_t);
Ulong_t Ulong_udist(Ulong_t, Ulong_t);
Ullong_t Ullong_udist(Ullong_t, Ullong_t);

#define udist(X, Y) (_Generic((X), \
    Uchar_t: Uchar_udist, \
    Ushrt_t: Ushrt_udist, \
    Uint_t: Uint_udist, \
    Ulong_t: Ulong_udist, \
    Ullong_t: Ullong_udist)((X), (Y)))

// Non-destructive add with overflow test
bool Uchar_test_add(Uchar_t *, Uchar_t);
bool Ushrt_test_add(Ushrt_t *, Ushrt_t);
bool Uint_test_add(Uint_t *, Uint_t);
bool Ulong_test_add(Ulong_t *, Ulong_t);
bool Ullong_test_add(Ullong_t *, Ullong_t);

#define test_add(P_RES, X) (_Generic(*(P_RES), \
    Uchar_t: Uchar_test_add, \
    Ushrt_t: Ushrt_test_add, \
    Uint_t: Uint_test_add, \
    Ulong_t: Ulong_test_add, \
    Ullong_t: Ullong_test_add)((P_RES), (X)))

// Non-destructive subtract with overflow test
bool Uchar_test_sub(Uchar_t *, Uchar_t);
bool Ushrt_test_sub(Ushrt_t *, Ushrt_t);
bool Uint_test_sub(Uint_t *, Uint_t);
bool Ulong_test_sub(Ulong_t *, Ulong_t);
bool Ullong_test_sub(Ullong_t *, Ullong_t);

#define test_sub(P_RES, X) (_Generic(*(P_RES), \
    Uchar_t: Uchar_test_sub, \
    Ushrt_t: Ushrt_test_sub, \
    Uint_t: Uint_test_sub, \
    Ulong_t: Ulong_test_sub, \
    Ullong_t: Ullong_test_sub)((P_RES), (X)))

// Non-destructive multiply with overflow test
bool Uchar_test_mul(Uchar_t *, Uchar_t);
bool Ushrt_test_mul(Ushrt_t *, Ushrt_t);
bool Uint_test_mul(Uint_t *, Uint_t);
bool Ulong_test_mul(Ulong_t *, Ulong_t);
bool Ullong_test_mul(Ullong_t *, Ullong_t);

#define test_mul(P_RES, X) (_Generic(*(P_RES), \
    Uchar_t: Uchar_test_mul, \
    Ushrt_t: Ushrt_test_mul, \
    Uint_t: Uint_test_mul, \
    Ulong_t: Ulong_test_mul, \
    Ullong_t: Ullong_test_mul)((P_RES), (X)))

// Non-destructive multiply–accumulate with overflow test
bool Uchar_test_ma(Uchar_t *, Uchar_t, Uchar_t);
bool Ushrt_test_ma(Ushrt_t *, Ushrt_t, Ushrt_t);
bool Uint_test_ma(Uint_t *, Uint_t, Uint_t);
bool Ulong_test_ma(Ulong_t *, Ulong_t, Ulong_t);
bool Ullong_test_ma(Ullong_t *, Ullong_t, Ullong_t);

#define test_ma(P_RES, X, Y) (_Generic(*(P_RES), \
    Uchar_t: Uchar_test_ma, \
    Ushrt_t: Ushrt_test_ma, \
    Uint_t: Uint_test_ma, \
    Ulong_t: Ulong_test_ma, \
    Ullong_t: Ullong_test_ma)((P_RES), (X), (Y)))

// Sum with overflow test
size_t Uchar_test_sum(Uchar_t *, Uchar_t *, size_t);
size_t Ushrt_test_sum(Ushrt_t *, Ushrt_t *, size_t);
size_t Uint_test_sum(Uint_t *, Uint_t *, size_t);
size_t Ulong_test_sum(Ulong_t *, Ulong_t *, size_t);
size_t Ullong_test_sum(Ullong_t *, Ullong_t *, size_t);

#define test_sum(P_RES, L, C) (_Generic(*(P_RES), \
    Uchar_t: Uchar_test_sum, \
    Ushrt_t: Ushrt_test_sum, \
    Uint_t: Uint_test_sum, \
    Ulong_t: Ulong_test_sum, \
    Ullong_t: Ullong_test_sum)((P_RES), (L), (C)))

// Product with overflow test
size_t Uchar_test_prod(Uchar_t *, Uchar_t *, size_t);
size_t Ushrt_test_prod(Ushrt_t *, Ushrt_t *, size_t);
size_t Uint_test_prod(Uint_t *, Uint_t *, size_t);
size_t Ulong_test_prod(Ulong_t *, Ulong_t *, size_t);
size_t Ullong_test_prod(Ullong_t *, Ullong_t *, size_t);

#define test_prod(P_RES, L, C) (_Generic(*(P_RES), \
    Uchar_t: Uchar_test_prod, \
    Ushrt_t: Ushrt_test_prod, \
    Uint_t: Uint_test_prod, \
    Ulong_t: Ulong_test_prod, \
    Ullong_t: Ullong_test_prod)((P_RES), (L), (C)))

// Bit scan reverse
Uchar_t Uchar_bsr(Uchar_t);
Ushrt_t Ushrt_bsr(Ushrt_t);
Uint_t Uint_bsr(Uint_t);
Ulong_t Ulong_bsr(Ulong_t);
Ullong_t Ullong_bsr(Ullong_t);

#define bsr(X) (_Generic((X), \
    Uchar_t: Uchar_bsr, \
    Ushrt_t: Ushrt_bsr, \
    Uint_t: Uint_bsr, \
    Ulong_t: Ulong_bsr, \
    Ullong_t: Ullong_bsr)((X)))

// Bit scan forward
Uchar_t Uchar_bsf(Uchar_t);
Ushrt_t Ushrt_bsf(Ushrt_t);
Uint_t Uint_bsf(Uint_t);
Ulong_t Ulong_bsf(Ulong_t);
Ullong_t Ullong_bsf(Ullong_t);

#define bsf(X) (_Generic((X), \
    Uchar_t: Uchar_bsf, \
    Ushrt_t: Ushrt_bsf, \
    Uint_t: Uint_bsf, \
    Ulong_t: Ulong_bsf, \
    Ullong_t: Ullong_bsf)((X)))

// Population count
Uchar_t Uchar_pcnt(Uchar_t);
Ushrt_t Ushrt_pcnt(Ushrt_t);
Uint_t Uint_pcnt(Uint_t);
Ulong_t Ulong_pcnt(Ulong_t);
Ullong_t Ullong_pcnt(Ullong_t);

#define pcnt(X) (_Generic((X), \
    Uchar_t: Uchar_pcnt, \
    Ushrt_t: Ushrt_pcnt, \
    Uint_t: Uint_pcnt, \
    Ulong_t: Ulong_pcnt, \
    Ullong_t: Ullong_pcnt)((X)))

// Bit test
bool Uchar_bt(Uchar_t *, size_t);
bool Ushrt_bt(Ushrt_t *, size_t);
bool Uint_bt(Uint_t *, size_t);
bool Ulong_bt(Ulong_t *, size_t);
bool Ullong_bt(Ullong_t *, size_t);

#define bt(ARR, POS) (_Generic(*(ARR), \
    Uchar_t: Uchar_bt, \
    Ushrt_t: Ushrt_bt, \
    Uint_t: Uint_bt, \
    Ulong_t: Ulong_bt, \
    Ullong_t: Ullong_bt)((ARR), (POS)))

// Bit test and set
bool Uchar_bts(Uchar_t *, size_t);
bool Ushrt_bts(Ushrt_t *, size_t);
bool Uint_bts(Uint_t *, size_t);
bool Ulong_bts(Ulong_t *, size_t);
bool Ullong_bts(Ullong_t *, size_t);

#define bts(ARR, POS) (_Generic(*(ARR), \
    Uchar_t: Uchar_bts, \
    Ushrt_t: Ushrt_bts, \
    Uint_t: Uint_bts, \
    Ulong_t: Ulong_bts, \
    Ullong_t: Ullong_bts)((ARR), (POS)))

// Bit test and reset
bool Uchar_btr(Uchar_t *, size_t);
bool Ushrt_btr(Ushrt_t *, size_t);
bool Uint_btr(Uint_t *, size_t);
bool Ulong_btr(Ulong_t *, size_t);
bool Ullong_btr(Ullong_t *, size_t);

#define btr(ARR, POS) (_Generic(*(ARR), \
    Uchar_t: Uchar_btr, \
    Ushrt_t: Ushrt_btr, \
    Uint_t: Uint_btr, \
    Ulong_t: Ulong_btr, \
    Ullong_t: Ullong_btr)((ARR), (POS)))

// Bit set
void Uchar_bs(Uchar_t *, size_t);
void Ushrt_bs(Ushrt_t *, size_t);
void Uint_bs(Uint_t *, size_t);
void Ulong_bs(Ulong_t *, size_t);
void Ullong_bs(Ullong_t *, size_t);

#define bs(ARR, POS) (_Generic(*(ARR), \
    Uchar_t: Uchar_bs, \
    Ushrt_t: Ushrt_bs, \
    Uint_t: Uint_bs, \
    Ulong_t: Ulong_bs, \
    Ullong_t: Ullong_bs)((ARR), (POS)))

// Bit reset
void Uchar_br(Uchar_t *, size_t);
void Ushrt_br(Ushrt_t *, size_t);
void Uint_br(Uint_t *, size_t);
void Ulong_br(Ulong_t *, size_t);
void Ullong_br(Ullong_t *, size_t);

#define br(ARR, POS) (_Generic(*(ARR), \
    Uchar_t: Uchar_br, \
    Ushrt_t: Ushrt_br, \
    Uint_t: Uint_br, \
    Ulong_t: Ulong_br, \
    Ullong_t: Ullong_br)((ARR), (POS)))

// Bit fetch burst
Uchar_t Uchar_bt_burst(Uchar_t *, size_t, size_t);
Ushrt_t Ushrt_bt_burst(Ushrt_t *, size_t, size_t);
Uint_t Uint_bt_burst(Uint_t *, size_t, size_t);
Ulong_t Ulong_bt_burst(Ulong_t *, size_t, size_t);
Ullong_t Ullong_bt_burst(Ullong_t *, size_t, size_t);

#define bt_burst(ARR, POS, STRIDE) (_Generic(*(ARR), \
    Uchar_t: Uchar_bt_burst, \
    Ushrt_t: Ushrt_bt_burst, \
    Uint_t: Uint_bt_burst, \
    Ulong_t: Ulong_bt_burst, \
    Ullong_t: Ullong_bt_burst)((ARR), (POS), (STRIDE)))

// Bit fetch and set burst
Uchar_t Uchar_bts_burst(Uchar_t *, size_t, size_t, Uchar_t);
Ushrt_t Ushrt_bts_burst(Ushrt_t *, size_t, size_t, Ushrt_t);
Uint_t Uint_bts_burst(Uint_t *, size_t, size_t, Uint_t);
Ulong_t Ulong_bts_burst(Ulong_t *, size_t, size_t, Ulong_t);
Ullong_t Ullong_bts_burst(Ullong_t *, size_t, size_t, Ullong_t);

#define bts_burst(ARR, POS, STRIDE, MSK) (_Generic(*(ARR), \
    Uchar_t: Uchar_bts_burst, \
    Ushrt_t: Ushrt_bts_burst, \
    Uint_t: Uint_bts_burst, \
    Ulong_t: Ulong_bts_burst, \
    Ullong_t: Ullong_bts_burst)((ARR), (POS), (STRIDE), (MSK)))

// Bit fetch and reset burst
Uchar_t Uchar_btr_burst(Uchar_t *, size_t, size_t, Uchar_t);
Ushrt_t Ushrt_btr_burst(Ushrt_t *, size_t, size_t, Ushrt_t);
Uint_t Uint_btr_burst(Uint_t *, size_t, size_t, Uint_t);
Ulong_t Ulong_btr_burst(Ulong_t *, size_t, size_t, Ulong_t);
Ullong_t Ullong_btr_burst(Ullong_t *, size_t, size_t, Ullong_t);

#define btr_burst(ARR, POS, STRIDE, MSK) (_Generic(*(ARR), \
    Uchar_t: Uchar_btr_burst, \
    Ushrt_t: Ushrt_btr_burst, \
    Uint_t: Uint_btr_burst, \
    Ulong_t: Ulong_btr_burst, \
    Ullong_t: Ullong_btr_burst)((ARR), (POS), (STRIDE), (MSK)))

// Bit set burst
void Uchar_bs_burst(Uchar_t *, size_t, size_t, Uchar_t);
void Ushrt_bs_burst(Ushrt_t *, size_t, size_t, Ushrt_t);
void Uint_bs_burst(Uint_t *, size_t, size_t, Uint_t);
void Ulong_bs_burst(Ulong_t *, size_t, size_t, Ulong_t);
void Ullong_bs_burst(Ullong_t *, size_t, size_t, Ullong_t);

#define bs_burst(ARR, POS, STRIDE, MSK) (_Generic(*(ARR), \
    Uchar_t: Uchar_bs_burst, \
    Ushrt_t: Ushrt_bs_burst, \
    Uint_t: Uint_bs_burst, \
    Ulong_t: Ulong_bs_burst, \
    Ullong_t: Ullong_bs_burst)((ARR), (POS), (STRIDE), (MSK)))

// Bit reset burst
void Uchar_br_burst(Uchar_t *, size_t, size_t, Uchar_t);
void Ushrt_br_burst(Ushrt_t *, size_t, size_t, Ushrt_t);
void Uint_br_burst(Uint_t *, size_t, size_t, Uint_t);
void Ulong_br_burst(Ulong_t *, size_t, size_t, Ulong_t);
void Ullong_br_burst(Ullong_t *, size_t, size_t, Ullong_t);

#define br_burst(ARR, POS, STRIDE, MSK) (_Generic(*(ARR), \
    Uchar_t: Uchar_br_burst, \
    Ushrt_t: Ushrt_br_burst, \
    Uint_t: Uint_br_burst, \
    Ulong_t: Ulong_br_burst, \
    Ullong_t: Ullong_br_burst)((ARR), (POS), (STRIDE), (MSK)))

// Atomic load
bool bool_atomic_load_mo(volatile bool *, enum atomic_mo);
Uchar_t Uchar_atomic_load_mo(volatile Uchar_t *, enum atomic_mo);
Ushrt_t Ushrt_atomic_load_mo(volatile Ushrt_t *, enum atomic_mo);
Uint_t Uint_atomic_load_mo(volatile Uint_t *, enum atomic_mo);
Ulong_t Ulong_atomic_load_mo(volatile Ulong_t *, enum atomic_mo);
Ullong_t Ullong_atomic_load_mo(volatile Ullong_t *, enum atomic_mo);
Ptr_t Ptr_atomic_load_mo(Ptr_t volatile *, enum atomic_mo);

#define atomic_load_mo(SRC, MO) (_Generic((SRC), \
    volatile bool *: bool_atomic_load_mo, \
    volatile Uchar_t *: Uchar_atomic_load_mo, \
    volatile Ushrt_t *: Ushrt_atomic_load_mo, \
    volatile Uint_t *: Uint_atomic_load_mo, \
    volatile Ulong_t *: Ulong_atomic_load_mo, \
    volatile Ullong_t *: Ullong_atomic_load_mo, \
    Ptr_t volatile *: Ptr_atomic_load_mo)((SRC), (MO)))

#define atomic_load(SRC) \
    (atomic_load_mo((SRC), ATOMIC_ACQUIRE))

// Atomic store
void bool_atomic_store_mo(volatile bool *, bool, enum atomic_mo);
void Uchar_atomic_store_mo(volatile Uchar_t *, Uchar_t, enum atomic_mo);
void Ushrt_atomic_store_mo(volatile Ushrt_t *, Ushrt_t, enum atomic_mo);
void Uint_atomic_store_mo(volatile Uint_t *, Uint_t, enum atomic_mo);
void Ulong_atomic_store_mo(volatile Ulong_t *, Ulong_t, enum atomic_mo);
void Ullong_atomic_store_mo(volatile Ullong_t *, Ullong_t, enum atomic_mo);
void Ptr_atomic_store_mo(Ptr_t volatile *, void *, enum atomic_mo);

#define atomic_store_mo(DST, VAL, MO) (_Generic((DST), \
    volatile bool *: bool_atomic_store_mo, \
    volatile Uchar_t *: Uchar_atomic_store_mo, \
    volatile Ushrt_t *: Ushrt_atomic_store_mo, \
    volatile Uint_t *: Uint_atomic_store_mo, \
    volatile Ulong_t *: Ulong_atomic_store_mo, \
    volatile Ullong_t *: Ullong_atomic_store_mo, \
    Ptr_t volatile *: Ptr_atomic_store_mo)((DST), (VAL), (MO)))

#define atomic_store(DST, VAL) \
    (atomic_store_mo((DST), (VAL), ATOMIC_RELEASE))

// Atomic compare and swap
bool Uchar_atomic_cmp_xchg_mo(volatile Uchar_t *, Uchar_t *, Uchar_t, bool, enum atomic_mo, enum atomic_mo);
bool Ushrt_atomic_cmp_xchg_mo(volatile Ushrt_t*, Ushrt_t *, Ushrt_t, bool, enum atomic_mo, enum atomic_mo);
bool Uint_atomic_cmp_xchg_mo(volatile Uint_t *, Uint_t *, Uint_t, bool, enum atomic_mo, enum atomic_mo);
bool Ulong_atomic_cmp_xchg_mo(volatile Ulong_t *, Ulong_t *, Ulong_t, bool, enum atomic_mo, enum atomic_mo);
bool Ullong_atomic_cmp_xchg_mo(volatile Ullong_t *, Ullong_t *, Ullong_t, bool, enum atomic_mo, enum atomic_mo);
bool Ptr_atomic_cmp_xchg_mo(Ptr_t volatile *, void **, void *, bool, enum atomic_mo, enum atomic_mo);
IF_X64(bool Uint128_atomic_cmp_xchg_mo(volatile Uint128_t *, Uint128_t *, Uint128_t, bool, enum atomic_mo, enum atomic_mo);)

#define atomic_cmp_xchg_mo(DST, P_CMP, XCHG, WEAK, MO_SUCC, MO_FAIL) (_Generic((DST), \
    volatile Uchar_t *: Uchar_atomic_cmp_xchg_mo, \
    volatile Ushrt_t *: Ushrt_atomic_cmp_xchg_mo, \
    volatile Uint_t *: Uint_atomic_cmp_xchg_mo, \
    volatile Ulong_t *: Ulong_atomic_cmp_xchg_mo, \
    volatile Ullong_t *: Ullong_atomic_cmp_xchg_mo,\
    Ptr_t volatile *: Ptr_atomic_cmp_xchg_mo \
    IF_X64(, volatile Uint128_t *: Uint128_atomic_cmp_xchg_mo))((DST), (P_CMP), (XCHG), (WEAK), (MO_SUCC), (MO_FAIL)))

#define atomic_cmp_xchg(DST, P_CMP, XCHG) \
    (atomic_cmp_xchg_mo((DST), (P_CMP), (XCHG), 0, ATOMIC_ACQ_REL, ATOMIC_ACQUIRE))

// Atomic fetch-AND
Uchar_t Uchar_atomic_fetch_and_mo(volatile Uchar_t *, Uchar_t, enum atomic_mo);
Ushrt_t Ushrt_atomic_fetch_and_mo(volatile Ushrt_t *, Ushrt_t, enum atomic_mo);
Uint_t Uint_atomic_fetch_and_mo(volatile Uint_t *, Uint_t, enum atomic_mo);
Ulong_t Ulong_atomic_fetch_and_mo(volatile Ulong_t *, Ulong_t, enum atomic_mo);
Ullong_t Ullong_atomic_fetch_and_mo(volatile Ullong_t *, Ullong_t, enum atomic_mo);

#define atomic_fetch_and_mo(DST, ARG, MO) (_Generic((DST), \
    volatile Uchar_t *: Uchar_atomic_fetch_and_mo, \
    volatile Ushrt_t *: Ushrt_atomic_fetch_and_mo, \
    volatile Uint_t *: Uint_atomic_fetch_and_mo, \
    volatile Ulong_t *: Ulong_atomic_fetch_and_mo, \
    volatile Ullong_t *: Ullong_atomic_fetch_and_mo)((DST), (ARG), (MO)))

#define atomic_fetch_and(DST, ARG) \
    (atomic_fetch_and_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Atomic fetch-OR
Uchar_t Uchar_atomic_fetch_or_mo(volatile Uchar_t *, Uchar_t, enum atomic_mo);
Ushrt_t Ushrt_atomic_fetch_or_mo(volatile Ushrt_t *, Ushrt_t, enum atomic_mo);
Uint_t Uint_atomic_fetch_or_mo(volatile Uint_t *, Uint_t, enum atomic_mo);
Ulong_t Ulong_atomic_fetch_or_mo(volatile Ulong_t *, Ulong_t, enum atomic_mo);
Ullong_t Ullong_atomic_fetch_or_mo(volatile Ullong_t *, Ullong_t, enum atomic_mo);

#define atomic_fetch_or_mo(DST, ARG, MO) (_Generic((DST), \
    volatile Uchar_t *: Uchar_atomic_fetch_or_mo, \
    volatile Ushrt_t *: Ushrt_atomic_fetch_or_mo, \
    volatile Uint_t *: Uint_atomic_fetch_or_mo, \
    volatile Ulong_t *: Ulong_atomic_fetch_or_mo, \
    volatile Ullong_t *: Ullong_atomic_fetch_or_mo)((DST), (ARG), (MO)))

#define atomic_fetch_or(DST, ARG) \
    (atomic_fetch_or_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Atomic fetch-XOR
Uchar_t Uchar_atomic_fetch_xor_mo(volatile Uchar_t *, Uchar_t, enum atomic_mo);
Ushrt_t Ushrt_atomic_fetch_xor_mo(volatile Ushrt_t *, Ushrt_t, enum atomic_mo);
Uint_t Uint_atomic_fetch_xor_mo(volatile Uint_t *, Uint_t, enum atomic_mo);
Ulong_t Ulong_atomic_fetch_xor_mo(volatile Ulong_t *, Ulong_t, enum atomic_mo);
Ullong_t Ullong_atomic_fetch_xor_mo(volatile Ullong_t *, Ullong_t, enum atomic_mo);

#define atomic_fetch_xor_mo(DST, ARG, MO) (_Generic((DST), \
    volatile Uchar_t *: Uchar_atomic_fetch_xor_mo, \
    volatile Ushrt_t *: Ushrt_atomic_fetch_xor_mo, \
    volatile Uint_t *: Uint_atomic_fetch_xor_mo, \
    volatile Ulong_t *: Ulong_atomic_fetch_xor_mo, \
    volatile Ullong_t *: Ullong_atomic_fetch_xor_mo)((DST), (ARG), (MO)))

#define atomic_fetch_xor(DST, ARG) \
    (atomic_fetch_xor_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Atomic fetch-add
Uchar_t Uchar_atomic_fetch_add_mo(volatile Uchar_t *, Uchar_t, enum atomic_mo);
Ushrt_t Ushrt_atomic_fetch_add_mo(volatile Ushrt_t *, Ushrt_t, enum atomic_mo);
Uint_t Uint_atomic_fetch_add_mo(volatile Uint_t *, Uint_t, enum atomic_mo);
Ulong_t Ulong_atomic_fetch_add_mo(volatile Ulong_t *, Ulong_t, enum atomic_mo);
Ullong_t Ullong_atomic_fetch_add_mo(volatile Ullong_t *, Ullong_t, enum atomic_mo);

#define atomic_fetch_add_mo(DST, ARG, MO) (_Generic((DST), \
    volatile Uchar_t *: Uchar_atomic_fetch_add_mo, \
    volatile Ushrt_t *: Ushrt_atomic_fetch_add_mo, \
    volatile Uint_t *: Uint_atomic_fetch_add_mo, \
    volatile Ulong_t *: Ulong_atomic_fetch_add_mo, \
    volatile Ullong_t *: Ullong_atomic_fetch_add_mo)((DST), (ARG), (MO)))

#define atomic_fetch_add(DST, ARG) \
    (atomic_fetch_add_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Atomic fetch-subtract
Uchar_t Uchar_atomic_fetch_sub_mo(volatile Uchar_t *, Uchar_t, enum atomic_mo);
Ushrt_t Ushrt_atomic_fetch_sub_mo(volatile Ushrt_t *, Ushrt_t, enum atomic_mo);
Uint_t Uint_atomic_fetch_sub_mo(volatile Uint_t *, Uint_t, enum atomic_mo);
Ulong_t Ulong_atomic_fetch_sub_mo(volatile Ulong_t *, Ulong_t, enum atomic_mo);
Ullong_t Ullong_atomic_fetch_sub_mo(volatile Ullong_t *, Ullong_t, enum atomic_mo);

#define atomic_fetch_sub_mo(DST, ARG, MO) (_Generic((DST), \
    volatile Uchar_t *: Uchar_atomic_fetch_sub_mo, \
    volatile Ushrt_t *: Ushrt_atomic_fetch_sub_mo, \
    volatile Uint_t *: Uint_atomic_fetch_sub_mo, \
    volatile Ulong_t *: Ulong_atomic_fetch_sub_mo, \
    volatile Ullong_t *: Ullong_atomic_fetch_sub_mo)((DST), (ARG), (MO)))

#define atomic_fetch_sub(DST, ARG) \
    (atomic_fetch_sub_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Atomic swap
Uchar_t Uchar_atomic_xchg_mo(volatile Uchar_t *, Uchar_t, enum atomic_mo);
Ushrt_t Ushrt_atomic_xchg_mo(volatile Ushrt_t *, Ushrt_t, enum atomic_mo);
Uint_t Uint_atomic_xchg_mo(volatile Uint_t *, Uint_t, enum atomic_mo);
Ulong_t Ulong_atomic_xchg_mo(volatile Ulong_t *, Ulong_t, enum atomic_mo);
Ullong_t Ullong_atomic_xchg_mo(volatile Ullong_t *, Ullong_t, enum atomic_mo);
void *Ptr_atomic_xchg_mo(Ptr_t volatile *, void *, enum atomic_mo);

#define atomic_xchg_mo(DST, ARG, MO) (_Generic((DST), \
    volatile Uchar_t *: Uchar_atomic_xchg_mo, \
    volatile Ushrt_t *: Ushrt_atomic_xchg_mo, \
    volatile Uint_t *: Uint_atomic_xchg_mo, \
    volatile Ulong_t *: Ulong_atomic_xchg_mo, \
    volatile Ullong_t *: Ullong_atomic_xchg_mo, \
    Ptr_t volatile *: Ptr_atomic_xchg_mo)((DST), (ARG), (MO)))

#define atomic_xchg(DST, ARG) \
    (atomic_xchg_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Atomic fetch-add saturated
Uchar_t Uchar_atomic_fetch_add_sat_mo(volatile Uchar_t *, Uchar_t, enum atomic_mo);
Ushrt_t Ushrt_atomic_fetch_add_sat_mo(volatile Ushrt_t *, Ushrt_t, enum atomic_mo);
Uint_t Uint_atomic_fetch_add_sat_mo(volatile Uint_t *, Uint_t, enum atomic_mo);
Ulong_t Ulong_atomic_fetch_add_sat_mo(volatile Ulong_t *, Ulong_t, enum atomic_mo);
Ullong_t Ullong_atomic_fetch_add_sat_mo(volatile Ullong_t *, Ullong_t, enum atomic_mo);

#define atomic_fetch_add_sat_mo(DST, ARG, MO) (_Generic((DST), \
    volatile Uchar_t *: Uchar_atomic_fetch_add_sat_mo, \
    volatile Ushrt_t *: Ushrt_atomic_fetch_add_sat_mo, \
    volatile Uint_t *: Uint_atomic_fetch_add_sat_mo, \
    volatile Ulong_t *: Ulong_atomic_fetch_add_sat_mo, \
    volatile Ullong_t *: Ullong_atomic_fetch_add_sat_mo)((DST), (ARG), (MO)))

#define atomic_fetch_add_sat(DST, ARG) \
    (atomic_fetch_add_sat_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Atomic fetch-add subtract
Uchar_t Uchar_atomic_fetch_sub_sat_mo(volatile Uchar_t *, Uchar_t, enum atomic_mo);
Ushrt_t Ushrt_atomic_fetch_sub_sat_mo(volatile Ushrt_t *, Ushrt_t, enum atomic_mo);
Uint_t Uint_atomic_fetch_sub_sat_mo(volatile Uint_t *, Uint_t, enum atomic_mo);
Ulong_t Ulong_atomic_fetch_sub_sat_mo(volatile Ulong_t *, Ulong_t, enum atomic_mo);
Ullong_t Ullong_atomic_fetch_sub_sat_mo(volatile Ullong_t *, Ullong_t, enum atomic_mo);

#define atomic_fetch_sub_sat_mo(DST, ARG, MO) (_Generic((DST), \
    volatile Uchar_t *: Uchar_atomic_fetch_sub_sat_mo, \
    volatile Ushrt_t *: Ushrt_atomic_fetch_sub_sat_mo, \
    volatile Uint_t *: Uint_atomic_fetch_sub_sat_mo, \
    volatile Ulong_t *: Ulong_atomic_fetch_sub_sat_mo, \
    volatile Ullong_t *: Ullong_atomic_fetch_sub_sat_mo)((DST), (ARG), (MO)))

#define atomic_fetch_sub_sat(DST, ARG) \
    (atomic_fetch_sub_sat_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Atomic bit test
bool Uchar_atomic_bt_mo(volatile Uchar_t *, size_t, enum atomic_mo);
bool Ushrt_atomic_bt_mo(volatile Ushrt_t *, size_t, enum atomic_mo);
bool Uint_atomic_bt_mo(volatile Uint_t *, size_t, enum atomic_mo);
bool Ulong_atomic_bt_mo(volatile Ulong_t *, size_t, enum atomic_mo);
bool Ullong_atomic_bt_mo(volatile Ullong_t *, size_t, enum atomic_mo);

#define atomic_bt_mo(ARR, POS, MO) (_Generic((ARR), \
    volatile Uchar_t *: Uchar_atomic_bt_mo, \
    volatile Ushrt_t *: Ushrt_atomic_bt_mo, \
    volatile Uint_t *: Uint_atomic_bt_mo, \
    volatile Ulong_t *: Ulong_atomic_bt_mo, \
    volatile Ullong_t *: Ullong_atomic_bt_mo)((ARR), (POS), (MO)))

#define atomic_bt(DST, ARG) \
    (atomic_bt_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Atomic bit test and set
bool Uchar_atomic_bts_mo(volatile Uchar_t *, size_t, enum atomic_mo);
bool Ushrt_atomic_bts_mo(volatile Ushrt_t *, size_t, enum atomic_mo);
bool Uint_atomic_bts_mo(volatile Uint_t *, size_t, enum atomic_mo);
bool Ulong_atomic_bts_mo(volatile Ulong_t *, size_t, enum atomic_mo);
bool Ullong_atomic_bts_mo(volatile Ullong_t *, size_t, enum atomic_mo);

#define atomic_bts_mo(ARR, POS, MO) (_Generic((ARR), \
    volatile Uchar_t *: Uchar_atomic_bts_mo, \
    volatile Ushrt_t *: Ushrt_atomic_bts_mo, \
    volatile Uint_t *: Uint_atomic_bts_mo, \
    volatile Ulong_t *: Ulong_atomic_bts_mo, \
    volatile Ullong_t *: Ullong_atomic_bts_mo)((ARR), (POS), (MO)))

#define atomic_bts(DST, ARG) \
    (atomic_bts_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Atomic bit test and reset
bool Uchar_atomic_btr_mo(volatile Uchar_t *, size_t, enum atomic_mo);
bool Ushrt_atomic_btr_mo(volatile Ushrt_t *, size_t, enum atomic_mo);
bool Uint_atomic_btr_mo(volatile Uint_t *, size_t, enum atomic_mo);
bool Ulong_atomic_btr_mo(volatile Ulong_t *, size_t, enum atomic_mo);
bool Ullong_atomic_btr_mo(volatile Ullong_t *, size_t, enum atomic_mo);

#define atomic_btr_mo(ARR, POS, MO) (_Generic((ARR), \
    volatile Uchar_t *: Uchar_atomic_btr_mo, \
    volatile Ushrt_t *: Ushrt_atomic_btr_mo, \
    volatile Uint_t *: Uint_atomic_btr_mo, \
    volatile Ulong_t *: Ulong_atomic_btr_mo, \
    volatile Ullong_t *: Ullong_atomic_btr_mo)((ARR), (POS), (MO)))

#define atomic_btr(DST, ARG) \
    (atomic_btr_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Atomic bit set
void Uchar_atomic_bs_mo(volatile Uchar_t *, size_t, enum atomic_mo);
void Ushrt_atomic_bs_mo(volatile Ushrt_t *, size_t, enum atomic_mo);
void Uint_atomic_bs_mo(volatile Uint_t *, size_t, enum atomic_mo);
void Ulong_atomic_bs_mo(volatile Ulong_t *, size_t, enum atomic_mo);
void Ullong_atomic_bs_mo(volatile Ullong_t *, size_t, enum atomic_mo);

#define atomic_bs_mo(ARR, POS, MO) (_Generic((ARR), \
    volatile Uchar_t *: Uchar_atomic_bs_mo, \
    volatile Ushrt_t *: Ushrt_atomic_bs_mo, \
    volatile Uint_t *: Uint_atomic_bs_mo, \
    volatile Ulong_t *: Ulong_atomic_bs_mo, \
    volatile Ullong_t *: Ullong_atomic_bs_mo)((ARR), (POS), (MO)))

#define atomic_bs(DST, ARG) \
    (atomic_bs_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Atomic bit reset
void Uchar_atomic_br_mo(volatile Uchar_t *, size_t, enum atomic_mo);
void Ushrt_atomic_br_mo(volatile Ushrt_t *, size_t, enum atomic_mo);
void Uint_atomic_br_mo(volatile Uint_t *, size_t, enum atomic_mo);
void Ulong_atomic_br_mo(volatile Ulong_t *, size_t, enum atomic_mo);
void Ullong_atomic_br_mo(volatile Ullong_t *, size_t, enum atomic_mo);

#define atomic_br_mo(ARR, POS, MO) (_Generic((ARR), \
    volatile Uchar_t *: Uchar_atomic_br_mo, \
    volatile Ushrt_t *: Ushrt_atomic_br_mo, \
    volatile Uint_t *: Uint_atomic_br_mo, \
    volatile Ulong_t *: Ulong_atomic_br_mo, \
    volatile Ullong_t *: Ullong_atomic_br_mo)((ARR), (POS), (MO)))

#define atomic_br(DST, ARG) \
    (atomic_br_mo((DST), (ARG), ATOMIC_ACQ_REL))

// Spinlock
typedef Uint_t spinlock_base;
typedef volatile spinlock_base spinlock;
void spinlock_acquire(spinlock *);
#define spinlock_release(SPINLOCK) atomic_store((SPINLOCK), 0); 

// Double lock
typedef void *(*double_lock_callback)(void *);
void *double_lock_execute(spinlock *, double_lock_callback, double_lock_callback, void *, void *);

// Atomic increment/decrement
#define atomic_fetch_inc_mo(DST, MO) \
    (atomic_fetch_add_mo((DST), 1, (MO)))
#define atomic_fetch_inc(DST) \
    (atomic_fetch_inc_mo((DST), ATOMIC_ACQ_REL))

#define atomic_fetch_dec_mo(DST, MO) \
    (atomic_fetch_sub_mo((DST), 1, (MO)))
#define atomic_fetch_dec(DST) \
    (atomic_fetch_dec_mo((DST), ATOMIC_ACQ_REL))

// Hash functions
Uint_t Uint_uhash(Uint_t);
Ulong_t Ulong_uhash(Ulong_t);
Ullong_t Ullong_uhash(Ullong_t);

#define uhash(X) (_Generic((X), \
    Uint_t: Uint_uhash, \
    Ulong_t: Ulong_uhash, \
    Ullong_t: Ullong_uhash)(X))

Uint_t Uint_uhash_inv(Uint_t);
Ulong_t Ulong_uhash_inv(Ulong_t);
Ullong_t Ullong_uhash_inv(Ullong_t);

#define uhash_inv(X) (_Generic((X), \
    Uint_t: Uint_uhash_inv, \
    Ulong_t: Ulong_uhash_inv, \
    Ullong_t: Ullong_uhash_inv)(X))

size_t str_djb2a_hash(const char *);

// Base 2 integer logarithm
#define ulog2(X, CEIL) (bsr(X) + ((CEIL) && (X) && ((X) & ((X) - 1))))

// Base 10 integer logarithm
Uchar_t Uchar_ulog10(Uchar_t, bool);
Ushrt_t Ushrt_ulog10(Ushrt_t, bool);
Uint_t Uint_ulog10(Uint_t, bool);
Ulong_t Ulong_ulog10(Ulong_t, bool);
Ullong_t Ullong_ulog10(Ullong_t, bool);

#define ulog10(X, C) (_Generic((X), \
    Uchar_t: Uchar_ulog10, \
    Ushrt_t: Ushrt_ulog10, \
    Uint_t: Uint_ulog10, \
    Ulong_t: Ulong_ulog10, \
    Ullong_t: Ullong_ulog10)((X), (C)))

// Zero byte scan reverse for 128-bit SSE register 
Uint_t m128i_nbsr8(__m128i); // Returns 'UINT_MAX' if argument is without zero bytes

// Zero byte scan forward for 128-bit SSE register
Uint_t m128i_nbsf8(__m128i); // Returns 'UINT_MAX' if argument is without zero bytes

// Correct sign of 64-bit float
int fp64_sign(double);

// Byte shift (positive 'y' -- right; negative 'y' -- left; 'a' -- 'arithmetic' shift, which makes most significant byte 'sticky')
__m128i m128i_szz8(__m128i, int, bool);

// Byte rotate (positive 'y' -- right, negative 'y' -- left)
__m128i m128i_roz8(__m128i, int);
