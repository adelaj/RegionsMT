#include "np.h"
#include "ll.h"
#include "memory.h"
#include "threadpool.h"
#include "threadsupp.h"

#include <string.h>
#include <stdlib.h>

void aggr_inc(volatile void *Mem, const void *arg, unsigned status)
{
    (void) arg;
    volatile struct inc_mem *mem = Mem;
    size_inc_interlocked((volatile void *[]) { &mem->fail, &mem->success, &mem->drop }[MIN(status, AGGR_DROP)]);
}

unsigned cond_inc(volatile void *Mem, const void *Tot)
{
    volatile struct inc_mem *mem = Mem;
    return size_load_acquire(&mem->success) + size_load_acquire(&mem->fail) + size_load_acquire(&mem->drop) == *(size_t *) Tot;
}

struct loop_data {
    volatile struct inc_mem mem;
    size_t tot;
    size_t ind[];
};

struct loop_generator_context {
    task_callback callback;
    struct task_cond cond;
    struct task_aggr aggr;
    size_t prod, cnt;
    void *context;
    struct loop_data *data;
};

static unsigned loop_tread_close(void *Data, void *context, void *tls)
{
    (void) context;
    (void) tls;
    volatile struct loop_data *data = Data;
    bool fail = size_load_acquire(&data->mem.fail), drop = size_load_acquire(&data->mem.drop);
    free(Data);
    return drop ? TASK_DROP : !fail;
}

void loop_generator(void *Task, size_t ind, void *Context)
{
    struct loop_generator_context *context = Context;
    *(struct task *) Task = ind < context->prod ?
        (struct task) {
            .callback = context->callback,
            .arg = context->data->ind + ind * context->cnt,
            .context = context->context,
            .cond = context->cond,
            .aggr = (struct task_aggr) { .callback = aggr_inc, .mem = &context->data->mem }
        } :
        (struct task) {
            .callback = loop_tread_close,
            .arg = context->data,
            .cond = (struct task_cond) { .callback = cond_inc, .mem = &context->data->mem, .arg = &context->data->tot },
            .aggr = context->aggr
        };
}

bool loop_init(struct thread_pool *pool, task_callback callback, struct task_cond cond, struct task_aggr aggr, void *context, size_t *restrict cntl, size_t cnt, size_t *restrict offl, bool hi, struct log *log)
{
    size_t prod, tot = 0;
    if (!crt_assert_impl(log, CODE_METRIC, !SIZE_PROD_TEST(&prod, cntl, cnt) || prod == SIZE_MAX || !SIZE_PROD_TEST_VA(&tot, prod, cnt) ? ERANGE : 0)) return 0;
    struct loop_data *data;
    if (!array_assert(log, CODE_METRIC, array_init(&data, NULL, fam_countof(struct loop_data, ind, tot), fam_sizeof(struct loop_data, ind), fam_diffof(struct loop_data, ind, tot), ARRAY_STRICT))) return 0;
    size_store_release(&data->mem.fail, 0);
    size_store_release(&data->mem.success, 0);
    size_store_release(&data->mem.drop, 0);
    data->tot = tot;
    if (prod) // Building index set
        if (offl) memcpy(data->ind, offl, cnt * sizeof(*data->ind));
        else memset(data->ind, 0, cnt * sizeof(*data->ind));
    for (size_t i = 1, j = cnt; i < prod; i++, j += cnt)
        for (size_t k = 0; k < cnt && (data->ind[j + k] = data->ind[j + k - cnt] + 1) == (offl ? offl[k] + cntl[k] : cntl[k]); data->ind[j + k] = 0, k++);
    return thread_pool_enqueue_yield(pool, loop_generator, &(struct loop_generator_context) { .callback = callback, .cond = cond, .aggr = aggr, .cnt = cnt, .prod = prod, .context = context, .data = data }, prod + 1, hi, log);
}

struct thread_pool {
    spinlock_handle spinlock, add;
    mutex_handle mutex;
    condition_handle condition;
    struct queue queue;
    volatile size_t cnt, task_cnt, task_hint;
    struct persistent_array dispatched_task;
    struct persistent_array arg;
    bool flag;
};

bool thread_pool_enqueue(struct thread_pool *pool, struct task *task, size_t cnt, bool hi, struct log *log)
{
    spinlock_acquire(&pool->spinlock);
    size_t car, probe = size_add(&car, size_load_acquire(&pool->task_hint), cnt);
    bool succ = 
        crt_assert_impl(log, CODE_METRIC, car ? ERANGE : 0) &&
        array_assert(log, CODE_METRIC, persistent_array_test(&pool->dispatched_task, probe, sizeof(struct dispatched_task), PERSISTENT_ARRAY_WEAK)) &&
        array_assert(log, CODE_METRIC, queue_enqueue(&pool->queue, hi, task, cnt, sizeof(*task)));
    spinlock_release(&pool->spinlock);
    return succ;
}

bool thread_pool_enqueue_yield(struct thread_pool *pool, generator_callback generator, void *context, size_t cnt, bool hi, struct log *log)
{
    spinlock_acquire(&pool->spinlock);
    size_t car, probe = size_add(&car, size_load_acquire(&pool->task_hint), cnt);
    bool succ =
        crt_assert_impl(log, CODE_METRIC, car ? ERANGE : 0) &&
        array_assert(log, CODE_METRIC, persistent_array_test(&pool->dispatched_task, probe, sizeof(struct dispatched_task), PERSISTENT_ARRAY_WEAK)) &&
        array_assert(log, CODE_METRIC, queue_enqueue_yield(&pool->queue, hi, generator, context, cnt, sizeof(struct task)));
    spinlock_release(&pool->spinlock);
    return succ;
}

size_t thread_pool_get_count(struct thread_pool *pool)
{
    return size_load_acquire(&pool->cnt);
}

enum thread_state {
    THREAD_ACTIVE = 0,
    THREAD_FLAGGED,
    THREAD_IDLE,
    THREAD_SHUTDOWN_QUERY,
    THREAD_SHUTDOWN
};

struct thread_arg {
    struct dispatched_task *volatile dispatched_task;
    void *tls;
    thread_handle thread;
    mutex_handle mutex;
    condition_handle condition;
    enum thread_state state;
};

static thread_return thread_callback_convention thread_proc(void *Arg)
{
    struct thread_arg *arg = Arg;
    struct thread_pool *pool = ((struct tls_base *) arg->tls)->pool;
    for (;;)
    {
        struct dispatched_task *dispatched_task = ptr_interlocked_exchange(&arg->dispatched_task, NULL);
        if (!dispatched_task) // No task for the current thread
        {
            // At first, try to steal task from a different thread
            size_t cnt = thread_pool_get_count(pool);
            for (size_t i = 0; i < cnt; i++)
            {
                dispatched_task = ptr_interlocked_exchange(&((struct thread_arg *) persistent_array_fetch(&pool->arg, i, sizeof(struct thread_arg)))->dispatched_task, NULL);
                if (dispatched_task) break;
            }
            // If still no task found, then go the sleep state
            if (!dispatched_task)
            {
                mutex_acquire(&arg->mutex);
                if (arg->state != THREAD_FLAGGED) 
                {
                    if (arg->state == THREAD_SHUTDOWN_QUERY)
                    {
                        arg->state = THREAD_SHUTDOWN;
                        mutex_release(&arg->mutex);
                        return (thread_return) 0;
                    }
                    arg->state = THREAD_IDLE;
                    condition_sleep(&arg->condition, &arg->mutex);                    
                }
                arg->state = THREAD_ACTIVE;
                mutex_release(&arg->mutex);
                continue;
            }
        }

        // Execute the task
        unsigned res = dispatched_task->callback ? dispatched_task->callback(dispatched_task->arg, dispatched_task->context, arg->tls): TASK_SUCCESS;
        if (res == TASK_YIELD)
        {
            bool_store_release(&dispatched_task->norphan, 0);
            continue;
        }
        if (dispatched_task->aggr.callback) dispatched_task->aggr.callback(dispatched_task->aggr.mem, dispatched_task->aggr.arg, res);
        bool_store_release(&dispatched_task->ngarbage, 0);
        size_dec_interlocked(&pool->task_hint);

        // Inform scheduller that global queue state has been changed
        mutex_acquire(&pool->mutex);
        pool->flag = 1;
        condition_signal(&pool->condition);
        mutex_release(&pool->mutex);
    }
}

void thread_pool_schedule(struct thread_pool *pool)
{
    for (;;)
    {
        // Locking the queue and the dispatch array
        spinlock_acquire(&pool->spinlock);

        // Checking queue
        bool dispatch = 0, pending = 0;
        size_t off = 0;
        struct task *task = NULL;        
        while (off < pool->queue.cnt)
        {
            task = queue_fetch(&pool->queue, off, sizeof(*task));
            switch (task->cond.callback ? task->cond.callback(task->cond.mem, task->cond.arg) : COND_EXECUTE)
            {
            case COND_WAIT:
                off++;
                pending = 1;
                continue;
            case COND_DROP:
                if (task->aggr.callback) task->aggr.callback(task->aggr.mem, task->aggr.arg, AGGR_DROP);
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
                dispatched_task->aggr.callback = task->aggr.callback;
                dispatched_task->aggr.arg = task->aggr.arg;
                dispatched_task->aggr.mem = task->aggr.mem;
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

        // Unlocking the queue and the dispatch array
        spinlock_release(&pool->spinlock);

        // Dispatch task to a thread
        if (dispatched_task)
        {
            if (dispatch || orphan)
            {
                size_t cnt = thread_pool_get_count(pool), ind = 0;
                for (; ind < cnt; ind++)
                {
                    struct thread_arg *arg = persistent_array_fetch(&pool->arg, ind, sizeof(struct thread_arg));
                    if (ptr_interlocked_compare_exchange(&arg->dispatched_task, NULL, dispatched_task)) continue;
                    mutex_acquire(&arg->mutex);
                    arg->state = THREAD_FLAGGED;
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

static bool thread_arg_init(struct thread_arg *arg, size_t tls_sz, struct log *log)
{
    if (array_assert(log, CODE_METRIC, array_init(&arg->tls, NULL, 1, tls_sz, 0, ARRAY_FAILSAFE | ARRAY_CLEAR)))
    {
        if (thread_assert(log, CODE_METRIC, mutex_init(&arg->mutex)))
        {
            if (thread_assert(log, CODE_METRIC, condition_init(&arg->condition)))
            {
                mutex_acquire(&arg->mutex);
                arg->state = THREAD_ACTIVE;
                mutex_release(&arg->mutex);
                ptr_store_release(&arg->dispatched_task, NULL);
                return 1;
            }
            mutex_close(&arg->mutex);
        }
        free(arg->tls);
    }
    return 0;
}

static void thread_arg_close(struct thread_arg *arg)
{
    if (!arg) return;
    condition_close(&arg->condition);
    mutex_close(&arg->mutex);
    free(arg->tls);
}

static bool thread_pool_init(struct thread_pool *pool, size_t cnt, size_t task_hint, struct log *log)
{
    if (array_assert(log, CODE_METRIC, persistent_array_init(&pool->arg, cnt, sizeof(struct thread_arg), 0)))
    {
        if (array_assert(log, CODE_METRIC, queue_init(&pool->queue, task_hint, sizeof(struct task))))
        {
            if (array_assert(log, CODE_METRIC, persistent_array_init(&pool->dispatched_task, task_hint, sizeof(struct dispatched_task), PERSISTENT_ARRAY_WEAK)))
            {
                if (thread_assert(log, CODE_METRIC, mutex_init(&pool->mutex)))
                {
                    if (thread_assert(log, CODE_METRIC, condition_init(&pool->condition)))
                    {
                        spinlock_release(&pool->spinlock); // Initializing the queue spinlock
                        spinlock_release(&pool->add); // Initializing the thread array spinlock
                        size_store_release(&pool->task_hint, 0); // The number of slots sufficient to store the queue intermediate data 
                        size_store_release(&pool->cnt, 0); // Number of initialized threads
                        size_store_release(&pool->task_cnt, 0);
                        mutex_acquire(&pool->mutex);
                        pool->flag = 0;
                        mutex_release(&pool->mutex);
                        return 1;
                    }
                    mutex_close(&pool->mutex);
                }                
                persistent_array_close(&pool->dispatched_task);
            }
            queue_close(&pool->queue);
        }
        persistent_array_close(&pool->arg);
    }
    return 0;
}

void *thread_pool_fetch_tls(struct thread_pool *pool, size_t ind)
{
    return ((struct thread_arg *) persistent_array_fetch(&pool->arg, ind, sizeof(struct thread_arg)))->tls;
}

static void thread_pool_finish_range(struct thread_pool *pool, size_t l, size_t r)
{
    for (size_t i = l; i < r; i++)
    {
        struct thread_arg *arg = persistent_array_fetch(&pool->arg, i, sizeof(*arg));
        mutex_acquire(&arg->mutex);
        arg->state = THREAD_SHUTDOWN_QUERY;
        condition_signal(&arg->condition);
        mutex_release(&arg->mutex);
        thread_return res; // Always zero
        thread_wait(&arg->thread, &res);
        thread_close(&arg->thread);
        thread_arg_close(arg);
    }
}

static size_t thread_pool_start_range(struct thread_pool *pool, size_t tls_sz, size_t l, size_t r, struct log *log)
{
    tls_sz = MAX(sizeof(struct tls_base), tls_sz);
    size_t pid = get_process_id();
    for (size_t i = l; i < r; i++) // Initializing per-thread resources
    {
        struct thread_arg *arg = persistent_array_fetch(&pool->arg, i, sizeof(*arg));
        if (thread_arg_init(arg, tls_sz, log))
        {
            *(struct tls_base *) arg->tls = (struct tls_base) { .pool = pool, .tid = i, .pid = pid };
            if (crt_assert_impl(log, CODE_METRIC, thread_init(&arg->thread, thread_proc, arg))) continue;
            thread_arg_close(arg);
        }
        return i;
    }
    return r;
}

static void thread_pool_close(struct thread_pool *pool)
{
    if (!pool) return;
    condition_close(&pool->condition);
    mutex_close(&pool->mutex);
    persistent_array_close(&pool->dispatched_task);
    queue_close(&pool->queue);
    persistent_array_close(&pool->arg);
}

struct thread_pool *thread_pool_create(size_t cnt, size_t tls_sz, size_t task_hint, struct log *log)
{
    struct thread_pool *pool;
    if (!array_assert(log, CODE_METRIC, array_init(&pool, NULL, 1, sizeof(*pool), 0, ARRAY_STRICT))) return NULL;
    if (thread_pool_init(pool, cnt, task_hint, log))
    {
        spinlock_acquire(&pool->add);
        size_t tmp = thread_pool_start_range(pool, tls_sz, 0, cnt, log);
        if (tmp == cnt)
        {
            size_store_release(&pool->cnt, cnt);
            spinlock_release(&pool->add);
            return pool;
        }
        thread_pool_finish_range(pool, 0, tmp);
        thread_pool_close(pool);
    }
    free(pool);    
    return NULL;
}

bool thread_pool_add(struct thread_pool *pool, size_t diff, size_t tls_sz, struct log *log)
{
    spinlock_acquire(&pool->add);
    size_t car, l = thread_pool_get_count(pool), r = size_add(&car, l, diff), tmp = 0;
    if (crt_assert_impl(log, CODE_METRIC, car ? ERANGE : 0) &&
        array_assert(log, CODE_METRIC, persistent_array_test(&pool->arg, r, sizeof(struct thread_arg), 0)))
    {
        tmp = thread_pool_start_range(pool, tls_sz, l, r, log);
        if (tmp == r) size_store_release(&pool->cnt, r);
        else thread_pool_finish_range(pool, 0, tmp);
    }
    spinlock_release(&pool->add);
    return tmp == r;
}

void thread_pool_dispose(struct thread_pool *pool)
{
    if (!pool) return;
    spinlock_acquire(&pool->add);
    thread_pool_finish_range(pool, 0, thread_pool_get_count(pool));
    thread_pool_close(pool);
    free(pool);
}
