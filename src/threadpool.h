#pragma once

#include "common.h"
#include "ll.h"
#include "log.h"
#include "memory.h"

enum {
    TASK_FAIL = 0,
    TASK_SUCCESS,
    TASK_DROP,
    TASK_YIELD
};

enum {
    COND_WAIT = 0,
    COND_EXECUTE,
    COND_DROP,
    COND_CNT
};

enum {
    AGGR_FAIL = 0,
    AGGR_SUCCESS,
    AGGR_DROP,
    AGGR_CNT
};

typedef unsigned (*task_callback)(void *, void *, void *);
typedef unsigned (*condition_callback)(volatile void *, const void *);
typedef void (*aggregator_callback)(volatile void *, const void *, unsigned);

struct inc_mem {
    size_t fail, success, drop;
};

void aggr_inc(volatile void *, const void *, unsigned);

struct tls_base {
    struct thread_pool *pool;
    size_t tid, pid, inc, reentrance;
    void *storage;
};

struct  task_cond {
    condition_callback callback;
    void *arg;
    volatile void *mem;
};

struct task_aggr {
    aggregator_callback callback;
    void *arg;
    volatile void *mem;
};

struct task {
    task_callback callback;
    void *arg, *context;
    struct task_cond cond;
    struct task_aggr aggr;
};

struct dispatched_task {
    task_callback callback;
    struct task_aggr aggr;
    void *arg, *context, *storage;
    size_t reentrance;
    volatile bool not_garbage, not_orphan;
};

// Opaque structure with OS-dependent implementation
struct thread_pool;

bool loop_mt(struct thread_pool *, task_callback, struct task_cond, struct task_aggr, void *, size_t *restrict, size_t, size_t *restrict, bool, struct log *);

bool thread_pool_enqueue(struct thread_pool *, struct task *, size_t, bool, struct log *);
bool thread_pool_enqueue_yield(struct thread_pool *, generator_callback, void *, size_t, bool, struct log *);
void thread_pool_schedule(struct thread_pool *);
struct thread_pool *thread_pool_create(size_t, size_t, size_t, struct log *);
void *thread_pool_fetch_tls(struct thread_pool *, size_t);
void thread_pool_dispose(struct thread_pool *);
size_t thread_pool_get_count(struct thread_pool *);

