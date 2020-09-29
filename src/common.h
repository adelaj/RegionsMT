#pragma once

#include <stdalign.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define OR(A, B, ...) \
    IF_ ## A(IF_ ## B(__VA_ARGS__)) IF_ ## A(IFN_ ## B(__VA_ARGS__)) IFN_ ## A(IF_ ## B(__VA_ARGS__))
#define OR2(A, B, ...) \
    IF_ ## A(IF_ ## B(__VA_ARGS__)) IF_ ## A(IFN_ ## B(__VA_ARGS__)) IFN_ ## A(IF_ ## B(__VA_ARGS__))
#define NAND(A, B, ...) \
    IFN_ ## A(IFN_ ## B(__VA_ARGS__))
#define NAND2(A, B, ...) \
    IFN_ ## A(IFN_ ## B(__VA_ARGS__))

#ifdef __GNUC__
#   define IF_GCC(...) __VA_ARGS__
#   define IFN_GCC(...)
#elif __clang__
#   define IF_LLVM(...) __VA_ARGS__
#   define IFN_LLVM(...)
#elif _MSC_BUILD
#   define IF_MSVC(...) __VA_ARGS__
#   define IFN_MSVC(...)
#endif

#ifndef IF_GCC
#   define IF_GCC(...)
#   define IFN_GCC(...) __VA_ARGS__
#endif

#ifndef IF_LLVM
#   define IF_LLVM(...)
#   define IFN_LLVM(...) __VA_ARGS__
#endif

#ifndef IF_MSVC
#   define IF_MSVC(...)
#   define IFN_MSVC(...) __VA_ARGS__
#endif

#if defined __x86_64__ || defined _M_X64
#   define IF_X64(...) __VA_ARGS__
#   define IFN_X64(...)
#elif defined __i386__ || defined _M_IX86
#   define IF_X32(...) __VA_ARGS__
#   define IFN_X32(...)
#endif

#ifndef IF_X64
#   define IF_X64(...)
#   define IFN_X64(...) __VA_ARGS__
#endif

#ifndef IF_X32
#   define IF_X32(...)
#   define IFN_X32(...) __VA_ARGS__
#endif

#define IF_GCC_LLVM(...) OR(GCC, LLVM, __VA_ARGS__)
#define IFN_GCC_LLVM(...) NAND(GCC, LLVM, __VA_ARGS__)
#define IF_MSVC_X32(...) OR(MSVC, X32, __VA_ARGS__)
#define IFN_MSVC_X32(...) NAND(MSVC, X32, __VA_ARGS__)
#define IF_GCC_LLVM_MSVC(...) OR2(GCC_LLVM, MSVC, __VA_ARGS__)
#define IFN_GCC_LLVM_MSVC(...) NAND2(GCC_LLVM, MSVC, __VA_ARGS__)

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

#define bitsof(X) (CHAR_BIT * sizeof(X))

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

#define GPUSH IF_GCC_LLVM(_Pragma("GCC diagnostic push"))
#define GWRN(WRN) IF_GCC_LLVM(_Pragma(TOSTR(GCC diagnostic ignored TOSTR(-W ## WRN))))
#define GPOP IF_GCC_LLVM(_Pragma("GCC diagnostic pop"))
#define MPUSH IF_MSVC(_Pragma("warning (push)"))
#define MWRN(WRN) IF_MSVC(_Pragma(TOSTR(warning (disable: WRN))))
#define MPOP IF_MSVC(_Pragma("warning (pop)"))

//MWRN(4116) // "Unnamed type definition in parentheses"
MWRN(4200) // "Zero-sized array in structure/union"
MWRN(4201) // "Nameless structure/union"
//MWRN(4204) // "Non-constant aggregate initializer"
MWRN(4221) // "Initialization by using the address of automatic variable"
//MWRN(4090) // "'=': different 'const' qualifiers" -- gives some false-positives
//MWRN(4706) // "Assignment within conditional expression"
