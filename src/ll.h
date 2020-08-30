#pragma once

///////////////////////////////////////////////////////////////////////////////
//
//  Functions providing low level facilities
//

#include "common.h"

#include <immintrin.h>

#if defined __GNUC__ || defined __clang__
typedef int spinlock_base;
#elif defined _MSC_BUILD
typedef long spinlock_base;
#endif

typedef volatile spinlock_base spinlock;

#if defined __GNUC__ || defined __clang__ || defined _M_IX86 || defined __i386__
#   if defined _M_X64 || defined __x86_64__
typedef unsigned __int128 Dsize_t;
#   elif defined _M_IX86 || defined __i386__
typedef unsigned long long Dsize_t;
#   endif
#elif defined _MSC_BUILD
typedef struct { size_t s[2]; } Dsize_t;
#endif

#if defined _MSC_BUILD && defined _M_X64
#   define DSIZEC(LO, HO) ((Dsize_t) { (LO), (HI) })
#   define DSIZE_LO(D) (D.s[0])
#   define DSIZE_HI(D) (D.s[1])
#else
#   define DSIZEC(LO, HI) ((Dsize_t) (LO) | (((Dsize_t) (HI)) << SIZE_BIT))
#   define DSIZE_LO(D) ((size_t) (D))
#   define DSIZE_HI(D) ((size_t) ((D) >> SIZE_BIT))
#endif

// Rounding down/up to the nearest power of 2 for the inline usage 
#define RP2_SUPP(X, Y) ((X) | ((X) >> (Y)))
#define RP2_64(X) RP2_SUPP(RP2_SUPP(RP2_SUPP(RP2_SUPP(RP2_SUPP(RP2_SUPP(0ull | (X), 1), 2), 4), 8), 16), 32)
#define RDP2(X) ((X) && ((X) & ((X) - 1)) ? (RP2_64(X) >> 1) + 1 : (X))
#define RUP2(X) ((X) && ((X) & ((X) - 1)) ? RP2_64(X) + 1 : (X))

#define BIT_TEST2_MASK01 (0x5555555555555555ull & UINT8_MAX)
#define BIT_TEST2_MASK10 (BIT_TEST2_MASK01 << 1)
_Static_assert((BIT_TEST2_MASK01 | BIT_TEST2_MASK10) == UINT8_MAX, "Wrong constant provided!");

#define TYPE_CNT(BIT, TOT) ((BIT) / (TOT) + !!((BIT) % (TOT))) // Not always equals to ((BIT) + (TOT) - 1) / (TOT)
#define NIBBLE_CNT(NIBBLE) TYPE_CNT(NIBBLE, CHAR_BIT * sizeof(uint8_t) >> 1)
#define UINT8_CNT(BIT) TYPE_CNT(BIT, CHAR_BIT * sizeof(uint8_t))

#define SIZE_BIT (sizeof(size_t) * CHAR_BIT)
#define SIZE_CNT(BIT) TYPE_CNT(BIT, SIZE_BIT)
#define UINT32_BIT (sizeof(uint32_t) * CHAR_BIT)
#define UINT64_BIT (sizeof(uint64_t) * CHAR_BIT)

#define SIZE_PROD_TEST(PROD, ARG, CNT) (size_prod_test((PROD), (ARG), (CNT)) == (CNT))
#define SIZE_PROD_TEST_VA(PROD, ...) (size_prod_test((PROD), ARG(size_t, __VA_ARGS__)) == countof(((size_t []) { __VA_ARGS__ })))

bool bool_load_acquire(volatile void *);
uint8_t uint8_load_acquire(volatile void *);
uint16_t uint16_load_acquire(volatile void *);
uint32_t uint32_load_acquire(volatile void *);
size_t size_load_acquire(volatile void *);
void *ptr_load_acquire(volatile void *);
void bool_store_release(volatile void *, bool);
void uint8_store_release(volatile void *, uint8_t);
void uint16_store_release(volatile void *, uint16_t);
void uint32_store_release(volatile void *, uint32_t);
void size_store_release(volatile void *, size_t);
void ptr_store_release(volatile void*, void *);

uint8_t uint8_interlocked_or(volatile void *, uint8_t);
uint16_t uint16_interlocked_or(volatile void *, uint16_t);
uint8_t uint8_interlocked_and(volatile void *, uint8_t);
uint16_t uint16_interlocked_and(volatile void *, uint16_t);
bool size_interlocked_compare_exchange(volatile void *, size_t *, size_t);
bool ptr_interlocked_compare_exchange(volatile void *, void **, void *);
bool Dsize_interlocked_compare_exchange(volatile void *, Dsize_t *, Dsize_t);
void *ptr_interlocked_exchange(volatile void *, void *);

size_t size_interlocked_inc(volatile void *);
size_t size_interlocked_dec(volatile void *);
size_t size_interlocked_add(volatile void *, size_t);
size_t size_interlocked_sub(volatile void *, size_t);
size_t size_interlocked_add_sat(volatile void *, size_t);
size_t size_interlocked_sub_sat(volatile void *, size_t);

size_t size_shl(size_t *, size_t, uint8_t);
size_t size_shr(size_t *, size_t, uint8_t);
size_t size_add(size_t *, size_t, size_t);
size_t size_sub(size_t *, size_t, size_t);
size_t size_sum(size_t *, size_t *, size_t);
size_t size_prod_test(size_t *, size_t *, size_t);
size_t size_mul(size_t *, size_t, size_t);

uint32_t uint32_bit_scan_reverse(uint32_t);
uint32_t uint32_bit_scan_forward(uint32_t);
uint32_t uint32_pop_cnt(uint32_t);
size_t size_bit_scan_reverse(size_t);
size_t size_bit_scan_forward(size_t);
size_t size_pop_cnt(size_t);

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
void spinlock_release(spinlock *);

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
void uint8_interlocked_bit_set_release(volatile void *, size_t);
void uint8_interlocked_bit_reset_release(volatile void *, size_t);
bool uint8_interlocked_bit_test_set(volatile void *, size_t);
bool uint8_interlocked_bit_test_reset(volatile void *, size_t);

bool size_bit_test(size_t *, size_t);
bool size_bit_test_set(size_t *, size_t);
bool size_bit_test_reset(size_t *, size_t);
void size_bit_set(size_t *, size_t);
void size_bit_reset(size_t *, size_t);

uint8_t uint8_rol(uint8_t, uint8_t);
uint16_t uint16_rol(uint16_t, uint16_t);
uint32_t uint32_rol(uint32_t, uint32_t);
uint64_t uint64_rol(uint64_t, uint64_t);
