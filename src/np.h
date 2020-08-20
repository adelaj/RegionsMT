#pragma once

///////////////////////////////////////////////////////////////////////////////
//
//  Wrappers for the non-portable features of the C standard library
//  Warning! This file should be included prior to any standard library header!
//

#if defined _MSC_BUILD || defined __MINGW32__
#   ifndef _CRT_SECURE_NO_WARNINGS
#       define _CRT_SECURE_NO_WARNINGS
#   endif

#   define _CRTDBG_MAP_ALLOC
#   include <crtdbg.h>

#   include <malloc.h>
#   define Alloca(SIZE) (_alloca((SIZE)))

#   include <errno.h>
typedef errno_t Errno_t;

#elif defined __GNUC__ || defined __clang__
// Required for some POSIX only functions
#   define _POSIX_C_SOURCE 200112L
#   define _DARWIN_C_SOURCE

// Required for the 'fseeko' and 'ftello' functions
#   define _FILE_OFFSET_BITS 64

#   include <alloca.h>
#   define Alloca(SIZE) (alloca((SIZE)))

#   include <errno.h>
typedef int Errno_t;

#endif

typedef Errno_t Errno_dom_t;

#include "common.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

// This can be safely passed by pointer
typedef struct { va_list va_list; } Va_list;
#define Va_arg(AP, T) va_arg((AP).va_list, T)
#define Va_start(AP, P) va_start((AP).va_list, P)
#define Va_end(AP) va_end((AP).va_list)
#define Va_copy(DST, SRC) va_copy((DST).va_list, (SRC).va_list)

bool aligned_alloca_chk(size_t, size_t, size_t);
#define aligned_alloca(SZ, ALIGN) ((void *) ((((uintptr_t) Alloca((SZ) + (ALIGN) - 1) + (ALIGN) - 1) / (ALIGN)) * (ALIGN)))

// Aligned memory allocation/deallocation
void *Aligned_alloc(size_t, size_t);
void Aligned_free(void *);

// File operations
FILE *Fopen(const char *, const char *);
FILE *Fdup(FILE *, const char *);
int Fclose(FILE *); // Tolerant to the 'NULL' and 'std###' streams
int Fseeki64(FILE *, int64_t, int);
int64_t Ftelli64(FILE *);

// Error and time
Errno_t Strerror_s(char *, size_t, Errno_t);
Errno_t Localtime_s(struct tm *result, const time_t *time);

// Ordinary string processing functions
int Stricmp(const char *, const char *);
int Strnicmp(const char *, const char *, size_t);
size_t Strnlen(const char *, size_t);
size_t Strchrnull(const char *, int);
size_t Strlncpy(char *, char *, size_t);
void *Memrchr(void const *, int, size_t);

bool file_is_tty(FILE *);
int64_t file_get_size(FILE *);
size_t get_processor_count(void);
size_t get_page_size(void);
size_t get_process_id(void);
uint64_t get_time(void);
