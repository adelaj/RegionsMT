#pragma once

#include <stddef.h>
#include <stdint.h>
#include <limits.h>

union int_val {
	int i;
	int8_t ib;
	int16_t iw;
	int32_t id;
	int64_t iq;
	ptrdiff_t iz;
	intmax_t ij;
	signed char ihh;
	short ih;
	long il;
	long long ill;
	unsigned u;
	uint8_t ub;
	uint16_t uw;
	uint32_t ud;
	uint64_t uq;
	size_t uz;
	uintmax_t uj;
	unsigned char uhh;
	unsigned short uh;
	unsigned long ul;
	unsigned long long ull;
};

#if INT_MAX < INT8_MAX
typedef int8_t Int8_dom_t;
#else
typedef int Int8_dom_t;
#endif

#if INT_MAX < INT16_MAX
typedef int16_t Int16_dom_t;
#else
typedef int Int16_dom_t;
#endif

#if INT_MAX < INT32_MAX
typedef int32_t Int32_dom_t;
#else
typedef int Int32_dom_t;
#endif

#if INT_MAX < INT64_MAX
typedef int64_t Int64_dom_t;
#else
typedef int Int64_dom_t;
#endif

#if INT_MAX < PTRDIFF_MAX
typedef ptrdiff_t Ptrdiff_dom_t;
#else
typedef int Ptrdiff_dom_t;
#endif

#if INT_MAX < INTMAX_MAX
typedef intmax_t Intmax_dom_t;
#else
typedef int Intmax_dom_t;
#endif

#if UINT_MAX < UINT8_MAX
typedef uint8_t Uint8_dom_t;
#else
typedef unsigned Uint8_dom_t;
#endif

#if UINT_MAX < UINT16_MAX
typedef uint16_t Uint16_dom_t;
#else
typedef unsigned Uint16_dom_t;
#endif

#if UINT_MAX < UINT32_MAX
typedef uint32_t Uint32_dom_t;
#else
typedef unsigned Uint32_dom_t;
#endif

#if UINT_MAX < UINT64_MAX
typedef uint64_t Uint64_dom_t;
#else
typedef unsigned Uint64_dom_t;
#endif

#if UINT_MAX < SIZE_MAX
typedef size_t Size_dom_t;
#else
typedef unsigned Size_dom_t;
#endif

#if UINT_MAX < UINTMAX_MAX
typedef uintmax_t Uintmax_dom_t;
#else
typedef unsigned Uintmax_dom_t;
#endif