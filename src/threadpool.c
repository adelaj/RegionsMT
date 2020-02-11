#include "np.h"
#include "ll.h"
#include "log.h"
#include "memory.h"
#include "threadpool.h"
#include "threadsupp.h"

#include <string.h>
#include <stdlib.h>

void bit_set_interlocked_p(volatile void *arr, const void *p_bit)
{
    bit_set_interlocked(arr, *(const size_t *) p_bit);
}

void bit_reset_interlocked_p(volatile void *arr, const void *p_bit)
{
    bit_reset_interlocked(arr, *(const size_t *) p_bit);
}

void bit_set2_interlocked_p(volatile void *arr, const void *p_bit)
{
    bit_set2_interlocked(arr, *(const size_t *) p_bit);
}

void size_inc_interlocked_p(volatile void *mem, const void *arg)
{
    (void) arg;
    size_inc_interlocked(mem);
}

void size_dec_interlocked_p(volatile void *mem, const void *arg)
{
    (void) arg;
    size_dec_interlocked(mem);
}

bool bit_test2_acquire_p(volatile void *arr, const void *p_bit)
{
    return bit_test2_acquire(arr, *(const size_t *) p_bit);
}

bool bit_test_range_acquire_p(volatile void *arr, const void *p_cnt)
{
    return bit_test_range_acquire(arr, *(const size_t *) p_cnt);
}

bool bit_test2_range01_acquire_p(volatile void *arr, const void *p_cnt)
{
    return bit_test2_range01_acquire(arr, *(const size_t *) p_cnt);
}

bool size_test_acquire_p(volatile void *mem, const void *arg)
{
    (void) arg;
    return size_test_acquire(mem);
}

struct task_queue {
    size_t cap, begin, cnt;
    struct task *tasks[]; // Array with copies of task pointers
};

struct thread_storage {
    size_t id, data_sz;
    char data[]; // Array with thread-local data 
};

/*
struct thread_pool {
    struct task_queue *queue;
    spinlock_handle spinlock, startuplock;
    mutex_handle mutex;
    condition_handle condition;
    tls_handle tls;
    struct thread_storage **storage;
    volatile size_t active, terminate;
    size_t cnt;
    uint8_t *thread_bits;
    thread_handle thread_arr[];
};
*/

/*
struct thread_data {
    thread_handle handle;
    void *storage, *volatile ptr;
};
*/

struct thread_data {
    struct thread_pool *pool; // Parrent thread pool
    mutex_handle mutex;
    condition_handle condition;
    size_t id, counter;
    void *storage;
    volatile struct task task;
    volatile unsigned state;
};

struct thread_pool {
    spinlock_handle spinlock;
    mutex_handle mutex;
    condition_handle condition;
    struct queue queue;
    volatile size_t cnt, task_cnt, task_hint;
    struct persistent_array dispatched_task;
    struct persistent_array data;
    bool flag;
    /*spinlock_handle spinlock, startuplock;
    mutex_handle mutex;
    condition_handle condition;
    tls_handle tls;
    struct thread_storage **storage;
    volatile size_t active, terminate;
    size_t cnt;
    uint8_t *thread_bits;
    thread_handle thread_arr[];*/
};

static struct task_queue *task_queue_create(size_t cap)
{
    struct task_queue *res;
    if (!array_init(&res, &cap, cap, sizeof(*res->tasks), sizeof(*res), 0).status) return NULL;
    
    res->cap = cap;
    res->begin = res->cnt = 0;
    return res;
}

static bool task_queue_test(struct task_queue **p_queue, size_t diff)
{
    struct task_queue *res = *p_queue;
    size_t cap = res->cap;
    if (!array_test(&res, &cap, sizeof(*res->tasks), sizeof(*res), 0, res->cnt, diff).status) return 0;
    if (cap == res->cap) return 1; // Queue has already enough space
    
    size_t left = res->begin + res->cnt;
    if (left > res->cap)
    {
        if (left > cap)
        {
            memcpy(res->tasks + res->cap, res->tasks, (cap - res->cap) * sizeof(*res->tasks));
            memmove(res->tasks, res->tasks + cap - res->cap, (left - cap) * sizeof(*res->tasks));
        }
        else memcpy(res->tasks + res->cap, res->tasks, (left - res->cap) * sizeof(*res->tasks));
    }    
    res->cap = cap;
    *p_queue = res;

    return 1;
}

static struct task *task_queue_peek(struct task_queue *restrict queue, size_t offset)
{
    if (queue->begin + offset >= queue->cap) return queue->tasks[queue->begin + offset - queue->cap];
    return queue->tasks[queue->begin + offset];
}

// In the next two functions it is assumed that queue->cap >= queue->cnt + cnt
static void task_queue_enqueue_lo(struct task_queue *restrict queue, struct task *newtasks, size_t cnt)
{
    size_t left = queue->begin + queue->cnt;
    if (left >= queue->cap) left -= queue->cap;
       
    if (left + cnt > queue->cap)
    {
        for (size_t i = left; i < queue->cap; queue->tasks[i++] = newtasks++);
        for (size_t i = 0; i < left + cnt - queue->cap; queue->tasks[i++] = newtasks++);
    }
    else 
        for (size_t i = left; i < left + cnt; queue->tasks[i++] = newtasks++);

    queue->cnt += cnt;
}

static void task_queue_enqueue_hi(struct task_queue *restrict queue, struct task *newtasks, size_t cnt)
{
    size_t left = queue->begin + queue->cap - cnt;
    if (left >= queue->cap) left -= queue->cap;
        
    if (left > queue->begin)
    {
        for (size_t i = left; i < queue->cap; queue->tasks[i++] = newtasks++);
        for (size_t i = 0; i < queue->begin; queue->tasks[i++] = newtasks++);
    }
    else
        for (size_t i = left; i < queue->begin; queue->tasks[i++] = newtasks++);

    queue->begin = left;
    queue->cnt += cnt;    
}

static void task_queue_dequeue(struct task_queue *restrict queue, size_t offset)
{
    if (offset)
    {
        if (queue->begin + offset >= queue->cap) queue->tasks[queue->begin + offset - queue->cap] = queue->tasks[queue->begin];
        else queue->tasks[queue->begin + offset] = queue->tasks[queue->begin];
    }

    queue->cnt--;
    queue->begin++;
    if (queue->begin >= queue->cap) queue->begin -= queue->cap;
}

/*
// Searches and removes tasks from thread pool queue (commonly used by cleanup routines to remove pending tasks which will be never executed)
size_t thread_pool_remove_tasks(struct thread_pool *pool, struct task *tasks, size_t cnt)
{
    size_t fndr = 0;    
    spinlock_acquire(&pool->spinlock);

    for (size_t i = 0; i < cnt; i++)
    {
        for (size_t j = 0; j < pool->queue->cnt;)
        {
            if (task_queue_peek(pool->queue, j) == &tasks[i])
            {
                task_queue_dequeue(pool->queue, j);
                fndr++;
            }
            else j++;
        }        
    }

    spinlock_release(&pool->spinlock);
    return fndr;
}
*/

/*
bool thread_pool_enqueue_tasks(struct thread_pool *pool, struct task *tasks, size_t tasks_cnt, bool hi)
{
    spinlock_acquire(&pool->spinlock);
    
    if (!task_queue_test(&pool->queue, tasks_cnt)) // Queue extension if required
    {
        spinlock_release(&pool->spinlock);
        return 0;
    }
    
    (hi ? task_queue_enqueue_hi : task_queue_enqueue_lo)(pool->queue, tasks, tasks_cnt);

    spinlock_release(&pool->spinlock);
    condition_broadcast(&pool->condition);
        
    return 1;
}
*/

size_t thread_pool_get_count(struct thread_pool *pool)
{
    return size_load_acquire(&pool->cnt);
}

// Determines current thread identifier. Returns '-1' for the main thread.
size_t thread_pool_get_thread_id(struct thread_pool *pool)
{
    //struct thread_storage **p_storage = tls_fetch(&pool->tls);
    //if (p_storage) return (*p_storage)->id;
    return SIZE_MAX;
}

enum thread_state {
    THREAD_ACTIVE = 0,
    THREAD_IDLE,
    THREAD_SHUTDOWN_QUERY,
    THREAD_SHUTDOWN
};

struct thread_arg {
    struct thread_pool *pool;
    struct dispatched_task *volatile dispatched_task;
    void *tls;
    mutex_handle mutex;
    condition_handle condition;
    unsigned state;
    bool flag;
};

static thread_return thread_callback_convention thread_proc(void *Arg)
{
    struct thread_arg *arg = Arg;    
    for (;;)
    {
        struct dispatched_task *dispatched_task = ptr_interlocked_exchange(&arg->dispatched_task, NULL);
        if (!dispatched_task) // No task for the current thread
        {
            // At first, try to steal task from a different thread
            size_t cnt = size_load_acquire(&arg->pool->cnt);
            for (size_t i = 0; i < cnt; i++)
            {
                dispatched_task = ptr_interlocked_exchange(&((struct thread_arg *) persistent_array_fetch(&arg->pool->data, i, sizeof(struct thread_arg)))->dispatched_task, NULL);
                if (dispatched_task) break;
            }
            // If no task found, then go the sleep state
            if (!dispatched_task)
            {
                mutex_acquire(&arg->mutex);
                if (!arg->flag)
                {
                    if (arg->state == THREAD_SHUTDOWN_QUERY)
                    {
                        arg->state = THREAD_SHUTDOWN;
                        mutex_release(&arg->mutex);
                        return (thread_return) 0;
                    }
                    arg->state = THREAD_IDLE;
                    condition_sleep(&arg->condition, &arg->mutex);
                    arg->state = THREAD_ACTIVE;
                }
                arg->flag = 0;
                mutex_release(&arg->mutex);
                continue;
            }
        }

        // Execute the task
        unsigned res = dispatched_task->callback(dispatched_task->arg, dispatched_task->context, arg->tls);
        if (res == TASK_YIELD)
        {
            bool_store_release(&dispatched_task->norphan, 0);
            continue;
        }
        dispatched_task->aggr(dispatched_task->aggr_mem, dispatched_task->aggr_arg, res);
        bool_store_release(&dispatched_task->ngarbage, 0);
        size_dec_interlocked(&arg->pool->task_hint);

        mutex_acquire(&arg->pool->mutex);
        arg->pool->flag = 1;
        condition_signal(&arg->pool->condition);
        mutex_release(&arg->pool->mutex);
    }
}

void thread_pool_schedule(struct thread_pool *pool)
{
    for (;;)
    {
        // Lock the queue and the dispatch array
        spinlock_acquire(&pool->spinlock);

        // Checking queue
        bool dispatch = 0, pending = 0;
        size_t off = 0;
        struct task *task = NULL;        
        while (off < pool->queue.cnt)
        {
            task = queue_fetch(&pool->queue, off, sizeof(*task));
            switch (task->cond(task->cond_mem, task->cond_arg))
            {
            case COND_WAIT:
                off++;
                pending = 1;
                continue;
            case COND_DROP:
                queue_dequeue(&pool->queue, off, sizeof(*task));
                continue;
            case COND_EXECUTE:
                dispatch = 1;
            }
            break;
        }
        
        // Searching for an empty dispatch slot or for an orphaned task. This search is failsafe
        bool garbage = 1, orphan = 0;
        struct dispatched_task *dispatched_task = NULL;
        for (size_t i = 0; i < pool->task_cnt; i++)
        {
            dispatched_task = persistent_array_fetch(&pool->dispatched_task, i, sizeof(*dispatched_task));
            if (!dispatch)
            {
                if (bool_load_acquire(&dispatched_task->ngarbage))
                {
                    if (!bool_load_acquire(&dispatched_task->norphan))
                    {
                        orphan = 1;
                        break;
                    }
                    garbage = 0;
                }
            }
            else if (!bool_load_acquire(&dispatched_task->ngarbage)) break;
        }
        
        if (dispatched_task) // Always true
        {
            if (dispatch)
            {
                dispatched_task->aggr = task->aggr;
                dispatched_task->aggr_arg = task->aggr_arg;
                dispatched_task->aggr_mem = task->aggr_mem;
                dispatched_task->arg = task->arg;
                dispatched_task->callback = task->callback;
                dispatched_task->context = task->context;
                bool_store_release(&dispatched_task->ngarbage, 1);
                bool_store_release(&dispatched_task->norphan, 1);
                size_inc_interlocked(&pool->task_hint);
                queue_dequeue(&pool->queue, off, sizeof(*task));
            }
            else if (orphan) bool_store_release(&dispatched_task->norphan, 1);
        }

        // Unlock the queue and the dispatch array
        spinlock_release(&pool->spinlock);

        // Dispatch task to a thread
        if (dispatched_task)
        {
            if (dispatch || orphan)
            {
                size_t cnt = size_load_acquire(&pool->cnt), ind = 0;
                for (; ind < cnt; ind++)
                {
                    struct thread_arg *arg = persistent_array_fetch(&pool->data, ind, sizeof(struct thread_arg));
                    if (ptr_interlocked_compare_exchange(&arg->dispatched_task, NULL, dispatched_task)) continue;
                    mutex_acquire(&arg->mutex);
                    arg->flag = 1;
                    condition_signal(&arg->condition);
                    mutex_release(&arg->mutex);
                    break;
                }
                if (ind < cnt) continue; // Continue the outer loop
                bool_store_release(&dispatched_task->norphan, 0);
            }
            else if (garbage && !pending) return; // Return if there is nothing to be done
        }

        // Go to the sleep state
        mutex_acquire(&pool->mutex);
        if (!pool->flag) condition_sleep(&pool->condition, &pool->mutex);
        pool->flag = 0;
        mutex_release(&pool->mutex);
    }   
}

bool thread_arg_init(struct thread_arg *arg, size_t tls_sz, struct log *log)
{
    if (array_assert(log, CODE_METRIC, array_init(&arg->tls, NULL, 1, tls_sz, 0, ARRAY_FAILSAFE | ARRAY_CLEAR)))
    {
        if (thread_assert(log, CODE_METRIC, mutex_init(&arg->mutex)))
        {
            if (thread_assert(log, CODE_METRIC, condition_init(&arg->condition)))
            {

            }
            mutex_close(&arg->mutex);
        }
        free(arg->tls);
    }
    return 0;
}

struct thread_pool *thread_pool_create(size_t cnt, size_t tls_sz, struct log *log)
{
    size_t ind;
    struct thread_pool *pool;
    if (array_assert(log, CODE_METRIC, array_init(&pool, NULL, 1, sizeof(*pool), 0, ARRAY_STRICT)))
    {
        if (array_assert(log, CODE_METRIC, persistent_array_init(&pool->data, cnt, sizeof(struct thread_arg), 0)))
        {
            // Initializing per-thread resources
            size_t ind = 0;
            for (; ind < cnt; ind++)
            {
                struct thread_arg *arg = persistent_array_fetch(&pool->data, ind, sizeof(*arg));
                if (array_init(&pool->storage[ind], NULL, 1, sizeof(*pool->storage[ind]), storage_sz, ARRAY_STRICT | ARRAY_CLEAR))
                    *pool->storage[ind] = (struct thread_storage) { .id = ind, .data_sz = storage_sz };
                else break;
            }
            if (ind == pool->cnt)
            {
                if (array_init(&pool->thread_bits, NULL, UINT8_CNT(pool->cnt), 1, 0, ARRAY_STRICT | ARRAY_CLEAR) && (pool->queue = task_queue_create(tasks_cnt)) != NULL)
                {
                    pool->spinlock = pool->startuplock = SPINLOCK_INIT;
                    if (mutex_init(&pool->mutex))
                    {
                        if (condition_init(&pool->condition))
                        {
                            if (tls_init(&pool->tls))
                            {
                                for (ind = 0; ind < pool->cnt && thread_init(&pool->thread_arr[ind], thread_proc, pool); ind++);
                                if (ind == pool->cnt) return pool;                                
                                while (ind--)
                                {
                                    //thread_terminate(pool->thread_arr + ind);
                                    thread_close(pool->thread_arr + ind);
                                }
                                ind = pool->cnt;
                                tls_close(&pool->tls);
                            }
                            condition_close(&pool->condition);
                        }
                        mutex_close(&pool->mutex);
                    }
                    free(pool->queue);
                    free(pool->thread_bits);
                }                
            }
            while (ind--) free(pool->storage[ind]);
            free(pool->storage);
        }
        free(pool);
    }    
    return NULL;
}

size_t thread_pool_dispose(struct thread_pool *pool, size_t *p_pend)
{
    if (!pool) return 1;

    size_t res = 0;
    mutex_acquire(&pool->mutex);
    pool->terminate = pool->cnt;
    mutex_release(&pool->mutex);
    condition_signal(&pool->condition);

    for (size_t i = 0; i < pool->cnt; i++)
    {
        thread_return temp = (thread_return) 0;
        thread_wait(&pool->thread_arr[i], &temp);
        thread_close(&pool->thread_arr[i]);

        if (temp) res++;
    }

    tls_close(&pool->tls);
    condition_close(&pool->condition);
    mutex_close(&pool->mutex);
    
    if (p_pend) *p_pend = pool->queue->cnt;

    free(pool->queue);
    free(pool->thread_bits);
    for (size_t i = 0; i < pool->cnt; free(pool->storage[i++]));
    free(pool->storage);
    free(pool);
    return res;
}
