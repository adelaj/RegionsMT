#pragma once

#include "common.h"
#include "ll.h"
#include "memory.h"

typedef bool (*cmp_callback)(const void *, const void *, void *); // Functional type for compare callbacks
typedef int (*stable_cmp_callback)(const void *, const void *, void *); // Functional type for stable compare callbacks

struct array_result pointers(uintptr_t **, const void *, size_t, size_t, cmp_callback, void *);
struct array_result pointers_stable(uintptr_t **, const void *, size_t, size_t, stable_cmp_callback, void *);
void orders_from_pointers_inplace(uintptr_t *, uintptr_t, size_t, size_t);
struct array_result orders_stable(uintptr_t **, const void *, size_t, size_t, stable_cmp_callback, void *);
struct array_result orders_stable_unique(uintptr_t **, const void *, size_t *, size_t, stable_cmp_callback, void *);
void ranks_from_pointers_impl(size_t *, const uintptr_t *, uintptr_t, size_t, size_t);
struct array_result ranks_from_pointers(size_t **, const uintptr_t *, uintptr_t, size_t, size_t);
struct array_result ranks_from_orders(size_t **, const uintptr_t *, size_t);
struct array_result ranks_unique(size_t **, const void *, size_t *, size_t, cmp_callback, void *);
void ranks_unique_from_pointers_impl(size_t *, const uintptr_t *, uintptr_t, size_t *, size_t, cmp_callback, void *);
struct array_result ranks_unique_from_pointers(size_t **, const uintptr_t *, uintptr_t, size_t *, size_t, cmp_callback, void *);
struct array_result ranks_unique_from_orders(size_t **, const uintptr_t *, const void *, size_t *, size_t, cmp_callback, void *);
void ranks_from_pointers_inplace_impl(uintptr_t *restrict, uintptr_t, size_t, size_t, uint8_t *restrict);
struct array_result ranks_from_pointers_inplace(uintptr_t *restrict, uintptr_t, size_t, size_t);
struct array_result ranks_from_orders_inplace(uintptr_t *restrict, size_t);
struct array_result ranks_stable(size_t **, const void *, size_t, size_t, stable_cmp_callback, void *);
void orders_apply_impl(uintptr_t *restrict, size_t, size_t, void *restrict, uint8_t *restrict, void *restrict, size_t);
struct array_result orders_apply(uintptr_t *restrict, size_t, size_t, void *restrict, void *restrict, size_t);

#define QUICK_SORT_CUTOFF 20u // The actual quick sort is applied only for arrays of counts, greater than this value
#define QUICK_SORT_CACHED // Enables the more optimal utilization of the CPU caches

Dsize_t quick_sort_partition(void *restrict, size_t, size_t, size_t, cmp_callback, void *, void *restrict, size_t);
void quick_sort(void *restrict, size_t, size_t, cmp_callback, void *, void *restrict, size_t);
void sort_unique(void *restrict, size_t *, size_t, cmp_callback, void *, void *restrict, size_t);

enum binary_search_flags {
    BINARY_SEARCH_RIGHTMOST = 1, // Searches for the rightmost occurrence of the key
    BINARY_SEARCH_CRITICAL = 2, // Searches for the left/rightmost occurrence of the key
    BINARY_SEARCH_IMPRECISE = 4, // Searches for the best approximation from left/right side
};

bool binary_search(size_t *, const void *restrict, const void *restrict, size_t, size_t, stable_cmp_callback, void *, enum binary_search_flags);

// Hash table
// Heavily based on the 'khash.h' and 'khashl.h': https://github.com/attractivechaos/klib
struct hash_table {
    uint8_t *bits;
    uint8_t *flags;
    size_t cnt, lcap, tot, hint;
    void *key, *val;
};

typedef size_t (*hash_callback)(const void *, void *);

enum hash_status {
    HASH_FAILURE = ARRAY_FAILURE,
    HASH_SUCCESS = ARRAY_SUCCESS,
    HASH_UNTOUCHED = ARRAY_UNTOUCHED,
    HASH_SUCCESS_UNTOUCHED = HASH_SUCCESS | HASH_UNTOUCHED,
    HASH_PRESENT = HASH_UNTOUCHED << 1,
};

struct array_result hash_table_init(struct hash_table *, size_t, size_t, size_t);
void hash_table_close(struct hash_table *);
bool hash_table_search(struct hash_table *, size_t *, const void *, size_t, cmp_callback, void *);
bool hash_table_remove(struct hash_table *, size_t, const void *, size_t, cmp_callback, void *);
void *hash_table_fetch_key(struct hash_table *, size_t, size_t);
void *hash_table_fetch_val(struct hash_table *, size_t, size_t);
struct array_result hash_table_alloc(struct hash_table *, size_t *, const void *, size_t, size_t, hash_callback, cmp_callback, void *, void *restrict, void *restrict);
void hash_table_dealloc(struct hash_table *, size_t);

// String pool
struct str_pool {
    struct buff buff;
    struct hash_table tbl;
};

enum {
    STR_POOL_FAILURE = HASH_FAILURE,
    STR_POOL_SUCCESS = HASH_SUCCESS,
    STR_POOL_UNTOUCHED = HASH_UNTOUCHED,
    STR_POOL_PRESENT = HASH_PRESENT,
};

#define str_hash str_djb2a_hash
size_t stro_hash(const void *, void *);

struct array_result str_pool_init(struct str_pool *, size_t, size_t, size_t);
void str_pool_close(struct str_pool *);
struct array_result str_pool_insert(struct str_pool *, const char *, size_t, size_t *, size_t, void *, void *restrict);
bool str_pool_fetch(struct str_pool *, const char *, size_t, void *);
