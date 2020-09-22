#pragma once

///////////////////////////////////////////////////////////////////////////////
//
//  Functions providing low level facilities
//

#include "common.h"

#include <immintrin.h>

#if defined __GNUC__ || defined __clang__
typedef unsigned spinlock_base;
#elif defined _MSC_BUILD
typedef unsigned long spinlock_base;
#endif

typedef volatile spinlock_base spinlock;

#if defined __GNUC__ || defined __clang__ || defined _M_IX86 || defined __i386__
#   if defined _M_X64 || defined __x86_64__
GPUSH GWRN(pedantic) 
typedef unsigned __int128 Dsize_t; 
GPOP
#   elif defined _M_IX86 || defined __i386__
typedef unsigned long long Dsize_t;
#   endif
#elif defined _MSC_BUILD
typedef struct { size_t s[2]; } Dsize_t;
#endif

#if defined _MSC_BUILD && defined _M_X64
#   define DSIZELC(LO) ((Dsize_t) { .s[0] = (LO) })
#   define DSIZEHC(HI) ((Dsize_t) { .s[1] = (HI) })
#   define DSIZEC(LO, HI) ((Dsize_t) { .s[0] = (LO), .s[1] = (HI) })
#   define DSIZE_LO(D) ((D).s[0])
#   define DSIZE_HI(D) ((D).s[1])
#else
#   define DSIZELC(LO) ((Dsize_t) (LO))
#   define DSIZEHC(HI) ((((Dsize_t) (HI)) << SIZE_BIT))
#   define DSIZEC(LO, HI) (DSIZELC(LO) | DSIZEHC(HI))
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

#include <tgmath.h>

#if defined __GNUC__ || defined __clang__

#	if (defined __GNUC__ && defined __x86_64__) || (defined __clang__ && defined __i386__)
//		For some reason gcc does not emit 'cmpxchg16b' (even when '-mcx16' is passed to it) via '__atomic_compare_exchange_n' intrinsic.
//		See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=84522 for more details.
//		The same applies for the 'cmpxchg8b' under i386 clang.
//		Thus the different method is used
bool Dsize_interlocked_compare_exchange(volatile Dsize_t *, Dsize_t *, Dsize_t);
#		define CASE_DSIZE_INTERLOCKED_COMPARE_EXCHANGE \
			volatile Dsize_t *: Dsize_interlocked_compare_exchange((DST), (P_CMP), (XCHG)),
#	else
#		define CASE_DSIZE_INTERLOCKED_COMPARE_EXCHANGE
#	endif

#	define load_acquire(SRC) \
		(__atomic_load_n((SRC), __ATOMIC_ACQUIRE))
#	define store_release(DST, VAL) \
		(__atomic_store_n((DST), (VAL), __ATOMIC_RELEASE))
#	define interlocked_compare_exchange(DST, P_CMP, XCHG) (_Generic(DST, \
		CASE_DSIZE_INTERLOCKED_COMPARE_EXCHANGE \
		default: __atomic_compare_exchange_n((DST), (P_CMP), (XCHG), 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)))
#	define interlocked_compare_exchange_acquire(DST, P_CMP, XCHG) \
		(__atomic_compare_exchange_n((DST), (P_CMP), (XCHG), 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
#	define interlocked_and(DST, ARG) \
		(__atomic_fetch_and((DST), (ARG), __ATOMIC_ACQ_REL))
#	define interlocked_and_release(DST, ARG) \
		(__atomic_fetch_and((DST), (ARG), __ATOMIC_RELEASE))
#	define interlocked_or(DST, ARG) \
		(__atomic_fetch_or((DST), (ARG), __ATOMIC_ACQ_REL))
#	define interlocked_or_release(DST, ARG) \
		(__atomic_fetch_or((DST), (ARG), __ATOMIC_RELEASE))
#	define interlocked_add(DST, ARG) \
		(__atomic_fetch_add((DST), (ARG), __ATOMIC_ACQ_REL))
#	define interlocked_sub(DST, ARG) \
		(__atomic_fetch_sub((DST), (ARG), __ATOMIC_ACQ_REL))
#	define interlocked_inc(DST, ARG) \
		(interlocked_add((DST), 1))
#	define interlocked_dec(DST, ARG) \
		(interlocked_sub((DST), 1))
#	define interlocked_exchange(DST, ARG) \
		(__atomic_exchange_n((DST), (ARG), __ATOMIC_ACQ_REL))

#	define bit_scan_reverse(x) (_Generic((x), \
        unsigned char: ussint_bit_scan_reverse(x), \ 
		unsigned: x ? sizeof(x) * CHAR_BIT - (unsigned) __builtin_clz((unsigned) x) - 1 : UINT_MAX, \
        unsigned long: x ? sizeof(x) * CHAR_BIT - (unsigned long) __builtin_clzl((unsigned long) x) - 1 : ULONG_MAX, \
        unsigned long long: x ? sizeof(x) * CHAR_BIT - (unsigned long long) __builtin_clzll((unsigned long long) x) - 1 : ULLONG_MAX))
#	define bit_scan_forward(x) (_Generic((x), \
		unsigned char: ussint_bit_scan_forward(x), \
        unsigned: x ? (unsigned) __builtin_ctz((unsigned) x) : UINT_MAX, \
        unsigned long: x ? (unsigned long) __builtin_ctzl((unsigned long) x) : ULONG_MAX, \
        unsigned long long: x ? (unsigned long long) __builtin_ctzll((unsigned long long) x) : ULLONG_MAX))
#	define pop_cnt(x) (_Generic((x), \
        unsigned: (unsigned) __builtin_popcount((unsigned) x), \
        unsigned long: (unsigned long) __builtin_popcountl((unsigned long) x), \
        unsigned long long: (unsigned long long) __builtin_popcountll((unsigned long long) x)))

#elif defined _MSC_BUILD
#   include <intrin.h>

bool ulint_interlocked_compare_exchange(volatile unsigned long *, unsigned long *, unsigned long);
bool ullint_interlocked_compare_exchange(volatile unsigned long long *, unsigned long long *, unsigned long long);
bool ptr_interlocked_compare_exchange(void *volatile *, void **, void *);

unsigned long ulint_bit_scan_reverse(unsigned long);
unsigned long ulint_bit_scan_forward(unsigned long);

#   ifdef _M_X64
#		define CASE_DSIZE_INTERLOCKED_COMPARE_EXCHANGE \
			volatile Dsize_t *: (_InterlockedCompareExchange128((DST), DSIZE_HI(XCHG), DSIZE_LO(XCHG), (long long *) (P_CMP)->s)),
unsigned long long ullint_bit_scan_reverse(unsigned long long);
#		define CASE_ULLINT_BIT_SCAN_REVERSE \
			unsigned long long: ullint_bit_scan_reverse(x),
unsigned long long ullint_bit_scan_forward(unsigned long long);
#		define CASE_ULLINT_BIT_SCAN_FORWARD \
			unsigned long long: ullint_bit_scan_forward(x),
#		define CASE_ULLINT_POP_CNT \
			unsigned long long: __popcnt64(x),
#	else
#		define CASE_DSIZE_INTERLOCKED_COMPARE_EXCHANGE
#		define CASE_ULLINT_BIT_SCAN_REVERSE
#		define CASE_ULLINT_BIT_SCAN_FORWARD
#		define CASE_ULLINT_POP_CNT 
#	endif

#	define VOLATILE_FIXED(PTR, ...) (_Generic((PTR), \
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
#	define load_acquire(SRC) \
		VOLATILE_FIXED((SRC), *(SRC))
#	define store_release(DST, VAL) \
		VOLATILE_FIXED((DST), *(DST) = (VAL))
#	define interlocked_compare_exchange(DST, P_CMP, XCHG) (_Generic((DST), \
		CASE_DSIZE_INTERLOCKED_COMPARE_EXCHANGE \
		volatile unsigned *: ulint_interlocked_compare_exchange((volatile unsigned long *) (DST), (unsigned long *) (P_CMP), (unsigned long) (XCHG)), \
		volatile unsigned long *: ulint_interlocked_compare_exchange((DST), (P_CMP), (XCHG)), \
		volatile unsigned long long *: ullint_interlocked_compare_exchange((DST), (P_CMP), (XCHG)),\
		void *volatile *: ptr_interlocked_compare_exchange((DST), (P_CMP), (XCHG))))
#	define interlocked_compare_exchange_acquire(DST, P_CMP, XCHG) \
		(interlocked_compare_exchange((DST), (P_CMP), (XCHG)))
#	define interlocked_and_release(DST, ARG) (_Generic((DST), \
		(interlocked_and(DST, ARG))
#	define interlocked_or_release(DST, ARG) \
		(__atomic_fetch_and((DST), (ARG), __ATOMIC_RELEASE))
#	define interlocked_and(DST, ARG) \
		(__atomic_fetch_and((DST), (ARG), __ATOMIC_ACQ_REL))
#	define interlocked_or(DST, ARG) \
		(__atomic_fetch_and((DST), (ARG), __ATOMIC_ACQ_REL))
#	define interlocked_add(DST, ARG) \
		(__atomic_fetch_add((DST), (ARG), __ATOMIC_ACQ_REL))
#	define interlocked_sub(DST, ARG) \
		(__atomic_fetch_add((DST), (ARG), __ATOMIC_ACQ_REL))
#	define interlocked_inc(DST, ARG) \
		(interlocked_add((DST), 1))
#	define interlocked_dec(DST, ARG) \
		(interlocked_sub((DST), 1))
#	define interlocked_exchange(DST, ARG) \
		(__atomic_exchange_n((DST), (ARG), __ATOMIC_ACQ_REL))

#	define bit_scan_reverse(x) (_Generic((x), \
		CASE_ULLINT_BIT_SCAN_REVERSE \
        unsigned char: ussint_bit_scan_reverse(x), \
		unsigned: (unsigned) ulint_bit_scan_reverse((unsigned long) (x)), \
		unsigned long: ulint_bit_scan_reverse(x)))
#	define bit_scan_forward(x) (_Generic((x), \
		CASE_ULLINT_BIT_SCAN_FORWARD \
		unsigned char: ussint_bit_scan_forward(x), \
		unsigned: (unsigned) ulint_bit_scan_forward((unsigned long) (x)), \
		unsigned long: ulint_bit_scan_forward(x), \
		unsigned long long: ullint_bit_scan_forward(x)))
#	define pop_cnt(x) (_Generic((x), \
		CASE_ULLINT_POP_CNT \
		unsigned short: __popcnt16(x), \
        unsigned: __popcnt(x), \
        unsigned long: (unsigned long) __popcnt((unsigned) x)))
#endif

unsigned char ussint_bit_scan_reverse(unsigned char);
unsigned char ussint_bit_scan_forward(unsigned char);


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
void *ptr_interlocked_exchange(volatile void *, void *);

size_t size_interlocked_add(volatile void *, size_t);
size_t size_interlocked_sub(volatile void *, size_t);
size_t size_interlocked_inc(volatile void *);
size_t size_interlocked_dec(volatile void *);
size_t size_interlocked_add_sat(volatile void *, size_t);
size_t size_interlocked_sub_sat(volatile void *, size_t);

bool size_interlocked_compare_exchange(volatile void *, size_t *, size_t);
bool ptr_interlocked_compare_exchange(volatile void *, void **, void *);

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
