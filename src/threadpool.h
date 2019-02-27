#pragma once

#include "common.h"
#include "ll.h"

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

typedef unsigned (*task_callback)(void *, void *);
typedef unsigned (*condition_callback)(volatile void *, const void *);
typedef void (*aggregator_callback)(volatile void *, const void *, unsigned);

struct task {
    task_callback callback;
    condition_callback cond;
    aggregator_callback aggr;
    void *arg, *context, *cond_arg, *aggr_arg;
    volatile void *cond_mem, *aggr_mem;
};

// Opaque structure with OS-dependent implementation
struct thread_pool;

size_t thread_pool_remove_tasks(struct thread_pool *, struct task *, size_t);
bool thread_pool_enqueue_tasks(struct thread_pool *, struct task *, size_t, bool);
struct thread_pool *thread_pool_create(size_t, size_t, size_t);
size_t thread_pool_dispose(struct thread_pool *, size_t *);
size_t thread_pool_get_count(struct thread_pool *);
size_t thread_pool_get_thread_id(struct thread_pool *);
void *thread_pool_get_thread_data(struct thread_pool *, size_t *, size_t *);
