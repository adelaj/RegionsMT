#pragma once

///////////////////////////////////////////////////////////////////////////////
//
//  Wrappers for the non-portable features of the C standard library
//  Warning! This file should be included prior to any standard library header!
//

#include "common.h"

#if TEST(IF_UNIX_APPLE)

// Required for some POSIX only functions
#   define _DEFAULT_SOURCE
#   define _POSIX_C_SOURCE 200112L
#   define _DARWIN_C_SOURCE

// Required for the 'fseeko' and 'ftello' functions
#   define _FILE_OFFSET_BITS 64

#   include <alloca.h>
#   include <errno.h>

#elif TEST(IF_WIN)

#   ifndef _CRT_SECURE_NO_WARNINGS
#       define _CRT_SECURE_NO_WARNINGS
#   endif

#   define _CRTDBG_MAP_ALLOC
#   include <crtdbg.h>

#   include <malloc.h>
#   include <errno.h>

#   define HAS_ALIGNED_REALLOC

#endif

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

typedef IF_UNIX_APPLE(int) IF_WIN(errno_t) Errno_t;
#define Alloca(SIZE) IF_UNIX_APPLE((alloca((SIZE)))) IF_WIN((_alloca((SIZE))))
#define Aligned_alloca(SZ, ALIGN) ((void *) ((((uintptr_t) Alloca((SZ) + (ALIGN) - 1) + (ALIGN) - 1) / (ALIGN)) * (ALIGN)))

// This can be safely passed by pointer
typedef struct { va_list va_list; } Va_list;
#define Va_arg(AP, T) va_arg((AP).va_list, T)
#define Va_start(AP, P) va_start((AP).va_list, P)
#define Va_end(AP) va_end((AP).va_list)
#define Va_copy(DST, SRC) va_copy((DST).va_list, (SRC).va_list)

// Aligned memory allocation/deallocation
void *Realloc(void *, size_t); // Releases the memory when size is zero
void *Aligned_malloc(size_t, size_t);
void *Aligned_realloc(void *, size_t, size_t); // Warning! This function returns NULL on all platforms except Windows! However, releases the memory when size is zero
void *Aligned_calloc(size_t, size_t, size_t);
void Aligned_free(void *);

// File operations
FILE *Fopen(const char *, const char *);
FILE *Fdup(FILE *, const char *);
size_t Fwrite_unlocked(const void *, size_t, size_t, FILE *);
int Fflush_unlocked(FILE *);
int Fseeki64(FILE *, int64_t, int);
int64_t Ftelli64(FILE *);
bool Fisatty(FILE *);
int64_t Fsize(FILE *);
int Fclose(FILE *); // Tolerant to the 'NULL'

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
void *Memrchr2(const void *, int, int, size_t);

size_t get_processor_count(void);
size_t get_page_size(void);
size_t get_process_id(void);

// Return UNIX time in microseconds
uint64_t get_time(void);
