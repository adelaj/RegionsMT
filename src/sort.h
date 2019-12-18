#pragma once

#include "common.h"
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

#define QUICK_SORT_CUTOFF 20 // The actual quick sort is applied only for arrays of counts, greater than this value
#define QUICK_SORT_CACHED // Enables the more optimal utilization of the CPU caches
void quick_sort(void *restrict, size_t, size_t, cmp_callback, void *, void *restrict, size_t);
void sort_unique(void *restrict, size_t *, size_t, cmp_callback, void *, void *restrict, size_t);

enum binary_search_flags {
    BINARY_SEARCH_RIGHTMOST = 1, // Searches for the rightmost occurrence of the key
    BINARY_SEARCH_CRITICAL = 2, // Searches for the left/rightmost occurrence of the key
    BINARY_SEARCH_INEXACT = 4, // Searches for the best approximation from left/right side
};

bool binary_search(size_t *, const void *restrict, const void *restrict, size_t, size_t, stable_cmp_callback, void *, enum binary_search_flags);

struct hash_table {
    uint8_t *flags;
    size_t cnt, lcap, tot, lim;
    void *key, *val;
};

typedef size_t (*hash_callback)(const void *, void *);

enum hash_status {
    HASH_FAILURE = ARRAY_FAILURE,
    HASH_SUCCESS = ARRAY_SUCCESS,
    HASH_UNTOUCHED = ARRAY_UNTOUCHED,
    HASH_PRESENT = HASH_UNTOUCHED << 1,
};

// Heavily based on the 'khash.h'
struct array_result hash_table_init(struct hash_table *, size_t, size_t, size_t);
void hash_table_close(struct hash_table *);
bool hash_table_search(struct hash_table *, size_t *, const void *, size_t, cmp_callback, void *);
bool hash_table_remove(struct hash_table *, size_t, const void *, size_t, cmp_callback, void *);
void *hash_table_fetch_key(struct hash_table *, size_t, size_t);
void *hash_table_fetch_val(struct hash_table *, size_t, size_t);
struct array_result hash_table_alloc(struct hash_table *, size_t *, const void *, size_t, size_t, hash_callback, cmp_callback, void *);
void hash_table_dealloc(struct hash_table *, size_t);
