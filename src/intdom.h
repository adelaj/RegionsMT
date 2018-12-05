#pragma once

#include <stddef.h>
#include <stdint.h>
#include <limits.h>

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