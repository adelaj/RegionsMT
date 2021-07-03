#include "ll.h"

#include <stdint.h>
#include <limits.h>
#include <errno.h>

#if TEST(IF_MSVC)
#   include <intrin.h>
#endif

#define DECL_1(TYPE, PREFIX, SUFFIX, BACKEND) \
    TYPE PREFIX ## _ ## SUFFIX(TYPE x) \
    { \
        return BACKEND(x); \
    }

#define DECL_1_EXT(TYPE, PREFIX, SUFFIX, OP_TYPE, WIDE, NARR) \
    TYPE PREFIX ## _ ## SUFFIX(TYPE x) \
    { \
        _Static_assert((WIDE) * sizeof(OP_TYPE) == (NARR) * sizeof(TYPE), ""); \
        return (TYPE) SUFFIX((OP_TYPE) x); \
    }

#define DECL_1_COPY(TYPE, PREFIX, SUFFIX, OP_TYPE) \
    DECL_1_EXT(TYPE, PREFIX, SUFFIX, OP_TYPE, 1, 1)

#define DECL_1_NARR(TYPE, PREFIX, SUFFIX) \
    DECL_1_EXT(TYPE, PREFIX, SUFFIX, D ## PREFIX ## _t, 1, 2)

#define DECL_2(TYPE, PREFIX, SUFFIX, ARG_TYPE, BACKEND) \
    TYPE PREFIX ## _ ## SUFFIX(TYPE x, ARG_TYPE y) \
    { \
        return BACKEND(x, y); \
    }

#define DECL_2_COPY(TYPE, PREFIX, SUFFIX, ARG_TYPE, OP_TYPE) \
    TYPE PREFIX ## _ ## SUFFIX(TYPE x, ARG_TYPE y) \
    { \
        _Static_assert(sizeof(OP_TYPE) == sizeof(TYPE), ""); \
        return (TYPE) SUFFIX((OP_TYPE) x, y); \
    }

#define DECL_WIDE_NARR(TYPE, PREFIX) \
    D ## PREFIX ## _t PREFIX ## _wide(TYPE lo, TYPE hi) \
    { \
        _Static_assert(sizeof(D ## PREFIX ## _t) == 2 * sizeof(TYPE), ""); \
        return (D ## PREFIX ## _t) (lo) | ((D ## PREFIX ## _t) (hi) << (bitsof(TYPE) >> 1)); \
    }

#define DECL_LO_WIDE(TYPE, PREFIX) \
    H ## PREFIX ## _t PREFIX ## _lo(TYPE x) \
    { \
        _Static_assert(2 * sizeof(H ## PREFIX ## _t) == sizeof(TYPE), ""); \
        return (H ## PREFIX ## _t) x; \
    }

#define DECL_HI_WIDE(TYPE, PREFIX) \
    H ## PREFIX ## _t PREFIX ## _hi(TYPE x) \
    { \
        _Static_assert(2 * sizeof(H ## PREFIX ## _t) == sizeof(TYPE), ""); \
        return (H ## PREFIX ## _t) (x >> (bitsof(H ## PREFIX ## _t))); \
    }

// Define redirected operation for similar types
#define DECL_OP_COPY(TYPE, PREFIX, SUFFIX, HI_TYPE, OP_TYPE, OP_HI_TYPE) \
    TYPE PREFIX ## _ ## SUFFIX(HI_TYPE *p_hi, TYPE x, TYPE y) \
    { \
        _Static_assert(sizeof(OP_TYPE) == sizeof(TYPE) && sizeof(OP_HI_TYPE) == sizeof(HI_TYPE), ""); \
        return (TYPE) SUFFIX((OP_HI_TYPE *) p_hi, (OP_TYPE) x, (OP_TYPE) y); \
    }

// Define operations with wide integers
#define DECL_OP_WIDE(TYPE, PREFIX, SUFFIX) \
    TYPE PREFIX ## _ ## SUFFIX(unsigned char *p_hi, TYPE x, TYPE y) \
    { \
        _Static_assert(2 * sizeof(H ## PREFIX ## _t) == sizeof(TYPE), ""); \
        H ## PREFIX ## _t lo = SUFFIX(p_hi, lo(x), lo(y)), hi = SUFFIX(p_hi, hi(x), hi(y)); \
        return wide(lo, hi); \
    }

// Define operations via intel intrinsics such as '_addcarry_u32' and '_subborrow_u32'
#define DECL_OP_INTEL(TYPE, PREFIX, SUFFIX, BACKEND) \
    TYPE PREFIX ## _ ## SUFFIX(unsigned char *p_hi, TYPE x, TYPE y) \
    { \
        TYPE res; \
        *p_hi = BACKEND(*p_hi, x, y, &res); \
        return res; \
    }

// Define operations via GCC intrinsics such as '__builtin_add_overflow' or '__builtin_sub_overflow'
#define DECL_OP_GCC(TYPE, PREFIX, SUFFIX, BACKEND) \
    TYPE PREFIX ## _ ## SUFFIX(unsigned char *p_hi, TYPE x, TYPE y) \
    { \
        TYPE res; \
        unsigned char hi = BACKEND(x, y, &res); \
        *p_hi = hi + BACKEND(res, *p_hi, &res); \
        return res; \
    }

#define DECL_MUL_NARR(TYPE, PREFIX) \
    TYPE PREFIX ## _mul(TYPE *p_hi, TYPE x, TYPE y) \
    { \
        _Static_assert(sizeof(D ## PREFIX ## _t) == 2 * sizeof(TYPE), ""); \
        D ## PREFIX ## _t res = (D ## PREFIX ## _t) x * (D ## PREFIX ## _t) y; \
        *p_hi = hi(res); \
        return lo(res); \
    }

#define DECL_MUL_WIDE(TYPE, PREFIX) \
    TYPE PREFIX ## _mul(TYPE *p_hi, TYPE x, TYPE y) \
    { \
        _Static_assert(2 * sizeof(H ## PREFIX ## _t) == sizeof(TYPE), ""); \
        unsigned char car1 = 0, car2 = 0; /* Warning! 'car1' and 'car2' are never set simultaneously */ \
        H ## PREFIX ## _t \
            xl = lo(x), xh = hi(x), \
            yl = lo(y), yh = hi(y), \
            llh, lll = mul(&llh, xl, yl), hlh, lhh, hhh, \
            l = add(&car2, add(&car1, llh, mul(&lhh, xl, yh)), mul(&hlh, xh, yl)), \
            h = add(&car2, add(&car1, mul(&hhh, xh, yh), lhh), hlh); \
        *p_hi = wide(h, hhh + car1 + car2); \
        return wide(lll, l); \
    }

// Computes 'x * y + z'
#define DECL_MA(TYPE, PREFIX) \
    TYPE PREFIX ## _ma(TYPE *p_hi, TYPE x, TYPE y, TYPE z) \
    { \
        TYPE hi, r = mul(&hi, x, y); \
        unsigned char car = 0; \
        r = add(&car, r, z); \
        *p_hi = hi + car; /* Never overflows! */ \
        return r; \
    }

#define DECL_SHL_NARR(TYPE, PREFIX) \
    TYPE PREFIX ## _shl(TYPE *p_hi, TYPE x, unsigned char y) \
    { \
        _Static_assert(sizeof(D ## PREFIX ## _t) == 2 * sizeof(TYPE), ""); \
        D ## PREFIX ## _t val = (D ## PREFIX ## _t) x << (D ## PREFIX ## _t) y % bitsof(TYPE); \
        *p_hi = hi(val); \
        return lo(val); \
    }

#define DECL_SHR_NARR(TYPE, PREFIX) \
    TYPE PREFIX ## _shr(TYPE *p_lo, TYPE x, unsigned char y) \
    { \
        _Static_assert(sizeof(D ## PREFIX ## _t) == 2 * sizeof(TYPE), ""); \
        D ## PREFIX ## _t val = (D ## PREFIX ## _t) x >> (D ## PREFIX ## _t) y % bitsof(TYPE); \
        *p_lo = lo(val); \
        return hi(val); \
    }

#define DECL_SHZ_SSE(TYPE, PREFIX, SUFFIX, SHL, SHR) \
    TYPE PREFIX ## _ ## SUFFIX(TYPE *p_hl, TYPE x, unsigned char y) \
    { \
        _Static_assert(2 * sizeof(TYPE) == sizeof(__m128i), ""); \
        _Static_assert(sizeof(TYPE) == sizeof(long long), ""); \
        TYPE res; \
        __m128i x0 = _mm_set_epi64x(0ll, (long long) x); \
        _mm_storeu_si64(&res, SHL(x0, y %= bitsof(TYPE))); \
        _mm_storeu_si64(p_hl, SHR(x0, bitsof(TYPE) - y)); /* Warning! SSE shift operations yield correct result even when y = 0! */ \
        return res; \
    }

#define DECL_ROZ_GCC(TYPE, PREFIX, SUFFIX, SHL, SHR) \
    TYPE PREFIX ## _ ## SUFFIX(TYPE x, unsigned char y) \
    { \
        return (x SHL y % bitsof(TYPE)) | (x SHR (0 - y) % bitsof(TYPE)); \
    }

#define DECL_ADD_SAT(TYPE, PREFIX) \
    TYPE PREFIX ## _add_sat(TYPE x, TYPE y) \
    { \
        return test_add(&x, y) ? x : umax(x); \
    }

#define DECL_SUB_SAT(TYPE, PREFIX) \
    TYPE PREFIX ## _sub_sat(TYPE x, TYPE y) \
    { \
        return test_sub(&x, y) ? x : 0; \
    }

#define DECL_USIGN(TYPE, PREFIX) \
    int PREFIX ## _usign(TYPE x, TYPE y) \
    { \
        return test_sub(&x, y) ? !!x : -1; \
    }

#define DECL_UDIST(TYPE, PREFIX) \
    TYPE PREFIX ## _udist(TYPE x, TYPE y) \
    { \
        unsigned char bor = 0; \
        TYPE diff = sub(&bor, x, y); \
        return bor ? 0 - diff : diff; \
    }

#define DECL_TEST_OP(TYPE, PREFIX, SUFFIX, HI_TYPE) \
    bool PREFIX ## _test_ ## SUFFIX(TYPE *p_res, TYPE x) \
    { \
        HI_TYPE hi = 0; \
        TYPE res = SUFFIX(&hi, *p_res, x); \
        if (hi) return 0; \
        *p_res = res; /* On failure '*p_res' is untouched */ \
        return 1; \
    }

#define DECL_TEST_MUL(TYPE, PREFIX) \
    DECL_TEST_OP(TYPE, PREFIX, mul, TYPE)

#define DECL_TEST_OP_GCC(TYPE, PREFIX, SUFFIX, BACKEND) \
    bool PREFIX ## _test_ ## SUFFIX(TYPE *p_res, TYPE x) \
    { \
        TYPE res; \
        if (BACKEND(*p_res, x, &res)) return 0; \
        *p_res = res; /* On failure '*p_res' is untouched */ \
        return 1; \
    }

#define DECL_TEST_MA(TYPE, PREFIX) \
    bool PREFIX ## _test_ma(TYPE *p_res, TYPE x, TYPE y) \
    { \
        if (!test_mul(&x, *p_res) || !test_add(&x, y)) return 0; \
        *p_res = x; /* On failure '*p_res' is untouched */ \
        return 1; \
    }

#define DECL_TEST_REP(TYPE, PREFIX, SUFFIX, ZERO, OP_SUFFIX) \
    size_t PREFIX ## _test_ ## SUFFIX(TYPE *p_res, TYPE *argl, size_t cnt) \
    { \
        if (!cnt) return (*p_res = (ZERO), 0); \
        TYPE res = argl[0]; \
        for (size_t i = 1; i < cnt; i++) if (!(test_ ## OP_SUFFIX(&res, argl[i]))) return i; \
        *p_res = res; \
        return cnt; \
    }

#define DECL_BSR_WIDE(TYPE, PREFIX) \
    TYPE PREFIX ## _bsr(TYPE x) \
    { \
        _Static_assert(2 * sizeof(H ## PREFIX ## _t) == sizeof(TYPE), ""); \
        H ## PREFIX ## _t lo = lo(x), hi = hi(x); \
        return hi ? (TYPE) bsr(hi) + bitsof(H ## PREFIX ## _t) : lo ? (TYPE) bsr(lo) : umax(x); \
    }

#define DECL_BSR_GCC(TYPE, PREFIX, BACKEND) \
    TYPE PREFIX ## _bsr(TYPE x) \
    { \
        return x ? bitsof(TYPE) - (TYPE) BACKEND(x) - 1 : umax(x); \
    }

#define DECL_BSF_WIDE(TYPE, PREFIX) \
    TYPE PREFIX ## _bsf(TYPE x) \
    { \
        _Static_assert(2 * sizeof(H ## PREFIX ## _t) == sizeof(TYPE), ""); \
        H ## PREFIX ## _t lo = lo(x), hi = hi(x); \
        return lo ? (TYPE) bsf(lo) : hi ? (TYPE) bsf(hi) + bitsof(H ## PREFIX ## _t) : umax(x); \
    }

#define DECL_BSF_GCC(TYPE, PREFIX, BACKEND) \
    TYPE PREFIX ## _bsf(TYPE x) \
    { \
        return  x ? (TYPE) BACKEND(x) : umax(x); \
    }

#define DECL_BSZ_MSVC(TYPE, PREFIX, SUFFIX, BACKEND) \
    TYPE PREFIX ## _ ## SUFFIX(TYPE x) \
    { \
        unsigned long res; \
        return BACKEND(&res, x) ? (TYPE) res : umax(x); \
    }

#define DECL_PCNT_WIDE(TYPE, PREFIX) \
    TYPE PREFIX ## _pcnt(TYPE x) \
    { \
        _Static_assert(2 * sizeof(H ## PREFIX ## _t) == sizeof(TYPE), ""); \
        return (TYPE) pcnt(lo(x)) + (TYPE) pcnt(hi(x)); \
    }

#define DECL_BT(TYPE, PREFIX) \
    bool PREFIX ## _bt(TYPE *arr, size_t bit) \
    { \
        return arr[bit / bitsof(TYPE)] & ((TYPE) 1 << bit % bitsof(TYPE)); \
    }

#define DECL_BTS(TYPE, PREFIX) \
    bool PREFIX ## _bts(TYPE *arr, size_t bit) \
    { \
        size_t div = bit / bitsof(TYPE); \
        TYPE msk = (TYPE) 1 << bit % bitsof(TYPE); \
        if (arr[div] & msk) return 1; \
        arr[div] |= msk; \
        return 0; \
    }

#define DECL_BTR(TYPE, PREFIX) \
    bool PREFIX ## _btr(TYPE *arr, size_t bit) \
    { \
        size_t div = bit / bitsof(TYPE); \
        TYPE msk = (TYPE) 1 << bit % bitsof(TYPE); \
        if (!(arr[div] & msk)) return 0; \
        arr[div] &= ~msk; \
        return 1; \
    }

#define DECL_BS(TYPE, PREFIX) \
    void PREFIX ## _bs(TYPE *arr, size_t bit) \
    { \
        arr[bit / bitsof(TYPE)] |= (TYPE) 1 << bit % bitsof(TYPE); \
    }

#define DECL_BR(TYPE, PREFIX) \
    void PREFIX ## _br(TYPE *arr, size_t bit) \
    { \
        arr[bit / bitsof(TYPE)] &= ~((TYPE) 1 << bit % bitsof(TYPE)); \
    }

#define DECL_BT_BURST(TYPE, PREFIX) \
    TYPE PREFIX ## _bt_burst(TYPE *arr, size_t pos, size_t stride) \
    { \
        return (arr[pos / (bitsof(TYPE) >> (stride - 1))] >> (pos % (bitsof(TYPE) >> (stride - 1)) << (stride - 1))) & (((TYPE) 1 << stride) - 1); \
    }

#define DECL_BTS_BURST(TYPE, PREFIX) \
    TYPE PREFIX ## _bts_burst(TYPE *arr, size_t pos, size_t stride, TYPE msk) \
    { \
        size_t div = pos / (bitsof(TYPE) >> (stride - 1)), rem = pos % (bitsof(TYPE) >> (stride - 1)) << (stride - 1); \
        TYPE res = (arr[div] >> rem) & msk; \
        if (res != msk) arr[div] |= msk << rem; \
        return res; \
    }

#define DECL_BTR_BURST(TYPE, PREFIX) \
    TYPE PREFIX ## _btr_burst(TYPE *arr, size_t pos, size_t stride, TYPE msk) \
    { \
        size_t div = pos / (bitsof(TYPE) >> (stride - 1)), rem = pos % (bitsof(TYPE) >> (stride - 1)) << (stride - 1); \
        TYPE res = (arr[div] >> rem) & msk; \
        if (res) arr[div] &= ~(msk << rem); \
        return res; \
    }

#define DECL_BS_BURST(TYPE, PREFIX) \
    void PREFIX ## _bs_burst(TYPE *arr, size_t pos, size_t stride, TYPE msk) \
    { \
        arr[pos / (bitsof(TYPE) >> (stride - 1))] |= msk << (pos % (bitsof(TYPE) >> (stride - 1)) << (stride - 1)); \
    }

#define DECL_BR_BURST(TYPE, PREFIX) \
    void PREFIX ## _br_burst(TYPE *arr, size_t pos, size_t stride, TYPE msk) \
    { \
        arr[pos / (bitsof(TYPE) >> (stride - 1))] &= ~(msk << (pos % (bitsof(TYPE) >> (stride - 1)) << (stride - 1))); \
    }

#define DECL_ATOMIC_LOAD_GCC(TYPE, PREFIX) \
    TYPE PREFIX ## _atomic_load_mo(TYPE volatile *src, enum atomic_mo mo) \
    { \
        return __atomic_load_n(src, mo); \
    }
    
#define DECL_ATOMIC_LOAD_MSVC(TYPE, PREFIX) \
    TYPE PREFIX ## _atomic_load_mo(TYPE volatile *src, enum atomic_mo mo) \
    { \
        (void) mo; \
        return *src; \
    }

#define DECL_ATOMIC_LOAD_SSE(TYPE, PREFIX) \
    TYPE PREFIX ## _atomic_load_mo(TYPE volatile *src, enum atomic_mo mo) \
    { \
        (void) mo; \
        _Static_assert(2 * sizeof(TYPE) == sizeof(__m128i), ""); \
        _Static_assert(sizeof(TYPE) == sizeof(long long), ""); \
        volatile unsigned long long res; /* Do not remove 'volatile'! */ \
        _mm_storeu_si64((void *) &res, _mm_loadu_si64((__m128i *) src)); \
        return res; \
    }

#define DECL_ATOMIC_STORE_GCC(TYPE, PREFIX) \
    void PREFIX ## _atomic_store_mo(TYPE volatile *src, TYPE val, enum atomic_mo mo) \
    { \
        __atomic_store_n(src, val, mo); \
    }

#define DECL_ATOMIC_STORE_MSVC(TYPE, PREFIX) \
    void PREFIX ## _atomic_store_mo(TYPE volatile *src, TYPE val, enum atomic_mo mo) \
    { \
        (void) mo; \
        *src = val; \
    }

#define DECL_ATOMIC_STORE_SSE(TYPE, PREFIX) \
    void PREFIX ## _atomic_store_mo(TYPE volatile *src, TYPE val, enum atomic_mo mo) \
    { \
        (void) mo; \
        _Static_assert(2 * sizeof(TYPE) == sizeof(__m128i), ""); \
        _Static_assert(sizeof(TYPE) == sizeof(long long), ""); \
        _mm_storeu_si64((void *) src, _mm_set_epi64x(0, (long long) val)); /* '_mm_cvtsi64_si128' is not univerally supported under 'i386' */ \
    }

#define DECL_ATOMIC_CMP_XCHG_GCC(TYPE, PREFIX) \
    bool PREFIX ## _atomic_cmp_xchg_mo(TYPE volatile *dst, TYPE *p_cmp, TYPE xchg, bool weak, enum atomic_mo mo_succ, enum atomic_mo mo_fail) \
    { \
        return __atomic_compare_exchange_n((TYPE volatile *) dst, p_cmp, xchg, weak, mo_succ, mo_fail); \
    }

#define DECL_ATOMIC_CMP_XCHG_COPY(TYPE, PREFIX, OP_TYPE) \
    bool PREFIX ## _atomic_cmp_xchg_mo(TYPE volatile *dst, TYPE *p_cmp, TYPE xchg, bool weak, enum atomic_mo mo_succ, enum atomic_mo mo_fail) \
    { \
        _Static_assert(sizeof(OP_TYPE) == sizeof(TYPE), ""); \
        return atomic_cmp_xchg_mo((OP_TYPE volatile *) dst, (OP_TYPE *) p_cmp, (OP_TYPE) xchg, weak, mo_succ, mo_fail); \
    }

// This should be used to force compiler to emit 'cmpxchg16b'/'cmpxchg8b' instuctions when:
// 1) gcc is targeted to 'x86_64', even if '-mcx16' flag set (see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=84522);
// 2) clang is targeted to 'i386';
// 3) MSVC is used (see also https://stackoverflow.com/questions/41572808/interlockedcompareexchange-optimization).
#define DECL_ATOMIC_CMP_XCHG(TYPE, PREFIX, BACKEND_TYPE, BACKEND) \
    bool PREFIX ## _atomic_cmp_xchg_mo(TYPE volatile *dst, TYPE *p_cmp, TYPE xchg, bool weak, enum atomic_mo mo_succ, enum atomic_mo mo_fail) \
    { \
        (void) weak, (void) mo_succ, (void) mo_fail; \
        _Static_assert(sizeof(BACKEND_TYPE) == sizeof(TYPE), ""); \
        TYPE cmp = *p_cmp; /* Warning! 'TYPE' may be a pointer type: do not fuse this line with the next one! */ \
        TYPE res = (TYPE) BACKEND((BACKEND_TYPE volatile *) dst, (BACKEND_TYPE) xchg, (BACKEND_TYPE) cmp); \
        if (res == cmp) return 1; \
        *p_cmp = res; \
        return 0; \
    }

#define DECL_ATOMIC_OP_COPY(TYPE, PREFIX, SUFFIX, RET_TYPE, ARG_TYPE, OP_TYPE) \
    RET_TYPE PREFIX ## _atomic_ ## SUFFIX ## _mo(TYPE volatile *dst, ARG_TYPE arg, enum atomic_mo mo) \
    { \
        _Static_assert(sizeof(OP_TYPE) == sizeof(TYPE), ""); \
        return (RET_TYPE) atomic_ ## SUFFIX ## _mo((OP_TYPE volatile *) dst, (OP_TYPE) arg, mo); \
    }

#define DECL_ATOMIC_OP_WIDE(TYPE, PREFIX, SUFFIX, OP, ...) \
    TYPE PREFIX ## _atomic_ ## SUFFIX ## _mo(TYPE volatile *mem, TYPE val, enum atomic_mo mo) \
    { \
        TYPE probe = *mem; /* Warning! Non-atomic load is legitimate here! */ \
        while (!atomic_cmp_xchg_mo(mem, &probe, (TYPE) OP(probe __VA_ARGS__ val), 1, mo, ATOMIC_RELAXED)) probe = *mem; \
        return probe; \
    }

#define DECL_ATOMIC_OP_GCC(TYPE, PREFIX, SUFFIX, BACKEND) \
    TYPE PREFIX ## _atomic_ ## SUFFIX ## _mo(TYPE volatile *dst, TYPE arg, enum atomic_mo mo) \
    { \
        return BACKEND(dst, arg, mo); \
    }

#define DECL_ATOMIC_OP_MSVC(TYPE, PREFIX, SUFFIX, BACKEND_TYPE, BACKEND) \
    TYPE PREFIX ## _atomic_ ## SUFFIX ## _mo(TYPE volatile *dst, TYPE arg, enum atomic_mo mo) \
    { \
        (void) mo; \
        _Static_assert(sizeof(BACKEND_TYPE) == sizeof(TYPE), ""); \
        return (TYPE) BACKEND((BACKEND_TYPE volatile *) dst, (BACKEND_TYPE) arg); \
    }

#define DECL_ATOMIC_FETCH_SUB_MSVC(TYPE, PREFIX) \
    TYPE PREFIX ## _atomic_fetch_sub_mo(TYPE volatile *dst, TYPE arg, enum atomic_mo mo) \
    { \
        return (TYPE) atomic_fetch_add_mo(dst, 0 - arg, mo); \
    }

#define DECL_ATOMIC_FETCH_ADD_SAT(TYPE, PREFIX) \
    TYPE PREFIX ## _atomic_fetch_add_sat_mo(TYPE volatile *mem, TYPE val, enum atomic_mo mo) \
    { \
        TYPE probe = atomic_load(mem); \
        while (probe < umax(probe) && !atomic_cmp_xchg_mo(mem, &probe, add_sat(probe, val), 1, mo, ATOMIC_RELAXED)) probe = atomic_load(mem); \
        return probe; \
    }

#define DECL_ATOMIC_FETCH_SUB_SAT(TYPE, PREFIX) \
    TYPE PREFIX ## _atomic_fetch_sub_sat(TYPE volatile *mem, TYPE val, enum atomic_mo mo) \
    { \
        TYPE probe = atomic_load(mem); \
        while (probe && !atomic_cmp_xchg_mo(mem, &probe, sub_sat(probe, val), 1, mo, ATOMIC_RELAXED)) probe = atomic_load(mem); \
        return probe; \
    }

#define DECL_ATOMIC_BT(TYPE, PREFIX) \
    bool PREFIX ## _atomic_bt_mo(TYPE volatile *arr, size_t bit, enum atomic_mo mo) \
    { \
        return atomic_load_mo(arr + bit / bitsof(TYPE), mo) & ((TYPE) 1 << bit % bitsof(TYPE)); \
    }

#define DECL_ATOMIC_BTS(TYPE, PREFIX) \
    bool PREFIX ## _atomic_bts_mo(TYPE volatile *arr, size_t bit, enum atomic_mo mo) \
    { \
        TYPE msk = (TYPE) 1 << bit % bitsof(TYPE), res = atomic_fetch_or_mo(arr + bit / bitsof(TYPE), msk, mo); \
        return res & msk; \
    }

#define DECL_ATOMIC_BTR(TYPE, PREFIX) \
    bool PREFIX ## _atomic_btr_mo(TYPE volatile *arr, size_t bit, enum atomic_mo mo) \
    { \
        TYPE msk = (TYPE) 1 << bit % bitsof(TYPE), res = atomic_fetch_and_mo(arr + bit / bitsof(TYPE), ~msk, mo); \
        return res & msk; \
    }

#define DECL_ATOMIC_BS(TYPE, PREFIX) \
    void PREFIX ## _atomic_bs_mo(TYPE volatile *arr, size_t bit, enum atomic_mo mo) \
    { \
        atomic_fetch_or_mo(arr + bit / bitsof(TYPE), (TYPE) 1 << bit % bitsof(TYPE), mo); \
    }

#define DECL_ATOMIC_BR(TYPE, PREFIX) \
    void PREFIX ## _atomic_br_mo(TYPE volatile *arr, size_t bit, enum atomic_mo mo) \
    { \
        atomic_fetch_and_mo(arr + bit / bitsof(TYPE), ~((TYPE) 1 << bit % bitsof(TYPE)), mo); \
    }

#define DECL_ATOMIC_BTZ_MSVC(TYPE, PREFIX, SUFFIX, BACKEND_TYPE, BACKEND) \
    bool PREFIX ## _atomic_ ## SUFFIX ## _mo(TYPE volatile *arr, size_t bit, enum atomic_mo mo) \
    { \
        (void) mo; \
        _Static_assert(sizeof(BACKEND_TYPE) == sizeof(TYPE), ""); \
        return BACKEND((BACKEND_TYPE volatile *) (arr + bit / bitsof(TYPE)), (BACKEND_TYPE) (bit % bitsof(TYPE))); \
    }

// Type of double width
DECL_WIDE_NARR(unsigned char, uchar)
DECL_WIDE_NARR(unsigned short, ushrt)
DECL_WIDE_NARR(unsigned, uint)
DECL_WIDE_NARR(unsigned long, ulong)
IF_GCC_LLVM(IF_X64(DECL_WIDE_NARR(unsigned long long, ullong)))

IF_MSVC_X32(Dullong_t ullong_wide(unsigned long long lo, unsigned long long hi)
{
    _Static_assert(sizeof(Dullong_t) == 2 * sizeof(unsigned long long), "");
    return (Dullong_t) { .v[0] = lo, .v[1] = hi };
})

// Lower part
DECL_LO_WIDE(unsigned short, ushrt)
DECL_LO_WIDE(unsigned, uint)
DECL_LO_WIDE(unsigned long, ulong)
DECL_LO_WIDE(unsigned long long, ullong)
IF_GCC_LLVM(IF_X64(DECL_LO_WIDE(Uint128_t, Uint128)))

IF_MSVC_X32(HUint128_t Uint128_lo(Uint128_t x)
{
    _Static_assert(2 * sizeof(HUint128_t) == sizeof(Uint128_t), "");
    return x.v[0];
})

// Higher part
DECL_HI_WIDE(unsigned short, ushrt)
DECL_HI_WIDE(unsigned, uint)
DECL_HI_WIDE(unsigned long, ulong)
DECL_HI_WIDE(unsigned long long, ullong)
IF_GCC_LLVM(IF_X64(DECL_HI_WIDE(Uint128_t, Uint128)))

IF_MSVC_X32(HUint128_t Uint128_hi(Uint128_t x)
{
    _Static_assert(2 * sizeof(HUint128_t) == sizeof(Uint128_t), "");
    return x.v[1];
})

// Add with input/output carry
IF_GCC_LLVM(DECL_OP_GCC(unsigned char, uchar, add, __builtin_add_overflow))
IF_MSVC(DECL_OP_INTEL(unsigned char, uchar, add, _addcarry_u8))
IF_GCC_LLVM(DECL_OP_GCC(unsigned short, ushrt, add, __builtin_add_overflow))
IF_MSVC(DECL_OP_INTEL(unsigned short, ushrt, add, _addcarry_u16))
IF_GCC_LLVM_MSVC(DECL_OP_INTEL(unsigned, uint, add, _addcarry_u32))
IF_GCC_LLVM_MSVC(DECL_OP_COPY(unsigned long, ulong, add, unsigned char, Eulong_t, unsigned char))
IF_GCC_LLVM_MSVC(IF_X32(DECL_OP_WIDE(unsigned long long, ullong)))
IF_GCC_LLVM_MSVC(IF_X64(DECL_OP_INTEL(unsigned long long, ullong, add, _addcarry_u64)))

// Subtract with input/output borrow
IF_GCC_LLVM(DECL_OP_GCC(unsigned char, uchar, sub, __builtin_sub_overflow))
IF_MSVC(DECL_OP_INTEL(unsigned char, uchar, sub, _subborrow_u8))
IF_GCC_LLVM(DECL_OP_GCC(unsigned short, ushrt, sub, __builtin_sub_overflow))
IF_MSVC(DECL_OP_INTEL(unsigned short, ushrt, sub, _subborrow_u16))
IF_GCC_LLVM_MSVC(DECL_OP_INTEL(unsigned, uint, sub, _subborrow_u32))
IF_GCC_LLVM_MSVC(DECL_OP_COPY(unsigned long, ulong, sub, unsigned char, Eulong_t, unsigned char))
IF_GCC_LLVM_MSVC(IF_X32(DECL_OP_WIDE(unsigned long long, ullong)))
IF_GCC_LLVM_MSVC(IF_X64(DECL_OP_INTEL(unsigned long long, ullong, sub, _subborrow_u64)))

// Multiply
IF_GCC_LLVM_MSVC(DECL_MUL_NARR(unsigned char, uchar))
IF_GCC_LLVM_MSVC(DECL_MUL_NARR(unsigned short, ushrt))
IF_GCC_LLVM_MSVC(DECL_MUL_NARR(unsigned, uint))
IF_GCC_LLVM_MSVC(DECL_OP_COPY(unsigned long, ulong, mul, unsigned long, Eulong_t, Eulong_t))
IF_GCC_LLVM_MSVC(IF_X32(DECL_MUL_WIDE(unsigned long long, ullong)))
IF_GCC_LLVM(IF_X64(DECL_MUL_NARR(unsigned long long, ullong)))
IF_X64(DECL_MUL_WIDE(Uint128_t, Uint128))

IF_MSVC(IF_X64(unsigned long long ullong_mul(unsigned long long *p_hi, unsigned long long x, unsigned long long y)
{
    return _umul128(x, y, p_hi);
}))

// Multiply–accumulate
DECL2(MA, )

// Shift left
IF_GCC_LLVM_MSVC(DECL_SHL_NARR(unsigned char, uchar))
IF_GCC_LLVM_MSVC(DECL_SHL_NARR(unsigned short, ushrt))
IF_GCC_LLVM_MSVC(DECL_SHL_NARR(unsigned, uint))
IF_GCC_LLVM_MSVC(DECL_SHL_NARR(unsigned long, ulong))
IF_GCC_LLVM_MSVC(IF_X32(DECL_SHZ_SSE(unsigned long long, ullong, shl, _mm_slli_epi64, _mm_srli_epi64)))
IF_GCC_LLVM(IF_X64(DECL_SHL_NARR(unsigned long long, ullong)))

IF_MSVC(IF_X64(unsigned long long ullong_shl(unsigned long long *p_hi, unsigned long long x, unsigned char y)
{
    *p_hi = __shiftleft128(x, 0, y); // Warning! 'y %= 64' is done internally!
    return x << y;
}))

// Shift right
IF_GCC_LLVM_MSVC(DECL_SHR_NARR(unsigned char, uchar))
IF_GCC_LLVM_MSVC(DECL_SHR_NARR(unsigned short, ushrt))
IF_GCC_LLVM_MSVC(DECL_SHR_NARR(unsigned, uint))
IF_GCC_LLVM_MSVC(DECL_SHR_NARR(unsigned long, ulong))
IF_GCC_LLVM_MSVC(IF_X32(DECL_SHZ_SSE(unsigned long long, ullong, shr, _mm_srli_epi64, _mm_slli_epi64)))
IF_GCC_LLVM(IF_X64(DECL_SHR_NARR(unsigned long long, ullong)))

IF_MSVC(IF_X64(unsigned long long ullong_shr(unsigned long long *p_lo, unsigned long long x, unsigned char y)
{
    *p_lo = __shiftright128(0, x, y); // Warning! 'y %= 64' is done internally!
    return x >> y;
}))

// Rotate left
IF_GCC_LLVM(DECL_ROZ_GCC(unsigned char, uchar, rol, <<, >>))
IF_MSVC(DECL_2(unsigned char, uchar, rol, unsigned char, _rotl8))
IF_GCC_LLVM(DECL_ROZ_GCC(unsigned short, ushrt, rol, <<, >>))
IF_MSVC(DECL_2(unsigned short, ushrt, rol, unsigned char, _rotl16))
IF_GCC_LLVM(DECL_ROZ_GCC(unsigned, uint, rol, <<, >>))
IF_MSVC(DECL_2(unsigned, uint, rol, unsigned char, _rotl))
IF_GCC_LLVM(DECL_ROZ_GCC(unsigned long, ulong, rol, <<, >>))
IF_MSVC(DECL_2_COPY(unsigned long, ulong, rol, unsigned char, unsigned))
IF_GCC_LLVM(DECL_ROZ_GCC(unsigned long long, ullong, rol, <<, >>))
IF_MSVC(DECL_2(unsigned long long, ullong, rol, unsigned char, _rotl64))

// Rotate right
IF_GCC_LLVM(DECL_ROZ_GCC(unsigned char, uchar, ror, >>, <<))
IF_MSVC(DECL_2(unsigned char, uchar, ror, unsigned char, _rotr8))
IF_GCC_LLVM(DECL_ROZ_GCC(unsigned short, ushrt, ror, >>, <<))
IF_MSVC(DECL_2(unsigned short, ushrt, ror, unsigned char, _rotr16))
IF_GCC_LLVM(DECL_ROZ_GCC(unsigned, uint, ror, >>, <<))
IF_MSVC(DECL_2(unsigned, uint, ror, unsigned char, _rotr))
IF_GCC_LLVM(DECL_ROZ_GCC(unsigned long, ulong, ror, >>, <<))
IF_MSVC(DECL_2_COPY(unsigned long, ulong, ror, unsigned char, unsigned))
IF_GCC_LLVM(DECL_ROZ_GCC(unsigned long long, ullong, ror, >>, <<))
IF_MSVC(DECL_2(unsigned long long, ullong, ror, unsigned char, _rotr64))

// Saturated add
DECL2(ADD_SAT, )

// Saturated subtract
DECL2(SUB_SAT, )

// Sign of the difference of unsigned integers
DECL2(USIGN, )

// Absolute difference of unsigned integers
DECL2(UDIST, )

// Non-destructive add with overflow test
IF_GCC_LLVM(DECL2(TEST_OP_GCC, , add, __builtin_add_overflow))
IF_MSVC(DECL2(TEST_OP, , add, unsigned char))

// Non-destructive subtract with overflow test
IF_GCC_LLVM(DECL2(TEST_OP_GCC, , sub, __builtin_sub_overflow))
IF_MSVC(DECL2(TEST_OP, , sub, unsigned char))

// Non-destructive multiply with overflow test
IF_GCC_LLVM(DECL2(TEST_OP_GCC, , mul, __builtin_mul_overflow))
IF_MSVC(DECL2(TEST_MUL, ))

// Non-destructive multiply-accumulate with overflow test
DECL2(TEST_MA, )

// Sum with overflow test
DECL2(TEST_REP, , sum, 0, add)

// Product with overflow test
DECL2(TEST_REP, , prod, 1, mul)

// Bit scan reverse
#define X1(...) __VA_ARGS__, __VA_ARGS__
#define X2(...) X1(X1(__VA_ARGS__))
#define X3(...) X2(X1(__VA_ARGS__))
#define X4(...) X3(X1(__VA_ARGS__))
#define X5(...) X4(X1(__VA_ARGS__))
#define X6(...) X5(X1(__VA_ARGS__))
#define X7(...) X6(X1(__VA_ARGS__))

unsigned char uchar_bsr(unsigned char x)
{
    static const unsigned char res[] = { umax(x), 0, X1(1), X2(2), X3(3), X4(4), X5(5), X6(6), X7(7) };
    _Static_assert(countof(res) - 1 == umax(x), "");
    return res[x];
}

IF_GCC_LLVM_MSVC(DECL_1_NARR(unsigned short, ushrt, bsr))
IF_GCC_LLVM(DECL_BSR_GCC(unsigned, uint, __builtin_clz))
IF_MSVC(DECL_1_COPY(unsigned, uint, bsr, unsigned long))
IF_GCC_LLVM(DECL_BSR_GCC(unsigned long, ulong, __builtin_clzl))
IF_MSVC(DECL_BSZ_MSVC(unsigned long, ulong, bsr, _BitScanReverse))
IF_GCC_LLVM_MSVC(IF_X32(DECL_BSR_WIDE(unsigned long long, ullong)))
IF_GCC_LLVM(IF_X64(DECL_BSR_GCC(unsigned long long, ullong, __builtin_clzll)))
IF_MSVC(IF_X64(DECL_BSZ_MSVC(unsigned long long, ullong, bsr, _BitScanReverse64)))

// Bit scan forward
#define XX1 0
#define XX2 XX1, 1, XX1
#define XX3 XX2, 2, XX2
#define XX4 XX3, 3, XX3
#define XX5 XX4, 4, XX4
#define XX6 XX5, 5, XX5
#define XX7 XX6, 6, XX6
#define XX8 XX7, 7, XX7

unsigned char uchar_bsf(unsigned char x) 
{ 
    static const unsigned char res[] = { umax(x), XX8 };
    _Static_assert(countof(res) - 1 == umax(x), "");
    return res[x];
}

IF_GCC_LLVM_MSVC(DECL_1_NARR(unsigned short, ushrt, bsf))
IF_GCC_LLVM(DECL_BSF_GCC(unsigned, uint, __builtin_ctz))
IF_MSVC(DECL_1_COPY(unsigned, uint, bsf, unsigned long))
IF_GCC_LLVM(DECL_BSF_GCC(unsigned long, ulong, __builtin_ctzl))
IF_MSVC(DECL_BSZ_MSVC(unsigned long, ulong, bsf, _BitScanForward))
IF_GCC_LLVM_MSVC(IF_X32(DECL_BSF_WIDE(unsigned long long, ullong)))
IF_GCC_LLVM(IF_X64(DECL_BSF_GCC(unsigned long long, ullong, __builtin_ctzll)))
IF_MSVC(IF_X64(DECL_BSZ_MSVC(unsigned long long, ullong, bsf, _BitScanForward64)))

// Population count
#define Y1(X) X, X + 1
#define Y2(X) Y1(X), Y1(X + 1)
#define Y3(X) Y2(X), Y2(X + 1)
#define Y4(X) Y3(X), Y3(X + 1)
#define Y5(X) Y4(X), Y4(X + 1)
#define Y6(X) Y5(X), Y5(X + 1)
#define Y7(X) Y6(X), Y6(X + 1)
#define Y8(X) Y7(X), Y7(X + 1)

unsigned char uchar_pcnt(unsigned char x)
{
    static const unsigned char res[] = { Y8(0) };
    _Static_assert(countof(res) - 1 == umax(x), "");
    return res[x];
}

IF_GCC_LLVM(DECL_1_NARR(unsigned short, ushrt, pcnt))
IF_MSVC(DECL_1(unsigned short, ushrt, pcnt, __popcnt16))
IF_GCC_LLVM(DECL_1(unsigned, uint, pcnt, __builtin_popcount))
IF_MSVC(DECL_1(unsigned, uint, pcnt, __popcnt))
IF_GCC_LLVM(DECL_1(unsigned long, ulong, pcnt, __builtin_popcountl))
IF_MSVC(DECL_1_COPY(unsigned long, ulong, pcnt, unsigned))
IF_GCC_LLVM(DECL_1(unsigned long long, ullong, pcnt, __builtin_popcountll))
IF_MSVC(IF_X32(DECL_PCNT_WIDE(unsigned long long, ullong)))
IF_MSVC(IF_X64(DECL_1(unsigned long long, ullong, pcnt, __popcnt64)))

// Bit test
DECL2(BT, )

// Bit test and set
DECL2(BTS, )

// Bit test and reset
DECL2(BTR, )

// Bit set
DECL2(BS, )

// Bit reset
DECL2(BR, )

// Bit fetch burst
DECL2(BT_BURST, )

// Bit fetch and set burst
DECL2(BTS_BURST, )

// Bit fetch and reset burst
DECL2(BTR_BURST, )

// Bit set burst
DECL2(BS_BURST, )

// Bit reset burst
DECL2(BR_BURST, )

// Atomic load
IF_GCC_LLVM(DECL_ATOMIC_LOAD_GCC(bool, bool))
IF_MSVC(DECL_ATOMIC_LOAD_MSVC(bool, bool))
IF_GCC_LLVM(DECL_ATOMIC_LOAD_GCC(unsigned char, uchar))
IF_MSVC(DECL_ATOMIC_LOAD_MSVC(unsigned char, uchar))
IF_GCC_LLVM(DECL_ATOMIC_LOAD_GCC(unsigned short, ushrt))
IF_MSVC(DECL_ATOMIC_LOAD_MSVC(unsigned short, ushrt))
IF_GCC_LLVM(DECL_ATOMIC_LOAD_GCC(unsigned, uint))
IF_MSVC(DECL_ATOMIC_LOAD_MSVC(unsigned, uint))
IF_GCC_LLVM(DECL_ATOMIC_LOAD_GCC(unsigned long, ulong))
IF_MSVC(DECL_ATOMIC_LOAD_MSVC(unsigned long, ulong))
IF_GCC_LLVM_MSVC(IF_X32(DECL_ATOMIC_LOAD_SSE(unsigned long long, ullong)))
IF_GCC_LLVM(IF_X64(DECL_ATOMIC_LOAD_GCC(unsigned long long, ullong)))
IF_MSVC(IF_X64(DECL_ATOMIC_LOAD_MSVC(unsigned long long, ullong)))
IF_GCC_LLVM(DECL_ATOMIC_LOAD_GCC(void *, ptr))
IF_MSVC(DECL_ATOMIC_LOAD_MSVC(void *, ptr))

// Atomic store
IF_GCC_LLVM(DECL_ATOMIC_STORE_GCC(bool, bool))
IF_MSVC(DECL_ATOMIC_STORE_MSVC(bool, bool))
IF_GCC_LLVM(DECL_ATOMIC_STORE_GCC(unsigned char, uchar))
IF_MSVC(DECL_ATOMIC_STORE_MSVC(unsigned char, uchar))
IF_GCC_LLVM(DECL_ATOMIC_STORE_GCC(unsigned short, ushrt))
IF_MSVC(DECL_ATOMIC_STORE_MSVC(unsigned short, ushrt))
IF_GCC_LLVM(DECL_ATOMIC_STORE_GCC(unsigned, uint))
IF_MSVC(DECL_ATOMIC_STORE_MSVC(unsigned, uint))
IF_GCC_LLVM(DECL_ATOMIC_STORE_GCC(unsigned long, ulong))
IF_MSVC(DECL_ATOMIC_STORE_MSVC(unsigned long, ulong))
IF_GCC_LLVM_MSVC(IF_X32(DECL_ATOMIC_STORE_SSE(unsigned long long, ullong)))
IF_GCC_LLVM(IF_X64(DECL_ATOMIC_STORE_GCC(unsigned long long, ullong)))
IF_MSVC(IF_X64(DECL_ATOMIC_STORE_MSVC(unsigned long long, ullong)))
IF_GCC_LLVM(DECL_ATOMIC_STORE_GCC(void *, ptr))
IF_MSVC(DECL_ATOMIC_STORE_MSVC(void *, ptr))

// Atomic compare and swap
#define sync_val_compare_and_swap(DST, XCHG, CMP) __sync_val_compare_and_swap(DST, CMP, XCHG)

IF_GCC_LLVM(DECL_ATOMIC_CMP_XCHG_GCC(unsigned char, uchar))
IF_MSVC(DECL_ATOMIC_CMP_XCHG(unsigned char, uchar, char, _InterlockedCompareExchange8))
IF_GCC_LLVM(DECL_ATOMIC_CMP_XCHG_GCC(unsigned short, ushrt))
IF_MSVC(DECL_ATOMIC_CMP_XCHG(unsigned short, ushrt, short, _InterlockedCompareExchange16))
IF_GCC_LLVM(DECL_ATOMIC_CMP_XCHG_GCC(unsigned, uint))
IF_MSVC(DECL_ATOMIC_CMP_XCHG_COPY(unsigned, uint, unsigned long))
IF_GCC_LLVM(DECL_ATOMIC_CMP_XCHG_GCC(unsigned long, ulong))
IF_MSVC(DECL_ATOMIC_CMP_XCHG(unsigned long, ulong, long, _InterlockedCompareExchange))
IF_GCC(IF_X32(DECL_ATOMIC_CMP_XCHG_GCC(unsigned long long, ullong)))
IF_LLVM(IF_X32(DECL_ATOMIC_CMP_XCHG(unsigned long long, ullong, unsigned long long, sync_val_compare_and_swap)))
IF_MSVC(DECL_ATOMIC_CMP_XCHG(unsigned long long, ullong, long long, _InterlockedCompareExchange64))
IF_GCC_LLVM(DECL_ATOMIC_CMP_XCHG_GCC(void *, ptr))
IF_MSVC(DECL_ATOMIC_CMP_XCHG(void *, ptr, void *, _InterlockedCompareExchangePointer))
IF_GCC(IF_X64(DECL_ATOMIC_CMP_XCHG(Uint128_t, Uint128, Uint128_t, sync_val_compare_and_swap)))
IF_LLVM(IF_X64(DECL_ATOMIC_CMP_XCHG_GCC(Uint128_t, Uint128)))

IF_MSVC(IF_X64(bool Uint128_atomic_cmp_xchg_mo(volatile Uint128_t *dst, Uint128_t *p_cmp, Uint128_t xchg, bool weak, enum atomic_mo mo_succ, enum atomic_mo mo_fail)
{
    (void) weak, (void) mo_succ, (void) mo_fail;
    _Static_assert(sizeof(dst->v) == 2 * sizeof(long long), "");
    return _InterlockedCompareExchange128((volatile long long *) dst->v, hi(xchg), lo(xchg), (long long *) p_cmp->v);
}))

// Atomic fetch AND
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned char, uchar, fetch_and, __atomic_fetch_and))
IF_MSVC(DECL_ATOMIC_OP_MSVC(unsigned char, uchar, fetch_and, char, _InterlockedAnd8))
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned short, ushrt, fetch_and, __atomic_fetch_and))
IF_MSVC(DECL_ATOMIC_OP_MSVC(unsigned short, ushrt, fetch_and, short, _InterlockedAnd16))
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned, uint, fetch_and, __atomic_fetch_and))
IF_MSVC(DECL_ATOMIC_OP_COPY(unsigned, uint, fetch_and, unsigned, unsigned, unsigned long))
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned long, ulong, fetch_and, __atomic_fetch_and))
IF_MSVC(DECL_ATOMIC_OP_MSVC(unsigned long, ulong, fetch_and, long, _InterlockedAnd))
IF_GCC_LLVM_MSVC(IF_X32(DECL_ATOMIC_OP_WIDE(unsigned long long, ullong, fetch_and, , &)))
IF_GCC_LLVM(IF_X64(DECL_ATOMIC_OP_GCC(unsigned long long, ullong, fetch_and, __atomic_fetch_and)))
IF_MSVC(IF_X64(DECL_ATOMIC_OP_MSVC(unsigned long long, ullong, fetch_and, long long, _InterlockedAnd64)))

// Atomic fetch-OR
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned char, uchar, fetch_or, __atomic_fetch_or))
IF_MSVC(DECL_ATOMIC_OP_MSVC(unsigned char, uchar, fetch_or, char, _InterlockedOr8))
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned short, ushrt, fetch_or, __atomic_fetch_or))
IF_MSVC(DECL_ATOMIC_OP_MSVC(unsigned short, ushrt, fetch_or, short, _InterlockedOr16))
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned, uint, fetch_or, __atomic_fetch_or))
IF_MSVC(DECL_ATOMIC_OP_COPY(unsigned, uint, fetch_or, unsigned, unsigned, unsigned long))
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned long, ulong, fetch_or, __atomic_fetch_or))
IF_MSVC(DECL_ATOMIC_OP_MSVC(unsigned long, ulong, fetch_or, long, _InterlockedOr))
IF_GCC_LLVM_MSVC(IF_X32(DECL_ATOMIC_OP_WIDE(unsigned long long, ullong, fetch_or, , |)))
IF_GCC_LLVM(IF_X64(DECL_ATOMIC_OP_GCC(unsigned long long, ullong, fetch_or, __atomic_fetch_or)))
IF_MSVC(IF_X64(DECL_ATOMIC_OP_MSVC(unsigned long long, ullong, fetch_or, long long, _InterlockedOr64)))

// Atomic fetch-XOR
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned char, uchar, fetch_xor, __atomic_fetch_xor))
IF_MSVC(DECL_ATOMIC_OP_MSVC(unsigned char, uchar, fetch_xor, char, _InterlockedXor8))
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned short, ushrt, fetch_xor, __atomic_fetch_xor))
IF_MSVC(DECL_ATOMIC_OP_MSVC(unsigned short, ushrt, fetch_xor, short, _InterlockedXor16))
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned, uint, fetch_xor, __atomic_fetch_xor))
IF_MSVC(DECL_ATOMIC_OP_COPY(unsigned, uint, fetch_xor, unsigned, unsigned, unsigned long))
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned long, ulong, fetch_xor, __atomic_fetch_xor))
IF_MSVC(DECL_ATOMIC_OP_MSVC(unsigned long, ulong, fetch_xor, long, _InterlockedXor))
IF_GCC_LLVM_MSVC(IF_X32(DECL_ATOMIC_OP_WIDE(unsigned long long, ullong, fetch_xor, , ^)))
IF_GCC_LLVM(IF_X64(DECL_ATOMIC_OP_GCC(unsigned long long, ullong, fetch_xor, __atomic_fetch_xor)))
IF_MSVC(IF_X64(DECL_ATOMIC_OP_MSVC(unsigned long long, ullong, fetch_xor, long long, _InterlockedXor64)))

// Atomic fetch-add
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned char, uchar, fetch_add, __atomic_fetch_add))
IF_MSVC(DECL_ATOMIC_OP_MSVC(unsigned char, uchar, fetch_add, char, _InterlockedExchangeAdd8))
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned short, ushrt, fetch_add, __atomic_fetch_add))
IF_MSVC(DECL_ATOMIC_OP_MSVC(unsigned short, ushrt, fetch_add, short, _InterlockedExchangeAdd16))
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned, uint, fetch_add, __atomic_fetch_add))
IF_MSVC(DECL_ATOMIC_OP_COPY(unsigned, uint, fetch_add, unsigned, unsigned, unsigned long))
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned long, ulong, fetch_add, __atomic_fetch_add))
IF_MSVC(DECL_ATOMIC_OP_MSVC(unsigned long, ulong, fetch_add, long, _InterlockedExchangeAdd))
IF_GCC_LLVM_MSVC(IF_X32(DECL_ATOMIC_OP_WIDE(unsigned long long, ullong, fetch_add, , +)))
IF_GCC_LLVM(IF_X64(DECL_ATOMIC_OP_GCC(unsigned long long, ullong, fetch_add, __atomic_fetch_add)))
IF_MSVC(IF_X64(DECL_ATOMIC_OP_MSVC(unsigned long long, ullong, fetch_add, long long, _InterlockedExchangeAdd64)))

// Atomic fetch-subtract
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned char, uchar, fetch_sub, __atomic_fetch_sub))
IF_MSVC(DECL_ATOMIC_FETCH_SUB_MSVC(unsigned char, uchar))
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned short, ushrt, fetch_sub, __atomic_fetch_sub))
IF_MSVC(DECL_ATOMIC_FETCH_SUB_MSVC(unsigned short, ushrt))
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned, uint, fetch_sub, __atomic_fetch_sub))
IF_MSVC(DECL_ATOMIC_FETCH_SUB_MSVC(unsigned, uint))
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned long, ulong, fetch_sub, __atomic_fetch_sub))
IF_MSVC(DECL_ATOMIC_FETCH_SUB_MSVC(unsigned long, ulong))
IF_GCC_LLVM(IF_X32(DECL_ATOMIC_OP_WIDE(unsigned long long, ullong, fetch_sub, , -)))
IF_GCC_LLVM(IF_X64(DECL_ATOMIC_OP_GCC(unsigned long long, ullong, fetch_sub, __atomic_fetch_sub)))
IF_MSVC(DECL_ATOMIC_FETCH_SUB_MSVC(unsigned long long, ullong))

// Atomic swap
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned char, uchar, xchg, __atomic_exchange_n))
IF_MSVC(DECL_ATOMIC_OP_MSVC(unsigned char, uchar, xchg, char, _InterlockedExchange8))
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned short, ushrt, xchg, __atomic_exchange_n))
IF_MSVC(DECL_ATOMIC_OP_MSVC(unsigned short, ushrt, xchg, short, _InterlockedExchangeAdd16))
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned, uint, xchg, __atomic_exchange_n))
IF_MSVC(DECL_ATOMIC_OP_COPY(unsigned, uint, xchg, unsigned, unsigned, unsigned long))
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(unsigned long, ulong, xchg, __atomic_exchange_n))
IF_MSVC(DECL_ATOMIC_OP_MSVC(unsigned long, ulong, xchg, long, _InterlockedExchange))
GPUSH GWRN(unused-value)
IF_GCC_LLVM_MSVC(IF_X32(DECL_ATOMIC_OP_WIDE(unsigned long long, ullong, xchg, , , )))
GPOP
IF_GCC_LLVM(IF_X64(DECL_ATOMIC_OP_GCC(unsigned long long, ullong, xchg, __atomic_exchange_n)))
IF_MSVC(IF_X64(DECL_ATOMIC_OP_MSVC(unsigned long long, ullong, xchg, long long, _InterlockedExchange64)))
IF_GCC_LLVM(DECL_ATOMIC_OP_GCC(void *, ptr, xchg, __atomic_exchange_n))
IF_MSVC(DECL_ATOMIC_OP_MSVC(void *, ptr, xchg, void *, _InterlockedExchangePointer))

// Atomic fetch-add saturated
DECL2(ATOMIC_FETCH_ADD_SAT, )

// Atomic fetch-add subtract
DECL2(ATOMIC_FETCH_SUB_SAT, )

// Atomic bit test
DECL2(ATOMIC_BT, )

// Atomic bit test and set
IF_GCC_LLVM_MSVC(DECL_ATOMIC_BTS(unsigned char, uchar))
IF_GCC_LLVM_MSVC(DECL_ATOMIC_BTS(unsigned short, ushrt))
IF_GCC_LLVM(DECL_ATOMIC_BTS(unsigned, uint))
IF_MSVC(DECL_ATOMIC_OP_COPY(unsigned, uint, bts, bool, size_t, unsigned long))
IF_GCC_LLVM(DECL_ATOMIC_BTS(unsigned long, ulong))
IF_MSVC(DECL_ATOMIC_BTZ_MSVC(unsigned long, ulong, bts, long, _interlockedbittestandset))
IF_GCC_LLVM(DECL_ATOMIC_BTS(unsigned long long, ullong))
IF_MSVC(IF_X32(DECL_ATOMIC_BTS(unsigned long long, ullong)))
IF_MSVC(IF_X64(DECL_ATOMIC_BTZ_MSVC(unsigned long long, ullong, bts, long long, _interlockedbittestandset64)))

// Atomic bit test and reset
IF_GCC_LLVM_MSVC(DECL_ATOMIC_BTR(unsigned char, uchar))
IF_GCC_LLVM_MSVC(DECL_ATOMIC_BTR(unsigned short, ushrt))
IF_GCC_LLVM(DECL_ATOMIC_BTR(unsigned, uint))
IF_MSVC(DECL_ATOMIC_OP_COPY(unsigned, uint, btr, bool, size_t, unsigned long))
IF_GCC_LLVM(DECL_ATOMIC_BTR(unsigned long, ulong))
IF_MSVC(DECL_ATOMIC_BTZ_MSVC(unsigned long, ulong, btr, long, _interlockedbittestandreset))
IF_GCC_LLVM(DECL_ATOMIC_BTR(unsigned long long, ullong))
IF_MSVC(IF_X32(DECL_ATOMIC_BTR(unsigned long long, ullong)))
IF_MSVC(IF_X64(DECL_ATOMIC_BTZ_MSVC(unsigned long long, ullong, btr, long long, _interlockedbittestandreset64)))

// Atomic bit set
DECL2(ATOMIC_BS, )

// Atomic bit reset
DECL2(ATOMIC_BR, )

// Spinlock
void spinlock_acquire(spinlock *spinlock)
{
    while (!atomic_cmp_xchg_mo(spinlock, &(spinlock_base) { 0 }, 1, 1, ATOMIC_ACQUIRE, ATOMIC_RELAXED)) // Weak CAS is sufficient 
        while (atomic_load_mo(spinlock, ATOMIC_ACQUIRE)) _mm_pause();
}

// Double lock
GPUSH GWRN(implicit-fallthrough)
void *double_lock_execute(spinlock *spinlock, double_lock_callback init, double_lock_callback comm, void *init_args, void *comm_args)
{
    void *res = NULL;
    switch (atomic_load(spinlock))
    {
    case 0:
        if (atomic_cmp_xchg_mo(spinlock, &(spinlock_base) { 0 }, 1, 0, ATOMIC_ACQUIRE, ATOMIC_RELAXED)) // Strong CAS required here!
        {
            if (init) res = init(init_args);
            atomic_store(spinlock, 2);
            break;
        }
    case 1: 
        while (atomic_load(spinlock) != 2) _mm_pause();
    case 2:
        if (comm) res = comm(comm_args);
    }
    return res;
}
GPOP

// Hash functions
unsigned uint_uhash(unsigned x)
{
    _Static_assert(umax(x) == UINT32_MAX, "");
    // Copy-pasted from https://github.com/skeeto/hash-prospector
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

IF_GCC_LLVM_MSVC(IF_MSVC_X32(DECL_1_COPY(unsigned long, ulong, uhash, unsigned)))
IF_GCC_LLVM(IF_X64(DECL_1_COPY(unsigned long, ulong, uhash, unsigned long long)))

unsigned long long ullong_uhash(unsigned long long x)
{
    _Static_assert(umax(x) == UINT64_MAX, "");
    // Copy-pasted from https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ull;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebull;
    x ^= x >> 31;
    return x;
}

unsigned uint_uhash_inv(unsigned x)
{
    _Static_assert(umax(x) == UINT32_MAX, "");
    x ^= x >> 16;
    x *= 0x43021123u;
    x ^= x >> 15 ^ x >> 30;
    x *= 0x1d69e2a5u;
    x ^= x >> 16;
    return x;
}

IF_GCC_LLVM_MSVC(IF_MSVC_X32(DECL_1_COPY(unsigned long, ulong, uhash_inv, unsigned)))
IF_GCC_LLVM(IF_X64(DECL_1_COPY(unsigned long, ulong, uhash_inv, unsigned long long)))

unsigned long long ullong_uhash_inv(unsigned long long x)
{
    _Static_assert(umax(x) == UINT64_MAX, "");
    x ^= x >> 31 ^ x >> 62;
    x *= 0x319642b2d24d8ec3ull;
    x ^= x >> 27 ^ x >> 54;
    x *= 0x96de1b173f119089ull;
    x ^= x >> 30 ^ x >> 60;
    return x;
}

size_t str_djb2a_hash(const char *str)
{
    size_t hash = 5381;
    for (char ch = *str++; ch; ch = *str++) hash = ((hash << 5) + hash) ^ ch;
    return hash;
}

// Base 10 integer logarithm (based on expanded binary search)
#define EXP10(T, N) ((T) 1e ## N)
#define LOG10_LEAF_0(X, C) \
    ((X) <= 1 ? (X) == 1 ? 0 : umax(X) : (C))
#define LOG10_LEAF_1(T, X, N1, C, LEAF) \
    ((X) <= EXP10(T, N1) ? (X) == EXP10(T, N1) ? (N1) : (LEAF) : (T) ((N1) + (C)))
#define LOG10_LEAF_2(T, X, N1, N2, C, LEAF) \
    ((X) <= EXP10(T, N1) ? (X) == EXP10(T, N1) ? (N1) : \
    LOG10_LEAF_1(T, X, N1, C, LEAF) : \
    LOG10_LEAF_1(T, X, N2, C, (T) ((N1) + (C))))
#define LOG10_LEAF_4(T, X, N1, N2, N3, N4, C, LEAF) \
    (LOG10_LEAF_2(T, X, N3, N4, C, LOG10_LEAF_2(T, X, N1, N2, C, LEAF)))
#define LOG10_LEAF_8(T, X, C, LEAF, P) \
    ((X) <= EXP10(T, P ## 5) ? (X) == EXP10(T, P ## 5) ? P ## 5 : \
    LOG10_LEAF_4(T, (X), P ## 1, P ## 2, P ## 3, P ## 4, (C), LEAF) : \
    LOG10_LEAF_4(T, (X), P ## 6, P ## 7, P ## 8, P ## 9, (C), (T) ((P ## 5) + (C))))
#define LOG10_LEAF_16(T, X, C) \
    ((X) <= EXP10(T, 10) ? (X) == EXP10(T, 10) ? 10 : \
    LOG10_LEAF_8(T, (X), (C), LOG10_LEAF_0(X, C), ) : \
    LOG10_LEAF_8(T, (X), (C), (T) (10 + (C)), 1))

unsigned char uchar_ulog10(unsigned char x, bool ceil)
{
    _Static_assert(LOG10(umax(x), 0) == 2 && LOG10(umax(x), 1) == 3, "");
    return LOG10_LEAF_2(unsigned char, x, 1, 2, ceil, LOG10_LEAF_0(x, ceil));
}

unsigned short ushrt_ulog10(unsigned short x, bool ceil)
{
    _Static_assert(LOG10(umax(x), 0) == 4 && LOG10(umax(x), 1) == 5, "");
    return LOG10_LEAF_4(unsigned short, x, 1, 2, 3, 4, ceil, LOG10_LEAF_0(x, ceil));
}

unsigned uint_ulog10(unsigned x, bool ceil)
{
    _Static_assert(LOG10(umax(x), 0) == 9 && LOG10(umax(x), 1) == 10, "");
    return LOG10_LEAF_8(unsigned, x, ceil, LOG10_LEAF_0(x, ceil), );
}

IF_GCC_LLVM_MSVC(IF_MSVC_X32(DECL_2_COPY(unsigned long, ulong, ulog10, bool, unsigned)))
IF_GCC_LLVM(IF_X64(DECL_2_COPY(unsigned long, ulong, ulog10, bool, unsigned long long)))

unsigned long long ullong_ulog10(unsigned long long x, bool ceil)
{
    _Static_assert(LOG10(umax(x), 0) == 19 && LOG10(umax(x), 1) == 20, "");
    return LOG10_LEAF_16(unsigned long long, x, ceil);
}

// Zero byte scan reverse for 128-bit SSE register
unsigned m128i_nbsr8(__m128i x)
{
    return bsr((unsigned) _mm_movemask_epi8(_mm_cmpeq_epi8(x, _mm_setzero_si128())));
}

// Zero byte scan forward for 128-bit SSE register
unsigned m128i_nbsf8(__m128i x)
{
    return bsf((unsigned) _mm_movemask_epi8(_mm_cmpeq_epi8(x, _mm_setzero_si128())));
}

// Correct sign of the 64-bit float: 'return x == 0. ? 0 : 1 - 2 * signbit(x)'
int fp64_sign(double x)
{
    int res = _mm_movemask_pd(_mm_castsi128_pd(_mm_cmpeq_epi64(
        _mm_and_si128(_mm_castpd_si128(_mm_loaddup_pd(&x)), _mm_set_epi64x(0x8000000000000000, 0x7fffffffffffffff)), _mm_setzero_si128())));
    return res & 1 ? 0 : res - 1;
}

// Byte shift (positive 'y' -- right; negative 'y' -- left; 'a' -- 'arithmetic' shift, which makes most significant byte 'sticky')
#define C15(X) X, X1(X), X2(X), X3(X)
#define R15 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14
#define R16 R15, 15

__m128i m128i_szz8(__m128i x, int y, bool a)
{
    static const char shuf[] = { X1(C15(-128), R16), C15(15) };
    return (unsigned) (y + 15) <= 30u ? _mm_shuffle_epi8(x, _mm_loadu_si128((__m128i *) (&shuf[15 + (a << 5) - a] + y))) : _mm_setzero_si128();
}

// Byte rotate (positive 'y' -- right, negative 'y' -- left)
__m128i m128i_roz8(__m128i x, int y)
{
    static const char shuf[] = { R16, R15 };
    return _mm_shuffle_epi8(x, _mm_loadu_si128((__m128i *) (shuf + ((unsigned) y & 15u))));
}
