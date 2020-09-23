#pragma once

#include <stdalign.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define MAX(a, b) \
    ((a) >= (b) ? (a) : (b))
#define MIN(a, b) \
    ((a) <= (b) ? (a) : (b))
#define DIST(a, b) \
    ((a) >= (b) ? (a) - (b) : (b) - (a))
#define ABS(a) \
    ((a) >= 0 ? (a) : -(a))
#define CLAMP(x, a, b) \
    ((x) >= (a) ? (x) <= (b) ? (x) : (b) : (a))
#define SIGN(x, y) \
    ((x) > (y) ? 1 : (x) < (y) ? -1 : 0)

// Number of elements of array
#define countof(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

// Length of string literal (without null-terminator)
#define lengthof(STR) (countof((STR)) - 1)

#define dimxof(DIMY, MAT) (countof(MAT) / (DIMY))
#define dimyof(DIMX, MAT) (countof(MAT) / (DIMX))

// Helper macros evaluating and inserting the count of arguments
#define INIT(T, ...) { __VA_ARGS__ }, countof(((T []) { __VA_ARGS__ }))
#define ARG(T, ...) ((T []) { __VA_ARGS__ }), countof(((T []) { __VA_ARGS__ }))

// Convert value (which is often represented by a macro) to the string literal
#define TOSTR_IMPL(Z) #Z
#define TOSTR(Z) TOSTR_IMPL(Z)

#define STRC(STR) ((char *) (STR)), lengthof(STR)
#define STRI(STR) { STRC(STR) }
#define ARRC(ARR) (ARR), countof(ARR)
#define ARRI(ARR) { ARRC(ARR) }
#define MATCX(DIMY, ...) (__VA_ARGS__), dimxof(DIMY, (__VA_ARGS__)), DIMY, countof((__VA_ARGS__))
#define MATCY(DIMX, ...) (__VA_ARGS__), DIMX, dimyof(DIMX, (__VA_ARGS__)), countof((__VA_ARGS__))

// In the case of compound literal extra parentheses should be added
#define CLIC(...) ARRC((__VA_ARGS__))
#define CLII(...) ARRI((__VA_ARGS__))

// Common value for the size of temporary buffer used for file writing
#define BLOCK_WRITE 4096

// Common value for the size of temporary buffer used for file reading
#define BLOCK_READ 4096

#if defined __GNUC__ || defined __clang__
#   define GPUSH _Pragma("GCC diagnostic push")
#   define GWRN(WRN) _Pragma(TOSTR(GCC diagnostic ignored TOSTR(-W ## WRN)))
#   define GPOP _Pragma("GCC diagnostic pop")
#   define MPUSH
#   define MWRN(WRN)
#   define MPOP
#elif defined _MSC_BUILD
#   define GPUSH
#   define GWRN(WRN)
#   define GPOP
#   define MPUSH _Pragma("warning (push)")
#   define MWRN(WRN) _Pragma(TOSTR(warning (disable: WRN)))
#   define MPOP _Pragma("warning (pop)")
#endif

//MWRN(4116) // "Unnamed type definition in parentheses"
MWRN(4200) // "Zero-sized array in structure/union"
MWRN(4201) // "Nameless structure/union"
//MWRN(4204) // "Non-constant aggregate initializer"
MWRN(4221) // "Initialization by using the address of automatic variable"
//MWRN(4090) // "'=': different 'const' qualifiers" -- gives some false-positives
//MWRN(4706) // "Assignment within conditional expression"
