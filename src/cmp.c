#include "np.h"
#include "ll.h"
#include "cmp.h"

#include <immintrin.h>
#include <string.h>

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

// Warning! The approach from the 'DECL_STABLE_CMP_ASC' marcro doesn't work here
#define DECL_FLT64_STABLE_CMP_NAN(INFIX, DIR) \
    int fp64_stable_cmp ## INFIX ## _nan(const void *a, const void *b, void *thunk) \
    { \
        (void) thunk; \
        __m128d ab = _mm_loadh_pd(_mm_load_sd(a), b); \
        __m128i res = _mm_sub_epi32(_mm_castpd_si128(_mm_cmpunord_pd(ab, ab)), _mm_castpd_si128(_mm_cmp_pd(ab, _mm_permute_pd(ab, 1), (DIR)))); \
        return _mm_extract_epi32(res, 2) - _mm_cvtsi128_si32(res); \
    }

int size_stable_cmp_dsc(const void *A, const void *B, void *thunk)
{
    (void) thunk;
    return usign(*(size_t *) B, *(size_t *) A);
}

DECL_STABLE_CMP_ASC(size, )

bool size_cmp_dsc(const void *A, const void *B, void *thunk)
{
    (void) thunk;
    return *(size_t *) B > * (size_t *) A;
}

DECL_CMP_ASC(size, )

int fp64_stable_cmp_dsc(const void *a, const void *b, void *thunk)
{
    (void) thunk;
    __m128d ab = _mm_loadh_pd(_mm_load_sd(a), b);
    __m128i res = _mm_castpd_si128(_mm_cmplt_pd(ab, _mm_permute_pd(ab, 1)));
    return _mm_extract_epi32(res, 2) - _mm_cvtsi128_si32(res);
}

DECL_STABLE_CMP_ASC(fp64, )

int fp64_stable_cmp_dsc_abs(const void *a, const void *b, void *thunk)
{
    (void) thunk;
    __m128d ab = _mm_and_pd(_mm_loadh_pd(_mm_load_sd(a), b), _mm_castsi128_pd(_mm_set1_epi64x(0x7fffffffffffffff)));
    __m128i res = _mm_castpd_si128(_mm_cmplt_pd(ab, _mm_permute_pd(ab, 1)));
    return _mm_extract_epi32(res, 2) - _mm_cvtsi128_si32(res);
}

DECL_STABLE_CMP_ASC(fp64, _abs)

DECL_FLT64_STABLE_CMP_NAN(_dsc, _CMP_NLE_UQ)
DECL_FLT64_STABLE_CMP_NAN(_asc, _CMP_NGE_UQ)

int char_cmp(const void *a, const void *b, void *context)
{
    (void) context;
    return (int) *(const char *) a - (int) *(const char *) b;
}

bool str_eq_unsafe(const void *A, const void *B, void *context)
{
    (void) context;
    return !Strcmp_unsafe((const char *) A, (const char *) B);
}

bool stro_str_eq_unsafe(const void *A, const void *B, void *Str)
{
    return str_eq_unsafe((const char *) Str + *(size_t *) A, B, NULL);
}

bool stro_eq_unsafe(const void *A, const void *B, void *Str)
{
    return str_eq_unsafe((const char *) Str + *(size_t *) A, (const char *) Str + *(size_t *) B, NULL);
}

int stro_stable_cmp(const void *A, const void *B, void *Str)
{
    return strcmp((const char *) Str + *(size_t *) A, (const char *) Str + *(size_t *) B);
}

bool stro_cmp(const void *A, const void *B, void *Str)
{
    return stro_stable_cmp(A, B, Str) > 0;
}

int str_strl_stable_cmp(const void *Str, const void *Strl, void *p_Len)
{
    const struct strl *strl = Strl;
    size_t len = *(size_t *) p_Len;
    int res = strncmp((const char *) Str, strl->str, len);
    return res ? res : len < strl->len ? INT_MIN : 0;
}
