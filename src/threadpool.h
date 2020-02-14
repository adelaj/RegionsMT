#pragma once

#include "common.h"
#include "ll.h"
#include "log.h"
#include "memory.h"

enum {
    TASK_FAIL = 0,
    TASK_SUCCESS,
    TASK_YIELD
};

enum {
    COND_WAIT = 0,
    COND_EXECUTE,
    COND_DROP
};

enum {
    AGGR_FAIL = 0,
    AGGR_SUCCESS,
    AGGR_DROP
};

typedef unsigned (*task_callback)(void *, void *, void *);
typedef unsigned (*condition_callback)(volatile void *, const void *);
typedef void (*aggregator_callback)(volatile void *, const void *, unsigned);

void bit_set_interlocked_p(volatile void *, const void *);
void bit_reset_interlocked_p(volatile void *, const void *);
void bit_set2_interlocked_p(volatile void *, const void *);
void size_inc_interlocked_p(volatile void *, const void *);
void size_dec_interlocked_p(volatile void *, const void *);

bool bit_test2_acquire_p(volatile void *, const void *);
bool bit_test_range_acquire_p(volatile void *, const void *);
bool bit_test2_range01_acquire_p(volatile void *, const void *);

bool size_test_acquire_p(volatile void *, const void *);

struct tls_base {
    struct thread_pool *pool;
    size_t tid, pid, inc;
};

struct task {
    task_callback callback;
    condition_callback cond;
    aggregator_callback aggr;
    void *arg, *context, *cond_arg, *aggr_arg;
    volatile void *cond_mem, *aggr_mem;
};

struct dispatched_task {
    task_callback callback;
    aggregator_callback aggr;
    void *arg, *context, *aggr_arg;
    volatile void *aggr_mem;
    volatile bool ngarbage, norphan;
};

// Opaque structure with OS-dependent implementation
struct thread_pool;

bool thread_pool_enqueue(struct thread_pool *, struct task *, size_t, bool, struct log *);
bool thread_pool_enqueue_yield(struct thread_pool *, generator_callback, void *, size_t, bool, struct log *);
struct thread_pool *thread_pool_create(size_t, size_t, size_t, struct log *);
void thread_pool_dispose(struct thread_pool *);
size_t thread_pool_get_count(struct thread_pool *);

