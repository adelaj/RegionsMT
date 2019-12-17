#pragma once

///////////////////////////////////////////////////////////////////////////////
//
//  Safe facilities for a memory management
//

#include "common.h"

enum array_flags {
    ARRAY_CLEAR = 1,
    ARRAY_STRICT = 2,
    ARRAY_REDUCE = 4,
    ARRAY_REALLOC = 8, // Default in 'array_test'
    ARRAY_FAILSAFE = 16 // Disables internal checks
};

struct array_result {
    unsigned status;
    unsigned error;
    size_t tot; // Total number of allocated bytes
};

enum array_status {
    ARRAY_FAILURE = 0,
    ARRAY_SUCCESS,
    ARRAY_UNTOUCHED,
};

enum array_error {
    ARRAY_NO_ERROR = 0,
    ARRAY_OUT_OF_MEMORY,
    ARRAY_OVERFLOW,
};

void array_broadcast(void *, size_t, size_t, void *);

struct array_result matrix_init(void *, size_t *restrict, size_t, size_t, size_t, size_t, size_t, enum array_flags);
struct array_result array_init(void *, size_t *restrict, size_t, size_t, size_t, enum array_flags);
struct array_result array_test_impl(void *, size_t *restrict, size_t, size_t, enum array_flags, size_t *restrict, size_t);

#define array_test(ARR, P_CAP, SZ, DIFF, FLAGS, ...) \
    (array_test_impl((ARR), (P_CAP), (SZ), (DIFF), (FLAGS), ARG(size_t, __VA_ARGS__)))

struct queue {
    void *arr;
    size_t cap, begin, cnt;
};

typedef void (*generator_callback)(void *, size_t, void *);

void queue_close(struct queue *);
struct array_result queue_init(struct queue *, size_t, size_t);
struct array_result queue_test(struct queue *, size_t, size_t);
void *queue_fetch(struct queue *, size_t, size_t);
struct array_result queue_enqueue(struct queue *restrict, bool, void *restrict, size_t, size_t);
struct array_result queue_enqueue_yield(struct queue *restrict, bool, generator_callback, void *, size_t, size_t);
void queue_dequeue(struct queue *, size_t, size_t);

struct persistent_array {
    void **ptr;
    size_t off, bck, cap;
};

struct array_result persistent_array_init(struct persistent_array *, size_t, size_t);
void persistent_array_close(struct persistent_array *);
struct array_result persistent_array_test(struct persistent_array *, size_t, size_t);
void *persistent_array_fetch(struct persistent_array *, size_t, size_t);
